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
#include "lcevc_bin.h"

#include "utility/byte_order.h"
#include "utility/log_util.h"

#include <array>
#include <cinttypes>
#include <cstddef>
#include <cstring>

namespace vnova::helper::bin {
static constexpr uint32_t kLCEVCBinMagicLength = 8;
static constexpr uint8_t kLCEVCBinMagic[kLCEVCBinMagicLength] = {0x6C, 0x63, 0x65, 0x76,
                                                                 0x63, 0x62, 0x69, 0x6E};
static constexpr uint32_t kLCEVCBinVersion = 1;

namespace {
    // \brief Generic helper function for performing a big-endian read from
    //        a file.
    template <typename T>
    bool fileReadBE(std::unique_ptr<utility::io::FileIORead> const & file, T& value)
    {
        std::array<std::byte, sizeof(T)> buf{};
        if (file->read(buf.data(), buf.size()) != buf.size()) {
            return false;
        }

        std::memcpy(&value, buf.data(), buf.size());
        value = utility::ByteOrder<T>::ToHost(value);
        return true;
    }

    // \brief Generic helper function for performing a big-endian write to
    //        a file.
    template <typename T>
    bool fileWriteBE(std::unique_ptr<utility::io::FileIOWrite> const & file, T value)
    {
        T networkValue = utility::ByteOrder<T>::ToNetwork(value);
        std::array<std::byte, sizeof(T)> buf{};
        std::memcpy(buf.data(), &networkValue, buf.size());
        if (file->write(buf.data(), buf.size()) != buf.size()) {
            return false;
        }

        return true;
    }

    // \brief Helper class for performing structured reads from a raw byte buffer.
    class ParseHelper
    {
    public:
        ParseHelper(const uint8_t* data, uint32_t size)
            : m_data(data)
            , m_end(data + size)
        {}

        const uint8_t* GetCurrent() const { return m_data; }

        uint32_t GetRemaining() const { return static_cast<uint32_t>(m_end - m_data); }

        template <typename T>
        bool ReadBE(T& value)
        {
            const uint32_t readAmount = sizeof(T);
            const uint8_t* next = m_data + readAmount;

            if (next >= m_end) {
                return false;
            }

            T dataAsT;
            std::memcpy(&dataAsT, m_data, sizeof(T));

            value = utility::ByteOrder<T>::ToHost(dataAsT);
            m_data = next;

            return true;
        }

    private:
        const uint8_t* m_data;
        const uint8_t* m_end;
    };

} // namespace

LCEVCBinReader::~LCEVCBinReader() { release(); }

bool LCEVCBinReader::initialise(const std::filesystem::path& path)
{
    if (path.empty()) {
        VNLOG_ERROR("Could not initialise LCEVCBinReader, filepath is not valid");
        return false;
    }

    auto file = std::make_unique<utility::io::FileIORead>(path);

    if (!file->isValid()) {
        VNLOG_ERROR("Failed to open LCEVC bin file for reading");
        return false;
    }

    // u8[8]: Read header and ensure it's valid.
    uint8_t magicMarker[kLCEVCBinMagicLength] = {};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    if (file->read(reinterpret_cast<std::byte*>(magicMarker), kLCEVCBinMagicLength) != kLCEVCBinMagicLength) {
        VNLOG_ERROR("Failed to read magic marker");
        return false;
    }

    if (memcmp(kLCEVCBinMagic, magicMarker, kLCEVCBinMagicLength) != 0) {
        VNLOG_ERROR("LCEVC Bin unrecognised magic bytes received [0x%02x, 0x%02x, 0x%02x, "
                    "0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x]",
                    magicMarker[0], magicMarker[1], magicMarker[2], magicMarker[3], magicMarker[4],
                    magicMarker[5], magicMarker[6], magicMarker[7]);
        return false;
    }

    // u32: Read version and ensure we support it.
    if (!fileReadBE(file, m_version)) {
        VNLOG_ERROR("Failed to read version");
        return false;
    }

    if (m_version > kLCEVCBinVersion) {
        VNLOG_ERROR("Unsupported bin file version loaded. Loaded version: %u, "
                    "supported version: %u",
                    m_version, kLCEVCBinVersion);
        return false;
    }

    m_file = std::move(file);

    return true;
}

void LCEVCBinReader::release() { m_file = nullptr; }

LCEVCBinReadResult LCEVCBinReader::readBlock(LCEVCBinBlock& data)
{
    if (!m_file) {
        return LCEVCBinReadResult::Fail;
    }

    // u16, u32: Read block header
    uint16_t blockType = 0;
    uint32_t blockSize = 0;

    if (!fileReadBE(m_file, blockType)) {
        // Always expect full blocks - the only time EOF should be hit will
        // be after this attempted read for the next block after the last
        // block
        if (m_file->isEndOfFile()) {
            return LCEVCBinReadResult::NoMore;
        }

        VNLOG_ERROR("Failed to read block type");
        return LCEVCBinReadResult::Fail;
    }

    if (!fileReadBE(m_file, blockSize)) {
        VNLOG_ERROR("Failed to read block size");
        return LCEVCBinReadResult::Fail;
    }

    // Verify block-type
    data.blockType = static_cast<LCEVCBinBlockType>(blockType);

    if (data.blockType != LCEVCBinBlockType::LCEVCPayload) {
        VNLOG_ERROR("Unrecognised block type specified: %u", blockType);
        return LCEVCBinReadResult::Fail;
    }

    // Read block data
    data.blockData.resize(blockSize);

    if (blockSize) {
        // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
        const uint64_t readAmount =
            m_file->read(reinterpret_cast<std::byte*>(data.blockData.data()), blockSize);
        // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

        if (readAmount != blockSize) {
            VNLOG_ERROR("Failed to read block of size %u, only read %" PRIu64 " bytes", blockSize,
                        readAmount);
            return LCEVCBinReadResult::Fail;
        }
    }

    return m_file->isErrored() ? LCEVCBinReadResult::Fail : LCEVCBinReadResult::Success;
}

bool LCEVCBinReader::parseBlockAsLCEVCPayload(const LCEVCBinBlock& data, LCEVCBinLCEVCPayload& destination)
{
    if (data.blockType != LCEVCBinBlockType::LCEVCPayload) {
        VNLOG_ERROR("Failed to parse LCEVC payload since the blockType != "
                    "LCEVCBinBlockType::LCEVCPayload");
        return false;
    }

    const uint8_t* raw = data.blockData.data();
    const auto rawSize = static_cast<uint32_t>(data.blockData.size());

    ParseHelper reader = {raw, rawSize};

    int64_t decodeIndex = 0;
    int64_t presentationIndex = 0;

    // i64:   decode_index
    if (!reader.ReadBE(decodeIndex)) {
        VNLOG_ERROR("Failed to parse LCEVC payload decode index from block");
        return false;
    }

    // i64:   presentation_index
    if (!reader.ReadBE(presentationIndex)) {
        VNLOG_ERROR("Failed to parse LCEVC payload encode index from block");
        return false;
    }

    destination.decodeIndex = decodeIndex;
    destination.presentationIndex = presentationIndex;

    // bytes: lcevc_data
    destination.data = reader.GetCurrent();
    destination.dataSize = reader.GetRemaining();

    return true;
}

LCEVCBinWriter::~LCEVCBinWriter() { release(); }

bool LCEVCBinWriter::initialise(const std::filesystem::path& path)
{
    if (path.empty()) {
        VNLOG_ERROR("Could not initialise LCEVCBinWriter, filepath is not valid");
        return false;
    }

    auto file = std::make_unique<utility::io::FileIOWrite>(path);

    if (!file->isValid()) {
        VNLOG_ERROR("Failed to open LCEVC bin file for writing");
        return false;
    }

    m_file = std::move(file);

    return true;
}

void LCEVCBinWriter::release()
{
    if (m_file) {
        m_file->flush();
        m_file.reset();
    }
}

bool LCEVCBinWriter::writeLCEVCPayload(const LCEVCBinLCEVCPayload& payload)
{
    if (!m_file) {
        return false;
    }

    const uint32_t blockSize = payload.dataSize + (sizeof(int64_t) * 2);

    // Write block data
    if (!writeBlockHeader(LCEVCBinBlockType::LCEVCPayload, blockSize)) {
        VNLOG_ERROR("Failed to write block header");
        return false;
    }

    // i64: decode_index
    if (!fileWriteBE(m_file, payload.decodeIndex)) {
        VNLOG_ERROR("Failed to write decode index");
        return false;
    }

    // i64: presentation_index
    if (!fileWriteBE(m_file, payload.presentationIndex)) {
        VNLOG_ERROR("Failed to write presentation index");
        return false;
    }

    // bytes: lcevc_data
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    if (m_file->write(reinterpret_cast<const std::byte*>(payload.data), payload.dataSize) != payload.dataSize) {
        VNLOG_ERROR("Failed to write LCEVC payload data");
        return false;
    }

    return true;
}

bool LCEVCBinWriter::writeFileHeader()
{
    // u8[8]: Write 8-byte magic marker and version.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    if (m_file->write(reinterpret_cast<const std::byte*>(kLCEVCBinMagic), kLCEVCBinMagicLength) !=
        kLCEVCBinMagicLength) {
        VNLOG_ERROR("Failed to write file header magic-marker");
        return false;
    }

    // u32: Write version
    if (!fileWriteBE(m_file, kLCEVCBinVersion)) {
        VNLOG_ERROR("Failed to write version");
        return false;
    }

    m_bWrittenHeader = true;
    return true;
}

bool LCEVCBinWriter::writeBlockHeader(LCEVCBinBlockType blockType, uint32_t blockSize)
{
    // Check & write file header.
    if (!m_bWrittenHeader && !writeFileHeader()) {
        VNLOG_ERROR("Failed to write file header");
        return false;
    }

    // Write block header

    // u16: Block type
    const auto blockTypeValue = static_cast<uint16_t>(blockType);
    if (!fileWriteBE(m_file, blockTypeValue)) {
        VNLOG_ERROR("Failed to write block type: %u", static_cast<uint32_t>(blockType));
        return false;
    }

    // u32: Block size
    if (!fileWriteBE(m_file, blockSize)) {
        VNLOG_ERROR("Failed to write block size: %u", blockSize);
        return false;
    }

    return true;
}

} // namespace vnova::helper::bin
