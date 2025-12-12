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
#ifndef VN_UTILITY_NAL_HEADER_H_
#define VN_UTILITY_NAL_HEADER_H_

#include "utility/base_type.h"
#include "utility/types_util.h"

#include <cstddef>
#include <cstdint>

namespace vnova::utility {

enum class AnnexBStartCode : uint8_t
{
    None = 0,
    Three = 3,
    Four = 4
};

// CodecType covers base codecs and LCEVC so common parsing utilities
// can operate across all supported NAL unit types.
enum class CodecType : uint8_t
{
    H264,
    HEVC,
    VVC,
    LCEVC,
    Unknown
};

// Forward declaration so overloads below can call it
inline CodecType toCodecType(BaseType::Enum base);

struct ParsedNALHeader
{
    CodecType codec = CodecType::Unknown;
    AnnexBStartCode startCode = AnnexBStartCode::None; // Annex B only
    uint8_t forbiddenZeroBit = 0;                      // where applicable
    uint8_t nalUnitType = 0;                           // codec-specific width
    uint8_t nuhLayerId = 0;                            // HEVC/VVC
    uint8_t temporalIdPlus1 = 0;                       // HEVC/VVC
    uint8_t nalRefIdc = 0;                             // H.264
    uint32_t headerSize = 0;                           // bytes consumed incl. start code (Annex B)
};

// Detects a 4-byte or 3-byte Annex B start code at `data`.
// Returns true and sets `startCodeSize` when matched.
bool matchAnnexBStartCode(const uint8_t* data, size_t length, uint32_t& startCodeSize);
inline bool matchAnnexBStartCode(const DataBuffer& buf, uint32_t& startCodeSize)
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
inline bool parseAnnexBHeader(const uint8_t* data, size_t length, BaseType::Enum base, ParsedNALHeader& out)
{
    return parseAnnexBHeader(data, length, toCodecType(base), out);
}
inline bool parseAnnexBHeader(const uint8_t* data, size_t length, BaseType::Enum base)
{
    return parseAnnexBHeader(data, length, toCodecType(base));
}
inline bool parseAnnexBHeader(const DataBuffer& buf, CodecType codec, ParsedNALHeader& out)
{
    return parseAnnexBHeader(buf.data(), buf.size(), codec, out);
}
inline bool parseAnnexBHeader(const DataBuffer& buf, CodecType codec)
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
inline bool parseLengthPrefixedHeader(const uint8_t* data, size_t length, BaseType::Enum base,
                                      ParsedNALHeader& out)
{
    return parseLengthPrefixedHeader(data, length, toCodecType(base), out);
}

// Convenience helpers
bool isSEI(const ParsedNALHeader& header);
bool isSPS(const ParsedNALHeader& header);
bool isPPS(const ParsedNALHeader& header);
bool isVPS(const ParsedNALHeader& header);
bool isAUD(const ParsedNALHeader& header);
bool isLcevcIDR(const ParsedNALHeader& header);
bool isLcevcNonIDR(const ParsedNALHeader& header);

// Helper to convert from BaseType (existing analyzer concept) to CodecType
inline CodecType toCodecType(BaseType::Enum base)
{
    switch (base) {
        case BaseType::Enum::H264: return CodecType::H264;
        case BaseType::Enum::HEVC: return CodecType::HEVC;
        case BaseType::Enum::VVC: return CodecType::VVC;
        default: return CodecType::Unknown;
    }
}

} // namespace vnova::utility

#endif // VN_UTILITY_NAL_HEADER_H_
