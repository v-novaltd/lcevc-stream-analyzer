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
#include "simulate_buffer_algorithm.h"

#include "simulate_buffer_algorithm_internal.h"
#include "utility/log_util.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace vnova::analyzer::simulate_buffer {

namespace internal {

    std::optional<size_t> stepCount(const second_t stepDuration, const second_t totalDuration)
    {
        if (stepDuration <= 0.0) {
            VNLOG_ERROR("stepDuration value must be positive");
            return std::nullopt;
        }
        if (totalDuration <= 0.0) {
            VNLOG_ERROR("totalDuration value must be positive");
            return std::nullopt;
        }
        if (totalDuration <= stepDuration) {
            VNLOG_ERROR("totalDuration value must be greater than stepDuration value");
            return std::nullopt;
        }
        return std::ceil(totalDuration / stepDuration) + 1;
    }

    std::optional<bytes_t> fillBytesPerStep(const second_t stepDuration, const bits_per_second_t fillBitRate)
    {
        if (stepDuration <= 0.0) {
            VNLOG_ERROR("stepDuration value must be positive");
            return std::nullopt;
        }
        if (fillBitRate <= 0.0) {
            VNLOG_ERROR("fillBitRate value must be positive.");
            return std::nullopt;
        }
        return fillBitRate * stepDuration / static_cast<bits_t>(BITS_PER_BYTE);
    }

    double frameTimeFromPTS(const second_t timeBase, const std::vector<int64_t>& framePts,
                            const size_t frameIndex)
    {
        const int64_t zeroOffsetPts = framePts.at(frameIndex) - framePts.at(0);
        return static_cast<double>(zeroOffsetPts) * timeBase;
    }
} // namespace internal

std::optional<TimingParameters> calculateSimulationTiming(const second_t timeBase,
                                                          const second_t firstReleaseTime,
                                                          const std::vector<int64_t>& framePts,
                                                          const second_t stepDuration)
{
    if (stepDuration <= 0) {
        VNLOG_ERROR("stepDuration must be positive");
        return std::nullopt;
    }

    if (framePts.empty()) {
        VNLOG_ERROR("Cannot calculate timing if there are no frame release times provided");
        return std::nullopt;
    }

    std::vector<second_t> frameReleaseTimes(framePts.size());
    for (size_t frameIndex = 0; frameIndex < framePts.size(); frameIndex++) {
        frameReleaseTimes.at(frameIndex) =
            firstReleaseTime + internal::frameTimeFromPTS(timeBase, framePts, frameIndex);
    }

    if (frameReleaseTimes.empty()) {
        VNLOG_ERROR("Cannot calculate timing if there are no frame release times");
        return std::nullopt;
    }

    const size_t stepCount = 1 + static_cast<size_t>(std::ceil(frameReleaseTimes.back() / stepDuration));

    std::vector<second_t> sampleTimes;
    sampleTimes.reserve(stepCount);
    for (size_t stepIndex = 0; stepIndex < stepCount; ++stepIndex) {
        sampleTimes.push_back(static_cast<second_t>(stepIndex) * stepDuration);
    }

    return std::make_optional<TimingParameters>({stepDuration, sampleTimes, frameReleaseTimes});
}

std::optional<BufferOutput> simulateBuffer(const TimingParameters& timing, const BufferModel& model,
                                           const std::vector<int64_t>& frameSizes)
{
    const std::optional<bytes_t> fillOpt =
        internal::fillBytesPerStep(timing.stepDuration, model.fillBitRate);
    if (fillOpt.has_value() == false) {
        VNLOG_ERROR("fillOpt value invalid");
        return std::nullopt;
    }
    const bytes_t fillPerStep = fillOpt.value();

    if (model.initialFill <= 0) {
        VNLOG_ERROR("initialFill must be positive");
        return std::nullopt;
    }

    BufferOutput output;

    std::vector<bytes_t> plotFillLevels;
    std::vector<bytes_t> plotTimePoints;
    std::vector<bytes_t> stepLevels;
    std::vector<bytes_t> frameLevels;

    bytes_t fillLevel = 0;
    size_t frameIndex = 0;

    const auto frameCount = std::min(timing.frameRelease.size(), frameSizes.size());

    for (const auto& stepTime : timing.samples) {
        plotTimePoints.push_back(stepTime);
        plotFillLevels.push_back(fillLevel);

        // Only one stepLevels entry per step.
        stepLevels.push_back(fillLevel);

        if ((frameIndex < frameCount) && (timing.frameRelease.at(frameIndex) <= stepTime)) {
            // If the next frame release time has passed, decrement the buffer by the size of the frame.

            if (frameSizes.at(frameIndex) <= 0) {
                VNLOG_ERROR("Inexplicably got a negative frame size");
                return std::nullopt;
            }

            fillLevel -= static_cast<bytes_t>(frameSizes.at(frameIndex));
            frameLevels.push_back(fillLevel);

            // We add a plot point here which has the same time, but the decreased fill level.
            // This renders as as a sawtooth waveform with gradient ascent and sudden drops to show
            // frame release clearly.
            //
            // For analysis, you might prefer to use `level` instead of `plotValues`.
            plotTimePoints.push_back(stepTime);
            plotFillLevels.push_back(fillLevel);
            frameIndex += 1;
        }

        // Always fill the buffer by the number of bytes received since last stepDuration.
        fillLevel += fillPerStep;
    }

    return std::make_optional<BufferOutput>({model, stepLevels, plotTimePoints, plotFillLevels, frameLevels});
}

} // namespace vnova::analyzer::simulate_buffer
