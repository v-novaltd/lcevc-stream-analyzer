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
#include "simulate_buffer/simulate_buffer_algorithm.h"
#include "simulate_buffer/simulate_buffer_algorithm_internal.h"

#include <gtest/gtest.h>

namespace {

TEST(SimulateBufferAlgorithmTest, StepCount) // NOLINT
{
    using namespace vnova::analyzer::simulate_buffer::internal;
    {
        const auto zeroStep = stepCount(0.0, 1.0);
        EXPECT_FALSE(zeroStep.has_value());
    }
    {
        const auto negativeEnd = stepCount(0.5, -1.0);
        EXPECT_FALSE(negativeEnd.has_value());
    }
    {
        const auto endNotGreater = stepCount(1.0, 0.5);
        EXPECT_FALSE(endNotGreater.has_value());
    }
    {
        const auto count = stepCount(0.4, 1.0);
        ASSERT_TRUE(count.has_value());
        EXPECT_EQ(count.value(), 4U);
    }
}

TEST(SimulateBufferAlgorithmTest, FillBytesPerStep) // NOLINT
{
    using namespace vnova::analyzer::simulate_buffer::internal;
    {
        const auto zeroStep = fillBytesPerStep(0.0, 1000.0);
        EXPECT_FALSE(zeroStep.has_value());
    }
    {
        const auto zeroBandwidth = fillBytesPerStep(0.1, 0.0);
        EXPECT_FALSE(zeroBandwidth.has_value());
    }
    {
        const auto fill = fillBytesPerStep(0.25, 8000.0);
        ASSERT_TRUE(fill.has_value());
        EXPECT_DOUBLE_EQ(fill.value(), 250.0);
    }
}

} // namespace
