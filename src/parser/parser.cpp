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
#include "parser.h"

#include "app/config_types.h"
#include "helper/entropy_decoder.h"
#include "helper/extracted_frame.h"
#include "helper/nal_unit.h"
#include "helper/stream_reader.h"
#include "parser/parsed_types.h"
#include "utility/bit_field.h"
#include "utility/json_util.h"
#include "utility/log_util.h"
#include "utility/math_util.h"
#include "utility/output_util.h"

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cstdarg>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <vector>

// Include order matters - optional before json
#include <json.hpp>

using namespace vnova::utility;
using namespace vnova::helper;
using vnova::utility::math::ceilDiv;

namespace vnova::analyzer {
namespace {
    constexpr int32_t kNumLevels = 2;
    constexpr uint8_t kSupportedVersion = 2;

    constexpr uint32_t getPlaneCount(const PlaneMode value)
    {
        switch (value) {
            case PlaneMode::Y: return 1;
            case PlaneMode::YUV: return 3;
            case PlaneMode::UNKNOWN: VNAssert(false); break; // NOLINT(misc-static-assert)
        }
        return 0;
    }

    constexpr uint32_t getCoeffGroupCount(const TransformType value)
    {
        switch (value) {
            case TransformType::TRANSFORM_2X2: return 4;
            case TransformType::TRANSFORM_4X4: return 16;
            case TransformType::UNKNOWN: VNAssert(false); break; // NOLINT(misc-static-assert)
        }
        return 0;
    }

    constexpr uint32_t getTUSize(const TransformType value)
    {
        switch (value) {
            case TransformType::TRANSFORM_2X2: return 2;
            case TransformType::TRANSFORM_4X4: return 4;
            case TransformType::UNKNOWN: VNAssert(false); break; // NOLINT(misc-static-assert)
        }
        return 0;
    }

    std::optional<std::array<uint16_t, 2>> getValidatedTileDimensions(const GlobalConfig& globalConfig)
    {
        uint16_t tileWidth = 0;
        uint16_t tileHeight = 0;
        switch (globalConfig.tile_dimensions_type) {
            case TilingMode::TILE_512X256:
                tileWidth = 512;
                tileHeight = 256;
                break;
            case TilingMode::TILE_1024X512:
                tileWidth = 1024;
                tileHeight = 512;
                break;
            case TilingMode::CUSTOM:
                if (globalConfig.custom_tile.has_value()) {
                    tileWidth = globalConfig.custom_tile->at(0);
                    tileHeight = globalConfig.custom_tile->at(1);
                }
                break;
            default: break;
        }

        if (tileWidth == 0 || tileHeight == 0) {
            VNLOG_ERROR("tile dimensions are invalid, a dimension of 0 "
                        "received from global "
                        "config block, skipping block cannot determine number of "
                        "tiles. tile_width: %u "
                        "tile_height: %u",
                        tileWidth, tileHeight);
            return std::nullopt;
        }

        // NOLINTNEXTLINE(clang-analyzer-core.DivideZero)
        if (const uint32_t tuSize = getTUSize(globalConfig.transform_type);
            tileHeight % tuSize != 0 || tileWidth % tuSize != 0) {
            VNLOG_ERROR("tile dimensions are invalid, tile dimension must be "
                        "divisible by TU "
                        "size. Skipping block. tile_width: %u "
                        "tile_height: %u, TU size: %u",
                        tileWidth, tileHeight, tuSize);
            return std::nullopt;
        }

        return std::array<uint16_t, 2>{tileWidth, tileHeight};
    }

    constexpr uint8_t kBlockSizeTypeReserved = 6;
    constexpr uint8_t kBlockSizeTypeCustom = 7;
    constexpr std::array<uint32_t, 7> kBlockSizeTypeLUT = {
        0,         1, 2, 3, 4, 5,
        0xFFFFFFFF // reserved
    };

    struct SyntaxResolution
    {
        uint16_t width;
        uint16_t height;
    };

    constexpr uint32_t kResolutionTypeCustom = 63;
    constexpr std::array<SyntaxResolution, 51> kResolutionTypeLUT = {
        SyntaxResolution{0, 0}, // Unused
        SyntaxResolution{360, 200},   SyntaxResolution{400, 240},   SyntaxResolution{480, 320},
        SyntaxResolution{640, 360},   SyntaxResolution{640, 480},   SyntaxResolution{768, 480},
        SyntaxResolution{800, 600},   SyntaxResolution{852, 480},   SyntaxResolution{854, 480},
        SyntaxResolution{856, 480},   SyntaxResolution{960, 540},   SyntaxResolution{960, 640},
        SyntaxResolution{1024, 576},  SyntaxResolution{1024, 600},  SyntaxResolution{1024, 768},
        SyntaxResolution{1152, 864},  SyntaxResolution{1280, 720},  SyntaxResolution{1280, 800},
        SyntaxResolution{1280, 1024}, SyntaxResolution{1360, 768},  SyntaxResolution{1366, 768},
        SyntaxResolution{1400, 1050}, SyntaxResolution{1440, 900},  SyntaxResolution{1600, 1200},
        SyntaxResolution{1680, 1050}, SyntaxResolution{1920, 1080}, SyntaxResolution{1920, 1200},
        SyntaxResolution{2048, 1080}, SyntaxResolution{2048, 1152}, SyntaxResolution{2048, 1536},
        SyntaxResolution{2160, 1440}, SyntaxResolution{2560, 1440}, SyntaxResolution{2560, 1600},
        SyntaxResolution{2560, 2048}, SyntaxResolution{3200, 1800}, SyntaxResolution{3200, 2048},
        SyntaxResolution{3200, 2400}, SyntaxResolution{3440, 1440}, SyntaxResolution{3840, 1600},
        SyntaxResolution{3840, 2160}, SyntaxResolution{3840, 2400}, SyntaxResolution{4096, 2160},
        SyntaxResolution{4096, 3072}, SyntaxResolution{5120, 2880}, SyntaxResolution{5120, 3200},
        SyntaxResolution{5120, 4096}, SyntaxResolution{6400, 4096}, SyntaxResolution{6400, 4800},
        SyntaxResolution{7680, 4320}, SyntaxResolution{7680, 4800},
    };

    constexpr uint32_t kVUIAspectRatioIDCExtendedSAR = 255;

    constexpr std::array<uint8_t, 4> kT35VNovaCode = {0xb4, 0x00, 0x50, 0x00};

    uint32_t calculateNumTilesLevel2(uint32_t width, uint32_t height, uint32_t tileWidth, uint32_t tileHeight)
    {
        return ceilDiv(width, tileWidth) * ceilDiv(height, tileHeight);
    }

    uint32_t calculateNumTilesLevel1(uint32_t width, uint32_t height, uint32_t tileWidth,
                                     uint32_t tileHeight, ScalingMode scalingModeLvl2)
    {
        switch (scalingModeLvl2) {
            case ScalingMode::SCALING_0D:
                return calculateNumTilesLevel2(width, height, tileWidth, tileHeight);
            case ScalingMode::SCALING_1D:
                return ceilDiv(ceilDiv(width, 2u), tileWidth) * ceilDiv(height, tileHeight);
            case ScalingMode::SCALING_2D:
                return ceilDiv(ceilDiv(width, 2u), tileWidth) * ceilDiv(ceilDiv(height, 2u), tileHeight);
            case ScalingMode::UNKNOWN: VNAssert(false); break; // NOLINT(misc-static-assert)
        }
        return 0;
    }

    std::optional<FrameBase> baseFrameAsOutputStruct(const uint64_t frameCount, const int64_t baseWireSize,
                                                     const int64_t combinedWireSize,
                                                     const std::optional<BaseFrame> frameOpt)
    {
        if (frameOpt.has_value() == false) {
            return std::nullopt;
        }
        const auto& frame = frameOpt.value();
        return FrameBase{frameCount,   std::string(toString(frame.type)),
                         frame.dts,    frame.pts,
                         frame.height, frame.width,
                         baseWireSize, combinedWireSize};
    }

} // namespace

Parser::Parser(const Config& config)
    : m_config(config)
{
    if ((config.analyzeLogFormat == LogFormat::TEXT) && (config.analyzeLogPath.empty() == false)) {
        m_logFile.open(config.analyzeLogPath, std::ios::trunc);
        if (!m_logFile.is_open()) {
            VNLOG_ERROR("Failed to open log file %s", config.analyzeLogPath.c_str());
            throw utility::file::FileError("Failed to open log file");
        }
    }
    m_jsonLog.version = config.version;
}

std::optional<ParsedFrame> Parser::parse(const LCEVCWithBase& lcevcWithBase)
{
    const auto& lcevc = lcevcWithBase.lcevc;

    const size_t lcevcPayloadSize = lcevc.data.size();
    const auto* payloadHead = lcevc.data.data();
    size_t headerSize = 0;

    const auto nalType = parseHeaderAutoLCEVC(payloadHead, lcevcPayloadSize, headerSize);
    if (nalType == FrameTypeLCEVC::INVALID) {
        VNLOG_ERROR("Unrecognized LCEVC NAL unit type (neither IDR or Non-IDR)");
        return std::nullopt;
    }

    std::vector<uint8_t> payloadBuffer(lcevcPayloadSize, 0);

    payloadHead += headerSize;
    const int64_t lcevcRawSize =
        nalUnencapsulate(payloadBuffer.data(), payloadHead, lcevcPayloadSize - headerSize);

    const int64_t lcevcWireSize =
        lcevc.lcevcWireSize > 0 ? lcevc.lcevcWireSize : static_cast<int64_t>(lcevcPayloadSize);

    // Sometimes the remainingWireSize is not available. Estimate the frame base size by subtracting lcevc size from the total packet size.
    const int64_t fallbackBaseWireSize =
        m_isBaseStreamSizeCountable ? (lcevc.totalWireSize - lcevcWireSize) : 0;
    const int64_t baseWireSize =
        lcevc.remainingWireSize > 0 ? lcevc.remainingWireSize : fallbackBaseWireSize;
    const int64_t combinedWireSize = lcevcWireSize + baseWireSize;

    if (baseWireSize < 0) {
        VNLOG_DEBUG("Frame %" PRIu64 " had negative baseWireSize %" PRId64 "", m_frameCount, baseWireSize);
        return std::nullopt;
    }

    m_lcevcLayerSize += lcevcPayloadSize;

    ParsedFrame parsedFrame{
        m_frameCount, lcevc, lcevcWithBase.base, lcevcWireSize, baseWireSize, combinedWireSize,
    };

    if (m_config.subcommand.at(Subcommand::ANALYZE) == true) {
        printFrame(parsedFrame, nalType, lcevcRawSize);
    }

    FrameLCEVC jsonLCEVC = {toString(nalType), static_cast<int64_t>(lcevcPayloadSize), lcevcRawSize,
                            lcevcWireSize};

    m_frameCount += 1;

    if (m_config.analyzeVerboseOutput) {
        if (auto jsonOpt = parseLCEVC(payloadBuffer, lcevcRawSize, nalType); jsonOpt.has_value()) {
            jsonLCEVC.blocks = jsonOpt.value();
        } else {
            return std::nullopt;
        }
    } else {
        // Just parse to gather e.g. layer size stats.
        if (parseLCEVC(payloadBuffer, lcevcRawSize, nalType).has_value() == false) {
            return std::nullopt;
        }
    }

    m_jsonLog.frames.push_back(
        Frame{baseFrameAsOutputStruct(m_frameCount, baseWireSize, combinedWireSize, lcevcWithBase.base),
              jsonLCEVC});

    return parsedFrame;
}

void Parser::printStreamSummary(FILE* file, const Summary& summary)
{
    fprintf(file, VN_STR_0 "\n", "stream_summary");

    if (summary.base.has_value()) {
        fprintf(file, VN_STR_1 "%" PRId64 "\n", "base_frame_count", summary.base.value().frame_count);
    } else {
        fprintf(file, VN_STR_1 "%s\n", "base_frame_count", "N/A");
    }

    if (summary.framerate.has_value()) {
        fprintf(file, VN_STR_1 "%.5f fps\n", "base_layer_framerate", summary.framerate.value());
    } else {
        fprintf(file, VN_STR_1 "%s\n", "base_layer_framerate", "N/A");
    }

    if (summary.base.has_value() && summary.base.value().layer_bitrate.has_value()) {
        fprintf(file, VN_STR_1 "%.0f kbps\n", "base_layer_bitrate",
                summary.base.value().layer_bitrate.value() / 1000.0);
    } else {
        fprintf(file, VN_STR_1 "%s\n", "base_layer_bitrate", "N/A");
    }

    if (summary.base.has_value()) {
        fprintf(file, VN_STR_1 "%.0f kb\n", "base_layer_size",
                static_cast<double>(summary.base.value().layer_size) / 1000.0);
    } else {
        fprintf(file, VN_STR_1 "%s\n", "base_layer_size", "N/A");
    }

    fprintf(file, VN_STR_1 "%" PRId64 "\n", "lcevc_frame_count", summary.lcevc.frame_count);
    fprintf(file, VN_STR_1 "%.0f kb\n", "lcevc_layer_size",
            static_cast<double>(summary.lcevc.layer_size) / 1000.0);
    fprintf(file, VN_STR_2 "%.0f kb\n", "level_1_size",
            static_cast<double>(summary.lcevc.level_1_size) / 1000.0);
    fprintf(file, VN_STR_2 "%.0f kb\n", "level_2_size",
            static_cast<double>(summary.lcevc.level_2_size) / 1000.0);
    fprintf(file, VN_STR_2 "%.0f kb\n", "temporal_size",
            static_cast<double>(summary.lcevc.temporal_size) / 1000.0);

    if (summary.lcevc.layer_bitrate.has_value()) {
        fprintf(file, VN_STR_1 "%.0f kbps\n", "lcevc_bitrate", summary.lcevc.layer_bitrate.value() / 1000.0);
    } else {
        fprintf(file, VN_STR_1 "%s\n", "lcevc_bitrate", "N/A");
    }

    if (summary.base.has_value()) {
        fprintf(file, VN_STR_1 "%.3f\n", "lcevc_base_ratio", summary.base.value().lcevc_base_ratio);
    } else {
        fprintf(file, VN_STR_1 "%3s\n", "lcevc_base_ratio", "N/A");
    }

    fprintf(file, VN_STR_1 "%s\n", "pts_consistency", summary.pts.consistent ? "OK" : "FAILED");
    fprintf(file, VN_STR_1 "%zu\n", "pts_interval_count", summary.pts.interval_count);
}

void Parser::writeOut(FILE* file, const Summary& summary)
{
    printStreamSummary(file, summary);

    m_jsonLog.summary = summary;

    if (m_config.analyzeLogFormat == LogFormat::JSON) {
        const nlohmann::ordered_json jsonLog = m_jsonLog;
        // Always dump to stdout for passing to other tools
        utility::json::dump(stdout, jsonLog);

        // If a log path was set, also write it out to file
        if (m_config.analyzeLogPath.empty() == false) {
            utility::json::write(m_config.analyzeLogPath, jsonLog);
        }
    }
}

void Parser::printFrame(const ParsedFrame& data, const FrameTypeLCEVC nalType, const int64_t lcevcRawSize)
{
    if (data.base.has_value()) {
        Output(VN_STR_0 "%10zu  |  LCEVC [%3s] dts:%10" PRId64 ", pts:%10" PRId64 ", siz:%10" PRId64
                        "  |  BASE [%3s] dts:%10" PRId64 ", pts:%10" PRId64 ", siz:%10" PRId64
                        "  |  COMBINED siz:%10" PRId64 "\n",
               "frame", data.index, toString(nalType), data.lcevc.dts, data.lcevc.pts,
               data.lcevcWireSize, toString(data.base.value().type), data.base.value().dts,
               data.base.value().pts, data.baseWireSize, data.combinedWireSize);
    } else {
        Output(VN_STR_0 "%10zu  |  LCEVC [%3s] dts:%10" PRId64 ", pts:%10" PRId64 ", siz:%10" PRId64
                        ", raw:%10" PRId64 "\n",
               "frame", data.index, toString(nalType), data.lcevc.dts, data.lcevc.pts,
               data.lcevcWireSize, lcevcRawSize);
    }
}

std::optional<nlohmann::ordered_json> Parser::parseLCEVC(std::vector<uint8_t>& payloadBuffer,
                                                         const int64_t lcevcRawSize,
                                                         const FrameTypeLCEVC nalType)
{
    nlohmann::ordered_json blocksJson = nlohmann::ordered_json::array();

    m_reader.Reset(payloadBuffer.data(), lcevcRawSize);

    // Blocks
    while (m_reader.IsValid()) {
        // Block header.
        auto blockHeader = m_reader.ReadValue<uint8_t>();
        uint8_t blockSizeType = ((blockHeader & 0xE0) >> 5); // blockHeader.11100000
        uint32_t blockSize = 0;

        if (blockSizeType == kBlockSizeTypeCustom) {
            blockSize = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
            if (blockSize == std::numeric_limits<uint32_t>::max()) {
                VNLOG_ERROR("Block size is max uint, this is likely wrong, check size "
                            "type LUT is fully populated with latest values");
                return std::nullopt;
            }
        } else if (blockSizeType == kBlockSizeTypeReserved) {
            VNLOG_ERROR("Reserved payload_size_type (%" PRId32 ")", kBlockSizeTypeReserved);
            return std::nullopt;
        } else if (blockSizeType < kBlockSizeTypeLUT.size()) {
            blockSize = kBlockSizeTypeLUT[blockSizeType];
        } else {
            VNLOG_ERROR("Unrecognised block size signaling");
            return std::nullopt;
        }

        auto blockType = FromValue<BlockType>(blockHeader & 0x1F); // blockHeader.00011111
        const auto payloadSizeType = static_cast<PayloadSizeType>(blockSizeType);
        BaseConfig base{blockType, payloadSizeType, blockSize};
        uint64_t expectedPosition = m_reader.GetPosition() + blockSize;
        bool bBlockDecodeResult = true;

        switch (blockType) {
            case BlockType::SEQUENCE_CONFIG: {
                SequenceConfig config(base);
                bBlockDecodeResult = parseSequenceConfig(config);
                if (bBlockDecodeResult) {
                    m_blockJson = nlohmann::ordered_json(config);
                    OutputConfig(config);
                }
            } break;
            case BlockType::GLOBAL_CONFIG: {
                GlobalConfig config(base);
                bBlockDecodeResult = parseGlobalConfig(config);
                if (bBlockDecodeResult) {
                    m_blockJson = nlohmann::ordered_json(config);
                    OutputConfig(config);
                }
            } break;
            case BlockType::PICTURE_CONFIG: {
                PictureConfig config(base);
                bBlockDecodeResult = parsePictureConfig(config, nalType);
                if (bBlockDecodeResult) {
                    m_blockJson = nlohmann::ordered_json(config);
                    OutputConfig(config);
                }
            } break;
            case BlockType::ENCODED_DATA: {
                EncodedData config(base);
                bBlockDecodeResult = parseEncodedData(config);
                if (bBlockDecodeResult) {
                    m_blockJson = nlohmann::ordered_json(config);
                    OutputLayeredConfig(config);
                }
            } break;
            case BlockType::ENCODED_TILED_DATA: {
                EncodedTileData config(base);
                bBlockDecodeResult = parseEncodedTileData(config, blockSize);
                if (bBlockDecodeResult) {
                    m_blockJson = nlohmann::ordered_json(config);
                    OutputLayeredConfig(config);
                }
            } break;
            case BlockType::ADDITIONAL_INFO: {
                AdditionalInfo config(base);
                bBlockDecodeResult = parseAdditionalInfo(config, blockSize);
                if (bBlockDecodeResult) {
                    m_blockJson = nlohmann::ordered_json(config);
                    OutputConfig(config);
                }
            } break;
            case BlockType::FILLER: {
                OutputVerbose("\tSkipping block\n");
                parseSkipBlock(blockSize);
                break;
            }
            default:
                VNLOG_ERROR("Failed to parse block of unknown type: %u", blockType);
                return std::nullopt;
        }

        if (!bBlockDecodeResult) {
            VNLOG_ERROR("Failed to decode block [%u - %s] correctly", blockType,
                        ToString(blockType).c_str());
            return std::nullopt;
        }

        if (expectedPosition != m_reader.GetPosition()) {
            VNLOG_ERROR(
                "Failed to parse %s block correctly, offset is not what is expected: %" PRIu64
                ", got: %" PRIu64 "",
                ToString(blockType).c_str(), expectedPosition, m_reader.GetPosition());
            return std::nullopt;
        }
        blocksJson.push_back(m_blockJson);
        m_blockJson.clear();
    }

    return blocksJson;
}

bool Parser::checkProfileAndLevel(uint8_t profile, uint8_t level) const
{
    if (profile != m_lvccProfile) {
        VNLOG_WARN("Profile value differs from the lvcC Atom");
        return false;
    }
    if (level != m_lvccLevel) {
        VNLOG_WARN("Level value differs from the lvcC Atom");
        return false;
    }
    return true;
}

bool Parser::parseSequenceConfig(SequenceConfig& config)
{
    const auto field0 = m_reader.ReadValue<uint8_t>();
    const auto field1 = m_reader.ReadValue<uint8_t>();
    const auto profileIdc = static_cast<uint8_t>((field0 & 0xF0) >> 4); // field0.11110000
    const auto levelIdc = static_cast<uint8_t>(field0 & 0x0F);          // field0.00001111

    config.profile_idc = FromValue<ProfileType>(profileIdc);
    config.level_idc = FromValue<LevelType>(levelIdc);
    config.sublevel_idc = static_cast<SublevelType>((field1 & 0xC0) >> 6); // field1.11000000
    config.conformance_window_flag = (field1 & 0x20) >> 5;                 // field1.00100000

    if (const uint8_t seqReservedZeros5 = (field1 & 0x1F); // field1.00011111
        seqReservedZeros5 != 0) {
        VNLOG_WARN("Reserved bits (sequence_config) not zero: 0x%02x", seqReservedZeros5);
    }

    if (m_bLvccPresent) {
        checkProfileAndLevel(profileIdc, levelIdc);
    }

    if (config.profile_idc == ProfileType::EXTENDED || config.level_idc == LevelType::EXTENDED) {
        const auto extField = m_reader.ReadValue<uint8_t>();

        config.extended = SequenceConfigExt{};
        config.extended->extended_profile_idc = (extField & 0xE0) >> 5; // extField.11100000
        config.extended->extended_level_idc = (extField & 0x1E) >> 1;   // extField.00011110

        const uint8_t seqReservedZero1 = (extField & 0x01); // extField.00000001
        if (seqReservedZero1 != 0) {
            VNLOG_WARN("Reserved bit (sequence_config extension) not zero");
        }
    }

    if (config.conformance_window_flag) {
        config.conf_win = SequenceConfigWindow{};
        config.conf_win->conf_win_left_offset = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
        config.conf_win->conf_win_right_offset = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
        config.conf_win->conf_win_top_offset = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
        config.conf_win->conf_win_bottom_offset = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
    }

    return true;
}

bool Parser::parseGlobalConfig(GlobalConfig& config)
{
    const auto field0 = m_reader.ReadValue<uint8_t>();
    const auto field1 = m_reader.ReadValue<uint8_t>();
    const auto field2 = m_reader.ReadValue<uint8_t>();
    const auto field3 = m_reader.ReadValue<uint8_t>();

    config.processed_planes_type_flag = ((field0 & 0x80) >> 7) != 0; // field0.10000000
    config.resolution_type = (field0 & 0x7E) >> 1;                   // field0.01111110
    config.transform_type = FromValue<TransformType>(field0 & 0x01); // field0.00000001
    config.chroma_sampling_type = FromValue<ChromaSamplingType>((field1 & 0xC0) >> 6); // field1.11000000
    config.base_depth_type = FromValue<BitDepthType>((field1 & 0x30) >> 4); // field1.00110000
    config.enhancement_depth_type = FromValue<BitDepthType>((field1 & 0x0C) >> 2); // field1.00001100
    config.temporal_step_width_modifier_signalled_flag = ((field1 & 0x02) >> 1) != 0; // field1.00000010
    config.predicted_residual_mode_flag = (field1 & 0x01) != 0; // field1.00000001
    config.temporal_tile_intra_signalling_enabled_flag = ((field2 & 0x80) >> 7) != 0; // field2.10000000
    config.temporal_enabled_flag = ((field2 & 0x40) >> 6) != 0;                // field2.01000000
    config.upsample_type = FromValue<UpsampleType>((field2 & 0x38) >> 3);      // field2.00111000
    config.level1_filtering_signalled_flag = ((field2 & 0x04) >> 2) != 0;      // field2.00000100
    config.scaling_mode_level1 = FromValue<ScalingMode>(field2 & 0x03);        // field2.00000011
    config.scaling_mode_level2 = FromValue<ScalingMode>((field3 & 0xC0) >> 6); // field3.11000000
    config.tile_dimensions_type = FromValue<TilingMode>((field3 & 0x30) >> 4); // field3.00110000
    config.user_data_enabled = FromValue<UserDataMode>((field3 & 0x0C) >> 2);  // field3.00001100
    config.level1_depth_flag = ((field3 & 0x02) >> 1) != 0;                    // field3.00000010
    config.chroma_step_width_flag = (field3 & 0x01) != 0;                      // field3.00000001

    if (config.processed_planes_type_flag) {
        const auto planeModeField = m_reader.ReadValue<uint8_t>();
        config.planes_type = FromValue<PlaneMode>((planeModeField & 0xF0) >> 4); // planeModeField.11110000
        const auto planesReservedZeros4 = static_cast<uint8_t>(planeModeField & 0x0F); // planeModeField.00001111
        if (planesReservedZeros4 != 0) {
            VNLOG_WARN("Reserved bits (planes_type) not zero: 0x%02x", planesReservedZeros4);
        }
    }

    // Conditional stuff now
    if (config.temporal_step_width_modifier_signalled_flag) {
        config.temporal_step_width_modifier = m_reader.ReadValue<uint8_t>();
    }

    if (config.upsample_type == UpsampleType::ADAPTIVE_CUBIC) {
        config.adaptive_cubic_kernel_coeffs = {
            m_reader.ReadValue<uint16_t>(), m_reader.ReadValue<uint16_t>(),
            m_reader.ReadValue<uint16_t>(), m_reader.ReadValue<uint16_t>()};
    }

    if (config.level1_filtering_signalled_flag) {
        const auto deblockField = m_reader.ReadValue<uint8_t>();
        config.level1_filtering = {static_cast<uint8_t>((deblockField & 0xF0) >> 4), // deblockField.11110000
                                   static_cast<uint8_t>(deblockField & 0x0F)}; // deblockField.00001111
    }

    if (config.tile_dimensions_type != TilingMode::NONE) {
        if (config.tile_dimensions_type == TilingMode::CUSTOM) {
            const auto tileWidth = m_reader.ReadValue<uint16_t>();
            const auto tileHeight = m_reader.ReadValue<uint16_t>();
            config.custom_tile = {tileWidth, tileHeight};
        }

        const auto tilingField = m_reader.ReadValue<uint8_t>();
        if ((tilingField & 0xF8) != 0) { // tilingField.11111000
            VNLOG_WARN("Reserved bits (tiling) not zero: 0x%02x",
                       static_cast<unsigned>(tilingField & 0xF8)); // tilingField.11111000
        }
        config.compression_type = GlobalConfigCompressionType();
        config.compression_type->compression_type_entropy_enabled_per_tile_flag =
            static_cast<EntropyEnabledPerTileType>((tilingField & 0x04) >> 2); // tilingField.00000100
        config.compression_type->compression_type_size_per_tile =
            FromValue<TiledSizeCompressionType>(tilingField & 0x03); // tilingField.00000011
    }

    if (config.resolution_type == kResolutionTypeCustom) {
        config.custom_resolution->at(0) = m_reader.ReadValue<uint16_t>();
        config.custom_resolution->at(1) = m_reader.ReadValue<uint16_t>();
        config._resolution = config.custom_resolution.value();
    } else if (config.resolution_type > 0 && config.resolution_type < kResolutionTypeLUT.size()) {
        const SyntaxResolution& res = kResolutionTypeLUT[config.resolution_type];
        config._resolution = {res.width, res.height};
    }

    if (config.chroma_step_width_flag) {
        config.chroma_step_width_multiplier = m_reader.ReadValue<uint8_t>();
    }

    m_globalConfig = config;

    return true;
}

bool Parser::parsePictureConfig(PictureConfig& config, const FrameTypeLCEVC nalType)
{
    if (!m_globalConfig.has_value()) {
        VNLOG_ERROR("Encoded Data received before a Global Config");
        return false;
    }
    const auto& globalConfig = m_globalConfig.value();

    const auto field0 = m_reader.ReadValue<uint8_t>();

    config.no_enhancement_bit_flag = (field0 & 0x80) >> 7; // field0.10000000
    config.picture_type_bit_flag = static_cast<PictureType>((field0 & 0x04) >> 2); // field0.00000100
    config.temporal_refresh_bit_flag = ((field0 & 0x02) >> 1) != 0; // field0.00000010

    if (config.no_enhancement_bit_flag == 0) {
        const auto field1 = m_reader.ReadValue<uint16_t>();
        config.quant_matrix_mode = FromValue<QuantMatrixMode>((field0 & 0x70) >> 4); // field0.01110000
        config.dequant_offset_signalled_flag = ((field0 & 0x08) >> 3) != 0; // field0.00001000
        config.step_width_sublayer1_enabled_flag = (field0 & 0x01) != 0;    // field0.00000001
        config.step_width_sublayer2 = static_cast<uint16_t>((field1 & 0xFFFE) >> 1); // field1.1111111111111110
        config.dithering_control_flag = static_cast<uint16_t>((field1 & 0x01) != 0); // field1.0000000000000001

        config.temporal_signalling_present_flag =
            globalConfig.temporal_enabled_flag && !config.temporal_refresh_bit_flag;
        m_ditheringControlFlagLast = config.dithering_control_flag;

    } else {
        config.temporal_signalling_present_flag = (field0 & 0x01) != 0; // field0.00000001

        // Inferred values
        config.quant_matrix_mode = QuantMatrixMode::USE_PREVIOUS;
        config.dequant_offset_signalled_flag = false;
        config.step_width_sublayer1_enabled_flag = false;

        // When dithering_control_flag is not present, it is inferred to be equal to 0 for IDR picture
        // For pictures other than the IDR picture, it is inferred to be equal to the value of dithering_control_flag for the preceding picture
        config.dithering_control_flag =
            (nalType == FrameTypeLCEVC::IDR) ? false : m_ditheringControlFlagLast;
        m_ditheringControlFlagLast = config.dithering_control_flag;
    }

    if (config.picture_type_bit_flag == PictureType::FIELD) {
        const auto fieldField = m_reader.ReadValue<uint8_t>();
        config.field_type_bit_flag =
            std::make_optional<FieldType>(static_cast<FieldType>((fieldField & 0x80) >> 7)); // fieldField.10000000

        if ((fieldField & 0x7F) != 0) { // fieldField.01111111
            VNLOG_WARN("Reserved bits (field_type) not zero: 0x%02x",
                       static_cast<unsigned>(fieldField & 0x7F)); // fieldField.01111111
        }
    }

    if (config.step_width_sublayer1_enabled_flag) {
        const auto sublayer1Field = m_reader.ReadValue<uint16_t>();
        const uint16_t stepWidthSublayer1 = (sublayer1Field & 0xFFFE) >> 1; // sublayer1Field.1111111111111110
        const bool filteringLevel1Enabled = (sublayer1Field & 0x0001) != 0; // sublayer1Field.0000000000000001
        config.step_width_sublayer1 =
            std::make_optional<PictureConfigSublayer1>({stepWidthSublayer1, filteringLevel1Enabled});
    }

    if (config.quant_matrix_mode == QuantMatrixMode::CUSTOM_BOTH ||
        config.quant_matrix_mode == QuantMatrixMode::CUSTOM_SUBLAYER_2 ||
        config.quant_matrix_mode == QuantMatrixMode::CUSTOM_BOTH_UNIQUE) {
        const uint32_t layerCount = getCoeffGroupCount(globalConfig.transform_type);
        config.qm_coefficient_0 = std::make_optional<std::vector<uint8_t>>();
        for (uint32_t layerIdx = 0; layerIdx < layerCount; ++layerIdx) {
            const auto val = m_reader.ReadValue<uint8_t>();
            config.qm_coefficient_0->push_back(val);
        }
    }

    if (config.quant_matrix_mode == QuantMatrixMode::CUSTOM_SUBLAYER_1 ||
        config.quant_matrix_mode == QuantMatrixMode::CUSTOM_BOTH_UNIQUE) {
        const uint32_t layerCount = getCoeffGroupCount(globalConfig.transform_type);
        config.qm_coefficient_1 = std::make_optional<std::vector<uint8_t>>();
        for (uint32_t layerIdx = 0; layerIdx < layerCount; ++layerIdx) {
            const auto val = m_reader.ReadValue<uint8_t>();
            config.qm_coefficient_1->push_back(val);
        }
    }

    if (config.dequant_offset_signalled_flag) {
        const auto dequantOffsetField = m_reader.ReadValue<uint8_t>();

        const bool dequantOffsetMode = ((dequantOffsetField & 0x80) >> 7) != 0; // dequantOffsetField.10000000
        const uint8_t dequantOffset = (dequantOffsetField & 0x7F); // dequantOffsetField.01111111

        config.dequant_offset =
            std::make_optional<PictureConfigDequantOffset>({dequantOffsetMode, dequantOffset});
    }

    if (config.dithering_control_flag) {
        const auto ditheringField = m_reader.ReadValue<uint8_t>();

        config.dithering = std::make_optional<PictureConfigDithering>();
        config.dithering->dithering_type =
            FromValue<DitherType>((ditheringField & 0xC0) >> 6); // ditheringField.11000000;

        if (config.dithering->dithering_type != DitherType::NONE) {
            const uint8_t ditheringStrength = ditheringField & 0x1F; // ditheringField.00011111
            config.dithering->dithering_strength = std::make_optional<uint8_t>(ditheringStrength);

        } else if ((ditheringField & 0x1F) != 0) { // ditheringField.00011111
            VNLOG_WARN("Reserved bits (dithering) not zero: 0x%02x",
                       static_cast<unsigned>(ditheringField & 0x1F)); // ditheringField.00011111
        }
    }

    m_pictureConfig = std::make_optional<PictureConfig>(config);
    return true;
}

bool Parser::parseEncodedData(EncodedData& config)
{
    constexpr uint8_t surfacePropCount = 2;
    constexpr uint8_t maxSurfacePropByteCount = (((((2 * 3 * 16) + 3) * surfacePropCount) + 7) & ~7) >> 3;

    if (!m_globalConfig.has_value()) {
        VNLOG_ERROR("Encoded Data received before a Global Config");
        return false;
    }
    if (!m_pictureConfig.has_value()) {
        VNLOG_ERROR("Encoded Data received before a Picture Config");
        return false;
    }

    const auto& globalConfig = m_globalConfig.value();
    const auto& pictureConfig = m_pictureConfig.value();

    // RLE/Prefix Coding choice.
    config._plane_count = getPlaneCount(globalConfig.planes_type);
    config._coefficient_group_count = getCoeffGroupCount(globalConfig.transform_type);
    int64_t chunkCount = (pictureConfig.no_enhancement_bit_flag
                              ? 0
                              : (2 * config._plane_count * config._coefficient_group_count)) +
                         (pictureConfig.temporal_signalling_present_flag ? config._plane_count : 0);
    config._surface_header_bytes = (((chunkCount * surfacePropCount) + 7) & ~7) >> 3;

    std::array<uint8_t, maxSurfacePropByteCount> surfaceProps = {0};
    m_reader.ReadBytes(surfaceProps.data(), static_cast<uint64_t>(config._surface_header_bytes));
    BitfieldDecoderStream<uint8_t> surfacePropsDecoder(
        surfaceProps.data(), static_cast<uint32_t>(config._surface_header_bytes));

    for (int64_t planeIndex = 0; planeIndex < config._plane_count; ++planeIndex) {
        if (pictureConfig.no_enhancement_bit_flag == 0) {
            for (const auto& sublayer : EncodedDataSubLayerList) {
                for (int64_t coeffGroup = 0; coeffGroup < config._coefficient_group_count; ++coeffGroup) {
                    EncodedDataLayer layer;
                    layer.plane = planeIndex;
                    layer.sublayer = sublayer;
                    layer.coefficient_group = coeffGroup;

                    layer.entropy_enabled_flag = surfacePropsDecoder.ReadBit();
                    layer.rle_only_flag = surfacePropsDecoder.ReadBit();

                    if (layer.entropy_enabled_flag) {
                        uint32_t chunkDataSize = 0;
                        try {
                            chunkDataSize = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
                        } catch (const std::overflow_error&) {
                            VNLOG_ERROR("Invalid chunk size (overflow) in encoded data");
                            return false;
                        }
                        m_sublayerSizes[sublayer] += chunkDataSize;

                        m_reader.SeekForward(chunkDataSize);
                        layer.size = chunkDataSize;
                        config.layers.push_back(layer);
                    }
                }
            }
        }

        if (pictureConfig.temporal_signalling_present_flag) {
            EncodedDataLayer layer;
            layer.plane = planeIndex;
            layer.sublayer = EncodedDataSubLayer::TEMPORAL;

            layer.entropy_enabled_flag = surfacePropsDecoder.ReadBit();
            layer.rle_only_flag = surfacePropsDecoder.ReadBit();

            if (layer.entropy_enabled_flag) {
                uint32_t chunkDataSize = 0;
                try {
                    chunkDataSize = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
                } catch (const std::overflow_error&) {
                    VNLOG_ERROR("Invalid chunk size (overflow) in temporal layer");
                    return false;
                }
                layer.size = chunkDataSize;
                config.layers.push_back(layer);

                m_reader.SeekForward(chunkDataSize);
                m_sublayerSizes[EncodedDataSubLayer::TEMPORAL] += chunkDataSize;
            }
        }
    }
    return true;
}

bool Parser::parseEncodedTileData(EncodedTileData& config, uint32_t blockSize)
{
    if (!m_globalConfig.has_value()) {
        VNLOG_ERROR("Encoded Tiled Data received before a Global Config");
        return false;
    }
    if (!m_pictureConfig.has_value()) {
        VNLOG_ERROR("Encoded Tiled Data received before a Picture Config");
        return false;
    }

    const auto& globalConfig = m_globalConfig.value();
    const auto& pictureConfig = m_pictureConfig.value();

    const auto compressionType = globalConfig.compression_type.value_or(GlobalConfigCompressionType());

    const auto tileDimensions = getValidatedTileDimensions(globalConfig);
    if (!tileDimensions.has_value()) {
        parseSkipBlock(blockSize);
        return false;
    }

    const uint16_t tileWidth = tileDimensions->at(0);
    const uint16_t tileHeight = tileDimensions->at(1);
    config._tile_dimensions = {tileWidth, tileHeight};

    config._level2_tile_count = static_cast<int32_t>(calculateNumTilesLevel2(
        globalConfig._resolution[0], globalConfig._resolution[1], tileWidth, tileHeight));
    config._level1_tile_count = static_cast<int32_t>(
        calculateNumTilesLevel1(globalConfig._resolution[0], globalConfig._resolution[1], tileWidth,
                                tileHeight, globalConfig.scaling_mode_level2));

    config._plane_count = static_cast<int32_t>(getPlaneCount(globalConfig.planes_type));
    config._coefficient_group_count =
        static_cast<int32_t>(getCoeffGroupCount(globalConfig.transform_type));

    // Parse per-layer RLE only flag
    const size_t layerRLEOnlyFlagCount =
        (pictureConfig.no_enhancement_bit_flag
             ? 0
             : static_cast<size_t>(kNumLevels * config._coefficient_group_count * config._plane_count)) +
        (static_cast<size_t>(pictureConfig.temporal_signalling_present_flag) *
         static_cast<size_t>(config._plane_count));
    const size_t numBytesRLEOnlyFlags = ((layerRLEOnlyFlagCount + 7U) & ~static_cast<size_t>(7)) >> 3;

    if (numBytesRLEOnlyFlags > (std::numeric_limits<uint32_t>::max)()) {
        throw std::runtime_error("RLE flag byte count exceeds supported width");
    }
    const auto rleFlagBytes = static_cast<uint32_t>(numBytesRLEOnlyFlags);

    std::vector<uint8_t> rleFlagDataBuffer(numBytesRLEOnlyFlags);
    m_reader.ReadBytes(rleFlagDataBuffer.data(), numBytesRLEOnlyFlags);
    BitfieldDecoderStream<uint8_t> rleFlagDecoder(rleFlagDataBuffer.data(), rleFlagBytes);

    // Parse per-tile entropy enabled flag
    const size_t chunkCount =
        (pictureConfig.no_enhancement_bit_flag
             ? 0U
             : (static_cast<size_t>(config._plane_count * config._coefficient_group_count) *
                static_cast<size_t>(config._level1_tile_count + config._level2_tile_count))) +
        (static_cast<size_t>(pictureConfig.temporal_signalling_present_flag) *
         static_cast<size_t>(config._plane_count) * static_cast<size_t>(config._level2_tile_count));

    std::vector<uint8_t> entropyEnabledFlags(chunkCount, 0);

    if (compressionType.compression_type_entropy_enabled_per_tile_flag ==
        EntropyEnabledPerTileType::RUN_LENGTH_ENCODING) {
        if (!parseCompressedEntropyEnabledFlags(entropyEnabledFlags)) {
            return false;
        }
    } else {
        const size_t entropyEnabledByteSize = ((chunkCount + 7U) & ~static_cast<size_t>(7)) >> 3;
        if (entropyEnabledByteSize > (std::numeric_limits<uint32_t>::max)()) {
            throw std::runtime_error("Entropy flag byte count exceeds supported width");
        }
        const auto entropyFlagBytes = static_cast<uint32_t>(entropyEnabledByteSize);

        std::vector<uint8_t> entropyEnabledFlagBuffer(entropyEnabledByteSize);
        m_reader.ReadBytes(entropyEnabledFlagBuffer.data(), entropyEnabledByteSize);
        BitfieldDecoderStream<uint8_t> entropyEnabledFlagDecoder(entropyEnabledFlagBuffer.data(),
                                                                 entropyFlagBytes);

        for (auto& enabledFlag : entropyEnabledFlags) {
            enabledFlag = entropyEnabledFlagDecoder.ReadBit();
        }
    }

    int32_t chunkIndex = 0;

    // Parsing of chunk size
    for (int64_t plane = 0; plane < config._plane_count; plane++) {
        if (pictureConfig.no_enhancement_bit_flag == false) {
            for (const auto& sublayer : EncodedDataSubLayerList) {
                const int64_t tileCount =
                    (sublayer == EncodedDataSubLayer::SUBLAYER_1 ? config._level2_tile_count
                                                                 : config._level1_tile_count);

                for (int32_t coeffGroup = 0; coeffGroup < config._coefficient_group_count; coeffGroup++) {
                    const auto rleOnlyFlag = rleFlagDecoder.ReadBit();

                    std::vector<uint32_t> decompressedChunkSizes;
                    uint32_t decompressedChunkSizesIndex = 0;

                    if (compressionType.compression_type_size_per_tile != TiledSizeCompressionType::NONE) {
                        // Determine number of enabled flags.
                        uint32_t signalledSizes = 0;
                        for (int32_t i = chunkIndex; i < chunkIndex + tileCount; ++i) {
                            signalledSizes += entropyEnabledFlags[i] ? 1 : 0;
                        }

                        // Decompress sizes into buffer.
                        entropyDecodeSizes(m_reader, signalledSizes,
                                           compressionType.compression_type_size_per_tile,
                                           decompressedChunkSizes);
                    }

                    for (int64_t tile = 0; tile < tileCount; tile++) {
                        EncodedTileDataLayer layer;
                        layer.plane = plane;
                        layer.sublayer = sublayer;
                        layer.coefficient_group = static_cast<uint32_t>(coeffGroup);
                        layer.tile = tile;

                        layer.entropy_enabled_flag = entropyEnabledFlags[chunkIndex];
                        chunkIndex += 1;

                        layer.rle_only_flag = rleOnlyFlag;

                        if (layer.entropy_enabled_flag) {
                            uint32_t chunkDataSize = 0;

                            if (compressionType.compression_type_size_per_tile ==
                                TiledSizeCompressionType::NONE) {
                                try {
                                    chunkDataSize = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
                                } catch (const std::exception&) {
                                    VNLOG_ERROR("Invalid chunk size (overflow) in tiled layer");
                                    return false;
                                }
                            } else {
                                // Read from decompressed representation.
                                if (decompressedChunkSizesIndex >=
                                    static_cast<uint32_t>(decompressedChunkSizes.size())) {
                                    VNLOG_ERROR("Invalid decompressed chunk size "
                                                "index in residual layer");
                                    return false;
                                }

                                chunkDataSize = decompressedChunkSizes[decompressedChunkSizesIndex++];
                            }

                            m_sublayerSizes[sublayer] += chunkDataSize;

                            layer.size = chunkDataSize;
                            config.layers.push_back(layer);

                            m_reader.SeekForward(chunkDataSize);
                        }
                    }
                }
            }
        }

        if (pictureConfig.temporal_signalling_present_flag) {
            const auto rleOnlyFlag = rleFlagDecoder.ReadBit();

            std::vector<uint32_t> decompressedChunkSizes;
            uint32_t decompressedChunkSizesIndex = 0;

            if (compressionType.compression_type_size_per_tile != TiledSizeCompressionType::NONE) {
                // Determine number of enabled flags.
                uint32_t signalledSizes = 0;
                for (int32_t i = chunkIndex; i < chunkIndex + config._level2_tile_count; ++i) {
                    signalledSizes += entropyEnabledFlags[i] ? 1 : 0;
                }

                // Decompress sizes into buffer.
                entropyDecodeSizes(m_reader, signalledSizes, compressionType.compression_type_size_per_tile,
                                   decompressedChunkSizes);
            }

            for (int32_t tile = 0; tile < config._level2_tile_count; tile++) {
                EncodedTileDataLayer layer;
                layer.plane = static_cast<uint32_t>(plane);
                layer.tile = static_cast<uint32_t>(tile);
                layer.sublayer = EncodedDataSubLayer::TEMPORAL;

                layer.entropy_enabled_flag = entropyEnabledFlags[chunkIndex];
                chunkIndex += 1;

                layer.rle_only_flag = rleOnlyFlag;

                if (layer.entropy_enabled_flag) {
                    uint32_t chunkDataSize = 0;

                    if (compressionType.compression_type_size_per_tile == TiledSizeCompressionType::NONE) {
                        try {
                            chunkDataSize = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
                        } catch (const std::exception&) {
                            VNLOG_ERROR("Invalid chunk size (overflow) in tiled temporal layer");
                            return false;
                        }
                    } else {
                        // Read from decompressed representation.
                        if (decompressedChunkSizesIndex >=
                            static_cast<uint32_t>(decompressedChunkSizes.size())) {
                            VNLOG_ERROR("Invalid decompressed chunk size index in temporal "
                                        "layer");
                            return false;
                        }

                        chunkDataSize = decompressedChunkSizes[decompressedChunkSizesIndex++];
                    }

                    layer.size = chunkDataSize;
                    config.layers.push_back(layer);
                    m_reader.SeekForward(chunkDataSize);
                    m_sublayerSizes[EncodedDataSubLayer::TEMPORAL] += chunkDataSize;
                }
            }
        }
    }

    return true;
}

bool Parser::parseAdditionalInfo(AdditionalInfo& config, uint32_t blockSize)
{
    const auto additionalInfoType = FromValue<AdditionalInfoType>(m_reader.ReadValue<uint8_t>());
    config.info_type = additionalInfoType;

    if (additionalInfoType == AdditionalInfoType::S_FILTER) {
        const auto field0 = m_reader.ReadValue<uint8_t>();
        config.s_filter = AdditionalInfoSFilter{FromValue<SFilterMode>((field0 & 0xE0) >> 5), // field0.11100000
                                                static_cast<uint8_t>(field0 & 0x1F)}; // field0.00011111
    } else if (additionalInfoType == AdditionalInfoType::BASE_HASH) {
        config.base_hash = m_reader.ReadValue<uint64_t>();
    } else if (additionalInfoType == AdditionalInfoType::SEI_PAYLOAD) {
        auto seiPayloadType = FromValue<SEIPayloadType>(m_reader.ReadValue<uint8_t>());
        config.sei = AdditionalInfoSEI{};
        config.sei->sei_payload_type = seiPayloadType;

        if (seiPayloadType == SEIPayloadType::MASTERING_DISPLAY_COLOUR_VOLUME) {
            AdditionalInfoSEIMasteringDisplayColourVolume masteringDisplayColourVolume;
            for (size_t i = 0; i < masteringDisplayColourVolume.display_primaries_x.size(); ++i) {
                masteringDisplayColourVolume.display_primaries_x[i] = m_reader.ReadValue<uint16_t>();
                masteringDisplayColourVolume.display_primaries_y[i] = m_reader.ReadValue<uint16_t>();
            }

            masteringDisplayColourVolume.white_point_x = m_reader.ReadValue<uint16_t>();
            masteringDisplayColourVolume.white_point_y = m_reader.ReadValue<uint16_t>();
            masteringDisplayColourVolume.max_display_mastering_luminance =
                m_reader.ReadValue<uint32_t>();
            masteringDisplayColourVolume.min_display_mastering_luminance =
                m_reader.ReadValue<uint32_t>();

            config.sei->mastering_display_colour_volume = masteringDisplayColourVolume;
        } else if (seiPayloadType == SEIPayloadType::CONTENT_LIGHT_LEVEL_INFO) {
            config.sei->content_light_level_info = AdditionalInfoSEIContentLightLevelInfo{
                m_reader.ReadValue<uint16_t>(), m_reader.ReadValue<uint16_t>()};
        } else if (seiPayloadType == SEIPayloadType::USER_DATA_REGISTERED) {
            const auto countryCode = m_reader.ReadValue<uint8_t>();
            uint32_t readAmount = 0;
            bool bFound = false;
            AdditionalInfoSEIUserDataRegistered userDataRegistered;
            userDataRegistered.t35_country_code = countryCode;

            // UK country code.
            if (countryCode == kT35VNovaCode[0]) {
                // Check for V-Nova code.
                std::array<uint8_t, kT35VNovaCode.size() - 1> t35Remainder = {};
                m_reader.ReadBytes(t35Remainder.data(), kT35VNovaCode.size() - 1);
                bFound = (memcmp(kT35VNovaCode.data() + 1, t35Remainder.data(),
                                 kT35VNovaCode.size() - 1) == 0);

                userDataRegistered.t35_remaining_code = {t35Remainder[0], t35Remainder[1],
                                                         t35Remainder[2]};

                readAmount = static_cast<uint32_t>(kT35VNovaCode.size());
            } else {
                readAmount = 1;
            }

            if (bFound) {
                // Parse V-Nova
                auto bitstreamVersion = m_reader.ReadValue<uint8_t>();
                if (bitstreamVersion != kSupportedVersion) {
                    VNLOG_ERROR("Bitstream version %u is unsupported. "
                                "Supported version is %u",
                                bitstreamVersion, kSupportedVersion);
                    return false;
                }
                userDataRegistered.v_nova_payload = AdditionalInfoSEIVNovaPayload{bitstreamVersion};
            } else {
                // Skip unknown
                parseSkipBlock(blockSize - readAmount);
            }
            config.sei->user_data_registered = userDataRegistered;
        } else {
            config.sei->sei_error = "Invalid SEI Payload Type";
        }
    } else if (additionalInfoType == AdditionalInfoType::VUI_PARAMETERS) {
        config.vui = AdditionalInfoVUI{};
        config.vui->aspect_ratio_info_present = m_reader.ReadBit();

        if (config.vui->aspect_ratio_info_present) {
            const auto aspectRatioIdc = m_reader.ReadBits<uint8_t>();
            config.vui->aspect_ratio_idc = aspectRatioIdc;

            if (aspectRatioIdc == kVUIAspectRatioIDCExtendedSAR) {
                const auto sarWidth = m_reader.ReadBits<uint16_t>();
                const auto sarHeight = m_reader.ReadBits<uint16_t>();

                config.vui->sar_width = sarWidth;
                config.vui->sar_height = sarHeight;
            }
        }

        // Overscan Info
        config.vui->overscan_info_present = m_reader.ReadBit();

        if (config.vui->overscan_info_present) {
            config.vui->overscan_appropriate_flag = m_reader.ReadBit();
        }

        // Video Signal Info
        config.vui->video_signal_type_present = m_reader.ReadBit();

        if (config.vui->video_signal_type_present) {
            config.vui->video_format = m_reader.ReadBits<uint8_t, 3>();
            config.vui->video_full_range = m_reader.ReadBit();

            // Colour Description Info
            const bool colourDescriptionPresentFlag = m_reader.ReadBit();
            config.vui->colour_description_present = colourDescriptionPresentFlag;

            if (colourDescriptionPresentFlag) {
                config.vui->colour_primaries = m_reader.ReadBits<uint8_t>();
                config.vui->transfer_characteristics = m_reader.ReadBits<uint8_t>();
                config.vui->matrix_coefficients = m_reader.ReadBits<uint8_t>();
            }
        }

        // Chroma LOC Info
        config.vui->chroma_loc_info_present = m_reader.ReadBit();

        if (config.vui->chroma_loc_info_present) {
            config.vui->chroma_sample_loc_type_top_field = m_reader.ReadUnsignedExpGolombBits();
            config.vui->chroma_sample_loc_type_bottom_field = m_reader.ReadUnsignedExpGolombBits();
        }

        // Flushing bits
        m_reader.ResetBitfield();
    } else if (additionalInfoType == AdditionalInfoType::HDR) {
        config.hdr = AdditionalInfoHDR{m_reader.ReadBit(), m_reader.ReadBits<uint8_t, 5>(),
                                       m_reader.ReadBit(), m_reader.ReadBit()};

        if (config.hdr->tone_mapper_type == 31) {
            config.hdr->tone_mapper_type_extended = m_reader.ReadBits<uint8_t, 8>();
        }

        if (config.hdr->deinterlacer_enabled) {
            config.hdr->deinterlacer_type = m_reader.ReadBits<uint8_t, 4>();
            config.hdr->top_field_first = m_reader.ReadBit();

            const auto zeroBits = m_reader.ReadBits<uint8_t, 3>();
            VNUnused(zeroBits);
        }
    } else {
        // Skip remainder of the block.
        config.unhandled_info_type = static_cast<uint32_t>(additionalInfoType);
        parseSkipBlock(blockSize - sizeof(uint8_t));
    }

    return true;
}

bool Parser::parseSkipBlock(uint32_t blockSize)
{
    m_reader.SeekForward(blockSize);
    return true;
}

bool Parser::parseCompressedEntropyEnabledFlags(std::vector<uint8_t>& destination)
{
    uint32_t count = 0;
    const auto total = static_cast<uint32_t>(destination.size());
    uint8_t* dst = destination.data();

    // Initial symbol is sent raw.
    auto symbol = m_reader.ReadValue<uint8_t>();

    if (symbol != 0 && symbol != 1) {
        VNLOG_ERROR("Invalid initial symbol state, expected to be 0 or 1");
        return false;
    }

    while (count < total) {
        // Read current symbol run-length and write that many values into destination
        uint32_t runLength = 0;
        try {
            runLength = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
        } catch (const std::overflow_error&) {
            VNLOG_ERROR("Invalid run-length (overflow) in entropy flags RLE");
            return false;
        }

        // Safety first.
        if ((count + runLength) > total) {
            VNLOG_ERROR("Invalid run-length, attempting to decode more symbols that "
                        "available");
            return false;
        }

        memset(dst, symbol, sizeof(uint8_t) * runLength);
        count += runLength;
        dst += runLength;

        // Flip symbol for next run
        symbol = 1 - symbol;
    }

    return true;
}

VNAttributeFormat(printf, 2, 3) void Parser::Output(const char* fmt, ...)
{
    FILE* stdStream = m_config.analyzeLogFormat == LogFormat::TEXT ? stdout : stderr;
    va_list args;
    va_start(args, fmt);
    output::writeToStdAndFile(stdStream, &m_logFile, fmt, args);
    va_end(args);
}

VNAttributeFormat(printf, 2, 3) void Parser::OutputVerbose(const char* fmt, ...)
{
    if (m_config.subcommand.at(Subcommand::ANALYZE) == false) {
        return;
    }
    if (m_config.analyzeVerboseOutput == false) {
        return;
    }
    if (m_config.analyzeLogFormat != LogFormat::TEXT) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    FILE* stdStream = m_config.analyzeLogFormat == LogFormat::TEXT ? stdout : stderr;
    output::writeToStdAndFile(stdStream, &m_logFile, fmt, args);
    va_end(args);
}

void Parser::OutputConfig(nlohmann::ordered_json json)
{
    constexpr std::array skip = {"type", "size_type", "size"};

    OutputVerbose(VN_STR_1 "typ:%-30s siz_type:%-8s siz:%-8s\n", "block", json["type"].dump().c_str(),
                  json["size_type"].dump().c_str(), json["size"].dump().c_str());

    for (const auto& [key, value] : json.items()) {
        if (std::find(skip.begin(), skip.end(), key) != skip.end()) {
            continue;
        }
        OutputVerbose(VN_STR_2 "%s\n", key.c_str(), value.dump().c_str());
    }
}

void Parser::OutputLayeredConfig(nlohmann::ordered_json jsonWithLayers)
{
    OutputVerbose(VN_STR_1 "typ:%-30s siz_type:%-8s siz:%-8s\n", "block",
                  jsonWithLayers["type"].dump().c_str(), jsonWithLayers["size_type"].dump().c_str(),
                  jsonWithLayers["size"].dump().c_str());

    for (const auto& json : jsonWithLayers["layers"]) {
        nlohmann::ordered_json filtered;
        for (const auto& [key, value] : json.items()) {
            if (value.is_null() == false) {
                filtered[key] = value;
            }
        }
        OutputVerbose(VN_STR_2 "%s\n", "layer", filtered.dump().c_str());
    }
}

} // namespace vnova::analyzer
