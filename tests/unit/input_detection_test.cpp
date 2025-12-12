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

#include "config_types.h"
#include "utility/input_detection.h"

#include <gtest/gtest.h>

using namespace vnova::analyzer;
using vnova::utility::BaseType;

namespace {
// Helper to build Annex B NALU with start code and header bytes
std::vector<uint8_t> makeAnnexBNal(const std::vector<uint8_t>& body)
{
    static constexpr uint8_t startCode[] = {0x00, 0x00, 0x01};
    std::vector<uint8_t> data;
    data.insert(data.end(), std::begin(startCode), std::end(startCode));
    data.insert(data.end(), body.begin(), body.end());
    return data;
}
} // namespace

TEST(InputDetection, DetectsBinMagic) // NOLINT
{
    const uint8_t magic[] = {0x6C, 0x63, 0x65, 0x76, 0x63, 0x62, 0x69, 0x6E};
    auto result = detectInputFormatFromMemory(magic, sizeof(magic));
    EXPECT_EQ(result.inputType, InputType::BIN);
}

TEST(InputDetection, DetectsMp4Ftyp) // NOLINT
{
    // minimal ftyp: size=24, "ftyp", then dummy
    const uint8_t ftyp[] = {0x00, 0x00, 0x00, 0x18, 'f', 't', 'y', 'p', 'i', 's', 'o', 'm',
                            0x00, 0x00, 0x02, 0x00, 'i', 's', 'o', 'm', 'a', 'v', 'c', '1'};
    auto result = detectInputFormatFromMemory(ftyp, sizeof(ftyp));
    EXPECT_EQ(result.inputType, InputType::MP4);
}

TEST(InputDetection, DetectsMpegTs) // NOLINT
{
    std::vector<uint8_t> buf(static_cast<std::vector<uint8_t>::size_type>(188) * 3U, 0xFF);
    buf[0] = buf[188] = buf[static_cast<std::vector<uint8_t>::size_type>(188) * 2U] = 0x47;
    auto result = detectInputFormatFromMemory(buf.data(), buf.size());
    EXPECT_EQ(result.inputType, InputType::TS);
}

TEST(InputDetection, DetectsWebmEbml) // NOLINT
{
    const uint8_t ebml[] = {0x1A, 0x45, 0xDF, 0xA3, 0x00};
    auto result = detectInputFormatFromMemory(ebml, sizeof(ebml));
    EXPECT_EQ(result.inputType, InputType::WEBM);
}

TEST(InputDetection, DetectsH264AnnexB) // NOLINT
{
    // H.264 header: forbidden_zero_bit=0 nal_ref_idc=3 nal_unit_type=7 (SPS)
    auto nalu = makeAnnexBNal({0x67, 0x00, 0x00});
    auto result = detectInputFormatFromMemory(nalu.data(), nalu.size());
    EXPECT_EQ(result.inputType, InputType::ELEMENTARY);
    EXPECT_EQ(result.baseType, BaseType::Enum::H264);
    EXPECT_TRUE(result.isLikelyAnnexB);
}

TEST(InputDetection, DetectsHevcAnnexB) // NOLINT
{
    // HEVC header: forbidden_zero_bit=0 nal_unit_type=33 (SPS), nuh_layer_id=0, temporal_id_plus1=1
    auto nalu = makeAnnexBNal({0x42, 0x01, 0x01});
    auto result = detectInputFormatFromMemory(nalu.data(), nalu.size());
    EXPECT_EQ(result.inputType, InputType::ELEMENTARY);
    EXPECT_EQ(result.baseType, BaseType::Enum::HEVC);
    EXPECT_TRUE(result.isLikelyAnnexB);
}

TEST(InputDetection, DetectsVvcAnnexB) // NOLINT
{
    // VVC header: forbidden_zero_bit=0 reserved=0 nuh_layer_id=0 nal_unit_type=15 (SPS), temporal_id_plus1=1
    auto nalu = makeAnnexBNal({0x00, 0x79}); // forbidden=0, reserved=0, layer=0, nal_type=15 (SPS), tid=1
    auto result = detectInputFormatFromMemory(nalu.data(), nalu.size());
    EXPECT_EQ(result.inputType, InputType::ELEMENTARY);
    EXPECT_EQ(result.baseType, BaseType::Enum::VVC);
    EXPECT_TRUE(result.isLikelyAnnexB);
}

TEST(InputDetection, DetectsLcevcAnnexB) // NOLINT
{
    // LCEVC header: forbidden_zero_bit=0 forbidden_one_bit=1 nal_unit_type=29 (NonIDR) reserved=0x1FF
    auto nalu = makeAnnexBNal({0x7B, 0xFF});
    auto result = detectInputFormatFromMemory(nalu.data(), nalu.size());
    EXPECT_EQ(result.inputType, InputType::LCEVC);
    EXPECT_TRUE(result.isLikelyAnnexB);
}

TEST(InputDetection, DetectsAvccLengthPrefixed) // NOLINT
{
    // length-prefixed H.264 SPS (4-byte length = 4)
    const uint8_t data[] = {0x00, 0x00, 0x00, 0x04, 0x67, 0x00, 0x00, 0x00};
    auto result = detectInputFormatFromMemory(data, sizeof(data));
    EXPECT_EQ(result.inputType, InputType::ELEMENTARY);
    EXPECT_EQ(result.baseType, BaseType::Enum::H264);
    EXPECT_TRUE(result.isLikelyAvcc);
}
