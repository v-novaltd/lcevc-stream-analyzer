/*
 * Copyright (C) 2014-2026 V-Nova International Limited
 *
 *     * All rights reserved.
 *     * This software is licensed under the BSD-3-Clause-Clear License.
 *     * No patent licenses are granted under this license. For enquiries about patent
 *       licenses, please contact legal@v-nova.com.
 *     * The software is a stand-alone project and is NOT A CONTRIBUTION to any other
 *       project.
 *     * If the software is incorporated into another project, THE TERMS OF THE
 *       BSD-3-CLAUSE-CLEAR LICENSE AND THE ADDITIONAL LICENSING INFORMATION CONTAINED
 *       IN THIS FILE MUST BE MAINTAINED, AND THE SOFTWARE DOES NOT AND MUST NOT ADOPT
 *       THE LICENSE OF THE INCORPORATING PROJECT. However, the software may be
 *       incorporated into a project under a compatible license provided the
 *       requirements of the BSD-3-Clause-Clear license are respected, and V-Nova
 *       International Limited remains licensor of the software ONLY UNDER the
 *       BSD-3-Clause-Clear license (not the compatible license).
 *
 * ANY ONWARD DISTRIBUTION, WHETHER STAND-ALONE OR AS PART OF ANY OTHER PROJECT,
 * REMAINS SUBJECT TO THE EXCLUSION OF PATENT LICENSES PROVISION OF THE
 * BSD-3-CLAUSE-CLEAR LICENSE.
 */

#include "app/analyze.h"
#include "app/simulate_buffer.h"
#include "config.h"
#include "config_types.h"
#include "extractor/extractor.h"
#include "helper/reorder.h"
#include "parser/parsed_frame.h"
#include "parser/parser.h"
#include "sink/sink.h"
#include "utility/log_util.h"

#include <memory>

using namespace vnova::utility;
using namespace vnova::analyzer;
using namespace vnova::helper;

size_t extractFrames(Reorder& buffer, std::vector<LCEVCFrame>& lcevcFrames, const std::unique_ptr<Sink>& sink)
{
    size_t framesRead = 0;
    for (auto it = lcevcFrames.begin(); it != lcevcFrames.end();) {
        // Write as is in DTS order.
        sink->write(*it);

        // Reorder into PTS order as LCEVC must be parsed in PTS order.
        buffer.enqueue(*it);
        it = lcevcFrames.erase(it);

        framesRead++;
    }
    return framesRead;
}

// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char* argv[])
{
    auto configOpt = parseArgs(argc, argv);
    if (configOpt.has_value() == false) {
        return static_cast<int>(ReturnCode::ArgParseFail);
    }
    auto config = configOpt.value();

    auto extractor = Extractor::factory(config);
    if (!extractor) {
        VNLOG_ERROR("Failed to create LCEVC extractor");
        return static_cast<int>(ReturnCode::ExtractorFactoryFail);
    }

    Reorder buffer;
    BaseFrameQueue frameQueue;
    std::vector<LCEVCFrame> lcevcFrames;
    ParsedFrameList parsedFrames;

    const auto sink = Sink::factory(config);
    if (!sink) {
        VNLOG_ERROR("Extract subcommand used but failed to create data sink");
        return static_cast<int>(ReturnCode::SinkFactoryFail);
    }

    std::unique_ptr<Parser> parser = nullptr;
    try {
        parser = std::make_unique<Parser>(config);
    } catch (const vnova::utility::file::FileError& e) {
        VNLOG_ERROR("%s", e.what());
        return static_cast<int>(ReturnCode::ParseFail);
    }

    bool flagsSet = false;

    size_t framesRead = 0;
    while (extractor->next(lcevcFrames, frameQueue)) {
        if (!flagsSet) {
            analyze::configureFlags(*extractor, buffer, *parser);
            flagsSet = true;
        }

        framesRead += extractFrames(buffer, lcevcFrames, sink);

        if (analyze::parseFrames(buffer, *parser, *extractor, frameQueue, parsedFrames) == false) {
            return static_cast<int>(ReturnCode::ParseFail);
        }

        if ((config.frameLimit > 0) && (framesRead >= config.frameLimit)) {
            break;
        }
    }

    if (extractor->getError()) {
        VNLOG_ERROR("Failure during LCEVC extraction");
        return static_cast<int>(ReturnCode::ExtractionFail);
    }

    while (extractor->flush(frameQueue, lcevcFrames)) {
        framesRead += extractFrames(buffer, lcevcFrames, sink);
    }

    // Reached end of stream. Flush remaining frames in queue
    buffer.setEndOfStream(true);

    if (analyze::parseFrames(buffer, *parser, *extractor, frameQueue, parsedFrames) == false) {
        return static_cast<int>(ReturnCode::DecodeFail);
    }

    if (parser->getIsBaseStreamSizeCountable() && (extractor->getBaseFrameCount() != parsedFrames.size())) {
        VNLOG_ERROR("Only parsed %d of %d extracted frames", parser->getTotal(),
                    extractor->getBaseFrameCount());
        return static_cast<int>(ReturnCode::ParseFail);
    }

    // Allow user to override framerate used for summary calculations. Also allows user to provide
    // a framerate for bin stream analysis, as there will never be an inferred framerate without a
    // base stream.
    if (config.inputFramerate > 0) {
        extractor->setBaseFramerate(config.inputFramerate);
    }

    if (config.subcommand.at(Subcommand::SIMULATE_BUFFER) == true) {
        const auto timebaseOpt = extractor->getBaseTimebase();
        if (timebaseOpt.has_value() == false) {
            VNLOG_ERROR("Failed to retrieve timebase from extractor");
            return static_cast<int>(ReturnCode::BufferSimulationFail);
        }

        if (simulate_buffer::simulate(config, timebaseOpt.value(), parsedFrames) == false) {
            VNLOG_ERROR("Failed to simulate HRD");
            return static_cast<int>(ReturnCode::BufferSimulationFail);
        }
    }

    const auto ptsPerFrameOpt = parsedFrames.GetPts();
    if (ptsPerFrameOpt.has_value() == false) {
        VNLOG_ERROR("Failed to determine PTS values from parsed frames");
        return static_cast<int>(ReturnCode::PTSValidationFail);
    }

    const auto validPtsSequenceOpt = vnova::helper::timestamp::validateSequence(ptsPerFrameOpt.value());
    if (validPtsSequenceOpt.has_value() == false) {
        VNLOG_ERROR("Failed to validate PTS sequence");
        return static_cast<int>(ReturnCode::PTSValidationFail);
    }
    const auto& validPtsSequence = validPtsSequenceOpt.value();

    if (config.subcommand.at(Subcommand::ANALYZE) == true) {
        FILE* file = config.analyzeLogFormat == LogFormat::TEXT ? stdout : stderr;
        analyze::summarize(file, *extractor, *parser, validPtsSequence);
    }

    if ((config.ptsValidation == true) && (validPtsSequence.consistent == false)) {
        VNLOG_ERROR("PTS ordering or cadence invalid");
        return static_cast<int>(ReturnCode::PTSOrderingFail);
    }

    sink->release();

    // Return success.
    return static_cast<int>(ReturnCode::Success);
}
