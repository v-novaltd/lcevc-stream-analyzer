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
#ifndef VN_PARSER_PARSER_H_
#define VN_PARSER_PARSER_H_

#include "config.h"
#include "extractor/extractor.h"
#include "helper/stream_reader.h"
#include "json.hpp"
#include "utility/format_attribute.h"
#include "utility/nal_unit.h"

#include <cstddef>
#include <fstream>

using ordered_json = nlohmann::ordered_json;

namespace vnova::analyzer {

class FileError : public std::runtime_error
{
public:
    FileError(const std::string& message)
        : std::runtime_error(message)
    {}
};

struct PlaneMode
{
    enum Enum
    {
        Y,
        YUV,
        Invalid
    };

    static Enum FromValue(uint8_t val);
    static const char* ToString(Enum val);
    static uint32_t getPlaneCount(Enum val);
};

struct TransformType
{
    enum class Enum
    {
        Transform2x2 = 0,
        Transform4x4,
        Invalid
    };

    static Enum FromValue(uint8_t val);
    static const char* ToString(Enum val);
    static uint32_t getCoeffGroupCount(Enum val);
    static uint32_t getTUSize(Enum val);
};

struct TiledSizeCompressionType
{
    enum class Enum
    {
        None = 0,
        Prefix,
        PrefixDiff,
        Invalid
    };

    static Enum FromValue(uint8_t val);
    static const char* ToString(Enum val);
};

class Parser
{
public:
    explicit Parser(const Config& config);
    ~Parser();

    bool parse(const LCEVC& lcevc);
    uint64_t getTotal() const { return m_total; }
    bool isBaseStream = false;
    bool isFourBytePrefix = false;
    bool bLvccPresent = false;
    uint8_t lvccProfile = 0;
    uint8_t lvccLevel = 0;
    uint64_t sublayer1Size = 0;
    uint64_t sublayer2Size = 0;
    uint64_t temporalSize = 0;
    uint64_t lcevcLayerSize = 0;

private:
    bool parseSequenceConfig();
    bool parseGlobalConfig();
    bool parsePictureConfig();
    bool parseEncodedData();
    bool parseEncodedTileData(uint32_t blockSize);
    bool parseAdditionalInfo(uint32_t blockSize);
    bool parseSkipBlock(uint32_t blockSize);
    bool parseCompressedEntropyEnabledFlags(std::vector<uint8_t>& destination);
    void checkProfileandLevel(uint8_t profile, uint8_t level) const;

    // Template declarations
    template <std::size_t N, typename... Args>
    void Output(const char (&fmt)[N], Args&&... args);
    template <std::size_t N, typename... Args>
    void OutputVerbose(const char (&fmt)[N], Args&&... args);

    const Config& m_config;
    StreamReader m_reader;
    std::ofstream m_logFile;
    ordered_json m_jsonLog;
    ordered_json m_blockJson;
    PlaneMode::Enum m_planeMode = PlaneMode::Invalid;
    TransformType::Enum m_transformType = TransformType::Enum::Invalid;
    uint8_t m_scalingModeLevel2 = 0;
    uint16_t m_width = 0;
    uint16_t m_height = 0;
    uint16_t m_tileWidth = 0;
    uint16_t m_tileHeight = 0;
    uint8_t m_compressionEntropyEnabledPerTileFlag = 0;
    TiledSizeCompressionType::Enum m_compressionTypeSizePerTile = TiledSizeCompressionType::Enum::Invalid;
    bool m_bEnhancementEnabled = false;
    bool m_globalConfigReceived = false;
    bool m_temporalEnabled = false;
    bool m_temporalSignaled = false;
    uint64_t m_total = 1;
    uint8_t m_bitstreamVersion = 0;
    utility::lcevc::NalUnitType::Enum m_nalType = utility::lcevc::NalUnitType::Enum::Invalid;
    uint8_t m_ditheringControlFlag = static_cast<uint8_t>(utility::lcevc::NalUnitType::Enum::Invalid);
};

} // namespace vnova::analyzer

#endif // VN_PARSER_PARSER_H_
