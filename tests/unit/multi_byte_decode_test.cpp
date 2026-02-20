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

#include "utility/multi_byte.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {

struct Reader
{
    explicit Reader(std::initializer_list<uint8_t> bytes)
        : data(bytes)
    {}
    uint8_t operator()() { return (index < data.size()) ? data[index++] : 0; }
    std::vector<uint8_t> data;
    size_t index{0};
};

} // namespace

TEST(MultiByteDecode, SingleByte) // NOLINT
{
    // 0x7F => 127
    Reader r{0x7F};
    auto val = vnova::utility::MultiByte::Decode([&]() { return r(); });
    EXPECT_EQ(val, 127u);
}

TEST(MultiByteDecode, TwoBytes128) // NOLINT
{
    // 128 => 0x81 0x00 in big-endian VLQ (7-bit groups)
    Reader r{0x81, 0x00};
    auto val = vnova::utility::MultiByte::Decode([&]() { return r(); });
    EXPECT_EQ(val, 128u);
}

TEST(MultiByteDecode, TwoBytes255) // NOLINT
{
    // 255 => 0x81 0x7F
    Reader r{0x81, 0x7F};
    auto val = vnova::utility::MultiByte::Decode([&]() { return r(); });
    EXPECT_EQ(val, 255u);
}

TEST(MultiByteDecode, OverflowThrowsOnSixthByte) // NOLINT
{
    // More than 5 bytes should throw (e.g., 6 bytes total)
    Reader r{0x81, 0x80, 0x80, 0x80, 0x80, 0x00};
    EXPECT_THROW((void)vnova::utility::MultiByte::Decode([&]() { return r(); }), std::overflow_error);
}
