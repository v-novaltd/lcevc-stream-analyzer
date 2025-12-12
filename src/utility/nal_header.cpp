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
#include "nal_header.h"

#include "utility/bit_field.h"

namespace vnova::utility {

constexpr uint8_t kH264HeaderBytes = 1;
constexpr uint8_t kHEVCHeaderBytes = 2;
constexpr uint8_t kVVCHeaderBytes = 2;
constexpr uint8_t kLCEVCHeaderBytes = 2;

bool matchAnnexBStartCode(const uint8_t* data, size_t length, uint32_t& startCodeSize)
{
    startCodeSize = 0;
    if (!data) {
        return false;
    }
    if (length >= 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01) {
        startCodeSize = 4;
        return true;
    }
    if (length >= 3 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) {
        startCodeSize = 3;
        return true;
    }
    return false;
}

static void parseH264HeaderBytes(const uint8_t* headerPtr, ParsedNALHeader& out)
{
    vnova::BitfieldDecoderStream<uint8_t> bits(headerPtr, kH264HeaderBytes);
    out.forbiddenZeroBit = bits.readBits<uint8_t>(1);
    out.nalRefIdc = bits.readBits<uint8_t>(2);
    out.nalUnitType = bits.readBits<uint8_t>(5);
}

static void parseHEVCHeaderBytes(const uint8_t* headerPtr, ParsedNALHeader& out)
{
    vnova::BitfieldDecoderStream<uint8_t> bits(headerPtr, kHEVCHeaderBytes);
    out.forbiddenZeroBit = bits.readBits<uint8_t>(1);
    out.nalUnitType = bits.readBits<uint8_t>(6);
    out.nuhLayerId = bits.readBits<uint8_t>(6);
    out.temporalIdPlus1 = bits.readBits<uint8_t>(3);
}

static void parseVVCHeaderBytes(const uint8_t* headerPtr, ParsedNALHeader& out)
{
    vnova::BitfieldDecoderStream<uint8_t> bits(headerPtr, kVVCHeaderBytes);
    out.forbiddenZeroBit = bits.readBits<uint8_t>(1);
    bits.readBits<uint8_t>(1); // reserved_bit (ignored)
    out.nuhLayerId = bits.readBits<uint8_t>(6);
    out.nalUnitType = bits.readBits<uint8_t>(5);
    out.temporalIdPlus1 = bits.readBits<uint8_t>(3);
}

static bool parseLCEVCHeaderBytes(const uint8_t* headerPtr, ParsedNALHeader& out)
{
    vnova::BitfieldDecoderStream<uint8_t> bits(headerPtr, kLCEVCHeaderBytes);
    const auto forbiddenZeroBit = bits.readBits<uint8_t>(1);
    const auto forbiddenOneBit = bits.readBits<uint8_t>(1);
    out.nalUnitType = bits.readBits<uint8_t>(5);
    const auto reservedFlag = bits.readBits<uint16_t>(9);

    if (forbiddenZeroBit != 0 || forbiddenOneBit != 1 || reservedFlag != 0x1FF) {
        return false;
    }

    out.forbiddenZeroBit = forbiddenZeroBit;
    return true;
}

bool parseAnnexBHeader(const uint8_t* data, size_t length, CodecType codec, ParsedNALHeader& out)
{
    out = ParsedNALHeader{};
    out.codec = codec;

    uint32_t startCodeSize = 0;
    if (!matchAnnexBStartCode(data, length, startCodeSize)) {
        return false;
    }

    size_t remain = length - startCodeSize;
    const uint8_t* payloadPtr = data + startCodeSize;
    out.startCode = (startCodeSize == 4) ? AnnexBStartCode::Four : AnnexBStartCode::Three;

    switch (codec) {
        case CodecType::H264:
            if (remain < kH264HeaderBytes) {
                return false;
            }
            parseH264HeaderBytes(payloadPtr, out);
            out.headerSize = startCodeSize + kH264HeaderBytes;
            return true;
        case CodecType::HEVC:
            if (remain < kHEVCHeaderBytes) {
                return false;
            }
            parseHEVCHeaderBytes(payloadPtr, out);
            out.headerSize = startCodeSize + kHEVCHeaderBytes;
            return true;
        case CodecType::VVC:
            if (remain < kVVCHeaderBytes) {
                return false;
            }
            parseVVCHeaderBytes(payloadPtr, out);
            out.headerSize = startCodeSize + kVVCHeaderBytes;
            return true;
        case CodecType::LCEVC: {
            if (remain < kLCEVCHeaderBytes) {
                return false;
            }
            if (!parseLCEVCHeaderBytes(payloadPtr, out)) {
                return false;
            }
            out.headerSize = startCodeSize + kLCEVCHeaderBytes;
            return true;
        }
        default: return false;
    }
}

bool parseLengthPrefixedHeader(const uint8_t* data, size_t length, CodecType codec, ParsedNALHeader& out)
{
    out = ParsedNALHeader{};
    out.codec = codec;
    out.startCode = AnnexBStartCode::None;

    switch (codec) {
        case CodecType::H264:
            if (length < kH264HeaderBytes) {
                return false;
            }
            parseH264HeaderBytes(data, out);
            out.headerSize = kH264HeaderBytes;
            return true;
        case CodecType::HEVC:
            if (length < kHEVCHeaderBytes) {
                return false;
            }
            parseHEVCHeaderBytes(data, out);
            out.headerSize = kHEVCHeaderBytes;
            return true;
        case CodecType::VVC:
            if (length < kVVCHeaderBytes) {
                return false;
            }
            parseVVCHeaderBytes(data, out);
            out.headerSize = kVVCHeaderBytes;
            return true;
        case CodecType::LCEVC:
            if (length < kLCEVCHeaderBytes) {
                return false;
            }
            if (!parseLCEVCHeaderBytes(data, out)) {
                return false;
            }
            out.headerSize = kLCEVCHeaderBytes;
            return true;
        default: return false;
    }
}

bool isSEI(const ParsedNALHeader& header)
{
    switch (header.codec) {
        case CodecType::H264: return header.nalUnitType == 6; // SEI
        case CodecType::HEVC:
            return header.nalUnitType == 39 || header.nalUnitType == 40; // prefix/suffix SEI
        case CodecType::VVC:
            return header.nalUnitType == 23 || header.nalUnitType == 24; // prefix/suffix SEI
        default: return false;
    }
}

bool isSPS(const ParsedNALHeader& header)
{
    switch (header.codec) {
        case CodecType::H264: return header.nalUnitType == 7;
        case CodecType::HEVC: return header.nalUnitType == 33;
        case CodecType::VVC: return header.nalUnitType == 15;
        default: return false;
    }
}

bool isPPS(const ParsedNALHeader& header)
{
    switch (header.codec) {
        case CodecType::H264: return header.nalUnitType == 8;
        case CodecType::HEVC: return header.nalUnitType == 34;
        case CodecType::VVC: return header.nalUnitType == 16;
        default: return false;
    }
}

bool isVPS(const ParsedNALHeader& header)
{
    switch (header.codec) {
        case CodecType::HEVC: return header.nalUnitType == 32;
        case CodecType::VVC: return header.nalUnitType == 14;
        default: return false;
    }
}

bool isAUD(const ParsedNALHeader& header)
{
    switch (header.codec) {
        case CodecType::H264: return header.nalUnitType == 9;
        case CodecType::HEVC: return header.nalUnitType == 35;
        case CodecType::VVC: return header.nalUnitType == 20;
        default: return false;
    }
}

bool isLcevcIDR(const ParsedNALHeader& header)
{
    return header.codec == CodecType::LCEVC && header.nalUnitType == 30;
}

bool isLcevcNonIDR(const ParsedNALHeader& header)
{
    return header.codec == CodecType::LCEVC && header.nalUnitType == 29;
}

} // namespace vnova::utility
