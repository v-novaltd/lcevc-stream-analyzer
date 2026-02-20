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

#include "entropy_decoder.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>

namespace vnova::helper {
// Number of bits used to store codes
constexpr static unsigned bitWidth(unsigned size)
{
    constexpr uint8_t sLUT[32] = {
        0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, // 0   - 15
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 16  - 31
    };

    VNAssert(size < 32);

    return sLUT[size];
}

class BitstreamReader
{
public:
    explicit BitstreamReader(StreamReader& byteReader)
        : m_byteReader(byteReader)
    {}

    // Read 0..32 bits into unsigned integer
    uint32_t Read(unsigned numBits)
    {
        assert(numBits <= 32);

        uint32_t result = 0;

        while (numBits) {
            if (m_bitRemaining == 0) {
                m_byte = m_byteReader.ReadValue<uint8_t>();
                m_bitRemaining = 8;
            }

            // How many bits can be read from this byte?
            const unsigned bits = std::min(m_bitRemaining, numBits);

            // Extract bits and merge into result
            result = (result << bits) | ((m_byte >> (m_bitRemaining - bits)) & ((1 << bits) - 1));

            // Update running state.
            m_bitRemaining -= bits;
            numBits -= bits;
        }

        return result;
    }

private:
    StreamReader& m_byteReader;

    uint32_t m_bitRemaining = 0;
    uint8_t m_byte = 0;
};

struct PrefixCode
{
    PrefixCode(unsigned symbol, unsigned bits)
        : symbol(symbol)
        , bits(bits)
    {}

    unsigned symbol;
    unsigned bits;
    unsigned value = 0;
    unsigned count = 0;
};

class PrefixDecoder
{
public:
    void ReadCodes(BitstreamReader& reader)
    {
        const unsigned minCodeLength = reader.Read(5);
        const unsigned maxCodeLength = reader.Read(5);

        // Empty table
        if (minCodeLength == 31 && maxCodeLength == 31) {
            return;
        }

        // Single code
        if (minCodeLength == 0 && maxCodeLength == 0) {
            m_singleSymbol = reader.Read(8);
            return;
        }

        const unsigned lengthBits = bitWidth(maxCodeLength - minCodeLength);

        // Presence bitmap
        if (reader.Read(1)) {
            // Symbols are described by a 'presence' bitmap over all possible symbols
            for (unsigned symbol = 0; symbol < 256; ++symbol) {
                // Presence bit
                if (reader.Read(1)) {
                    m_codes.emplace_back(symbol, reader.Read(lengthBits) + minCodeLength);
                }
            }
        } else {
            // Sparse symbols: count * (symbol, length}
            const unsigned count = reader.Read(5);
            for (unsigned i = 0; i < count; ++i) {
                const unsigned symbol = reader.Read(8);
                m_codes.emplace_back(symbol, reader.Read(lengthBits) + minCodeLength);
            }
        }

        if (m_codes.empty()) {
            return;
        }

        // Generate the encoded values - canonical prefix coding
        // Sort codes by (descending coded length, ascending symbol)
        std::sort(m_codes.begin(), m_codes.end(), [](const PrefixCode& a, const PrefixCode& b) {
            if (a.bits == b.bits) {
                return a.symbol > b.symbol;
            }
            return a.bits < b.bits;
        });

        // Number the codes
        //
        unsigned currentLength = maxCodeLength;
        unsigned currentValue = 0;

        for (auto code = m_codes.rbegin(); code != m_codes.rend(); ++code) {
            if (code->bits < currentLength) {
                currentValue >>= (currentLength - code->bits);
                currentLength = code->bits;
            }
            code->value = currentValue++;
        }
    }

    unsigned DecodeSymbol(BitstreamReader& reader)
    {
        if (m_codes.empty()) {
            return m_singleSymbol;
        }

        // Codes are sorted by increasing bit-length
        // Check each code, adding bits to current value as necessary a match is
        // found
        unsigned bits = 0;
        unsigned value = 0;

        for (auto& code : m_codes) {
            while (bits < code.bits) {
                value = (value << 1) | reader.Read(1);
                ++bits;
            }

            if (value == code.value) {
                code.count++;
                return code.symbol;
            }
        }

        throw std::runtime_error("Failed to find matching symbol in Prefix Coding decoder, "
                                 "possible stream corruption");
    }

    const std::vector<PrefixCode>& Codes() { return m_codes; }

private:
    unsigned m_singleSymbol = 0;

    std::vector<PrefixCode> m_codes;
};

class SymbolSourcePrefix
{
public:
    SymbolSourcePrefix(unsigned numStates, StreamReader& reader)
        : m_states(numStates)
        , m_reader(reader)
    {}

    void Start()
    {
        for (auto& state : m_states) {
            state.ReadCodes(m_reader);
        }
    }

    unsigned Get(unsigned state)
    {
        assert(state < m_states.size());
        return m_states[state].DecodeSymbol(m_reader);
    }

private:
    std::vector<PrefixDecoder> m_states;
    BitstreamReader m_reader;
};

enum class SizesState
{
    LSB,
    MSB,
    COUNT
};

static uint8_t readSymbol(SymbolSourcePrefix& source, SizesState state)
{
    const unsigned symbol = source.Get(static_cast<unsigned int>(state));
    if (symbol > (std::numeric_limits<uint8_t>::max)()) {
        throw std::runtime_error("Prefix code symbol exceeds byte range");
    }
    return static_cast<uint8_t>(symbol);
}

static int16_t decodeUnsignedSize(SymbolSourcePrefix& source)
{
    // As there is only 15-bits signaled the numeric range is guaranteed to
    // be within (2^15)-1 - so output of int16_t is perfectly acceptable.
    const uint8_t lsb = readSymbol(source, SizesState::LSB);

    // If bit 0 is set, an MS byte follows
    if (lsb & 0x01) {
        const uint8_t msb = readSymbol(source, SizesState::MSB);
        return static_cast<int16_t>((lsb >> 1) + (static_cast<int16_t>(msb) << 7));
    }
    return static_cast<int16_t>(lsb >> 1);
}

static int16_t decodeSignedSize(SymbolSourcePrefix& source)
{
    const uint8_t lsb = readSymbol(source, SizesState::LSB);
    int16_t res = 0;

    // If bit 0 is set, an MS byte follows
    if (lsb & 0x01) {
        // Since the data is in 2's complement and we only signal 15-bits,
        // and we've ensured that the valid range of values fits within
        // -2^14 -> (2^14)-1 - all that needs to happen is extend the 15th
        // up to the 16th bit
        const uint8_t msb = readSymbol(source, SizesState::MSB);
        res = static_cast<int16_t>((lsb >> 1) + (static_cast<int16_t>(msb) << 7));
        const int32_t extended = static_cast<int32_t>(res) | ((static_cast<int32_t>(res) & 0x4000) << 1);
        res = static_cast<int16_t>(extended);

        return res;
    }

    // Same as the MSB case, but valid range is -2^6 -> (2^6)-1
    const uint8_t val = lsb >> 1;
    res = static_cast<int16_t>(static_cast<int16_t>(val) | (static_cast<int16_t>(val & 0x40) << 1));

    return res;
}

void entropyDecodeSizes(StreamReader& reader, uint32_t count,
                        analyzer::TiledSizeCompressionType compressionType, std::vector<uint32_t>& out)
{
    out.clear();
    out.reserve(count);

    // Do not attempted to read sizes if none are signaled.
    if (count == 0) {
        return;
    }

    // Read prefix coding tables
    SymbolSourcePrefix source(static_cast<unsigned int>(SizesState::COUNT), reader);
    source.Start();

    int64_t signedSize = 0;

    for (uint32_t i = 0; i < count; ++i) {
        if (compressionType == analyzer::TiledSizeCompressionType::PREFIX_CODING) {
            out.push_back(decodeUnsignedSize(source));
        } else {
            VNAssert(compressionType == analyzer::TiledSizeCompressionType::PREFIX_CODING_ON_DIFFERENCES);
            signedSize += decodeSignedSize(source);

            if (signedSize < 0 || signedSize > (std::numeric_limits<uint32_t>::max)()) {
                throw std::runtime_error("Invalid size determined for PrefixDiff compressed size");
            }

            out.push_back(static_cast<uint32_t>(signedSize));
        }
    }
}

} // namespace vnova::helper
