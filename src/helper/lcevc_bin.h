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
#ifndef VN_BIN_LCEVC_BIN_H_
#define VN_BIN_LCEVC_BIN_H_

#include "utility/file_io.h"
#include "utility/types_util.h"

#include <memory>

namespace vnova::helper::bin {
/*
    This file contains utility functionality to assist with the reading and
    writing of raw LCEVC data into a structured file-format.

    The file format of an LCEVC bin file is implemented with a block based
    approach  to allow future expansion of the file format. There is a header
    that is guaranteed to be present followed by consecutive blocks.

    All data is written in big-endian ordering, this is to ensure that the
    endianness of this file format mirrors that of the raw LCEVC data.

    LCEVC format syntax
    -------------------

        <BEGIN_FILE>

        u8 [8]: magic_marker [0x6C 0x63 0x65 0x76 0x63 0x62 0x69 0x6E]
        u32   : version

        while more_data():
            u16:   block_type
            u32:   block_size
            bytes: block_specific_data

        <END_FILE>

    LCEVC format semantics
    ----------------------

    magic_marker
        This is a fixed set of bytes that spell out "lcevcbin" at the
        beginning of the file. This is used for quick and dirty file validation
        and identification.

    version
        The version of the syntax contained in this file. As the syntax is
        modified the version must be bumped forward.

    more_data()
        This is a function that indicates that there is more data available
        in the file. It provides no data itself, it is mentioned to point out
        the fact that the file format is block based and that the blocks are
        expected to be consecutive until the end of the file with no partial
        blocks.

    block_type
        The file contains N blocks of data, there can be various different
        types of block, this entry defines the block type for the current
        block. Available block types are:

        Value     | Definition
        ----------|--------------
        0         | LCEVC payload
        1 - 65534 | Reserved
        65535     | Extension block

    block_size
        The size in bytes of the block_specific_data within the block.

    block_specific_data
        The byte data for a given block, the number of bytes in this data is
        defined by the block_size entry immediately preceding this data.


    Block type: LCEVC payload syntax
    --------------------------------
        Syntax
        ------
            i64:   decode_index
            i64:   presentation_index
            bytes: lcevc_data

        Semantics
        ---------
            decode_index
                The decode "timestamp" of the LCEVC data as preserved by the
                encoder. This is expected to be monotonic. The units are in
                frame indices. (TODO: Maybe a clock-time is more useful).

            presentation_index
                The presentation "timestamp" of the LCEVC data as preserved by
                the encoder. This may not be monotonic depending on the encoding
                configuration. The units are in frame indices. (TODO: Same as
                decode_index).

            lcevc_data:
                The raw LCEVC byte data. The byte size of this is defined as:

                lcevc_data_size = block_size - 16

    Block type: Extension block
    ---------------------------
    An extension block allows further block types to be defined should the
   current block_type table become saturated. This is the ultimate future
   proof... if we end up with more than 65536 different block types then we've
   probably gotten a bit carried away!

        Syntax
        ------
            u16:   extension_block_type
            bytes: extension_block_specific_data

        Semantics
        ---------
            extension_block_type
                Extension block type for this extension block. All values are
   currently reserved.

            extension_block_specific_data
                The bytes for the extension block. The byte size of this is
   defined as:

                    extension_block_specific_data_size = block_size - 2
*/

enum class LCEVCBinBlockType
{
    LCEVCPayload = 0,
    Extension = 65535,
    Unknown = 65536
};

struct LCEVCBinBlock
{
    LCEVCBinBlockType blockType;
    utility::DataBuffer blockData;
};

struct LCEVCBinLCEVCPayload
{
    int64_t decodeIndex;
    int64_t presentationIndex;
    const uint8_t* data;
    uint32_t dataSize;
};

enum class LCEVCBinReadResult
{
    Success, //< Read was successful
    Fail,    //< Read was unsuccessful and an error occurred.
    NoMore   //< No more data
};

class LCEVCBinReader
{
public:
    LCEVCBinReader() = default;
    virtual ~LCEVCBinReader();

    // \brief Initialisation API
    //
    // This requires that a valid path is passed as an argument.
    //
    // \return Returns false if it fails, true otherwise.
    bool initialise(const std::filesystem::path& path);
    void release();

    bool isValid() { return m_file && m_file->isValid(); }

    // \brief Read the next block from the input file.
    //
    // \return Result of block read defined by LCEVCBinReadResult.
    virtual LCEVCBinReadResult readBlock(LCEVCBinBlock& data);

    // \brief Helper API to parse a generic block into an LCEVC Payload block.
    //
    // This requires that data.blockType == LCEVCBinBlockType::LCEVCPayload, and
    // that the lifetime of the data object matches or exceeds the lifetime of the
    // destination object - there is no memory copying here, the parsed
    // destination only holds a pointer to the memory backed in the block data.
    //
    // \param data        [in]  A loaded block of data.
    // \param destination [out] The location to parse the block data into.
    // \return True if the block was parsed successfully.
    static bool parseBlockAsLCEVCPayload(const LCEVCBinBlock& data, LCEVCBinLCEVCPayload& destination);

private:
    uint32_t m_version = 0;
    std::unique_ptr<utility::io::FileIORead> m_file;
};

class LCEVCBinWriter
{
public:
    LCEVCBinWriter() = default;
    ~LCEVCBinWriter();

    // \brief Initialisation API
    //
    // This requires that a valid path is passed as an argument.
    //
    // \return Returns false if it fails, true otherwise.
    bool initialise(const std::filesystem::path& path);
    void release();

    bool isValid() { return m_file && m_file->isValid(); }

    // \brief Write the LCEVCBinLCEVCPayload passed as an argument.
    //
    // \return Result of block write, false if it's a failure, true otherwise.
    bool writeLCEVCPayload(const LCEVCBinLCEVCPayload& payload);

private:
    bool writeFileHeader();
    bool writeBlockHeader(LCEVCBinBlockType blockType, uint32_t blockSize);

    std::unique_ptr<utility::io::FileIOWrite> m_file;
    bool m_bWrittenHeader = false;
};

} // namespace vnova::helper::bin

#endif // VN_BIN_LCEVC_BIN_H_
