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
#include "stream_reader.h"

#include "utility/multi_byte.h"
#include "utility/unsigned_exp_golomb.h"

#include <cstring>

namespace vnova::analyzer {
StreamReader::StreamReader() = default;

void StreamReader::Reset(const uint8_t* data, size_t dataLength)
{
    m_data = data;
    m_dataLength = dataLength;
    m_position = 0;
}

size_t StreamReader::GetLength() const { return m_dataLength; }

uint64_t StreamReader::GetPosition() const { return m_position; }

bool StreamReader::IsValid() const { return (m_data && (m_position < m_dataLength)); }

const uint8_t* StreamReader::Current()
{
    // Just return front rather than null.
    if (IsValid()) {
        return (m_data + m_position);
    }

    return m_data;
}

void StreamReader::SeekForward(uint64_t numBytes) { m_position += numBytes; }

bool StreamReader::ReadBit()
{
    if (m_readBitfield.getShiftAccumulator() == 0 || m_readBitfield.getShiftAccumulator() == 8) {
        m_readBitfield.reset(BitfieldReverse(ReadValue<uint8_t>()));
    }
    const auto bit = m_readBitfield.getField<uint8_t>(1);
    return (bit != 0);
}

uint64_t StreamReader::ReadMultiByteValue()
{
    return vnova::utility::MultiByte::decode([this]() {
        uint8_t byte = 0;
        ReadBytes(&byte, 1);
        return byte;
    });
}

uint32_t StreamReader::ReadUnsignedExpGolombBits()
{
    return vnova::utility::UnsignedExpGolomb::decode([this]() { return this->ReadBit(); });
}

void StreamReader::ResetBitfield() { m_readBitfield.reset(0); }

} // namespace vnova::analyzer
