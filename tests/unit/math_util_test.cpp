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

#include "utility/math_util.h"

#include <gtest/gtest.h>

#include <map>
#include <vector>

namespace {

TEST(MathUtilTest, DiffReturnsEmptyForTooSmallInput) // NOLINT
{
    const std::vector<int> input{42};
    const std::vector<int> delta = vnova::utility::math::diff(input);
    EXPECT_TRUE(delta.empty());
}

TEST(MathUtilTest, DiffComputesAbsoluteAdjacentDelta) // NOLINT
{
    const std::vector<int> input{10, 7, 7, 3};
    const std::vector<int> expected{3, 0, 4};
    const std::vector<int> delta = vnova::utility::math::diff(input);
    EXPECT_EQ(delta, expected);
}

TEST(MathUtilTest, CountOccurrencesReturnsCounts) // NOLINT
{
    const std::vector<int> input{1, 2, 2, 5, 5, 5};
    const std::map<int, size_t> expected{{1, 1}, {2, 2}, {5, 3}};
    const std::map<int, size_t> counts = vnova::utility::math::countOccurrences(input);
    EXPECT_EQ(counts, expected);
}

TEST(MathUtilTest, InvertMapProducesMultimap) // NOLINT
{
    const std::map<int, size_t> input{{1, 1}, {2, 2}, {5, 3}};
    const std::multimap<size_t, int> expected{{1, 1}, {2, 2}, {3, 5}};
    const std::multimap<size_t, int> inverted = vnova::utility::math::invertMap(input);
    EXPECT_EQ(inverted, expected);
}

} // namespace
