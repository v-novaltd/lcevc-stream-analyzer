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

#ifndef VN_SIMULATE_BUFFER_ALGORITHM_INTERNAL_H
#define VN_SIMULATE_BUFFER_ALGORITHM_INTERNAL_H

#include "simulate_buffer_algorithm.h"

namespace vnova::analyzer::simulate_buffer::internal {

// Calculate simulation steps. Always overruns totalDuration by one stepDuration.
std::optional<size_t> stepCount(const second_t stepDuration, const second_t totalDuration);

// How many bytes flow into the buffer per time step?
std::optional<bytes_t> fillBytesPerStep(const second_t stepDuration, const bits_per_second_t fillBitRate);

// Convert PTS to timestamp in seconds, offset such that frame 0 is at time 0.
double frameTimeFromPTS(const second_t timeBase, const std::vector<int64_t>& framePts,
                        const size_t frameIndex);

} // namespace vnova::analyzer::simulate_buffer::internal

#endif
