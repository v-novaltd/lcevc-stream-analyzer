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

#include "helper/stream_reader.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

using vnova::analyzer::StreamReader;

TEST(StreamReaderReadBit, ReadsBitsMostSignificantFirst) // NOLINT
{
    // 0b10110000: bits [7..0] = 1,0,1,1,0,0,0,0
    std::array<uint8_t, 1> data{{0b10110000}};

    StreamReader reader;
    reader.Reset(data.data(), data.size());

    EXPECT_TRUE(reader.ReadBit());  // bit 7
    EXPECT_FALSE(reader.ReadBit()); // bit 6
    EXPECT_TRUE(reader.ReadBit());  // bit 5
    EXPECT_TRUE(reader.ReadBit());  // bit 4
    EXPECT_FALSE(reader.ReadBit()); // bit 3
    EXPECT_FALSE(reader.ReadBit()); // bit 2
    EXPECT_FALSE(reader.ReadBit()); // bit 1
    EXPECT_FALSE(reader.ReadBit()); // bit 0
}

TEST(StreamReaderState, TracksPositionAndValidity) // NOLINT
{
    std::array<uint8_t, 3> data{{0xAA, 0xBB, 0xCC}};

    StreamReader reader;
    reader.Reset(data.data(), data.size());

    EXPECT_EQ(reader.GetLength(), data.size());
    EXPECT_EQ(reader.GetPosition(), 0u);
    EXPECT_TRUE(reader.IsValid());
    EXPECT_EQ(reader.Current(), data.data());

    reader.SeekForward(2);
    EXPECT_EQ(reader.GetPosition(), 2u);
    EXPECT_TRUE(reader.IsValid());

    reader.SeekForward(1);
    EXPECT_FALSE(reader.IsValid());
}

TEST(StreamReaderReadValue, ReadsNetworkOrderedValueAndAdvances) // NOLINT
{
    std::array<uint8_t, 4> data{{0x01, 0x02, 0xBE, 0xEF}};
    StreamReader reader;
    reader.Reset(data.data(), data.size());

    const auto first = reader.ReadValue<uint16_t>();
    EXPECT_EQ(first, 0x0102);
    EXPECT_EQ(reader.GetPosition(), 2u);

    std::array<uint8_t, 2> tail{{0, 0}};
    reader.ReadBytes(tail.data(), tail.size());
    EXPECT_EQ(tail[0], 0xBE);
    EXPECT_EQ(tail[1], 0xEF);
    EXPECT_EQ(reader.GetPosition(), 4u);
    EXPECT_FALSE(reader.IsValid());
}

TEST(StreamReaderReadMultiByte, DecodesVariableLengthValues) // NOLINT
{
    std::array<uint8_t, 3> data{{0x7F, 0x81, 0x7F}};
    StreamReader reader;
    reader.Reset(data.data(), data.size());

    EXPECT_EQ(reader.ReadMultiByteValue(), 127u);
    EXPECT_EQ(reader.GetPosition(), 1u);

    EXPECT_EQ(reader.ReadMultiByteValue(), 255u);
    EXPECT_EQ(reader.GetPosition(), 3u);
}

TEST(StreamReaderReadBits, ReadsAcrossByteBoundaries) // NOLINT
{
    std::array<uint8_t, 2> data{{0b11010110, 0b01110000}};
    StreamReader reader;
    reader.Reset(data.data(), data.size());

    const auto twelveBits = reader.ReadBits<uint16_t, 12>();
    EXPECT_EQ(twelveBits, 0xD67u); // 1101 0110 0111
    EXPECT_EQ(reader.GetPosition(), 2u);

    const auto nextFour = reader.ReadBits<uint8_t>(4);
    EXPECT_EQ(nextFour, 0u); // Remaining bits of second byte are zeros
}

TEST(StreamReaderExpGolomb, DecodesMultipleValuesSequentially) // NOLINT
{
    // Bitstream packs four Exp-Golomb-coded values: 0,1,2,3
    // Concatenated bits: 1 | 010 | 011 | 00100 = 0xA6 0x40
    std::array<uint8_t, 2> data{{0xA6, 0x40}};
    StreamReader reader;
    reader.Reset(data.data(), data.size());

    EXPECT_EQ(reader.ReadUnsignedExpGolombBits(), 0u);
    EXPECT_EQ(reader.ReadUnsignedExpGolombBits(), 1u);
    EXPECT_EQ(reader.ReadUnsignedExpGolombBits(), 2u);
    EXPECT_EQ(reader.ReadUnsignedExpGolombBits(), 3u);
}

TEST(StreamReaderResetBitfield, DiscardsPartialByteProgress) // NOLINT
{
    std::array<uint8_t, 2> data{{0b11100000, 0b00001111}};
    StreamReader reader;
    reader.Reset(data.data(), data.size());

    EXPECT_TRUE(reader.ReadBit());
    EXPECT_TRUE(reader.ReadBit());
    EXPECT_TRUE(reader.ReadBit()); // Consumed 3 bits of first byte

    reader.ResetBitfield(); // Should drop remaining bits and move to next byte
    EXPECT_EQ(reader.GetPosition(), 1u);

    EXPECT_FALSE(reader.ReadBit()); // Starts fresh at next byte (0b00001111 -> reversed -> leading 0)
    EXPECT_EQ(reader.GetPosition(), 2u);
}
