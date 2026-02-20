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
#ifndef VN_UTILITY_NAL_HEADER_H_
#define VN_UTILITY_NAL_HEADER_H_

#include "helper/base_type.h"
#include "utility/types_util.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace vnova::helper {

constexpr auto kThreeByteStartCode = utility::constructByteArray(0x00, 0x00, 0x01);
constexpr auto kFourByteStartCode = utility::constructByteArray(0x00, 0x00, 0x00, 0x01);

constexpr auto kRegisterUserDataSEIType = utility::constructByteArray(0x04);
constexpr auto kLCEVCITUHeader = utility::constructByteArray(0xb4, 0x00, 0x50, 0x00);

template <typename T>
bool isReservedNalUnitType(T value);

template <typename T>
bool isUnspecifiedNalUnitType(T value);

enum class AnnexBStartCode : uint8_t
{
    NOT_ANNEX_B = 0,
    ANNEX_B_3 = 3,
    ANNEX_B_4 = 4
};

// CodecType covers MPEG base codecs and LCEVC so common parsing utilities
// can operate across all supported NAL unit types.
enum class CodecType : int8_t
{
    UNKNOWN = -1,
    H264 = 1,
    HEVC = 2,
    VVC = 3,
    LCEVC = 4,
};

enum class NalUnitTypeLCEVC : int8_t
{
    DATA = 25,
    CONF = 27,
};
constexpr const char* toString(const NalUnitTypeLCEVC nalUnitType)
{
    switch (nalUnitType) {
        case NalUnitTypeLCEVC::DATA: return "DATA";
        case NalUnitTypeLCEVC::CONF: return "CONF";
    }
    return "???";
}

enum class NalUnitTypeH264 : int8_t
{
    SLICE = 1,
    SLICE_DPA = 2,
    SLICE_DPB = 3,
    SLICE_DPC = 4,
    IDR_SLICE = 5,
    SEI = 6,
    SPS = 7,
    PPS = 8,
    AUD = 9,
    E_O_SEQ = 10,
    E_O_STRM = 11,
    FILLER = 12,
    SPS_EXT = 13,
    PFX_NAL = 14,
    SUB_SPS = 15,
    AUX_SLICE = 19,
    EXTN_SLICE = 20,
};
template <>
constexpr bool isReservedNalUnitType<NalUnitTypeH264>(NalUnitTypeH264 value)
{
    return utility::staticContains<NalUnitTypeH264>({0, 24, 25, 26, 27, 28, 29, 30, 31}, value);
}
template <>
constexpr bool isUnspecifiedNalUnitType<NalUnitTypeH264>(NalUnitTypeH264 value)
{
    return utility::staticContains<NalUnitTypeH264>({16, 17, 18, 21, 22, 23}, value);
}
constexpr const char* toString(const NalUnitTypeH264 nalUnitType)
{
    if (isUnspecifiedNalUnitType(nalUnitType)) {
        return "*UNSPECIFIED*";
    }
    if (isReservedNalUnitType(nalUnitType)) {
        return "*RESERVED*";
    }

    switch (nalUnitType) {
        case NalUnitTypeH264::SLICE: return "SLICE";
        case NalUnitTypeH264::SLICE_DPA: return "SLICE_DPA";
        case NalUnitTypeH264::SLICE_DPB: return "SLICE_DPB";
        case NalUnitTypeH264::SLICE_DPC: return "SLICE_DPC";
        case NalUnitTypeH264::IDR_SLICE: return "IDR_SLICE";
        case NalUnitTypeH264::SEI: return "SEI";
        case NalUnitTypeH264::SPS: return "SPS";
        case NalUnitTypeH264::PPS: return "PPS";
        case NalUnitTypeH264::AUD: return "AUD";
        case NalUnitTypeH264::E_O_SEQ: return "E_O_SEQ";
        case NalUnitTypeH264::E_O_STRM: return "E_O_STRM";
        case NalUnitTypeH264::FILLER: return "FILLER";
        case NalUnitTypeH264::SPS_EXT: return "SPS_EXT";
        case NalUnitTypeH264::PFX_NAL: return "PFX_NAL";
        case NalUnitTypeH264::SUB_SPS: return "SUB_SPS";
        case NalUnitTypeH264::AUX_SLICE: return "AUX_SLICE";
        case NalUnitTypeH264::EXTN_SLICE: return "EXTN_SLICE";
    }
    return "???";
}

enum class NalUnitTypeHEVC : int8_t
{
    TRAIL_N = 0,
    TRAIL_R = 1,
    TSA_N = 2,
    TSA_R = 3,
    STSA_N = 4,
    STSA_R = 5,
    RADL_N = 6,
    RADL_R = 7,
    RASL_N = 8,
    RASL_R = 9,
    BLA_W_LP = 16,
    BLA_W_RADL = 17,
    BLA_N_LP = 18,
    IDR_W_RADL = 19,
    IDR_N_LP = 20,
    CRA_NUT = 21,
    VPS_NUT = 32,
    SPS_NUT = 33,
    PPS_NUT = 34,
    AUD_NUT = 35,
    EOS_NUT = 36,
    EOB_NUT = 37,
    FD_NUT = 38,
    PREFIX_SEI = 39,
    SUFFIX_SEI = 40,
};
template <>
constexpr bool isReservedNalUnitType<NalUnitTypeHEVC>(NalUnitTypeHEVC value)
{
    return utility::staticContains<NalUnitTypeHEVC>({10, 11, 12, 13, 14, 15, 22, 23, 24, 25, 26, 27,
                                                     28, 29, 30, 31, 41, 42, 43, 44, 45, 46, 47},
                                                    value);
}
template <>
constexpr bool isUnspecifiedNalUnitType<NalUnitTypeHEVC>(NalUnitTypeHEVC value)
{
    return utility::staticContains<NalUnitTypeHEVC>(
        {48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63}, value);
}
constexpr const char* toString(const NalUnitTypeHEVC nalUnitType)
{
    if (isUnspecifiedNalUnitType(nalUnitType)) {
        return "*UNSPECIFIED*";
    }
    if (isReservedNalUnitType(nalUnitType)) {
        return "*RESERVED*";
    }

    switch (nalUnitType) {
        case NalUnitTypeHEVC::TRAIL_N: return "TRAIL_N";
        case NalUnitTypeHEVC::TRAIL_R: return "TRAIL_R";
        case NalUnitTypeHEVC::TSA_N: return "TSA_N";
        case NalUnitTypeHEVC::TSA_R: return "TSA_R";
        case NalUnitTypeHEVC::STSA_N: return "STSA_N";
        case NalUnitTypeHEVC::STSA_R: return "STSA_R";
        case NalUnitTypeHEVC::RADL_N: return "RADL_N";
        case NalUnitTypeHEVC::RADL_R: return "RADL_R";
        case NalUnitTypeHEVC::RASL_N: return "RASL_N";
        case NalUnitTypeHEVC::RASL_R: return "RASL_R";
        case NalUnitTypeHEVC::BLA_W_LP: return "BLA_W_LP";
        case NalUnitTypeHEVC::BLA_W_RADL: return "BLA_W_RADL";
        case NalUnitTypeHEVC::BLA_N_LP: return "BLA_N_LP";
        case NalUnitTypeHEVC::IDR_W_RADL: return "IDR_W_RADL";
        case NalUnitTypeHEVC::IDR_N_LP: return "IDR_N_LP";
        case NalUnitTypeHEVC::CRA_NUT: return "CRA_NUT";
        case NalUnitTypeHEVC::VPS_NUT: return "VPS_NUT";
        case NalUnitTypeHEVC::SPS_NUT: return "SPS_NUT";
        case NalUnitTypeHEVC::PPS_NUT: return "PPS_NUT";
        case NalUnitTypeHEVC::AUD_NUT: return "AUD_NUT";
        case NalUnitTypeHEVC::EOS_NUT: return "EOS_NUT";
        case NalUnitTypeHEVC::EOB_NUT: return "EOB_NUT";
        case NalUnitTypeHEVC::FD_NUT: return "FD_NUT";
        case NalUnitTypeHEVC::PREFIX_SEI: return "PREFIX_SEI";
        case NalUnitTypeHEVC::SUFFIX_SEI: return "SUFFIX_SEI";
    }
    return "???";
}

enum class NalUnitTypeVVC : int8_t
{
    TRAIL = 0,
    STSA = 1,
    RADL = 2,
    RASL = 3,
    IDR_W_RADL = 7,
    IDR_N_LP = 8,
    CRA = 9,
    GDR = 10,
    OPI = 12,
    DCI = 13,
    VPS = 14,
    SPS = 15,
    PPS = 16,
    PREFIX_APS = 17,
    SUFFIX_APS = 18,
    PH = 19,
    AUD = 20,
    EOS = 21,
    EOB = 22,
    PREFIX_SEI = 23,
    SUFFIX_SEI = 24,
    FD = 25,
};
template <>
constexpr bool isReservedNalUnitType<NalUnitTypeVVC>(NalUnitTypeVVC value)
{
    return utility::staticContains<NalUnitTypeVVC>({4, 5, 6, 11, 26, 27}, value);
}
template <>
constexpr bool isUnspecifiedNalUnitType<NalUnitTypeVVC>(NalUnitTypeVVC value)
{
    return utility::staticContains<NalUnitTypeVVC>({28, 29, 30, 31}, value);
}
constexpr const char* toString(const NalUnitTypeVVC nalUnitType)
{
    if (isUnspecifiedNalUnitType(nalUnitType)) {
        return "*UNSPECIFIED*";
    }
    if (isReservedNalUnitType(nalUnitType)) {
        return "*RESERVED*";
    }

    switch (nalUnitType) {
        case NalUnitTypeVVC::TRAIL: return "TRAIL";
        case NalUnitTypeVVC::STSA: return "STSA";
        case NalUnitTypeVVC::RADL: return "RADL";
        case NalUnitTypeVVC::RASL: return "RASL";
        case NalUnitTypeVVC::IDR_W_RADL: return "IDR_W_RADL";
        case NalUnitTypeVVC::IDR_N_LP: return "IDR_N_LP";
        case NalUnitTypeVVC::CRA: return "CRA";
        case NalUnitTypeVVC::GDR: return "GDR";
        case NalUnitTypeVVC::OPI: return "OPI";
        case NalUnitTypeVVC::DCI: return "DCI";
        case NalUnitTypeVVC::VPS: return "VPS";
        case NalUnitTypeVVC::SPS: return "SPS";
        case NalUnitTypeVVC::PPS: return "PPS";
        case NalUnitTypeVVC::PREFIX_APS: return "PREFIX_APS";
        case NalUnitTypeVVC::SUFFIX_APS: return "SUFFIX_APS";
        case NalUnitTypeVVC::PH: return "PH";
        case NalUnitTypeVVC::AUD: return "AUD";
        case NalUnitTypeVVC::EOS: return "EOS";
        case NalUnitTypeVVC::EOB: return "EOB";
        case NalUnitTypeVVC::PREFIX_SEI: return "PREFIX_SEI";
        case NalUnitTypeVVC::SUFFIX_SEI: return "SUFFIX_SEI";
        case NalUnitTypeVVC::FD: return "FD";
    }
    return "???";
}

// Forward declaration so overloads below can call it
inline CodecType toCodecType(BaseType base);

struct ParsedNALHeader
{
    CodecType codec = CodecType::UNKNOWN;
    AnnexBStartCode startCode = AnnexBStartCode::NOT_ANNEX_B; // Annex B only
    uint8_t forbiddenZeroBit = 0;                             // where applicable
    uint8_t nalUnitValue = 0;                                 // codec-specific width
    uint8_t nuhLayerId = 0;                                   // HEVC/VVC
    uint8_t temporalIdPlus1 = 0;                              // HEVC/VVC
    uint8_t nalRefIdc = 0;                                    // H.264
    uint32_t headerSize = 0; // bytes consumed incl. start code (Annex B)
};

class InvalidDataPointerError : public std::runtime_error
{
public:
    InvalidDataPointerError()
        : std::runtime_error("Invalid data pointer")
    {}
};

// Detects a 4-byte or 3-byte Annex B start code at `data`.
// Returns true and sets `startCodeSize` when matched.
bool matchAnnexBStartCode(const uint8_t* data, size_t length, uint8_t& startCodeSize);
inline bool matchAnnexBStartCode(const utility::DataBuffer& buf, uint8_t& startCodeSize)
{
    return matchAnnexBStartCode(buf.data(), buf.size(), startCodeSize);
}

// Parses a NAL header for Annex B input beginning at `data`.
// Fills `out` and returns true when a valid header is parsed.
bool parseAnnexBHeader(const uint8_t* data, size_t length, CodecType codec, ParsedNALHeader& out);
// Convenience overload when the parsed header fields are not needed
inline bool parseAnnexBHeader(const uint8_t* data, size_t length, CodecType codec)
{
    ParsedNALHeader discard;
    return parseAnnexBHeader(data, length, codec, discard);
}
// Overloads for BaseType to avoid explicit toCodecType() at call sites
inline bool parseAnnexBHeader(const uint8_t* data, size_t length, BaseType base, ParsedNALHeader& out)
{
    return parseAnnexBHeader(data, length, toCodecType(base), out);
}
inline bool parseAnnexBHeader(const uint8_t* data, size_t length, BaseType base)
{
    return parseAnnexBHeader(data, length, toCodecType(base));
}
inline bool parseAnnexBHeader(const utility::DataBuffer& buf, CodecType codec, ParsedNALHeader& out)
{
    return parseAnnexBHeader(buf.data(), buf.size(), codec, out);
}
inline bool parseAnnexBHeader(const utility::DataBuffer& buf, CodecType codec)
{
    return parseAnnexBHeader(buf.data(), buf.size(), codec);
}

// Parses a NAL header for length-prefixed input (MP4/WebM). `data` points to the
// first header byte of the NAL unit payload. `out.headerSize` excludes any
// length field (caller already consumed it).
bool parseLengthPrefixedHeader(const uint8_t* data, size_t length, CodecType codec, ParsedNALHeader& out);
inline bool parseLengthPrefixedHeader(const uint8_t* data, size_t length, CodecType codec)
{
    ParsedNALHeader discard;
    return parseLengthPrefixedHeader(data, length, codec, discard);
}
// Overload for BaseType to avoid explicit toCodecType() at call sites
inline bool parseLengthPrefixedHeader(const uint8_t* data, size_t length, BaseType base, ParsedNALHeader& out)
{
    return parseLengthPrefixedHeader(data, length, toCodecType(base), out);
}

enum class FrameTypeLCEVC : int8_t
{
    INVALID = -1,
    /* 0 - 27: Unspecified */
    NON_IDR = 28,
    IDR = 29,
    /* 30: Reserved Level */
    /* 31: Unspecified */
};
constexpr const char* toString(FrameTypeLCEVC value)
{
    switch (value) {
        case FrameTypeLCEVC::NON_IDR: return "NON";
        case FrameTypeLCEVC::IDR: return "IDR";
        case FrameTypeLCEVC::INVALID: return "INVALID";
    }

    return "Invalid";
}

FrameTypeLCEVC parseHeaderAutoLCEVC(const uint8_t* data, size_t length, size_t& payloadOffset);

// Convenience helpers
bool isSEI(const ParsedNALHeader& header);
bool isSPS(const ParsedNALHeader& header);
bool isPPS(const ParsedNALHeader& header);
bool isVPS(const ParsedNALHeader& header);
bool isAUD(const ParsedNALHeader& header);
bool isLcevcIDR(const ParsedNALHeader& header);
bool isLcevcNonIDR(const ParsedNALHeader& header);

// Helper to convert from BaseType (existing analyzer concept) to CodecType
inline CodecType toCodecType(BaseType base)
{
    switch (base) {
        case BaseType::H264: return CodecType::H264;
        case BaseType::HEVC: return CodecType::HEVC;
        case BaseType::VVC: return CodecType::VVC;
        case BaseType::VP8:
        case BaseType::VP9:
        case BaseType::VC6:
        case BaseType::AV1:
        case BaseType::UNKNOWN:
        case BaseType::INVALID: return CodecType::UNKNOWN;
    }
    return CodecType::UNKNOWN;
}

const char* toString(uint8_t nalUnitType, const CodecType& codec);
const char* toString(uint8_t nalUnitType, const BaseType& codec);

const char* toString(uint8_t nalUnitType);

bool isSEINALUnit(BaseType baseType, const uint8_t* data, size_t length, uint32_t& headerSize);

bool isLCEVCITUPayload(const uint8_t* data);

// Unencapsulates a payload, replacing any sequence of [0, 0, 3, 0/1/2/3] with
// [0, 0, 0/1/2/3]. dst may be nullptr to determine the length of the
// unencapsulated data. Returns number of bytes written
uint32_t nalUnencapsulate(uint8_t* dst, const uint8_t* src, size_t length);

} // namespace vnova::helper

#endif // VN_UTILITY_NAL_HEADER_H_
