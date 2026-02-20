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

#ifndef VN_SIMULATE_BUFFER_ALGORITHM_H
#define VN_SIMULATE_BUFFER_ALGORITHM_H

#include <json.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace vnova::analyzer::simulate_buffer {

using hertz_t = double;
using bits_t = double;
using bytes_t = double;
using bits_per_second_t = double;
using second_t = double;

constexpr size_t BITS_PER_BYTE = 8;

struct BufferModel
{
    // Constant buffer fill rate.
    bits_per_second_t fillBitRate;

    // Max bytes we can use as buffer.
    bytes_t size;

    // Fraction (0-1) of buffer to fill before first frame is released.
    double initialFill;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BufferModel, fillBitRate, size, initialFill)

struct TriBufferModel
{
    BufferModel combined;
    BufferModel base;
    BufferModel enhancement;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TriBufferModel, combined, base, enhancement)

struct BufferOutput
{
    BufferModel model;
    std::vector<bytes_t> level;
    std::vector<second_t> plotTimes;
    std::vector<bytes_t> plotValues;
    std::vector<bytes_t> frameLevels;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BufferOutput, model, level, plotTimes, plotValues)

struct TimingParameters
{
    second_t stepDuration;
    std::vector<second_t> samples;
    std::vector<second_t> frameRelease;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TimingParameters, stepDuration, samples, frameRelease)

struct TriBufferOutput
{
    TimingParameters timing;
    BufferOutput combined;
    BufferOutput base;
    BufferOutput enhancement;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TriBufferOutput, timing, combined, base, enhancement)

std::optional<TimingParameters> calculateSimulationTiming(const second_t timeBase,
                                                          const second_t firstReleaseTime,
                                                          const std::vector<int64_t>& framePts,
                                                          const second_t stepDuration);

std::optional<BufferOutput> simulateBuffer(const TimingParameters& timing, const BufferModel& model,
                                           const std::vector<int64_t>& frameSizes);

} // namespace vnova::analyzer::simulate_buffer

#endif
