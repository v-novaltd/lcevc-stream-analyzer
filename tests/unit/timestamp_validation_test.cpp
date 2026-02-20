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

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

TEST(TimestampValidationTest, ReturnsNulloptForShortSequence) // NOLINT
{
    // nullopt represents algorithm failure.
    const std::vector<int64_t> timestamps{0};
    const auto result = vnova::helper::timestamp::validateSequence(timestamps);
    EXPECT_FALSE(result.has_value());
}

TEST(TimestampValidationTest, ReturnsTrueForUniformDelta) // NOLINT
{
    // Uniform delta values is consistent.
    const std::vector<int64_t> timestamps{0, 1, 2, 3, 4, 5};
    const auto result = vnova::helper::timestamp::validateSequence(timestamps);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().consistent);
    EXPECT_EQ(result.value().intervalCount, 1);
}

TEST(TimestampValidationTest, ReturnsTrueForTwoDeltaValues) // NOLINT
{
    // validateSequence() specifically allows either uniform deltas OR every element to have delta N or N+1 ONLY
    // This is because timestamp fractional FPS dithering allows sequences such as this, where a longer step is used
    //         * here
    // 10 12 14 17 19 21
    const std::vector<int64_t> timestamps{0, 1, 3, 4, 6};
    const auto result = vnova::helper::timestamp::validateSequence(timestamps);
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().consistent);
    EXPECT_EQ(result.value().intervalCount, 2);
}

TEST(TimestampValidationTest, ReturnsInconsistentForThreeDeltaValues) // NOLINT
{
    // Three or more delta values is inconsistent.
    const std::vector<int64_t> timestamps{0, 1, 2, 4, 7};
    const auto result = vnova::helper::timestamp::validateSequence(timestamps);
    EXPECT_TRUE(result.has_value());
    EXPECT_FALSE(result.value().consistent);
    EXPECT_EQ(result.value().intervalCount, 3);
}

} // namespace
