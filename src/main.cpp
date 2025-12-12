/*
 * Copyright (C) 2014-2025 V-Nova International Limited
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

#include "bin/lcevc_bin.h"
#include "config.h"
#include "extractor/extractor.h"
#include "helper/frame_queue.h"
#include "helper/reorder.h"
#include "parser/parser.h"
#include "sink/sink.h"
#include "utility/log_util.h"

#include <memory>

using namespace vnova::utility;
using namespace vnova::analyzer;
using vnova::VNLog;

int parseBufferedLCEVCFrames(Reorder& buffer, Parser& parser, LCEVC& lcevc, FrameQueue& frameBuffer);
void updateStreamFlags(Extractor const & extractor, Reorder& buffer, Parser& parser);
void printSummary(FILE* file, Extractor const & extractor, Parser const & parser);

// NOLINTNEXTLINE(bugprone-exception-escape)
int main(int argc, char* argv[])
{
    CLI::App app{"LCEVC Stream Analyzer", "lcevc_stream_analyzer"};
    argv = app.ensure_utf8(argv);
    app.set_version_flag("--version");

    auto configOpt = parseArgs(app, argc, argv);
    if (configOpt.has_value() == false) {
        return static_cast<int>(ReturnCode::ArgParseFail);
    }
    auto config = configOpt.value();

    const auto extractor = Extractor::factory(config);
    if (!extractor) {
        VNLog::Error("Failed to create LCEVC extractor\n");
        return static_cast<int>(ReturnCode::ExtractorFactoryFail);
    }

    const auto sink = Sink::factory(config);
    if (!sink) {
        VNLog::Error("Failed to create data sink\n");
        return static_cast<int>(ReturnCode::SinkFactoryFail);
    }

    std::unique_ptr<Parser> parser = nullptr;
    try {
        parser = std::make_unique<Parser>(config);
    } catch (const FileError& e) {
        VNLog::Error("%s\n", e.what());
        return static_cast<int>(ReturnCode::ParseFail);
    }

    LCEVC lcevc;
    Reorder buffer;
    FrameQueue frameBuffer;
    std::vector<LCEVC> lcevcFrames;

    bool flagsSet = false;

    while (extractor->next(lcevcFrames, frameBuffer)) {
        if (!flagsSet) {
            updateStreamFlags(*extractor, buffer, *parser);
            flagsSet = true;
        }

        for (auto it = lcevcFrames.begin(); it != lcevcFrames.end();) {
            // Write as is in DTS order.
            sink->write(*it);

            // Reorder into PTS order as LCEVC must be parsed in PTS order.
            buffer.enqueue(*it);

            it = lcevcFrames.erase(it);
        }

        if (parseBufferedLCEVCFrames(buffer, *parser, lcevc, frameBuffer) != 0) {
            return static_cast<int>(ReturnCode::ParseFail);
        }
    }

    if (extractor->hasError()) {
        VNLog::Error("ERROR: Failure during LCEVC extraction\n");
        return static_cast<int>(ReturnCode::ExtractionFail);
    }

    while (extractor->flush(frameBuffer, lcevcFrames)) {
        for (auto it = lcevcFrames.begin(); it != lcevcFrames.end();) {
            // Write as is in DTS order.
            sink->write(*it);

            // Reorder into PTS order as LCEVC must be parsed in PTS order.
            buffer.enqueue(*it);

            it = lcevcFrames.erase(it);
        }
    }

    // Reached end of stream. Flush remaining frames in queue
    buffer.setEndOfStream(true);
    if (parseBufferedLCEVCFrames(buffer, *parser, lcevc, frameBuffer) != 0) {
        return static_cast<int>(ReturnCode::DecodeFail);
    }

    FILE* file = config.logFormat == LogFormat::Text ? stdout : stderr;
    printSummary(file, *extractor, *parser);

    sink->release();

    // Return success.
    return static_cast<int>(ReturnCode::Success);
}

int parseBufferedLCEVCFrames(Reorder& buffer, Parser& parser, LCEVC& lcevc, FrameQueue& frameBuffer)
{
    while (buffer.dequeue(lcevc, frameBuffer)) {
        if (!parser.parse(lcevc)) {
            VNLog::Error("Failed to decode LCEVC data\n");
            return -1;
        }
    }

    return 0;
}

void updateStreamFlags(Extractor const & extractor, Reorder& buffer, Parser& parser)
{
    buffer.setRawstream(extractor.getRawStream());
    buffer.setBaseStream(extractor.getBaseStream());
    parser.isBaseStream = extractor.getBaseStream();
    parser.isFourBytePrefix = extractor.getFourBytePrefix();
    parser.lvccProfile = extractor.getLvccProfile();
    parser.lvccLevel = extractor.getLvccLevel();
    parser.bLvccPresent = extractor.hasLvccAtom();
}

void printSummary(FILE* file, Extractor const & extractor, Parser const & parser)
{
    if (parser.lcevcLayerSize == 0) {
        return;
    }

    const uint64_t baseFrameCount = extractor.baseFrameCount;
    const uint64_t totalPacketSize = extractor.totalPacketSize;

    size_t baseSize = 0;
    if (totalPacketSize > 0) {
        baseSize = totalPacketSize - parser.lcevcLayerSize;
    }
    const size_t lcevcSize = parser.lcevcLayerSize;

#ifdef VN_SHOW_BITRATE
    static_assert(false, "Bitrate calculations are not reliable.");
    double baseDuration = 0.0;
    double baseBitrate = 0.0;

    if (baseSize > 0) {
        baseDuration = (extractor.baseFps > 0)
                           ? static_cast<double>(baseFrameCount) / static_cast<double>(extractor.baseFps)
                           : extractor.durationSec;
        baseBitrate = (static_cast<double>(baseSize) * 8.0) / baseDuration;
    }

    const double lcevcDuration = (extractor.baseFps > 0) ? static_cast<double>(parser.getTotal() - 1) /
                                                               static_cast<double>(extractor.baseFps)
                                                         : extractor.durationSec;

    const double lcevcBitrate = (static_cast<double>(lcevcSize) * 8.0) / lcevcDuration;
#endif

    fprintf(file, "Summary : ");
    fprintf(file, "Base frames         : %" PRId64 "\n", baseFrameCount);
    fprintf(file, "\t  LCEVC frames        : %" PRId64 "\n", parser.getTotal() - 1);
    fprintf(file, "\t  Base Layer Size     : %.0f kb\n", static_cast<double>(baseSize) / 1024.0);

#ifdef VN_SHOW_BITRATE
    if (baseDuration > 0.0) {
        fprintf(file, "\t  Base Layer Bitrate  : %.0f kbps\n", baseBitrate / 1000.0);
    } else {
        fprintf(file, "\t  Base Layer Bitrate  : N/A\n");
    }
#endif

    fprintf(file, "\t  LCEVC Layer Size    : %.0f kb\n", static_cast<double>(lcevcSize) / 1024.0);

#ifdef VN_SHOW_BITRATE
    if (lcevcDuration > 0.0) {
        fprintf(file, "\t  LCEVC Bitrate       : %.0f kbps\n", lcevcBitrate / 1000.0);
    } else {
        fprintf(file, "\t  LCEVC Bitrate       : N/A\n");
    }
#endif

    fprintf(file, "\t  -- Level 1 Size     : %.0f kb\n", static_cast<double>(parser.sublayer1Size) / 1024.0);
    fprintf(file, "\t  -- Level 2 Size     : %.0f kb\n", static_cast<double>(parser.sublayer2Size) / 1024.0);
    fprintf(file, "\t  -- Temporal Size    : %.0f kb\n", static_cast<double>(parser.temporalSize) / 1024.0);

    if (baseSize > 0) {
        fprintf(file, "\t  LCEVC-Base Ratio    : %.3f\n",
                static_cast<double>(lcevcSize) / static_cast<double>(baseSize));
    } else {
        fprintf(file, "\t  LCEVC-Base Ratio    : N/A\n");
    }
}
