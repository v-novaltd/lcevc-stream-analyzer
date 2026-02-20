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
#include "helper/nal_unit.h"

#include "utility/bit_field.h"

namespace vnova::helper {

namespace {

    constexpr uint8_t kH264HeaderBytes = 1;
    constexpr uint8_t kHEVCHeaderBytes = 2;
    constexpr uint8_t kVVCHeaderBytes = 2;
    constexpr uint8_t kLCEVCHeaderBytes = 2;

    bool parseH264HeaderBytes(const uint8_t* headerPtr, const size_t length, ParsedNALHeader& out,
                              const uint32_t extraHeader)
    {
        if (length < kH264HeaderBytes) {
            return false;
        }

        utility::BitfieldDecoderStream<uint8_t> bits(headerPtr, kH264HeaderBytes);
        out.forbiddenZeroBit = bits.ReadBits<uint8_t>(1);
        if (out.forbiddenZeroBit != 0) {
            return false;
        }

        out.nalRefIdc = bits.ReadBits<uint8_t>(2);
        out.nalUnitValue = bits.ReadBits<uint8_t>(5);

        out.headerSize = kH264HeaderBytes + extraHeader;

        return true;
    }

    bool parseHEVCHeaderBytes(const uint8_t* headerPtr, const size_t length, ParsedNALHeader& out,
                              const uint32_t extraHeader)
    {
        if (length < kHEVCHeaderBytes) {
            return false;
        }

        utility::BitfieldDecoderStream<uint8_t> bits(headerPtr, kHEVCHeaderBytes);
        out.forbiddenZeroBit = bits.ReadBits<uint8_t>(1);
        if (out.forbiddenZeroBit != 0) {
            return false;
        }

        out.nalUnitValue = bits.ReadBits<uint8_t>(6);
        out.nuhLayerId = bits.ReadBits<uint8_t>(6);
        out.temporalIdPlus1 = bits.ReadBits<uint8_t>(3);

        out.headerSize = kHEVCHeaderBytes + extraHeader;

        return true;
    }

    bool parseVVCHeaderBytes(const uint8_t* headerPtr, const size_t length, ParsedNALHeader& out,
                             const uint32_t extraHeader)
    {
        if (length < kVVCHeaderBytes) {
            return false;
        }

        utility::BitfieldDecoderStream<uint8_t> bits(headerPtr, kVVCHeaderBytes);
        out.forbiddenZeroBit = bits.ReadBits<uint8_t>(1);
        if (out.forbiddenZeroBit != 0) {
            return false;
        }

        bits.ReadBits<uint8_t>(1); // reserved_bit (ignored)
        out.nuhLayerId = bits.ReadBits<uint8_t>(6);
        out.nalUnitValue = bits.ReadBits<uint8_t>(5);
        out.temporalIdPlus1 = bits.ReadBits<uint8_t>(3);

        out.headerSize = kVVCHeaderBytes + extraHeader;

        return true;
    }

    bool parseLCEVCHeaderBytes(const uint8_t* data, const size_t length, ParsedNALHeader& out,
                               const uint32_t extraHeader)
    {
        if (length < kLCEVCHeaderBytes) {
            return false;
        }

        utility::BitfieldDecoderStream<uint8_t> bits(data, kLCEVCHeaderBytes);
        const auto forbiddenZeroBit = bits.ReadBits<uint8_t>(1);
        const auto forbiddenOneBit = bits.ReadBits<uint8_t>(1);
        out.nalUnitValue = bits.ReadBits<uint8_t>(5);
        const auto reservedFlag = bits.ReadBits<uint16_t>(9);

        if (forbiddenZeroBit != 0 || forbiddenOneBit != 1 || reservedFlag != 0x1FF) {
            return false;
        }

        out.forbiddenZeroBit = forbiddenZeroBit;
        out.headerSize = kLCEVCHeaderBytes + extraHeader;

        return true;
    }

} // namespace

bool matchAnnexBStartCode(const uint8_t* data, size_t length, uint8_t& startCodeSize)
{
    startCodeSize = 0;

    if (!data) {
        return false;
    }

    if (length >= 4 && (memcmp(data, kFourByteStartCode.data(), kFourByteStartCode.size())) == 0) {
        startCodeSize = 4;
        return true;
    }

    if (length >= 3 && (memcmp(data, kThreeByteStartCode.data(), kThreeByteStartCode.size()) == 0)) {
        startCodeSize = 3;
        return true;
    }

    return false;
}

bool parseAnnexBHeader(const uint8_t* data, size_t length, CodecType codec, ParsedNALHeader& out)
{
    out = ParsedNALHeader{};
    out.codec = codec;

    uint8_t startCodeSize = 0;
    if (!matchAnnexBStartCode(data, length, startCodeSize)) {
        return false;
    }

    size_t remain = length - startCodeSize;
    const uint8_t* payloadPtr = data + startCodeSize;
    out.startCode = (startCodeSize == 4) ? AnnexBStartCode::ANNEX_B_4 : AnnexBStartCode::ANNEX_B_3;

    switch (codec) {
        case CodecType::H264: return parseH264HeaderBytes(payloadPtr, remain, out, startCodeSize);
        case CodecType::HEVC: return parseHEVCHeaderBytes(payloadPtr, remain, out, startCodeSize);
        case CodecType::VVC: return parseVVCHeaderBytes(payloadPtr, remain, out, startCodeSize);
        case CodecType::LCEVC: return parseLCEVCHeaderBytes(payloadPtr, remain, out, startCodeSize);
        default: return false;
    }
}

bool parseLengthPrefixedHeader(const uint8_t* data, size_t length, CodecType codec, ParsedNALHeader& out)
{
    out = ParsedNALHeader{};
    out.codec = codec;
    out.startCode = AnnexBStartCode::NOT_ANNEX_B;

    switch (codec) {
        case CodecType::H264: return parseH264HeaderBytes(data, length, out, 0);
        case CodecType::HEVC: return parseHEVCHeaderBytes(data, length, out, 0);
        case CodecType::VVC: return parseVVCHeaderBytes(data, length, out, 0);
        case CodecType::LCEVC: return parseLCEVCHeaderBytes(data, length, out, 0);
        default: return false;
    }
}

bool isSEI(const ParsedNALHeader& header)
{
    switch (header.codec) {
        case CodecType::H264:
            return header.nalUnitValue == static_cast<int8_t>(NalUnitTypeH264::SEI); // SEI
        case CodecType::HEVC:
            return header.nalUnitValue == static_cast<int8_t>(NalUnitTypeHEVC::PREFIX_SEI) ||
                   header.nalUnitValue == static_cast<int8_t>(NalUnitTypeHEVC::SUFFIX_SEI);
        case CodecType::VVC:
            return header.nalUnitValue == static_cast<int8_t>(NalUnitTypeVVC::PREFIX_SEI) ||
                   header.nalUnitValue == static_cast<int8_t>(NalUnitTypeVVC::SUFFIX_SEI);
        default: return false;
    }
}

bool isSPS(const ParsedNALHeader& header)
{
    switch (header.codec) {
        case CodecType::H264:
            return header.nalUnitValue == static_cast<int8_t>(NalUnitTypeH264::SPS);
        case CodecType::HEVC:
            return header.nalUnitValue == static_cast<int8_t>(NalUnitTypeHEVC::SPS_NUT);
        case CodecType::VVC: return header.nalUnitValue == static_cast<int8_t>(NalUnitTypeVVC::SPS);
        default: return false;
    }
}

bool isPPS(const ParsedNALHeader& header)
{
    switch (header.codec) {
        case CodecType::H264:
            return header.nalUnitValue == static_cast<int8_t>(NalUnitTypeH264::PPS);
        case CodecType::HEVC:
            return header.nalUnitValue == static_cast<int8_t>(NalUnitTypeHEVC::PPS_NUT);
        case CodecType::VVC: return header.nalUnitValue == static_cast<int8_t>(NalUnitTypeVVC::PPS);
        default: return false;
    }
}

bool isVPS(const ParsedNALHeader& header)
{
    switch (header.codec) {
        case CodecType::HEVC:
            return header.nalUnitValue == static_cast<int8_t>(NalUnitTypeHEVC::VPS_NUT);
        case CodecType::VVC: return header.nalUnitValue == static_cast<int8_t>(NalUnitTypeVVC::VPS);
        default: return false;
    }
}

bool isAUD(const ParsedNALHeader& header)
{
    switch (header.codec) {
        case CodecType::H264:
            return header.nalUnitValue == static_cast<int8_t>(NalUnitTypeH264::AUD);
        case CodecType::HEVC:
            return header.nalUnitValue == static_cast<int8_t>(NalUnitTypeHEVC::AUD_NUT);
        case CodecType::VVC: return header.nalUnitValue == static_cast<int8_t>(NalUnitTypeVVC::AUD);
        default: return false;
    }
}

bool isLcevcIDR(const ParsedNALHeader& header)
{
    return header.codec == CodecType::LCEVC &&
           header.nalUnitValue == static_cast<uint8_t>(FrameTypeLCEVC::IDR);
}

bool isLcevcNonIDR(const ParsedNALHeader& header)
{
    return header.codec == CodecType::LCEVC &&
           header.nalUnitValue == static_cast<uint8_t>(FrameTypeLCEVC::NON_IDR);
}

FrameTypeLCEVC parseHeaderAutoLCEVC(const uint8_t* data, size_t length, size_t& payloadOffset)
{
    payloadOffset = 0;
    ParsedNALHeader hdr{};

    // 1) Annex B probe (3 or 4 byte start code)
    // Must have at least the 3 AnnexB prefix + 2 header bytes
    if (length < 5) {
        return FrameTypeLCEVC::INVALID;
    }
    if (parseAnnexBHeader(data, length, CodecType::LCEVC, hdr)) {
        if (isLcevcIDR(hdr)) {
            payloadOffset = hdr.headerSize;
            return FrameTypeLCEVC::IDR;
        }
        if (isLcevcNonIDR(hdr)) {
            payloadOffset = hdr.headerSize;
            return FrameTypeLCEVC::NON_IDR;
        }
    }

    // 2) Length-prefixed probe (hard-coded 4 byte length)
    // Must have at least the 4 length prefix + 2 header bytes
    if (length < 6) {
        return FrameTypeLCEVC::INVALID;
    }
    const uint32_t nalLen = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    if (nalLen == 0 || nalLen > length - 4) {
        return FrameTypeLCEVC::INVALID;
    }

    if (parseLengthPrefixedHeader(data + 4, nalLen, CodecType::LCEVC, hdr)) {
        if (isLcevcIDR(hdr)) {
            payloadOffset = 6;
            return FrameTypeLCEVC::IDR;
        }
        if (isLcevcNonIDR(hdr)) {
            payloadOffset = 6;
            return FrameTypeLCEVC::NON_IDR;
        }
    }

    return FrameTypeLCEVC::INVALID;
}

const char* toString(const uint8_t nalUnitType, const CodecType& codec)
{
    switch (nalUnitType) {
        case static_cast<uint8_t>(NalUnitTypeLCEVC::DATA):
        case static_cast<uint8_t>(NalUnitTypeLCEVC::CONF):
            return toString(static_cast<NalUnitTypeLCEVC>(nalUnitType));
        default:
            switch (codec) {
                case CodecType::H264: return toString(static_cast<NalUnitTypeH264>(nalUnitType));
                case CodecType::HEVC: return toString(static_cast<NalUnitTypeHEVC>(nalUnitType));
                case CodecType::VVC: return toString(static_cast<NalUnitTypeVVC>(nalUnitType));
                default: return "???";
            }
    }
}

const char* toString(const uint8_t nalUnitType, const BaseType& codec)
{
    return toString(nalUnitType, toCodecType(codec));
}

const char* toString(const uint8_t nalUnitType)
{
    return toString(static_cast<NalUnitTypeVVC>(nalUnitType));
}

bool isSEINALUnit(const BaseType baseType, const uint8_t* data, const size_t length, uint32_t& headerSize)
{
    if (data == nullptr) {
        throw InvalidDataPointerError();
    }

    if (ParsedNALHeader hdr; parseAnnexBHeader(data, length, baseType, hdr)) {
        headerSize = hdr.headerSize;
        return isSEI(hdr);
    }

    headerSize = 0;
    return false;
}

bool isLCEVCITUPayload(const uint8_t* data)
{
    if (data == nullptr) {
        throw InvalidDataPointerError();
    }
    return memcmp(data, kLCEVCITUHeader.data(), kLCEVCITUHeader.size()) == 0;
}

uint32_t nalUnencapsulate(uint8_t* dst, const uint8_t* src, size_t length)
{
    // Last byte contains RBSP stop-bit
    const auto* end = src + length - 1;
    auto zeroes = 0u;
    auto size = 0u;

    // Check if last byte is the RBSP stop-bit
    if (*end != 0x80) {
        // Walk back over trailing_zero_8bits to find the RBSP stop-bit
        while (*end == 0x00) {
            --end;
        }
        // Last byte before the trailing_zero_8bits must be RBSP stop-bit
        if (*end != 0x80) {
            // @todo: Add error mechanism to this function?
            return 0;
        }
    }

    while (src != end) {
        auto byte = *(src++);

        if (zeroes == 2 && byte == 3) {
            zeroes = 0;
            continue; // skip it
        }

        if (byte == 0) {
            ++zeroes;
        } else {
            zeroes = 0;
        }

        if (dst != nullptr) {
            *(dst++) = byte;
        }

        ++size;
    }

    return size;
}

} // namespace vnova::helper
