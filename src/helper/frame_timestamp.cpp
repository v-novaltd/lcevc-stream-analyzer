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
#include "helper/frame_timestamp.h"

#include "utility/math_util.h"

namespace vnova::helper::timestamp {

std::optional<TimestampStats> validateSequence(const std::vector<int64_t>& pts)
{
    if (pts.size() < 2) {
        return std::nullopt;
    }

    const std::vector<int64_t> delta = utility::math::diff(pts);
    const std::map<int64_t, size_t> occurrences = utility::math::countOccurrences(delta);
    const std::multimap<size_t, int64_t> frequencies = utility::math::invertMap(occurrences);

    return std::make_optional<TimestampStats>(
        {(frequencies.size() == 1 || frequencies.size() == 2), frequencies.size(), frequencies});
}

} // namespace vnova::helper::timestamp
