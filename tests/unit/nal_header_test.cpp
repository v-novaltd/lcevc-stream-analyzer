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

#include "utility/nal_header.h"

#include <gtest/gtest.h>

#include <array>
#include <initializer_list>
#include <vector>

using namespace vnova::utility; // NOLINT

namespace {

constexpr std::array<uint8_t, 3> kThreeByteStartCode{{0x00, 0x00, 0x01}};
constexpr std::array<uint8_t, 4> kFourByteStartCode{{0x00, 0x00, 0x00, 0x01}};

std::vector<uint8_t> withStartCode(const std::array<uint8_t, 3>& startCode,
                                   std::initializer_list<uint8_t> payload)
{
    std::vector<uint8_t> data;
    data.reserve(startCode.size() + payload.size());
    for (uint8_t b : startCode) {
        data.push_back(b);
    }
    for (uint8_t b : payload) {
        data.push_back(b);
    }
    return data;
}

std::vector<uint8_t> withStartCode4(std::initializer_list<uint8_t> payload)
{
    std::vector<uint8_t> data;
    data.reserve(kFourByteStartCode.size() + payload.size());
    for (uint8_t b : kFourByteStartCode) {
        data.push_back(b);
    }
    for (uint8_t b : payload) {
        data.push_back(b);
    }
    return data;
}

std::vector<uint8_t> withStartCode3(std::initializer_list<uint8_t> payload)
{
    return withStartCode(kThreeByteStartCode, payload);
}

std::vector<uint8_t> makeVvcHeader(uint8_t nalType, uint8_t nuhLayerId, uint8_t temporalIdPlus1)
{
    // VVC: b0 = fzb(1)=0 | reserved(1)=0 | nuh_layer_id(6)
    //      b1 = nal_unit_type(5) | temporal_id_plus1(3)
    const auto b0 = static_cast<uint8_t>(nuhLayerId & 0x3F);
    const auto b1 = static_cast<uint8_t>(((nalType & 0x1F) << 3) | (temporalIdPlus1 & 0x07));
    return {b0, b1};
}

} // namespace

TEST(NalHeader, MatchAnnexBStartCode) // NOLINT
{
    uint32_t sc = 0;
    const auto four = withStartCode4({0x11});
    EXPECT_TRUE(matchAnnexBStartCode(four.data(), four.size(), sc));
    EXPECT_EQ(sc, 4u);

    const auto three = withStartCode3({0x67});
    EXPECT_TRUE(matchAnnexBStartCode(three.data(), three.size(), sc));
    EXPECT_EQ(sc, 3u);

    const std::vector<uint8_t> none{0x11, 0x22, 0x33};
    EXPECT_FALSE(matchAnnexBStartCode(none.data(), none.size(), sc));
}

TEST(NalHeaderParse, AnnexBH264HeaderSizeAndFields) // NOLINT
{
    // forbidden_zero_bit=0, nal_ref_idc=3, nal_unit_type=5 (IDR)
    const auto data = withStartCode3({0x65});

    ParsedNALHeader header{};
    ASSERT_TRUE(parseAnnexBHeader(data.data(), data.size(), CodecType::H264, header));
    EXPECT_EQ(header.startCode, AnnexBStartCode::Three);
    EXPECT_EQ(header.headerSize, 4u);
    EXPECT_EQ(header.nalRefIdc, 3);
    EXPECT_EQ(header.nalUnitType, 5);
}

TEST(NalHeaderParse, AnnexBLcevcValidHeader) // NOLINT
{
    // forbidden_zero_bit=0, forbidden_one_bit=1, nal_unit_type=21, reserved_flag=0x1FF
    const auto data = withStartCode3({0x6B, 0xFF});

    ParsedNALHeader header{};
    ASSERT_TRUE(parseAnnexBHeader(data.data(), data.size(), CodecType::LCEVC, header));
    EXPECT_EQ(header.startCode, AnnexBStartCode::Three);
    EXPECT_EQ(header.headerSize, 5u); // 3-byte start code + 2-byte header
    EXPECT_EQ(header.forbiddenZeroBit, 0);
    EXPECT_EQ(header.nalUnitType, 21);
}

TEST(NalHeaderParse, AnnexBLcevcInvalidBitsFailParse) // NOLINT
{
    // forbidden_one_bit=0 should be rejected
    const auto data = withStartCode3({0x2B, 0x00});

    ParsedNALHeader header{};
    EXPECT_FALSE(parseAnnexBHeader(data.data(), data.size(), CodecType::LCEVC, header));
}

TEST(NalHeaderParse, LengthPrefixedLcevcHeaderSize) // NOLINT
{
    const std::array<uint8_t, 2> header{{0x6B, 0xFF}};

    ParsedNALHeader parsedHeader{};
    ASSERT_TRUE(parseLengthPrefixedHeader(header.data(), header.size(), CodecType::LCEVC, parsedHeader));
    EXPECT_EQ(parsedHeader.startCode, AnnexBStartCode::None);
    EXPECT_EQ(parsedHeader.headerSize, 2u);
    EXPECT_EQ(parsedHeader.nalUnitType, 21);
}

TEST(NalHeaderParse, AnnexBInsufficientBytesFail) // NOLINT
{
    const auto data = withStartCode3({}); // only start code, no payload bytes

    ParsedNALHeader header{};
    EXPECT_FALSE(parseAnnexBHeader(data.data(), data.size(), CodecType::HEVC, header));
}

TEST(NalHeaderParse, AnnexBH264SEIWithFourByteStartCode) // NOLINT
{
    const auto nalu = withStartCode4({0x06}); // H264 SEI nal_unit_type = 6
    ParsedNALHeader header{};
    ASSERT_TRUE(parseAnnexBHeader(nalu.data(), nalu.size(), BaseType::Enum::H264, header));
    EXPECT_TRUE(isSEI(header));
    EXPECT_EQ(header.headerSize, 5u);
    EXPECT_EQ(header.nalUnitType, 6u);
}

TEST(NalHeaderParse, AnnexBHEVCSEI) // NOLINT
{
    // HEVC prefix SEI type = 39
    const uint8_t type = 39;
    const auto b0 = static_cast<uint8_t>((type & 0x3F) << 1); // fzb=0, type, nuh_layer_id msb=0
    const auto b1 = static_cast<uint8_t>(0x01); // nuh_layer_id low bits=0, temporal_id_plus1=1
    const auto nalu = withStartCode4({b0, b1});

    ParsedNALHeader header{};
    ASSERT_TRUE(parseAnnexBHeader(nalu.data(), nalu.size(), BaseType::Enum::HEVC, header));
    EXPECT_TRUE(isSEI(header));
    EXPECT_EQ(header.headerSize, 6u);
    EXPECT_EQ(header.nalUnitType, type);
    EXPECT_EQ(header.temporalIdPlus1, 1u);
}

TEST(NalHeaderParse, AnnexBVVCSEI) // NOLINT
{
    // VVC prefix SEI type = 23; temporal_id_plus1 = 1
    const auto nalu = withStartCode4({0x00, static_cast<uint8_t>((23u << 3) | 0x01)});

    ParsedNALHeader header{};
    ASSERT_TRUE(parseAnnexBHeader(nalu.data(), nalu.size(), BaseType::Enum::VVC, header));
    EXPECT_TRUE(isSEI(header));
    EXPECT_EQ(header.headerSize, 6u);
    EXPECT_EQ(header.nalUnitType, 23u);
    EXPECT_EQ(header.temporalIdPlus1, 1u);
}

TEST(NalHeaderParse, AnnexBVVCSEIWithIds) // NOLINT
{
    const auto vvcHeader = makeVvcHeader(/*nalType=*/23, /*nuhLayerId=*/17, /*temporalIdPlus1=*/6);
    const auto nalu = withStartCode4({vvcHeader[0], vvcHeader[1]});

    ParsedNALHeader header{};
    ASSERT_TRUE(parseAnnexBHeader(nalu.data(), nalu.size(), BaseType::Enum::VVC, header));
    EXPECT_TRUE(isSEI(header));
    EXPECT_EQ(header.headerSize, 6u);
    EXPECT_EQ(header.nalUnitType, 23u);
    EXPECT_EQ(header.nuhLayerId, 17u);
    EXPECT_EQ(header.temporalIdPlus1, 6u);
}
