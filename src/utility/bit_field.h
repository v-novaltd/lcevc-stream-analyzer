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
#ifndef VN_UTILITY_BIT_FIELD_H_
#define VN_UTILITY_BIT_FIELD_H_

#include "utility/platform.h"

#include <cstdint>

namespace vnova::utility {
template <typename T>
class BitfieldDecoder
{
public:
    explicit BitfieldDecoder(T value)
        : m_value(value)
    {}

    bool GetBit()
    {
        VNAssert(static_cast<size_t>(m_shiftAccumulator + 1) <= (sizeof(T) * 8));
        const auto out = static_cast<bool>(m_value & 1);
        m_value >>= 1;
        m_shiftAccumulator++;
        return out;
    }

    template <typename U>
    U GetField(uint8_t bitCount)
    {
        static_assert(sizeof(U) <= sizeof(T));
        const T bitMask = (1 << bitCount) - 1;
        VNAssert(static_cast<size_t>(m_shiftAccumulator + bitCount) <=
                 (sizeof(T) * 8)); // Ensure that we don't underflow the bit value.
        auto value = static_cast<U>(m_value & bitMask);
        m_value >>= bitCount;
        m_shiftAccumulator += bitCount;
        return value;
    }

    uint8_t GetShiftAccumulator() const { return m_shiftAccumulator; }

    void Reset(T value)
    {
        m_value = value;
        m_shiftAccumulator = 0;
    }

private:
    T m_value;
    uint8_t m_shiftAccumulator = 0;
};

template <typename T>
struct BitfieldReverseHelper
{};

template <>
struct BitfieldReverseHelper<uint8_t>
{
    static uint8_t Reverse(uint8_t value)
    {
        value = ((value & 0xF0) >> 4) | ((value & 0x0F) << 4); // Swap nibbles.
        value = ((value & 0xCC) >> 2) | ((value & 0x33) << 2); // Swap pairs within nibbles.
        value = static_cast<uint8_t>(((value & 0xAA) >> 1) | ((value & 0x55) << 1)); // Swap bits within pairs.
        return value;
    }
};

template <typename T>
T bitfieldReverse(T value)
{
    return BitfieldReverseHelper<T>::Reverse(value);
}

template <typename T, bool REVERSE = true>
class BitfieldDecoderStream
{
public:
    BitfieldDecoderStream(const T* bytes, uint32_t bytesCount)
        : m_readBitfield(0)
        , m_bytes(bytes)
        , m_bytesCount(bytesCount)
    {
        if (m_bytesCount > 0) {
            if constexpr (REVERSE) {
                m_readBitfield.Reset(bitfieldReverse(m_bytes[m_currentByteIndex]));
            } else {
                m_readBitfield.Reset(m_bytes[m_currentByteIndex]);
            }
        }
    }

    bool ReadBit()
    {
        const bool bitVal = m_readBitfield.GetBit();

        if (m_readBitfield.GetShiftAccumulator() == 8) {
            LoadByte();
        }

        return bitVal;
    }

    template <typename ReturnType>
    ReturnType ReadBits(uint8_t numBits)
    {
        ReturnType inputValue = 0;
        while (numBits--) {
            inputValue = static_cast<ReturnType>(inputValue << 1) |
                         static_cast<ReturnType>(ReadBit() ? 1 : 0);
        }

        return inputValue;
    }

    template <typename ReturnType>
    ReturnType ReadBytes()
    {
        const auto frontBits = static_cast<uint8_t>(8 - m_readBitfield.GetShiftAccumulator());
        const uint8_t backBits = m_readBitfield.GetShiftAccumulator();

        auto inputValue = static_cast<ReturnType>(m_readBitfield.template GetField<uint8_t>(frontBits));
        LoadByte();

        int8_t bitsRemaining = (8 * sizeof(ReturnType)) - frontBits;
        while (bitsRemaining >= 8) {
            inputValue = static_cast<ReturnType>(inputValue << 8) |
                         static_cast<ReturnType>(m_readBitfield.template GetField<uint8_t>(8));
            bitsRemaining -= 8;
            LoadByte();
        }
        VNAssert(static_cast<int8_t>(backBits) == bitsRemaining);
        inputValue = static_cast<ReturnType>(inputValue << backBits) |
                     static_cast<ReturnType>(m_readBitfield.template GetField<uint8_t>(backBits));
        return inputValue;
    }

private:
    void LoadByte()
    {
        if (m_currentByteIndex < (m_bytesCount - 1)) {
            ++m_currentByteIndex;
            if constexpr (REVERSE) {
                m_readBitfield.Reset(bitfieldReverse(m_bytes[m_currentByteIndex]));
            } else {
                m_readBitfield.Reset(m_bytes[m_currentByteIndex]);
            }
        } else {
            m_readBitfield.Reset(0);
        }
    }

    BitfieldDecoder<T> m_readBitfield;
    uint32_t m_currentByteIndex = 0;
    const T* m_bytes = nullptr;
    uint32_t m_bytesCount = 0;
};

} // namespace vnova::utility

#endif // VN_UTILITY_BIT_FIELD_H_
