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
#ifndef VN_PARSER_PARSER_H_
#define VN_PARSER_PARSER_H_

#include "app/config.h"
#include "helper/nal_unit.h"
#include "helper/stream_reader.h"
#include "parser/parsed_frame.h"
#include "parser/parsed_types.h"
#include "utility/format_attribute.h"

#include <cstdio>
#include <fstream>
#include <map>
#include <optional>

// Include order matters - optional before json
#include <json.hpp>

namespace vnova::analyzer {

class Parser
{
public:
    explicit Parser(const Config& config);
    ~Parser() = default;

    std::optional<ParsedFrame> parse(const helper::LCEVCWithBase& lcevcWithBase);
    void writeOut(FILE* file, const Summary& summary);

    uint64_t getTotal() const { return m_frameCount; }
    bool getIsBaseStreamSizeCountable() const noexcept { return m_isBaseStreamSizeCountable; }
    void setIsBaseStreamSizeCountable(bool isBaseStreamSizeCountable) noexcept
    {
        m_isBaseStreamSizeCountable = isBaseStreamSizeCountable;
    }
    bool getLvccPresent() const noexcept { return m_bLvccPresent; }
    void setLvccPresent(bool lvccPresent) noexcept { m_bLvccPresent = lvccPresent; }
    uint8_t getLvccProfile() const noexcept { return m_lvccProfile; }
    void setLvccProfile(uint8_t lvccProfile) noexcept { m_lvccProfile = lvccProfile; }
    uint8_t getLvccLevel() const noexcept { return m_lvccLevel; }
    void setLvccLevel(uint8_t lvccLevel) noexcept { m_lvccLevel = lvccLevel; }
    uint64_t getSublayer1Size() const noexcept
    {
        const auto it = m_sublayerSizes.find(EncodedDataSubLayer::SUBLAYER_1);
        return it == m_sublayerSizes.end() ? 0 : static_cast<uint64_t>(it->second);
    }
    uint64_t getSublayer2Size() const noexcept
    {
        const auto it = m_sublayerSizes.find(EncodedDataSubLayer::SUBLAYER_2);
        return it == m_sublayerSizes.end() ? 0 : static_cast<uint64_t>(it->second);
    }
    uint64_t getTemporalSize() const noexcept
    {
        const auto it = m_sublayerSizes.find(EncodedDataSubLayer::TEMPORAL);
        return it == m_sublayerSizes.end() ? 0 : static_cast<uint64_t>(it->second);
    }
    uint64_t getLcevcLayerSize() const noexcept { return m_lcevcLayerSize; }

private:
    // Output is implemented via utility/output_util.* to keep Parser logic focused on parsing.
    VNAttributeFormat(printf, 2, 3) void Output(const char* fmt, ...);
    VNAttributeFormat(printf, 2, 3) void OutputVerbose(const char* fmt, ...);

    void OutputConfig(nlohmann::ordered_json json);
    void OutputLayeredConfig(nlohmann::ordered_json jsonWithLayers);
    static void printStreamSummary(FILE* file, const Summary& summary);

    void printFrame(const ParsedFrame& data, helper::FrameTypeLCEVC nalType, int64_t lcevcRawSize);
    std::optional<nlohmann::ordered_json> parseLCEVC(std::vector<uint8_t>& payloadBuffer,
                                                     int64_t lcevcRawSize, helper::FrameTypeLCEVC nalType);

    bool parseSequenceConfig(SequenceConfig& config);
    bool parseGlobalConfig(GlobalConfig& config);
    bool parsePictureConfig(PictureConfig& config, helper::FrameTypeLCEVC nalType);
    bool parseEncodedData(EncodedData& config);
    bool parseEncodedTileData(EncodedTileData& config, uint32_t blockSize);
    bool parseAdditionalInfo(AdditionalInfo& config, uint32_t blockSize);
    bool parseSkipBlock(uint32_t blockSize);
    bool parseCompressedEntropyEnabledFlags(std::vector<uint8_t>& destination);
    bool checkProfileAndLevel(uint8_t profile, uint8_t level) const;

    const Config& m_config;
    helper::StreamReader m_reader;
    std::ofstream m_logFile;
    Root m_jsonLog;
    nlohmann::ordered_json m_blockJson;
    std::optional<GlobalConfig> m_globalConfig = std::nullopt;
    std::optional<PictureConfig> m_pictureConfig = std::nullopt;

    uint64_t m_frameCount = 0;

    bool m_ditheringControlFlagLast = false;

    bool m_isBaseStreamSizeCountable = false;
    bool m_bLvccPresent = false;
    uint8_t m_lvccProfile = 0;
    uint8_t m_lvccLevel = 0;
    std::map<EncodedDataSubLayer, int64_t> m_sublayerSizes;
    uint64_t m_lcevcLayerSize = 0;
};

} // namespace vnova::analyzer

#endif // VN_PARSER_PARSER_H_
