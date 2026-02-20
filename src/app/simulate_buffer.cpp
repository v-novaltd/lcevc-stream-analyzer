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

#include "app/simulate_buffer.h"

#include "config.h"
#include "simulate_buffer/simulate_buffer_algorithm.h"
#include "sink/sink.h"
#include "utility/json_util.h"
#include "utility/log_util.h"
#include "utility/output_util.h"

#include <cinttypes>

namespace vnova::analyzer::simulate_buffer {

namespace {
    char fillOk(const double level) { return (level < 0 || level > 100) ? 'X' : '.'; }
} // namespace

bool simulate(const Config& config, const double timebase, const ParsedFrameList& parsedFrames)
{
    auto jsonData = vnova::utility::json::read(config.simulateBufferModelPath);
    if (jsonData.has_value() == false) {
        return false;
    }
    TriBufferModel model = jsonData.value();

    // Until the initial fill threshold is reached, do not release any frames, by making the time at which it is released infinite.
    const double firstReleaseTime =
        model.combined.size * model.combined.initialFill * 8 / model.combined.fillBitRate;

    if (timebase <= 0.0) {
        VNLOG_ERROR("timebase must be positive");
        return false;
    }

    const auto ptsPerFrameOpt = parsedFrames.GetPts();
    if (ptsPerFrameOpt.has_value() == false) {
        return false;
    }
    const auto& ptsPerFrame = ptsPerFrameOpt.value();

    auto timingOpt = calculateSimulationTiming(timebase, firstReleaseTime, ptsPerFrame,
                                               config.simulateBufferSampleInterval);

    if (timingOpt.has_value() == false) {
        return false;
    }
    const auto& timing = timingOpt.value();

    TriBufferOutput output;
    output.timing = timing;

    if (auto buffer = simulateBuffer(timing, model.enhancement, parsedFrames.GetLcevcBytes());
        buffer.has_value() == true) {
        output.enhancement = buffer.value();
    }

    const auto baseCountableOpt = parsedFrames.HasBase();
    if (baseCountableOpt.has_value() == false) {
        return false;
    }

    if (baseCountableOpt.value()) {
        if (auto buffer = simulateBuffer(timing, model.base, parsedFrames.GetBaseBytes());
            buffer.has_value() == true) {
            output.base = buffer.value();
        }

        if (auto buffer = simulateBuffer(timing, model.combined, parsedFrames.GetCombinedBytes());
            buffer.has_value() == true) {
            output.combined = buffer.value();
        }
    }

    for (size_t frameIndex = 0; frameIndex < parsedFrames.size(); frameIndex++) {
        const double enhancementFill =
            100.0 * output.enhancement.frameLevels.at(frameIndex) / output.enhancement.model.size;

        const double baseFill =
            baseCountableOpt.value()
                ? 100.0 * output.base.frameLevels.at(frameIndex) / output.base.model.size
                : 0;
        const double combinedFill =
            baseCountableOpt.value()
                ? 100.0 * output.combined.frameLevels.at(frameIndex) / output.combined.model.size
                : 0;

        printFrame(parsedFrames.at(frameIndex), enhancementFill, baseFill, combinedFill);
    }

    return vnova::utility::json::write(config.simulateBufferOutputPath, nlohmann::json(output));
}

void printFrame(const ParsedFrame& data, const double lcevcFillLevel, const double baseFillLevel,
                const double combinedFillLevel)
{
    if (data.base.has_value()) {
        output::writeToStdAndFile(
            {}, nullptr,
            "%10zu %3s  |  LCEVC dts:%10" PRId64 ", pts:%10" PRId64 ", siz:%10" PRId64
            ", buf:%3.0f%% %c  |  BASE dts:%10" PRId64 ", pts:%10" PRId64 ", siz:%10" PRId64
            ", buf:%3.0f%% %c  |  COMBINED siz:%10" PRId64 ", buf:%3.0f%% %c\n",
            data.index, toString(data.base.value().type), data.lcevc.dts, data.lcevc.pts,
            data.lcevcWireSize, lcevcFillLevel, fillOk(lcevcFillLevel), data.base.value().dts,
            data.base.value().pts, data.baseWireSize, baseFillLevel, fillOk(baseFillLevel),
            data.combinedWireSize, combinedFillLevel, fillOk(combinedFillLevel));
    } else {
        output::writeToStdAndFile({}, nullptr,
                                  "%10zu %3s  |  LCEVC dts:%10" PRId64 ", pts:%10" PRId64
                                  ", siz:%10" PRId64 ", buf:%3.0f%% %c\n",
                                  data.index, "?", data.lcevc.dts, data.lcevc.pts,
                                  data.lcevcWireSize, lcevcFillLevel, fillOk(lcevcFillLevel));
    }
}

} // namespace vnova::analyzer::simulate_buffer
