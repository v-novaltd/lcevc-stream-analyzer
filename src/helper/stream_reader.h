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
#ifndef VN_HELPER_STREAM_READER_H_
#define VN_HELPER_STREAM_READER_H_

#include "utility/bit_field.h"
#include "utility/byte_order.h"

#include <bitset>
#include <cstring>
#include <type_traits>
#include <vector>

namespace vnova::analyzer {
// @brief Light-weight wrapper around a pointer & size to facilitate with
// reading data
//		   and specifically multi-byte values.
class StreamReader
{
public:
    StreamReader();

    // @brief Reset the stream reader instance with some fresh data.
    void Reset(const uint8_t* data, size_t dataLength);
    void Reset(uint8_t* data, size_t dataLength)
    {
        Reset(static_cast<const uint8_t*>(data), dataLength);
    }

    // @brief Get the length of data stored.
    size_t GetLength() const;

    // @brief Get the current position of the stream.
    uint64_t GetPosition() const;

    // @brief Determines if the reader is in a valid state. (ie. Not read past the
    // end of the current buffer).
    bool IsValid() const;

    // @brief Get a pointer to the current data location.
    const uint8_t* Current();

    // @brief Seek the stream forwards only by numBytes
    void SeekForward(uint64_t numBytes);

    // @brief Read numBytes bytes into a trivially copyable buffer and move read pointer.
    template <typename T>
    void ReadBytes(T* data, uint64_t numBytes)
    {
        static_assert(std::is_trivially_copyable_v<T>,
                      "readBytes expects trivially copyable types");
        if (IsValid()) {
            std::memcpy(data, Current(), numBytes);
            SeekForward(numBytes);
        }
    }

    // @brief Reads a single bit and returns bool. Read pointer is moved only when
    // a byte has been complete
    bool ReadBit();

    // @brief Read a single multi-byte value. The number of bytes read depends
    // upon the value
    //		   stored.
    uint64_t ReadMultiByteValue();

    // @brief Reads 0-Order exp-golomb bits to return an uint32_t
    uint32_t ReadUnsignedExpGolombBits();

    // @brief resets m_readBitfield for byte alignment. Should be called at the
    // end when you are done
    //	using bitfield. Since, bitfield is static, it might preserve the state
    // from the previous read iteration 	which can affect the current read. Hence,
    // reset it with 0.
    void ResetBitfield();

    // @brief Read and return a single value of type T. Make sure its a trivial
    // POD type returned. (ie. intX).
    template <typename T>
    T ReadValue()
    {
        T res = 0;
        ReadBytes(&res, sizeof(T));
        return utility::ByteOrder<T>::toHost(res);
    }

    // @brief Helper for reading numValues of type T.
    template <typename T>
    void ReadValues(T* encodedData, uint64_t numValues = 1)
    {
        ReadBytes(encodedData, numValues * sizeof(T));
    }

    template <typename T, uint8_t N = 8 * sizeof(T)>
    T ReadBits()
    {
        std::bitset<N> bits;
        for (size_t i = bits.size(); i > 0; i--) {
            bits[i - 1] = ReadBit();
        }
        return static_cast<T>(bits.to_ullong()); // Should cover upto 64 bits (uint64_t)
    }

    template <typename T>
    T ReadBits(uint8_t length)
    {
        T res = 0;
        for (uint8_t i = 0; i < length; ++i) {
            res <<= 1;
            res |= (ReadBit() ? 1 : 0);
        }

        return res;
    }

private:
    const uint8_t* m_data = nullptr;
    size_t m_dataLength = 0;
    uint64_t m_position = 0;
    BitfieldDecoder<uint8_t> m_readBitfield = BitfieldDecoder<uint8_t>(0);
};

} // namespace vnova::analyzer

#endif // VN_HELPER_STREAM_READER_H_
