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
#include "nal_unit.h"

namespace vnova::utility {
namespace {
    const uint8_t kForbiddenBits = 0b11000000;
    const uint8_t kTypeBits = 0b00111110;
    const uint8_t kReservedBits = 0b00000001;
} // namespace

namespace lcevc {
    NalUnitType::Enum NalUnitType::FromValue(uint8_t value)
    {
        switch (value) {
            case static_cast<uint8_t>(Enum::NonIDR): return Enum::NonIDR;
            case static_cast<uint8_t>(Enum::IDR): return Enum::IDR;
        }

        return Enum::Invalid;
    }

    const char* NalUnitType::ToString(Enum value)
    {
        switch (value) {
            case Enum::NonIDR: return "NonIDR";
            case Enum::IDR: return "IDR";
            case Enum::Invalid: return "Invalid";
        }

        return "Invalid";
    }

    NalUnitType::Enum readNalHeader(const uint8_t* src, size_t length, uint32_t headerSize)
    {
        const size_t headerIndex = headerSize - 2;

        if (length < headerSize) {
            return NalUnitType::Enum::Invalid;
        }

        if (headerSize == kHeaderSize && (src[0] != 0x00 || src[1] != 0x00 || src[2] != 0x01)) {
            return NalUnitType::Enum::Invalid;
        }

        if ((src[headerIndex] & kForbiddenBits) != 0b01000000 ||
            (src[headerIndex] & kReservedBits) != 0x1 || src[headerIndex + 1] != 0xFF) {
            return NalUnitType::Enum::Invalid;
        }

        return NalUnitType::FromValue((src[headerIndex] & kTypeBits) >> 1);
    }
} // namespace lcevc

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

} // namespace vnova::utility
