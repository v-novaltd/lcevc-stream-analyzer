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
#include "parser.h"

#include "helper/entropy_decoder.h"
#include "helper/frame_queue.h"
#include "helper/stream_reader.h"
#include "utility/bit_field.h"
#include "utility/log_util.h"
#include "utility/math_util.h"

#include <array>
#include <cstdarg>
#include <exception>
#include <filesystem>
#include <limits>
#include <sstream>
#include <vector>

using namespace vnova::utility;

namespace vnova::analyzer {
namespace {
    constexpr int32_t kNumLevels = 2;
    constexpr uint8_t kSupportedVersion = 2;

    struct BlockType
    {
        enum class Enum
        {
            SequenceConfig = 0,
            GlobalConfig = 1,
            PictureConfig = 2,
            EncodedData = 3,
            EncodedDataTiled = 4,
            AdditionalInfo = 5,
            Filler = 6,
            Count
        };

        static Enum FromValue(uint8_t value)
        {
            if (value < static_cast<uint8_t>(Enum::Count)) {
                return static_cast<Enum>(value);
            }

            return Enum::Count;
        }

        static const char* ToString(Enum value)
        {
            static constexpr std::array<const char*, 7> sNameLUT = {
                "Sequence Config",    "Global Config",   "Picture Config", "Encoded Data",
                "Encoded Tiled Data", "Additional Info", "Filler",
            };

            if (value < Enum::Count) {
                return sNameLUT[static_cast<uint8_t>(value)];
            }

            return "Unknown block type";
        }
    };

    struct ChromaSamplingType
    {
        enum class Enum
        {
            Monochrome,
            Chroma420,
            Chroma422,
            Chroma444,
            Invalid
        };

        static Enum FromValue(uint8_t value)
        {
            if (value < static_cast<uint8_t>(Enum::Invalid)) {
                return static_cast<Enum>(value);
            }

            return Enum::Invalid;
        }

        static const char* ToString(Enum value)
        {
            static constexpr std::array<const char*, 4> sNameLUT = {
                "Monochrome",
                "Chroma420",
                "Chroma422",
                "Chroma444",
            };

            if (value < Enum::Invalid) {
                return sNameLUT[static_cast<uint8_t>(value)];
            }

            return "Unknown chroma sampling type";
        }
    };

    struct BitDepthType
    {
        enum class Enum
        {
            Depth8 = 0,
            Depth10,
            Depth12,
            Depth14,
            Invalid
        };

        static Enum FromValue(uint8_t value)
        {
            if (value < static_cast<uint8_t>(Enum::Invalid)) {
                return static_cast<Enum>(value);
            }

            return Enum::Invalid;
        }

        static const char* ToString(Enum value)
        {
            static constexpr std::array<const char*, 4> sNameLUT = {
                "Depth8",
                "Depth10",
                "Depth12",
                "Depth14",
            };

            if (value < Enum::Invalid) {
                return sNameLUT[static_cast<uint8_t>(value)];
            }

            return "Unknown bit depth type";
        }
    };

    struct UpsampleType
    {
        enum class Enum
        {
            Nearest = 0,
            Bilinear,
            Cubic,
            ModifiedCubic,
            AdaptiveCubic,

            Count
        };

        static Enum FromValue(uint8_t value)
        {
            if (value < static_cast<uint8_t>(Enum::Count)) {
                return static_cast<Enum>(value);
            }

            return Enum::Count;
        }

        static const char* ToString(Enum value)
        {
            static constexpr std::array<const char*, 5> sNameLUT = {
                "Nearest", "Linear", "Cubic", "ModifiedCubic", "AdaptiveCubic",
            };

            if (value < Enum::Count) {
                return sNameLUT[static_cast<uint8_t>(value)];
            }

            return "Unknown upsample type";
        }
    };

    struct PictureType
    {
        enum class Enum
        {
            Frame = 0,
            Field,
            Invalid
        };

        static Enum FromValue(uint8_t value)
        {
            if (value < static_cast<uint8_t>(Enum::Invalid)) {
                return static_cast<Enum>(value);
            }

            return Enum::Invalid;
        }

        static const char* ToString(Enum value)
        {
            static constexpr std::array<const char*, 2> sNameLUT = {"Frame", "Field"};

            if (value < Enum::Invalid) {
                return sNameLUT[static_cast<uint8_t>(value)];
            }

            return "Unknown picture type";
        }
    };

    struct FieldType
    {
        enum class Enum
        {
            Top = 0,
            Bottom,
            Invalid
        };

        static Enum FromValue(uint8_t value)
        {
            if (value < static_cast<uint8_t>(Enum::Invalid)) {
                return static_cast<Enum>(value);
            }

            return Enum::Invalid;
        }

        static const char* ToString(Enum value)
        {
            static constexpr std::array<const char*, 2> sNameLUT = {"Top", "Bottom"};

            if (value < Enum::Invalid) {
                return sNameLUT[static_cast<uint8_t>(value)];
            }

            return "Unknown field type";
        }
    };

    struct DitherType
    {
        enum class Enum
        {
            None = 0,
            Uniform,
            Invalid
        };

        static Enum FromValue(uint8_t value)
        {
            if (value < static_cast<uint8_t>(Enum::Invalid)) {
                return static_cast<Enum>(value);
            }

            return Enum::Invalid;
        }

        static const char* ToString(Enum value)
        {
            static constexpr std::array<const char*, 2> sNameLUT = {"None", "Uniform"};

            if (value < Enum::Invalid) {
                return sNameLUT[static_cast<uint8_t>(value)];
            }

            return "Unknown dither type";
        }
    };

    struct ScalingMode
    {
        enum class Enum
        {
            Scaling0D = 0,
            Scaling1D,
            Scaling2D,
            Invalid
        };

        static Enum FromValue(uint8_t value)
        {
            if (value < static_cast<uint8_t>(Enum::Invalid)) {
                return static_cast<Enum>(value);
            }

            return Enum::Invalid;
        }

        static const char* ToString(Enum value)
        {
            static constexpr std::array<const char*, 3> sNameLUT = {
                "0D",
                "1D",
                "2D",
            };

            if (value < Enum::Invalid) {
                return sNameLUT[static_cast<uint8_t>(value)];
            }

            return "Unknown scaling mode";
        }
    };

    struct TilingMode
    {
        enum class Enum
        {
            None = 0,
            Tile512x256,
            Tile1024x512,
            Custom,
            Invalid
        };

        static Enum FromValue(uint8_t value)
        {
            if (value < static_cast<uint8_t>(Enum::Invalid)) {
                return static_cast<Enum>(value);
            }

            return Enum::Invalid;
        }

        static const char* ToString(Enum value)
        {
            static constexpr std::array<const char*, 4> sNameLUT = {
                "none",
                "512x256",
                "1024x512",
                "custom",
            };

            if (value < Enum::Invalid) {
                return sNameLUT[static_cast<uint8_t>(value)];
            }

            return "Unknown tiling mode";
        }
    };

    struct UserDataMode
    {
        enum class Enum
        {
            Disabled = 0,
            With2Bits,
            With6Bits,
            Invalid
        };

        static Enum FromValue(uint8_t value)
        {
            if (value < static_cast<uint8_t>(Enum::Invalid)) {
                return static_cast<Enum>(value);
            }

            return Enum::Invalid;
        }

        static const char* ToString(Enum value)
        {
            static constexpr std::array<const char*, 3> sNameLUT = {
                "Disabled",
                "With 2-Bits",
                "With 6-Bits",
            };

            if (value < Enum::Invalid) {
                return sNameLUT[static_cast<uint8_t>(value)];
            }

            return "Unknown user data mode";
        }
    };

    struct QuantMatrixMode
    {
        enum class Enum
        {
            UsePrevious = 0,
            UseDefault,
            CustomBoth,
            CustomSublayer2,
            CustomSublayer1,
            CustomBothUnique,
            Invalid
        };

        static Enum FromValue(uint8_t value)
        {
            if (value < static_cast<uint8_t>(Enum::Invalid)) {
                return static_cast<Enum>(value);
            }

            return Enum::Invalid;
        }

        static const char* ToString(Enum value)
        {
            static constexpr std::array<const char*, 6> sNameLUT = {
                "UsePrevious",       "UseDefault",        "CustomBoth",
                "Custom Sublayer 2", "Custom Sublayer 1", "Custom Both Unique",
            };

            if (value < Enum::Invalid) {
                return sNameLUT[static_cast<uint8_t>(value)];
            }

            return "Unknown quant matrix mode";
        }
    };

    struct AdditionalInfoType
    {
        enum class Enum
        {
            SEIPayload = 0,
            VUIParameters = 1,
            SFilter = 23,
            BaseHash = 24,
            HDR = 25,
            Invalid
        };

        static Enum FromValue(uint8_t value)
        {
            if (value < static_cast<uint8_t>(Enum::Invalid)) {
                return static_cast<Enum>(value);
            }

            return Enum::Invalid;
        }

        static const char* ToString(Enum value)
        {
            switch (value) {
                case Enum::SEIPayload: return "SEI Payload";
                case Enum::VUIParameters: return "VUI Parameters";
                case Enum::SFilter: return "S-Filter";
                case Enum::BaseHash: return "Base Hash";
                case Enum::HDR: return "HDR";
                case Enum::Invalid: break;
            }

            return "Unknown additional info type";
        }
    };

    struct SEIPayloadType
    {
        enum class Enum
        {
            MasteringDisplayColourVolume = 1,
            ContentLightLevelInfo = 2,
            UserDataRegistered = 4,
            Invalid
        };

        static Enum FromValue(uint8_t value)
        {
            if (value >= static_cast<uint8_t>(Enum::MasteringDisplayColourVolume) &&
                value < static_cast<uint8_t>(Enum::Invalid)) {
                return static_cast<Enum>(value);
            }

            return Enum::Invalid;
        }

        static const char* ToString(Enum value)
        {
            switch (value) {
                case Enum::MasteringDisplayColourVolume: return "Mastering Display Colour Volume";
                case Enum::ContentLightLevelInfo: return "Content Light Level Info";
                case Enum::UserDataRegistered: return "User Data Registered";
                case Enum::Invalid: break;
            }

            return "Unknown SEI Payload type";
        }
    };

    struct SFilterMode
    {
        enum class Enum
        {
            Disabled = 0,
            InLoop = 1,
            OutOfLoop = 2,
            Invalid
        };

        static Enum FromValue(uint8_t value)
        {
            if (value < static_cast<uint8_t>(Enum::Invalid)) {
                return static_cast<Enum>(value);
            }

            return Enum::Invalid;
        }

        static const char* ToString(Enum value)
        {
            static constexpr std::array<const char*, 3> sNameLUT = {"Disabled", "InLoop", "OutOfLoop"};

            if (value < Enum::Invalid) {
                return sNameLUT[static_cast<uint8_t>(value)];
            }

            return "Unknown s-filter mode";
        };
    };

    const uint8_t kBlockSizeTypeReserved = 6;
    const uint8_t kBlockSizeTypeCustom = 7;
    constexpr std::array<uint32_t, 7> kBlockSizeTypeLUT = {
        0,         1, 2, 3, 4, 5,
        0xFFFFFFFF // reserved
    };

    struct SyntaxResolution
    {
        SyntaxResolution(uint16_t width, uint16_t height)
            : width(width)
            , height(height)
        {}
        uint16_t width;
        uint16_t height;
    };

    const uint32_t kResolutionTypeCustom = 63;
    const std::vector<SyntaxResolution> kResolutionTypeLUT = {
        SyntaxResolution(0, 0), // Unused
        SyntaxResolution(360, 200),   SyntaxResolution(400, 240),   SyntaxResolution(480, 320),
        SyntaxResolution(640, 360),   SyntaxResolution(640, 480),   SyntaxResolution(768, 480),
        SyntaxResolution(800, 600),   SyntaxResolution(852, 480),   SyntaxResolution(854, 480),
        SyntaxResolution(856, 480),   SyntaxResolution(960, 540),   SyntaxResolution(960, 640),
        SyntaxResolution(1024, 576),  SyntaxResolution(1024, 600),  SyntaxResolution(1024, 768),
        SyntaxResolution(1152, 864),  SyntaxResolution(1280, 720),  SyntaxResolution(1280, 800),
        SyntaxResolution(1280, 1024), SyntaxResolution(1360, 768),  SyntaxResolution(1366, 768),
        SyntaxResolution(1400, 1050), SyntaxResolution(1440, 900),  SyntaxResolution(1600, 1200),
        SyntaxResolution(1680, 1050), SyntaxResolution(1920, 1080), SyntaxResolution(1920, 1200),
        SyntaxResolution(2048, 1080), SyntaxResolution(2048, 1152), SyntaxResolution(2048, 1536),
        SyntaxResolution(2160, 1440), SyntaxResolution(2560, 1440), SyntaxResolution(2560, 1600),
        SyntaxResolution(2560, 2048), SyntaxResolution(3200, 1800), SyntaxResolution(3200, 2048),
        SyntaxResolution(3200, 2400), SyntaxResolution(3440, 1440), SyntaxResolution(3840, 1600),
        SyntaxResolution(3840, 2160), SyntaxResolution(3840, 2400), SyntaxResolution(4096, 2160),
        SyntaxResolution(4096, 3072), SyntaxResolution(5120, 2880), SyntaxResolution(5120, 3200),
        SyntaxResolution(5120, 4096), SyntaxResolution(6400, 4096), SyntaxResolution(6400, 4800),
        SyntaxResolution(7680, 4320), SyntaxResolution(7680, 4800),
    };

    const uint32_t kVUIAspectRatioIDCExtendedSAR = 255;

    constexpr uint32_t kT35CodeLength = 4;
    constexpr uint8_t kT35VNovaCode[kT35CodeLength] = {0xb4, 0x00, 0x50, 0x00};

    uint32_t calculateNumTilesLevel2(uint32_t width, uint32_t height, uint32_t tileWidth, uint32_t tileHeight)
    {
        return ceilDiv(width, tileWidth) * ceilDiv(height, tileHeight);
    }

    uint32_t calculateNumTilesLevel1(uint32_t width, uint32_t height, uint32_t tileWidth,
                                     uint32_t tileHeight, ScalingMode::Enum scalingModeLvl2)
    {
        switch (scalingModeLvl2) {
            case ScalingMode::Enum::Scaling0D:
                return calculateNumTilesLevel2(width, height, tileWidth, tileHeight);
            case ScalingMode::Enum::Scaling1D:
                return ceilDiv(ceilDiv(width, 2u), tileWidth) * ceilDiv(height, tileHeight);
            case ScalingMode::Enum::Scaling2D:
                return ceilDiv(ceilDiv(width, 2u), tileWidth) * ceilDiv(ceilDiv(height, 2u), tileHeight);
            case ScalingMode::Enum::Invalid: VNAssert(false); break; // NOLINT(misc-static-assert)
        }
        return 0;
    }

    const char* frameTypeToString(FrameType type)
    {
        switch (type) {
            case FrameType::I: return "I-frame";
            case FrameType::P: return "P-frame";
            case FrameType::B: return "B-frame";
            case FrameType::KeyFrame: return "KeyFrame";
            case FrameType::InterFrame: return "InterFrame";
            case FrameType::Unknown: break;
        }

        return "Invalid Frame";
    }

} // namespace

PlaneMode::Enum PlaneMode::FromValue(uint8_t val)
{
    if (val < static_cast<uint8_t>(Enum::Invalid)) {
        return static_cast<Enum>(val);
    }

    VNAssert(false); // NOLINT(misc-static-assert)
    return Enum::Invalid;
}

const char* PlaneMode::ToString(PlaneMode::Enum val)
{
    static constexpr std::array<const char*, 2> sPlaneModeLUT = {"Y", "YUV"};

    if (val < Enum::Invalid) {
        return sPlaneModeLUT[static_cast<uint8_t>(val)];
    }

    return "Unknown plane mode";
}

uint32_t PlaneMode::getPlaneCount(PlaneMode::Enum val)
{
    switch (val) {
        case Enum::Y: return 1;
        case Enum::YUV: return 3;
        case Enum::Invalid: VNAssert(false); break; // NOLINT(misc-static-assert)
    }

    return 0;
}

TransformType::Enum TransformType::FromValue(uint8_t val)
{
    if (val < static_cast<uint8_t>(Enum::Invalid)) {
        return static_cast<Enum>(val);
    }

    VNAssert(false); // NOLINT(misc-static-assert)
    return Enum::Invalid;
}

const char* TransformType::ToString(TransformType::Enum val)
{
    static constexpr std::array<const char*, 2> sTransformTypeLUT = {"2x2", "4x4"};

    if (val < Enum::Invalid) {
        return sTransformTypeLUT[static_cast<uint8_t>(val)];
    }

    return "Unknown transform type";
}

uint32_t TransformType::getCoeffGroupCount(TransformType::Enum val)
{
    switch (val) {
        case Enum::Transform2x2: return 4;
        case Enum::Transform4x4: return 16;
        case Enum::Invalid: VNAssert(false); break; // NOLINT(misc-static-assert)
    }
    return 0;
}

uint32_t TransformType::getTUSize(Enum val)
{
    switch (val) {
        case Enum::Transform2x2: return 2;
        case Enum::Transform4x4: return 4;
        case Enum::Invalid: VNAssert(false); break; // NOLINT(misc-static-assert)
    }
    return 0;
}

TiledSizeCompressionType::Enum TiledSizeCompressionType::FromValue(uint8_t val)
{
    if (val < static_cast<uint8_t>(Enum::Invalid)) {
        return static_cast<Enum>(val);
    }

    VNAssert(false); // NOLINT(misc-static-assert)
    return Enum::Invalid;
}

const char* TiledSizeCompressionType::ToString(Enum val)
{
    static constexpr std::array<const char*, 3> sLUT = {"none", "prefix", "prefix_diff"};

    if (val < Enum::Invalid) {
        return sLUT[static_cast<uint8_t>(val)];
    }

    return "Unknown tiled size compression type";
}

const auto kToHex = [](uint8_t val) {
    std::ostringstream oss;
    oss << "0x" << std::uppercase << std::setfill('0') << std::hex << std::setw(2)
        << static_cast<int>(val);
    return oss.str();
};

Parser::Parser(const Config& config)
    : m_config(config)
{
    if (!config.logPath.empty()) {
        m_logFile.open(config.logPath, std::ios::trunc);
        if (!m_logFile.is_open()) {
            VNLog::Error("Failed to open log file %s\n", config.logPath.c_str());
            throw FileError("Failed to open log file");
        }
    }
    if (m_config.logFormat == LogFormat::JSON) {
        m_jsonLog.clear();
        m_jsonLog["frames"] = nlohmann::ordered_json::array();
    }
}

Parser::~Parser()
{
    if (m_config.logFormat == LogFormat::JSON) {
        // Always dump to stdout for passing to other tools
        try {
            fprintf(stdout, "%s\n", m_jsonLog.dump(4).c_str());
        } catch (const nlohmann::json::exception& e) {
            VNLog::Error("Failed to dump json to stdout: %s\n", e.what());
        }

        // If a log path was set, also write it out to file
        if (m_config.logPath.empty() == false) {
            try {
                m_logFile << m_jsonLog.dump(4) << std::endl;
            } catch (const nlohmann::json::exception& e) {
                VNLog::Error("Failed to dump json to file: %s\n", e.what());
            }
        }
    }
}

bool Parser::parse(const LCEVC& lcevc)
{
    m_blockJson.clear();
    nlohmann::ordered_json jsonFrame;

    if (isBaseStream) {
        Output("Base : [Frame Type : %s, Frame Size : %" PRId64 "]\n",
               frameTypeToString(lcevc.frameType), lcevc.frameSize);
        jsonFrame["FrameType"] = frameTypeToString(lcevc.frameType);
        jsonFrame["FrameSize"] = lcevc.frameSize;
    }

    lcevcLayerSize += lcevc.data.size();
    Output("LCEVC Parse - [pts: %" PRId64 ", dts: %" PRId64 ", size: %u, count: %" PRId64 "]\n",
           lcevc.pts, lcevc.dts, static_cast<uint32_t>(lcevc.data.size()), m_total++);

    ordered_json lcevcJson;
    lcevcJson["PTS"] = lcevc.pts;
    lcevcJson["DTS"] = lcevc.dts;
    lcevcJson["Size"] = static_cast<uint32_t>(lcevc.data.size());
    lcevcJson["count"] = m_total - 1;

    const auto* head = lcevc.data.data();

    const uint32_t headerSize = isFourBytePrefix ? lcevc::kPrefixHeaderSize : lcevc::kHeaderSize;
    m_nalType = lcevc::readNalHeader(head, lcevc.data.size(), headerSize);

    if (m_nalType == lcevc::NalUnitType::Enum::Invalid) {
        VNLog::Debug("\tERROR: Unrecognized NAL unit type");
        return false;
    }

    std::vector<uint8_t> buffer(lcevc.data.size(), 0);

    head += headerSize;
    const auto length = nalUnencapsulate(buffer.data(), head, lcevc.data.size() - headerSize);

    OutputVerbose("\tLCEVC %s NAL unit\n", lcevc::NalUnitType::ToString(m_nalType));
    Output("\tRaw LCEVC data of size %u\n", length);

    lcevcJson["NAL Type"] = lcevc::NalUnitType::ToString(m_nalType);
    lcevcJson["RawSize"] = length;

    ordered_json blocksJson = ordered_json::array();

    m_reader.Reset(buffer.data(), length);
    m_temporalSignaled = false;

    // Blocks
    while (m_reader.IsValid()) {
        // Block header.
        auto blockHeader = m_reader.ReadValue<uint8_t>();
        uint8_t blockSizeType = ((blockHeader & 0xE0) >> 5);
        uint32_t blockSize = 0;

        if (blockSizeType == kBlockSizeTypeCustom) {
            blockSize = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
            if (blockSize == std::numeric_limits<uint32_t>::max()) {
                VNLog::Error("\tERROR: Block size is max uint, this is likely wrong, check size "
                             "type LUT is fully populated with latest values\n");
                return false;
            }
        } else if (blockSizeType == kBlockSizeTypeReserved) {
            VNLog::Error("\tERROR: Reserved payload_size_type (%d)\n", kBlockSizeTypeReserved);
            return false;
        } else if (blockSizeType < kBlockSizeTypeLUT.size()) {
            blockSize = kBlockSizeTypeLUT[blockSizeType];
        } else {
            VNLog::Error("\tERROR: Unrecognised block size signaling\n");
            return false;
        }

        BlockType::Enum blockType = BlockType::FromValue(blockHeader & 0x1F);

        OutputVerbose("\t%s [size: %u, type: %u]\n", BlockType::ToString(blockType), blockSize,
                      blockSizeType);

        m_blockJson["BlockName"] = BlockType::ToString(blockType);
        m_blockJson["Size"] = blockSize;
        m_blockJson["Type"] = blockSizeType;

        uint64_t expectedPosition = m_reader.GetPosition() + blockSize;

        bool bBlockDecodeResult = true;

        switch (blockType) {
            case BlockType::Enum::SequenceConfig: bBlockDecodeResult = parseSequenceConfig(); break;
            case BlockType::Enum::GlobalConfig: bBlockDecodeResult = parseGlobalConfig(); break;
            case BlockType::Enum::PictureConfig: bBlockDecodeResult = parsePictureConfig(); break;
            case BlockType::Enum::EncodedData: bBlockDecodeResult = parseEncodedData(); break;
            case BlockType::Enum::EncodedDataTiled:
                bBlockDecodeResult = parseEncodedTileData(blockSize);
                break;
            case BlockType::Enum::AdditionalInfo:
                bBlockDecodeResult = parseAdditionalInfo(blockSize);
                break;
            case BlockType::Enum::Filler: {
                OutputVerbose("\tSkipping block\n");
                parseSkipBlock(blockSize);
                break;
            }
            default:
                VNLog::Error("\tERROR: Failed to parse block of unknown type: %u\n", blockType);
                return false;
        }

        if (!bBlockDecodeResult) {
            VNLog::Error("\tERROR: Failed to decode block [%u - %s] correctly\n", blockType,
                         BlockType::ToString(blockType));
            return false;
        }

        if (expectedPosition != m_reader.GetPosition()) {
            VNLog::Error("\tERROR: Failed to parse block [%s] correctly, offset is "
                         "not what is "
                         "expected: %" PRIu64 ", got: %" PRIu64 "\n",
                         BlockType::ToString(blockType), expectedPosition, m_reader.GetPosition());
            return false;
        }
        blocksJson.push_back(m_blockJson);
        m_blockJson.clear();
    }
    if (m_config.verboseOutput) {
        lcevcJson["Blocks"] = blocksJson;
    }
    jsonFrame["LCEVC"] = lcevcJson;

    m_jsonLog["frames"].push_back(jsonFrame);

    return true;
}

void Parser::checkProfileandLevel(uint8_t profile, uint8_t level) const
{
    if (profile != lvccProfile) {
        VNLog::Info("\tWarning: Profile value differs from the lvcC Atom.\n");
    }
    if (level != lvccLevel) {
        VNLog::Info("\tWarning: Level value differs from the lvcC Atom.\n");
    }
}

bool Parser::parseSequenceConfig()
{
    const auto field0 = m_reader.ReadValue<uint8_t>();
    const auto field1 = m_reader.ReadValue<uint8_t>();

    const uint8_t profile = (field0 & 0xF0) >> 4;
    const uint8_t level = (field0 & 0x0F);
    const uint8_t sublevel = (field1 & 0xC0) >> 6;
    const uint8_t window = (field1 & 0x20) >> 5;
    const uint8_t seqReservedZeros5 = (field1 & 0x1F);

    if (bLvccPresent) {
        checkProfileandLevel(profile, level);
    }

    OutputVerbose("\t\t%-20s| %u\n", "Profile", profile);
    OutputVerbose("\t\t%-20s| %u.%u\n", "Level", level, sublevel);
    OutputVerbose("\t\t%-20s| %u\n", "Window", window);
    if (seqReservedZeros5 != 0) {
        VNLog::Info("\tWarning: Reserved bits (sequence_config) not zero: 0x%02x\n", seqReservedZeros5);
    }

    m_blockJson["Profile"] = profile;
    m_blockJson["Level"] = level;
    m_blockJson["SubLevel"] = sublevel;
    m_blockJson["Window"] = window;

    if (profile == 15 || level == 15) {
        const auto extField = m_reader.ReadValue<uint8_t>();

        const uint8_t profileExt = (extField & 0xE0) >> 5;
        const uint8_t levelExt = (extField & 0x1E) >> 1;
        const uint8_t seqReservedZero1 = (extField & 0x01);

        OutputVerbose("\t\t%-20s| %u\n", "Profile Ext", profileExt);
        OutputVerbose("\t\t%-20s| %u\n", "Level Ext", levelExt);

        m_blockJson["ProfileExt"] = profileExt;
        m_blockJson["LevelExt"] = levelExt;
        if (seqReservedZero1 != 0) {
            VNLog::Info("\tWarning: Reserved bit (sequence_config extension) not zero\n");
        }
    }

    if (window) {
        const auto left = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
        const auto right = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
        const auto top = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
        const auto bottom = static_cast<uint32_t>(m_reader.ReadMultiByteValue());

        OutputVerbose("\t\t%-20s| %u\n", "Window Left", left);
        OutputVerbose("\t\t%-20s| %u\n", "Window Right", right);
        OutputVerbose("\t\t%-20s| %u\n", "Window Top", top);
        OutputVerbose("\t\t%-20s| %u\n", "Window Bottom", bottom);

        m_blockJson["WindowLeft"] = left;
        m_blockJson["WindowRight"] = right;
        m_blockJson["WindowTop"] = top;
        m_blockJson["WindowBottom"] = bottom;
    }

    return true;
}

bool Parser::parseGlobalConfig()
{
    const auto field0 = m_reader.ReadValue<uint8_t>();
    const auto field1 = m_reader.ReadValue<uint8_t>();
    const auto field2 = m_reader.ReadValue<uint8_t>();
    const auto field3 = m_reader.ReadValue<uint8_t>();

    const uint8_t planeModeFlag = (field0 & 0x80) >> 7;
    const uint8_t resolutionType = (field0 & 0x7E) >> 1;
    m_transformType = TransformType::FromValue(field0 & 0x01);
    const ChromaSamplingType::Enum chromaSamplingType =
        ChromaSamplingType::FromValue((field1 & 0xC0) >> 6);
    const BitDepthType::Enum baseDepthType = BitDepthType::FromValue((field1 & 0x30) >> 4);
    const BitDepthType::Enum enhancementDepthType = BitDepthType::FromValue((field1 & 0x0C) >> 2);
    const uint8_t temporalStepWidthModifierEnabled = (field1 & 0x02) >> 1;
    const uint8_t predictedAverage = (field1 & 0x01);
    const uint8_t temporalReducedSignalling = (field2 & 0x80) >> 7;
    const uint8_t temporalEnabled = (field2 & 0x40) >> 6;
    const UpsampleType::Enum upsampleType = UpsampleType::FromValue((field2 & 0x38) >> 3);
    const uint8_t deblocking = (field2 & 0x04) >> 2;
    const ScalingMode::Enum scalingModeLevel1 = ScalingMode::FromValue(field2 & 0x03);
    const ScalingMode::Enum scalingModeLevel2 = ScalingMode::FromValue((field3 & 0xC0) >> 6);
    const TilingMode::Enum tilingMode = TilingMode::FromValue((field3 & 0x30) >> 4);
    const UserDataMode::Enum userDataMode = UserDataMode::FromValue((field3 & 0x0C) >> 2);
    const uint8_t level1DepthFlag = (field3 & 0x02) >> 1;
    const uint8_t chromaMultiplierFlag = field3 & 0x01;

    m_scalingModeLevel2 = static_cast<uint8_t>(scalingModeLevel2);

    uint8_t chromaSWMultiplier = 64;

    OutputVerbose("\t\t%-20s| %u\n", "Plane mode flag", planeModeFlag);
    OutputVerbose("\t\t%-20s| %u\n", "Resolution type", resolutionType);
    OutputVerbose("\t\t%-20s| %s\n", "Transform type", TransformType::ToString(m_transformType));
    OutputVerbose("\t\t%-20s| %s\n", "Chroma Sampling Type",
                  ChromaSamplingType::ToString(chromaSamplingType));
    OutputVerbose("\t\t%-20s| %s\n", "Base depth type", BitDepthType::ToString(baseDepthType));
    OutputVerbose("\t\t%-20s| %s\n", "Enhancement depth type",
                  BitDepthType::ToString(enhancementDepthType));
    OutputVerbose("\t\t%-20s| %u\n", "Temporal Step Width Modifier", temporalStepWidthModifierEnabled);
    OutputVerbose("\t\t%-20s| %u\n", "Predicted Average", predictedAverage);
    OutputVerbose("\t\t%-20s| %u\n", "Temporal Reduced Signalling", temporalReducedSignalling);
    OutputVerbose("\t\t%-20s| %u\n", "Temporal Enabled", temporalEnabled);
    OutputVerbose("\t\t%-20s| %s\n", "Upsample type", UpsampleType::ToString(upsampleType));
    OutputVerbose("\t\t%-20s| %u\n", "Deblocking", deblocking);
    OutputVerbose("\t\t%-20s| %s\n", "Scaling Mode Level 1", ScalingMode::ToString(scalingModeLevel1));
    OutputVerbose("\t\t%-20s| %s\n", "Scaling Mode Level 2", ScalingMode::ToString(scalingModeLevel2));
    OutputVerbose("\t\t%-20s| %s\n", "Tiling Mode", TilingMode::ToString(tilingMode));
    OutputVerbose("\t\t%-20s| %s\n", "User Data", UserDataMode::ToString(userDataMode));
    OutputVerbose("\t\t%-20s| %u\n", "Level 1 Depth Flag", level1DepthFlag);

    m_blockJson["Plane mode flag"] = planeModeFlag;
    m_blockJson["Resolution type"] = resolutionType;
    m_blockJson["Transform type"] = TransformType::ToString(m_transformType);
    m_blockJson["Chroma Sampling Type"] = ChromaSamplingType::ToString(chromaSamplingType);
    m_blockJson["Base depth type"] = BitDepthType::ToString(baseDepthType);
    m_blockJson["Enhancement depth type"] = BitDepthType::ToString(enhancementDepthType);
    m_blockJson["Temporal Step Width Modifier"] = temporalStepWidthModifierEnabled;
    m_blockJson["Predicted Average"] = predictedAverage;
    m_blockJson["Temporal Reduced Signalling"] = temporalReducedSignalling;
    m_blockJson["Temporal Enabled"] = temporalEnabled;
    m_blockJson["Upsample type"] = UpsampleType::ToString(upsampleType);
    m_blockJson["Deblocking"] = deblocking;
    m_blockJson["Scaling Mode Level 1"] = ScalingMode::ToString(scalingModeLevel1);
    m_blockJson["Scaling Mode Level 2"] = ScalingMode::ToString(scalingModeLevel2);
    m_blockJson["Tiling Mode"] = TilingMode::ToString(tilingMode);
    m_blockJson["User Data"] = UserDataMode::ToString(userDataMode);
    m_blockJson["Level 1 Depth Flag"] = level1DepthFlag;

    if (planeModeFlag) {
        const auto planeModeField = m_reader.ReadValue<uint8_t>();
        m_planeMode = PlaneMode::FromValue((planeModeField & 0xF0) >> 4);
        const auto planesReservedZeros4 = static_cast<uint8_t>(planeModeField & 0x0F);
        if (planesReservedZeros4 != 0) {
            VNLog::Info("\tWarning: Reserved bits (planes_type) not zero: 0x%02x\n", planesReservedZeros4);
        }
    } else {
        m_planeMode = PlaneMode::Y;
    }
    OutputVerbose("\t\t%-20s| %s\n", "Plane mode", PlaneMode::ToString(m_planeMode));
    m_blockJson["Plane mode"] = PlaneMode::ToString(m_planeMode);

    // Conditional stuff now
    if (temporalStepWidthModifierEnabled) {
        const auto temporalStepWidthModifier = m_reader.ReadValue<uint8_t>();
        OutputVerbose("\t\t%-20s| %u\n", "Temporal Step Width Modifier", temporalStepWidthModifier);
        m_blockJson["Temporal Step Width Modifier Value"] = temporalStepWidthModifier;
    }

    if (upsampleType == UpsampleType::Enum::AdaptiveCubic) {
        const auto coeff0 = m_reader.ReadValue<uint16_t>();
        const auto coeff1 = m_reader.ReadValue<uint16_t>();
        const auto coeff2 = m_reader.ReadValue<uint16_t>();
        const auto coeff3 = m_reader.ReadValue<uint16_t>();

        OutputVerbose("\t\t%-20s| %u %u %u %u\n", "AdaptiveCubic Kernel Coeffs", coeff0, coeff1,
                      coeff2, coeff3);
        std::vector<uint32_t> coeffs = {coeff0, coeff1, coeff2, coeff3};
        m_blockJson["AdaptiveCubic Kernel Coeffs"] = nlohmann::json(coeffs);
    }

    if (deblocking) {
        const auto deblockField = m_reader.ReadValue<uint8_t>();
        const uint8_t coeff0 = (deblockField & 0xF0) >> 4;
        const uint8_t coeff1 = deblockField & 0x0F;

        OutputVerbose("\t\t%-20s| %u\n", "Deblock Coeff 0", coeff0);
        OutputVerbose("\t\t%-20s| %u\n", "Deblock Coeff 1", coeff1);
        m_blockJson["Deblock Coeff 0"] = coeff0;
        m_blockJson["Deblock Coeff 1"] = coeff1;
    }

    if (tilingMode != TilingMode::Enum::None) {
        if (tilingMode == TilingMode::Enum::Tile512x256) {
            m_tileWidth = 512;
            m_tileHeight = 256;
        } else if (tilingMode == TilingMode::Enum::Tile1024x512) {
            m_tileWidth = 1024;
            m_tileHeight = 512;
        }
        if (tilingMode == TilingMode::Enum::Custom) {
            m_tileWidth = m_reader.ReadValue<uint16_t>();
            m_tileHeight = m_reader.ReadValue<uint16_t>();

            OutputVerbose("\t\t%-20s| %u\n", "Custom Tile Width", m_tileWidth);
            OutputVerbose("\t\t%-20s| %u\n", "Custom Tile Height", m_tileHeight);
            m_blockJson["Custom Tile Width"] = m_tileWidth;
            m_blockJson["Custom Tile Height"] = m_tileHeight;
        }

        const auto tilingField = m_reader.ReadValue<uint8_t>();
        if ((tilingField & 0xF8) != 0) {
            VNLog::Info("\tWarning: Reserved bits (tiling) not zero: 0x%02x\n",
                        static_cast<unsigned>(tilingField & 0xF8));
        }

        m_compressionEntropyEnabledPerTileFlag = (tilingField & 0x04) >> 2;
        m_compressionTypeSizePerTile = TiledSizeCompressionType::FromValue(tilingField & 0x03);

        OutputVerbose("\t\t%-20s| %u\n", "Tiling Entropy Per Tile", m_compressionEntropyEnabledPerTileFlag);
        OutputVerbose("\t\t%-20s| %s [%u]\n", "Tiling Size Per Tile",
                      TiledSizeCompressionType::ToString(m_compressionTypeSizePerTile),
                      static_cast<uint8_t>(m_compressionTypeSizePerTile));
        m_blockJson["Tiling Entropy Per Tile"] = m_compressionEntropyEnabledPerTileFlag;
        m_blockJson["Tiling Size Per Tile"] =
            TiledSizeCompressionType::ToString(m_compressionTypeSizePerTile);
    }

    if (resolutionType == kResolutionTypeCustom) {
        m_width = m_reader.ReadValue<uint16_t>();
        m_height = m_reader.ReadValue<uint16_t>();
    } else if (resolutionType > 0 && resolutionType < kResolutionTypeLUT.size()) {
        const SyntaxResolution& res = kResolutionTypeLUT[resolutionType];
        m_width = res.width;
        m_height = res.height;
    }

    OutputVerbose("\t\t%-20s| %ux%u\n", "Resolution dimensions", m_width, m_height);
    m_blockJson["Resolution dimensions"] = std::to_string(m_width) + "x" + std::to_string(m_height);

    if (chromaMultiplierFlag) {
        chromaSWMultiplier = m_reader.ReadValue<uint8_t>();
    }

    OutputVerbose("\t\t%-20s| %u\n", "Chroma Step Width Multiplier", chromaSWMultiplier);
    m_blockJson["Chroma Step Width Multiplier"] = chromaSWMultiplier;

    m_temporalEnabled = (temporalEnabled != 0);
    m_globalConfigReceived = true;

    return true;
}

bool Parser::parsePictureConfig()
{
    const auto field0 = m_reader.ReadValue<uint8_t>();

    QuantMatrixMode::Enum quantMatrixMode = QuantMatrixMode::Enum::Invalid;
    uint8_t dequantOffsetEnabled = 0;
    PictureType::Enum pictureType = PictureType::Enum::Invalid;
    uint8_t temporalRefresh = 0;
    uint8_t stepWidthSublayer1Enabled = 0;
    uint8_t ditheringControlFlag = 0;

    const uint8_t noEnhancementBitFlag = (field0 & 0x80) >> 7;

    if (noEnhancementBitFlag == 0) {
        const auto field1 = m_reader.ReadValue<uint16_t>();

        quantMatrixMode = QuantMatrixMode::FromValue((field0 & 0x70) >> 4);
        dequantOffsetEnabled = (field0 & 0x08) >> 3;
        pictureType = PictureType::FromValue((field0 & 0x04) >> 2);
        temporalRefresh = (field0 & 0x02) >> 1;
        stepWidthSublayer1Enabled = field0 & 0x01;
        const uint16_t stepWidthSublayer2 = (field1 & 0xFFFE) >> 1;
        ditheringControlFlag = static_cast<uint8_t>(field1 & 0x01);

        OutputVerbose("\t\tLCEVC Enabled\n");
        OutputVerbose("\t\t%-20s| %s\n", "Quant matrix mode", QuantMatrixMode::ToString(quantMatrixMode));
        OutputVerbose("\t\t%-20s| %u\n", "Dequant offset enabled", dequantOffsetEnabled);
        OutputVerbose("\t\t%-20s| %s\n", "Picture type", PictureType::ToString(pictureType));
        OutputVerbose("\t\t%-20s| %u\n", "Temporal refresh", temporalRefresh);
        OutputVerbose("\t\t%-20s| %u\n", "Step width L-1 enabled", stepWidthSublayer1Enabled);
        OutputVerbose("\t\t%-20s| %u\n", "Step width L-2", stepWidthSublayer2);
        OutputVerbose("\t\t%-20s| %u\n", "Dithering Control Flag", ditheringControlFlag);

        m_blockJson["LCEVC Enabled"] = "true";
        m_blockJson["Quant Matrix Mode"] = QuantMatrixMode::ToString(quantMatrixMode);
        m_blockJson["Dequant Offset Enabled"] = dequantOffsetEnabled;
        m_blockJson["Picture Type"] = PictureType::ToString(pictureType);
        m_blockJson["Temporal Refresh"] = temporalRefresh;
        m_blockJson["Step Width L-1 Enabled"] = stepWidthSublayer1Enabled;
        m_blockJson["Step Width L-2"] = stepWidthSublayer2;
        m_blockJson["Dithering Control Flag"] = ditheringControlFlag;

        m_temporalSignaled = m_temporalEnabled && !temporalRefresh;
        m_bEnhancementEnabled = true;
        m_ditheringControlFlag = ditheringControlFlag;
    } else {
        pictureType = PictureType::FromValue((field0 & 0x04) >> 2);
        temporalRefresh = (field0 & 0x02) >> 1;
        const uint8_t temporalSignaled = (field0 & 0x01);

        // Inferred values
        stepWidthSublayer1Enabled = 0;
        quantMatrixMode = QuantMatrixMode::Enum::UsePrevious;
        dequantOffsetEnabled = 0;

        if (m_nalType == lcevc::NalUnitType::Enum::IDR) {
            // When dithering_control_flag is not present,
            // it is inferred to be equal to 0 for IDR picture
            ditheringControlFlag = 0;
        } else {
            // For pictures other than the IDR picture,
            // it is inferred to be equal to the value of
            // dithering_control_flag for the preceding picture
            ditheringControlFlag = m_ditheringControlFlag;
        }

        OutputVerbose("\t\tLCEVC Disabled\n");
        OutputVerbose("\t\t%-20s| %s\n", "Quant matrix mode", QuantMatrixMode::ToString(quantMatrixMode));
        OutputVerbose("\t\t%-20s| %u\n", "Dequant offset enabled", dequantOffsetEnabled);
        OutputVerbose("\t\t%-20s| %s\n", "Picture type", PictureType::ToString(pictureType));
        OutputVerbose("\t\t%-20s| %u\n", "Temporal refresh", temporalRefresh);
        OutputVerbose("\t\t%-20s| %u\n", "Temporal signalling present", temporalSignaled);
        OutputVerbose("\t\t%-20s| %u\n", "Step width L-1 enabled", stepWidthSublayer1Enabled);
        OutputVerbose("\t\t%-20s| %u\n", "Dithering Control Flag", ditheringControlFlag);

        m_blockJson["LCEVC Enabled"] = "false";
        m_blockJson["Quant Matrix Mode"] = QuantMatrixMode::ToString(quantMatrixMode);
        m_blockJson["Dequant Offset Enabled"] = dequantOffsetEnabled;
        m_blockJson["Picture Type"] = PictureType::ToString(pictureType);
        m_blockJson["Temporal Refresh"] = temporalRefresh;
        m_blockJson["Temporal Signalled"] = temporalSignaled;
        m_blockJson["Step Width L-1 Enabled"] = stepWidthSublayer1Enabled;
        m_blockJson["Dithering Control Flag"] = ditheringControlFlag;

        m_temporalSignaled = (temporalSignaled != 0);
        m_bEnhancementEnabled = false;
        m_ditheringControlFlag = ditheringControlFlag;
    }

    if (pictureType == PictureType::Enum::Field) {
        const auto fieldField = m_reader.ReadValue<uint8_t>();
        const FieldType::Enum fieldType = FieldType::FromValue((fieldField & 0x80) >> 7);
        if ((fieldField & 0x7F) != 0) {
            VNLog::Info("\tWarning: Reserved bits (field_type) not zero: 0x%02x\n",
                        static_cast<unsigned>(fieldField & 0x7F));
        }
        OutputVerbose("\t\t%-20s| %s\n", "Field type", FieldType::ToString(fieldType));
        m_blockJson["Field Type"] = FieldType::ToString(fieldType);
    }

    if (stepWidthSublayer1Enabled) {
        const auto sublayer1Field = m_reader.ReadValue<uint16_t>();
        const uint16_t stepWidthSublayer1 = (sublayer1Field & 0xFFFE) >> 1;
        const uint16_t filteringLevel1Enabled = (sublayer1Field & 0x0001);
        OutputVerbose("\t\t%-20s| %u\n", "Step width L-1", stepWidthSublayer1);
        OutputVerbose("\t\t%-20s| %u\n", "Filtering Level-1 enabled", filteringLevel1Enabled);

        m_blockJson["Step Width L-1"] = stepWidthSublayer1;
        m_blockJson["Filtering Level1 Enabled"] = filteringLevel1Enabled;
    }

    if (quantMatrixMode != QuantMatrixMode::Enum::UsePrevious &&
        quantMatrixMode != QuantMatrixMode::Enum::UseDefault) {
        const uint32_t coeffGroupCount = TransformType::getCoeffGroupCount(m_transformType);
        const uint8_t matrixCount = ((quantMatrixMode == QuantMatrixMode::Enum::CustomBoth) ||
                                     (quantMatrixMode == QuantMatrixMode::Enum::CustomSublayer2) ||
                                     (quantMatrixMode == QuantMatrixMode::Enum::CustomSublayer1))
                                        ? 1
                                        : 2;

        ordered_json quantMatrixJson;
        for (uint32_t matrixIdx = 0; matrixIdx < matrixCount; ++matrixIdx) {
            std::stringstream quantMatrixFormat;
            quantMatrixFormat << "{ ";
            std::vector<uint32_t> quantValues;

            for (uint32_t coeffGroupIdx = 0; coeffGroupIdx < coeffGroupCount; ++coeffGroupIdx) {
                const auto val = static_cast<uint32_t>(m_reader.ReadValue<uint8_t>());
                quantMatrixFormat << val;
                quantValues.push_back(val);

                if (coeffGroupIdx != (coeffGroupCount - 1)) {
                    quantMatrixFormat << ", ";
                }
            }

            quantMatrixFormat << " }";

            std::string key = "Matrix " + std::to_string(matrixIdx);
            quantMatrixJson[key] = quantValues;

            const auto quantMatrixStr = quantMatrixFormat.str();
            OutputVerbose("\t\t%-20s| Matrix %u = %s\n", "Custom quant matrix", matrixIdx,
                          quantMatrixStr.c_str());
        }
        m_blockJson["Custom Quant Matrices"] = quantMatrixJson;
    }

    if (dequantOffsetEnabled) {
        const auto dequantOffsetField = m_reader.ReadValue<uint8_t>();

        const uint8_t dequantOffsetMode = (dequantOffsetField & 0x80) >> 7;
        const uint8_t dequantOffset = (dequantOffsetField & 0x7F);

        OutputVerbose("\t\t%-20s| %u\n", "Dequant offset mode", dequantOffsetMode);
        OutputVerbose("\t\t%-20s| %u\n", "Dequant offset", dequantOffset);
        m_blockJson["Dequant Offset Mode"] = dequantOffsetMode;
        m_blockJson["Dequant Offset"] = dequantOffset;
    }

    if (ditheringControlFlag) {
        const auto ditheringField = m_reader.ReadValue<uint8_t>();
        const DitherType::Enum ditheringType = DitherType::FromValue((ditheringField & 0xC0) >> 6);
        OutputVerbose("\t\t%-20s| %s\n", "Dithering type", DitherType::ToString(ditheringType));
        m_blockJson["Dithering Type"] = DitherType::ToString(ditheringType);

        if (ditheringType != DitherType::Enum::None) {
            const uint8_t ditheringStrength = ditheringField & 0x1F;
            OutputVerbose("\t\t%-20s| %u\n", "Dithering strength", ditheringStrength);
            m_blockJson["Dithering Strength"] = ditheringStrength;
        } else if ((ditheringField & 0x1F) != 0) {
            VNLog::Info("\tWarning: Reserved bits (dithering) not zero: 0x%02x\n",
                        static_cast<unsigned>(ditheringField & 0x1F));
        }
    }

    return true;
}

bool Parser::parseEncodedData()
{
    if (!m_globalConfigReceived) {
        VNLog::Error("\tERROR: Encoded Data received before a Global Config\n");
        return false;
    }

    // RLE/Prefix Coding choice.
    uint32_t planeCount = PlaneMode::getPlaneCount(m_planeMode);
    uint32_t coeffGroupCount = TransformType::getCoeffGroupCount(m_transformType);
    uint32_t chunkCount = (!m_bEnhancementEnabled ? 0 : (2 * planeCount * coeffGroupCount)) +
                          (m_temporalSignaled ? planeCount : 0);
    static const uint8_t surfacePropCount = 2;
    uint8_t surfacePropByteCount = (((chunkCount * surfacePropCount) + 7) & ~7) >> 3;
    static const uint8_t maxSurfacePropByteCount =
        (((((2 * 3 * 16) + 3) * surfacePropCount) + 7) & ~7) >> 3;
    uint8_t surfaceProps[maxSurfacePropByteCount] = {0};

    m_reader.ReadBytes(surfaceProps, surfacePropByteCount);

    BitfieldDecoderStream<uint8_t> surfacePropsDecoder(surfaceProps, surfacePropByteCount);

    OutputVerbose("\t\t[Plane, Sub-layer, Coefficient Group]\n");

    ordered_json layeredData = ordered_json::array();

    for (uint32_t planeIndex = 0; planeIndex < planeCount; ++planeIndex) {
        if (m_bEnhancementEnabled) {
            for (uint32_t sublayer = 1; sublayer <= 2; ++sublayer) {
                for (uint32_t coeffGroup = 0; coeffGroup < coeffGroupCount; ++coeffGroup) {
                    bool bEntropyEnabled = surfacePropsDecoder.readBit();
                    bool bRLEOnly = surfacePropsDecoder.readBit();

                    if (bEntropyEnabled) {
                        uint32_t chunkDataSize = 0;
                        try {
                            chunkDataSize = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
                        } catch (const std::exception&) {
                            VNLog::Error(
                                "\tERROR: Invalid chunk size (overflow) in encoded data\n");
                            return false;
                        }
                        OutputVerbose("\t\t[%2u, %2u, %2u]   %-10s| %u\n", planeIndex, sublayer,
                                      coeffGroup, bRLEOnly ? "RLE" : "Prefix Coding", chunkDataSize);

                        layeredData.push_back({{"Plane", planeIndex},
                                               {"Sub-layer", sublayer},
                                               {"Coefficient Group", coeffGroup},
                                               {"Mode", bRLEOnly ? "RLE" : "Prefix Coding"},
                                               {"Size", chunkDataSize}});
                        m_reader.SeekForward(chunkDataSize);

                        if (sublayer == 1) {
                            sublayer1Size += chunkDataSize;
                        } else if (sublayer == 2) {
                            sublayer2Size += chunkDataSize;
                        }
                    } else {
                        OutputVerbose("\t\t[%2u, %2u, %2u]   %-10s\n", planeIndex, sublayer,
                                      coeffGroup, "Disabled");
                        layeredData.push_back({{"Plane", planeIndex},
                                               {"Sub-layer", sublayer},
                                               {"Coefficient Group", coeffGroup},
                                               {"Mode", "Disabled"}});
                    }
                }
            }
        } else {
            OutputVerbose("\t\t\tNo residual coefficient groups signaled\n");
            layeredData.push_back({{"Plane", planeIndex}, {"residual coefficient groups signaled", "No"}});
        }

        if (m_temporalSignaled) {
            bool bEntropyEnabled = surfacePropsDecoder.readBit();
            bool bRLEOnly = surfacePropsDecoder.readBit();

            if (bEntropyEnabled) {
                uint32_t chunkDataSize = 0;
                try {
                    chunkDataSize = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
                } catch (const std::exception&) {
                    VNLog::Error("\tERROR: Invalid chunk size (overflow) in temporal layer\n");
                    return false;
                }
                OutputVerbose("\t\t[%2u, temporal] %-10s| %u\n", planeIndex,
                              bRLEOnly ? "RLE" : "Prefix Coding", chunkDataSize);
                layeredData.push_back({{"Plane", planeIndex},
                                       {"Sub-layer", "temporal"},
                                       {"Mode", bRLEOnly ? "RLE" : "Prefix Coding"},
                                       {"Size", chunkDataSize}});

                m_reader.SeekForward(chunkDataSize);
                temporalSize += chunkDataSize;
            } else {
                OutputVerbose("\t\t[%2u, temporal] %-10s\n", planeIndex, "Disabled");
                layeredData.push_back(
                    {{"Plane", planeIndex}, {"Sub-layer", "temporal"}, {"Mode", "Disabled"}});
            }
        } else {
            OutputVerbose("\t\t[%2u] No temporal signaled\n", planeIndex);
            layeredData.push_back({{"Plane", planeIndex}, {"temporal signaled", "No"}});
        }
    }
    m_blockJson["Layered Data"] = layeredData;
    return true;
}

bool Parser::parseEncodedTileData(uint32_t blockSize)
{
    if (!m_globalConfigReceived) {
        VNLog::Error("\tERROR: Encoded Tiled Data received before a Global Config\n");
        return false;
    }

    if (m_tileWidth == 0 || m_tileHeight == 0) {
        VNLog::Error("\tERROR: tile dimensions are invalid, a dimension of 0 "
                     "received from global "
                     "config block, skipping block cannot determine number of "
                     "tiles. tile_width: %u "
                     "tile_height: %u\n",
                     m_tileWidth, m_tileHeight);
        parseSkipBlock(blockSize);
        return false;
    }

    // NOLINTNEXTLINE(clang-analyzer-core.DivideZero)
    if (m_tileHeight % TransformType::getTUSize(m_transformType) != 0 ||
        m_tileWidth % TransformType::getTUSize(m_transformType) != 0) {
        VNLog::Error("\tERROR: tile dimensions are invalid, tile dimension must be "
                     "divisible by TU "
                     "size. Skipping block. tile_width: %u "
                     "tile_height: %u, TU size: %u\n",
                     m_tileWidth, m_tileHeight, TransformType::getTUSize(m_transformType));
        parseSkipBlock(blockSize);
        return false;
    }

    const auto nTilesL2 =
        static_cast<int32_t>(calculateNumTilesLevel2(m_width, m_height, m_tileWidth, m_tileHeight));
    const auto nTilesL1 = static_cast<int32_t>(calculateNumTilesLevel1(
        m_width, m_height, m_tileWidth, m_tileHeight, ScalingMode::FromValue(m_scalingModeLevel2)));
    const auto planeCount = static_cast<int32_t>(PlaneMode::getPlaneCount(m_planeMode));
    const auto coeffGroupCount = static_cast<int32_t>(TransformType::getCoeffGroupCount(m_transformType));

    // Parse per-layer RLE only flag
    const size_t layerRLEOnlyFlagCount =
        (!m_bEnhancementEnabled ? 0 : static_cast<size_t>(kNumLevels * coeffGroupCount * planeCount)) +
        (static_cast<size_t>(m_temporalSignaled) * static_cast<size_t>(planeCount));
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
        (!m_bEnhancementEnabled ? 0U
                                : (static_cast<size_t>(planeCount * coeffGroupCount) *
                                   static_cast<size_t>(nTilesL1 + nTilesL2))) +
        (static_cast<size_t>(m_temporalSignaled) * static_cast<size_t>(planeCount) *
         static_cast<size_t>(nTilesL2));

    std::vector<uint8_t> entropyEnabledFlags(chunkCount, 0);

    if (m_compressionEntropyEnabledPerTileFlag) {
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
            enabledFlag = entropyEnabledFlagDecoder.readBit();
        }
    }

    OutputVerbose("\t\t[Plane, Sub-layer, Coefficient Group, Tile]\n");

    int32_t chunkIndex = 0;

    ordered_json layeredTileData = ordered_json::array();

    // Parsing of chunk size
    for (int32_t plane = 0; plane < planeCount; plane++) {
        if (m_bEnhancementEnabled) {
            for (uint32_t sublayer = 1; sublayer <= 2; ++sublayer) {
                const int32_t tileCount = (sublayer == 1 ? nTilesL2 : nTilesL1);

                for (int32_t coeffGroup = 0; coeffGroup < coeffGroupCount; coeffGroup++) {
                    bool bRLEOnlyFlag = rleFlagDecoder.readBit();

                    std::vector<uint32_t> decompressedChunkSizes;
                    uint32_t decompressedChunkSizesIndex = 0;

                    if (m_compressionTypeSizePerTile != TiledSizeCompressionType::Enum::None) {
                        // Determine number of enabled flags.
                        uint32_t signalledSizes = 0;
                        for (int32_t i = chunkIndex; i < chunkIndex + tileCount; ++i) {
                            signalledSizes += entropyEnabledFlags[i] ? 1 : 0;
                        }

                        // Decompress sizes into buffer.
                        entropyDecodeSizes(m_reader, signalledSizes, m_compressionTypeSizePerTile,
                                           decompressedChunkSizes);
                    }

                    for (int32_t tile = 0; tile < tileCount; tile++) {
                        const bool bEntropyEnabledFlag = entropyEnabledFlags[chunkIndex++];

                        if (bEntropyEnabledFlag) {
                            uint32_t chunkDataSize = 0;

                            if (m_compressionTypeSizePerTile == TiledSizeCompressionType::Enum::None) {
                                try {
                                    chunkDataSize = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
                                } catch (const std::exception&) {
                                    VNLog::Error(
                                        "\tERROR: Invalid chunk size (overflow) in tiled layer\n");
                                    return false;
                                }
                            } else {
                                // Read from decompressed representation.
                                if (decompressedChunkSizesIndex >=
                                    static_cast<uint32_t>(decompressedChunkSizes.size())) {
                                    VNLog::Error("\tERROR: Invalid decompressed chunk size "
                                                 "index in residual layer");
                                    return false;
                                }

                                chunkDataSize = decompressedChunkSizes[decompressedChunkSizesIndex++];
                            }

                            OutputVerbose("\t\t[%2u, %2u, %2u, %2u] %-10s| %u\n", plane, sublayer,
                                          coeffGroup, tile, bRLEOnlyFlag ? "RLE" : "Prefix Coding",
                                          chunkDataSize);

                            layeredTileData.push_back({{"Plane", plane},
                                                       {"Sub-layer", sublayer},
                                                       {"Coefficient Group", coeffGroup},
                                                       {"Tile", tile},
                                                       {"Mode", bRLEOnlyFlag ? "RLE" : "Prefix Coding"},
                                                       {"Size", chunkDataSize}});

                            m_reader.SeekForward(chunkDataSize);

                            if (sublayer == 1) {
                                sublayer1Size += chunkDataSize;
                            } else if (sublayer == 2) {
                                sublayer2Size += chunkDataSize;
                            }
                        } else {
                            OutputVerbose("\t\t[%2u, %2u, %2u, %2u] %-10s\n", plane, sublayer,
                                          coeffGroup, tile, "Disabled");
                            layeredTileData.push_back({{"Plane", plane},
                                                       {"Sub-layer", sublayer},
                                                       {"Coefficient Group", coeffGroup},
                                                       {"Tile", tile},
                                                       {"Mode", "Disabled"}});
                        }
                    }
                }
            }
        } else {
            OutputVerbose("\t\t\tNo residual coefficient groups signaled\n");
            layeredTileData.push_back({{"Plane", plane}, {"residual coefficient groups signaled", "No"}});
        }

        if (m_temporalSignaled) {
            bool bRLEOnlyFlag = rleFlagDecoder.readBit();

            std::vector<uint32_t> decompressedChunkSizes;
            uint32_t decompressedChunkSizesIndex = 0;

            if (m_compressionTypeSizePerTile != TiledSizeCompressionType::Enum::None) {
                // Determine number of enabled flags.
                uint32_t signalledSizes = 0;
                for (int32_t i = chunkIndex; i < chunkIndex + nTilesL2; ++i) {
                    signalledSizes += entropyEnabledFlags[i] ? 1 : 0;
                }

                // Decompress sizes into buffer.
                entropyDecodeSizes(m_reader, signalledSizes, m_compressionTypeSizePerTile,
                                   decompressedChunkSizes);
            }

            for (int32_t tile = 0; tile < nTilesL2; tile++) {
                const bool bEntropyEnabledFlag = entropyEnabledFlags[chunkIndex++];

                if (bEntropyEnabledFlag) {
                    uint32_t chunkDataSize = 0;

                    if (m_compressionTypeSizePerTile == TiledSizeCompressionType::Enum::None) {
                        try {
                            chunkDataSize = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
                        } catch (const std::exception&) {
                            VNLog::Error(
                                "\tERROR: Invalid chunk size (overflow) in tiled temporal layer\n");
                            return false;
                        }
                    } else {
                        // Read from decompressed representation.
                        if (decompressedChunkSizesIndex >=
                            static_cast<uint32_t>(decompressedChunkSizes.size())) {
                            VNLog::Error(
                                "\tERROR: Invalid decompressed chunk size index in temporal "
                                "layer");
                            return false;
                        }

                        chunkDataSize = decompressedChunkSizes[decompressedChunkSizesIndex++];
                    }

                    OutputVerbose("\t\t[%2u, %2u, temporal] %-10s| %u\n", plane, tile,
                                  bRLEOnlyFlag ? "RLE" : "Prefix Coding", chunkDataSize);
                    layeredTileData.push_back({{"Plane", plane},
                                               {"Tile", tile},
                                               {"Sub-layer", "temporal"},
                                               {"Mode", bRLEOnlyFlag ? "RLE" : "Prefix Coding"},
                                               {"Size", chunkDataSize}});
                    m_reader.SeekForward(chunkDataSize);
                    temporalSize += chunkDataSize;
                } else {
                    OutputVerbose("\t\t[%2u, %2u, temporal] %-10s\n", plane, tile, "Disabled");
                    layeredTileData.push_back(
                        {{"Plane", plane}, {"Tile", tile}, {"Sub-layer", "temporal"}, {"Mode", "Disabled"}});
                }
            }
        } else {
            OutputVerbose("\t\t[%2u] No temporal signaled\n", plane);
            layeredTileData.push_back({{"Plane", plane}, {"temporal signaled", "No"}});
        }
    }
    m_blockJson["Layered Tile Data"] = layeredTileData;

    return true;
}

bool Parser::parseAdditionalInfo(uint32_t blockSize)
{
    const AdditionalInfoType::Enum additionalInfoType =
        AdditionalInfoType::FromValue(m_reader.ReadValue<uint8_t>());

    OutputVerbose("\t\t%-20s| %s\n", "Info Type", AdditionalInfoType::ToString(additionalInfoType));
    m_blockJson["Info Type"] = AdditionalInfoType::ToString(additionalInfoType);

    if (additionalInfoType == AdditionalInfoType::Enum::SFilter) {
        const auto field0 = m_reader.ReadValue<uint8_t>();

        const SFilterMode::Enum sfilterMode = SFilterMode::FromValue((field0 & 0xE0) >> 5);
        const uint8_t sfilterStrength = field0 & 0x1F;

        OutputVerbose("\t\t%-20s| %s\n", "S-Filter Mode", SFilterMode::ToString(sfilterMode));
        OutputVerbose("\t\t%-20s| %u\n", "S-Filter Strength", sfilterStrength);
        m_blockJson["S-Filter Mode"] = SFilterMode::ToString(sfilterMode);
        m_blockJson["S-Filter Strength"] = sfilterStrength;
    } else if (additionalInfoType == AdditionalInfoType::Enum::BaseHash) {
        const auto baseHash = m_reader.ReadValue<uint64_t>();
        OutputVerbose("\t\t%-20s| %" PRIu64 "\n", "Base Hash", baseHash);
        m_blockJson["Base Hash"] = baseHash;
    } else if (additionalInfoType == AdditionalInfoType::Enum::SEIPayload) {
        SEIPayloadType::Enum seiPayloadType = SEIPayloadType::FromValue(m_reader.ReadValue<uint8_t>());
        OutputVerbose("\t\t%-20s| %s (%u)\n", "SEI Payload Type",
                      SEIPayloadType::ToString(seiPayloadType), seiPayloadType);
        m_blockJson["SEI Payload Type"] = SEIPayloadType::ToString(seiPayloadType);

        if (seiPayloadType == SEIPayloadType::Enum::MasteringDisplayColourVolume) {
            static constexpr uint32_t kMDCVNumPrimaries = 3;

            uint16_t displayPrimariesX[kMDCVNumPrimaries];
            uint16_t displayPrimariesY[kMDCVNumPrimaries];

            for (uint32_t i = 0; i < kMDCVNumPrimaries; i++) {
                displayPrimariesX[i] = m_reader.ReadValue<uint16_t>();
                displayPrimariesY[i] = m_reader.ReadValue<uint16_t>();
            }

            OutputVerbose("\t\t%-20s| %u %u %u\n", "Display Primaries X", displayPrimariesX[0],
                          displayPrimariesX[1], displayPrimariesX[2]);
            OutputVerbose("\t\t%-20s| %u %u %u\n", "Display Primaries Y", displayPrimariesY[0],
                          displayPrimariesY[1], displayPrimariesY[2]);
            m_blockJson["Display Primaries X"] = {displayPrimariesX[0], displayPrimariesX[1],
                                                  displayPrimariesX[2]};
            m_blockJson["Display Primaries Y"] = {displayPrimariesY[0], displayPrimariesY[1],
                                                  displayPrimariesY[2]};

            const auto whitePointX = m_reader.ReadValue<uint16_t>();
            const auto whitePointY = m_reader.ReadValue<uint16_t>();

            OutputVerbose("\t\t%-20s| %u\n", "White Point X", whitePointX);
            OutputVerbose("\t\t%-20s| %u\n", "White Point Y", whitePointY);
            m_blockJson["White Point X"] = whitePointX;
            m_blockJson["White Point Y"] = whitePointY;

            const auto maxDisplayMasteringLuminance = m_reader.ReadValue<uint32_t>();
            const auto minDisplayMasteringLuminance = m_reader.ReadValue<uint32_t>();

            OutputVerbose("\t\t%-20s| %u\n", "Max Display Mastering Luminance", maxDisplayMasteringLuminance);
            OutputVerbose("\t\t%-20s| %u\n", "Min Display Mastering Luminance", minDisplayMasteringLuminance);
            m_blockJson["Max Display Mastering Luminance"] = maxDisplayMasteringLuminance;
            m_blockJson["Min Display Mastering Luminance"] = minDisplayMasteringLuminance;
        } else if (seiPayloadType == SEIPayloadType::Enum::ContentLightLevelInfo) {
            const auto maxContentLightLevel = m_reader.ReadValue<uint16_t>();
            const auto maxPicAverageLightLevel = m_reader.ReadValue<uint16_t>();

            OutputVerbose("\t\t%-20s| %u\n", "Max Content Light Level", maxContentLightLevel);
            OutputVerbose("\t\t%-20s| %u\n", "Max Picture Average Light Level", maxPicAverageLightLevel);
            m_blockJson["Max Content Light Level"] = maxContentLightLevel;
            m_blockJson["Max Picture Average Light Level"] = maxPicAverageLightLevel;
        } else if (seiPayloadType == SEIPayloadType::Enum::UserDataRegistered) {
            const auto countryCode = m_reader.ReadValue<uint8_t>();
            uint32_t readAmount = 0;
            bool bFound = false;

            OutputVerbose("\t\t%-20s| 0x%02x\n", "T35 Country Code", countryCode);
            m_blockJson["T35 Country Code"] = countryCode;

            // UK country code.
            if (countryCode == kT35VNovaCode[0]) {
                // Check for V-Nova code.
                uint8_t t35Remainder[kT35CodeLength - 1] = {};
                m_reader.ReadBytes(t35Remainder, kT35CodeLength - 1);
                bFound = (memcmp(kT35VNovaCode + 1, t35Remainder, kT35CodeLength - 1) == 0);

                OutputVerbose("\t\t%-20s| [0x%02x, 0x%02x, 0x%02x]\n", "T35 Remaining Code",
                              t35Remainder[0], t35Remainder[1], t35Remainder[2]);

                std::vector<std::string> t35HexVec = {kToHex(t35Remainder[0]), kToHex(t35Remainder[1]),
                                                      kToHex(t35Remainder[2])};
                m_blockJson["T35 Remaining Code"] = t35HexVec;

                readAmount = kT35CodeLength;
            } else {
                readAmount = 1;
            }

            if (bFound) {
                // Parse V-Nova
                OutputVerbose("\t\t%-20s\n", "V-Nova Payload");
                m_bitstreamVersion = m_reader.ReadValue<uint8_t>();
                OutputVerbose("\t\t%-20s| %u\n", "Bitstream Version", m_bitstreamVersion);
                if (m_bitstreamVersion != kSupportedVersion) {
                    VNLog::Error("\tERROR: Bitstream version %u is unsupported. "
                                 "Supported version is %u.\n",
                                 m_bitstreamVersion, kSupportedVersion);
                    return false;
                }
                m_blockJson["V-Nova Payload"]["Bitstream Version"] = m_bitstreamVersion;
            } else {
                // Skip unknown
                parseSkipBlock(blockSize - readAmount);
            }
        } else {
            OutputVerbose("\t\t%-20s|\n", "Invalid SEI Payload Type");
            m_blockJson["SEI Error"] = "Invalid SEI Payload Type";
        }
    } else if (additionalInfoType == AdditionalInfoType::Enum::VUIParameters) {
        const bool bIsAspectRatioInfoPresentFlag = m_reader.ReadBit();
        OutputVerbose("\t\t%-20s| %s\n", "Aspect Ratio Info Present",
                      bIsAspectRatioInfoPresentFlag ? "true" : "false");
        m_blockJson["Aspect Ratio Info Present"] = bIsAspectRatioInfoPresentFlag;

        if (bIsAspectRatioInfoPresentFlag) {
            const auto aspectRatioIdc = m_reader.ReadBits<uint8_t>();
            OutputVerbose("\t\t%-20s| %u\n", "Aspect Ratio Idc", aspectRatioIdc);
            m_blockJson["Aspect Ratio Idc"] = aspectRatioIdc;

            if (aspectRatioIdc == kVUIAspectRatioIDCExtendedSAR) {
                const auto sarWidth = m_reader.ReadBits<uint16_t>();
                const auto sarHeight = m_reader.ReadBits<uint16_t>();

                OutputVerbose("\t\t%-20s| %u\n", "SAR Width", sarWidth);
                OutputVerbose("\t\t%-20s| %u\n", "SAR Height", sarHeight);
                m_blockJson["SAR Width"] = sarWidth;
                m_blockJson["SAR Height"] = sarHeight;
            }
        }

        // Overscan Info
        const bool overscanInfoPresentFlag = m_reader.ReadBit();
        OutputVerbose("\t\t%-20s| %s\n", "Overscan Info Present", overscanInfoPresentFlag ? "true" : "false");
        m_blockJson["Overscan Info Present"] = overscanInfoPresentFlag;

        if (overscanInfoPresentFlag) {
            const bool overscanAppropriateFlag = m_reader.ReadBit();

            OutputVerbose("\t\t%-20s| %s\n", "Overscan Appropriate Flag",
                          overscanAppropriateFlag ? "true" : "false");
            m_blockJson["Overscan Appropriate Flag"] = overscanAppropriateFlag;
        }

        // Video Signal Info
        const bool videoSignalTypePresentFlag = m_reader.ReadBit();
        OutputVerbose("\t\t%-20s| %s\n", "Video Signal Type Present",
                      videoSignalTypePresentFlag ? "true" : "false");
        m_blockJson["Video Signal Type Present"] = videoSignalTypePresentFlag;

        if (videoSignalTypePresentFlag) {
            const auto videoFormat = m_reader.ReadBits<uint8_t, 3>();
            const bool videoFullRangeFlag = m_reader.ReadBit();

            OutputVerbose("\t\t%-20s| %u\n", "Video Format", videoFormat);
            OutputVerbose("\t\t%-20s| %s\n", "Video Full Range", videoFullRangeFlag ? "true" : "false");

            m_blockJson["Video Format"] = videoFormat;
            m_blockJson["Video Full Range"] = videoFullRangeFlag;

            // Colour Description Info
            const bool colourDescriptionPresentFlag = m_reader.ReadBit();
            OutputVerbose("\t\t%-20s| %s\n", "Colour Description Present",
                          colourDescriptionPresentFlag ? "true" : "false");
            m_blockJson["Colour Description Present"] = colourDescriptionPresentFlag;

            if (colourDescriptionPresentFlag) {
                const auto colourPrimaries = m_reader.ReadBits<uint8_t>();
                const auto transferCharacteristics = m_reader.ReadBits<uint8_t>();
                const auto matrixCoefficients = m_reader.ReadBits<uint8_t>();

                OutputVerbose("\t\t%-20s| %u\n", "Colour Primaries", colourPrimaries);
                OutputVerbose("\t\t%-20s| %u\n", "Transfer Characteristics", transferCharacteristics);
                OutputVerbose("\t\t%-20s| %u\n", "Matrix Coefficients", matrixCoefficients);

                m_blockJson["Colour Primaries"] = colourPrimaries;
                m_blockJson["Transfer Characteristics"] = transferCharacteristics;
                m_blockJson["Matrix Coefficients"] = matrixCoefficients;
            }
        }

        // Chroma LOC Info
        const bool chromaLOCInfoPresentFlag = m_reader.ReadBit();
        OutputVerbose("\t\t%-20s| %s\n", "Chroma LOC Info Present",
                      chromaLOCInfoPresentFlag ? "true" : "false");
        m_blockJson["Chroma LOC Info Present"] = chromaLOCInfoPresentFlag;

        if (chromaLOCInfoPresentFlag) {
            const uint32_t chromaSampleLocTypeTopField = m_reader.ReadUnsignedExpGolombBits();
            const uint32_t chromaSampleLocTypeBottomField = m_reader.ReadUnsignedExpGolombBits();

            OutputVerbose("\t\t%-20s| %u\n", "Chroma Sample LOC Type Top Field", chromaSampleLocTypeTopField);
            OutputVerbose("\t\t%-20s| %u\n", "Chroma Sample LOC Type Bottom Field",
                          chromaSampleLocTypeBottomField);
            m_blockJson["Chroma Sample LOC Type Top Field"] = chromaSampleLocTypeTopField;
            m_blockJson["Chroma Sample LOC Type Bottom Field"] = chromaSampleLocTypeBottomField;
        }

        // Flushing bits
        m_reader.ResetBitfield();
    } else if (additionalInfoType == AdditionalInfoType::Enum::HDR) {
        const bool enhancementDynamicRangeFlag = m_reader.ReadBit();
        OutputVerbose("\t\t%-20s| %u\n", "Tonemapper Location", enhancementDynamicRangeFlag);

        const auto toneMapperType = m_reader.ReadBits<uint8_t, 5>();
        OutputVerbose("\t\t%-20s| %u\n", "Tone Mapper Type", toneMapperType);

        const bool tonemapperDataPresent = m_reader.ReadBit();
        OutputVerbose("\t\t%-20s| %s\n", "Tonemapper Data Present", tonemapperDataPresent ? "true" : "false");
        const bool deinterlacerEnabledFlag = m_reader.ReadBit();
        OutputVerbose("\t\t%-20s| %s\n", "Deinterlacer Enabled", deinterlacerEnabledFlag ? "true" : "false");

        m_blockJson["Tonemapper Location"] = enhancementDynamicRangeFlag;
        m_blockJson["Tone Mapper Type"] = toneMapperType;
        m_blockJson["Tonemapper Data Present"] = tonemapperDataPresent;
        m_blockJson["Deinterlacer Enabled"] = deinterlacerEnabledFlag;

        if (toneMapperType == 31) {
            const auto toneMapperTypeExtended = m_reader.ReadBits<uint8_t, 8>();
            OutputVerbose("\t\t%-20s| %u\n", "Tone Mapper Type Extended", toneMapperTypeExtended);
            m_blockJson["Tone Mapper Type Extended"] = toneMapperTypeExtended;
        }

        if (deinterlacerEnabledFlag) {
            const auto deinterlacerType = m_reader.ReadBits<uint8_t, 4>();
            OutputVerbose("\t\t%-20s| %u\n", "Deinterlacer Type", deinterlacerType);
            const bool topFieldFirstFlag = m_reader.ReadBit();
            OutputVerbose("\t\t%-20s| %s\n", "Top Field First", topFieldFirstFlag ? "true" : "false");

            m_blockJson["Deinterlacer Type"] = deinterlacerType;
            m_blockJson["Top Field First"] = topFieldFirstFlag;

            const auto zeroBits = m_reader.ReadBits<uint8_t, 3>();
            VNUnused(zeroBits);
        }
    } else {
        // Skip remainder of the block.
        OutputVerbose("\t\tUnhandled Additional Info Type - Skipping\n");
        m_blockJson["Unhandled Info Type"] = static_cast<uint32_t>(additionalInfoType);
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
        VNLog::Error("\tERROR: Invalid initial symbol state, expected to be 0 or 1");
        return false;
    }

    while (count < total) {
        // Read current symbol run-length and write that many values into destination
        uint32_t runLength = 0;
        try {
            runLength = static_cast<uint32_t>(m_reader.ReadMultiByteValue());
        } catch (const std::overflow_error&) {
            VNLog::Error("\tERROR: Invalid run-length (overflow) in entropy flags RLE\n");
            return false;
        }

        // Safety first.
        if ((count + runLength) > total) {
            VNLog::Error("\tERROR: Invalid run-length, attempting to decode more symbols that "
                         "available\n");
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

template <std::size_t N, typename... Args>
void Parser::Output(const char (&fmt)[N], Args&&... args)
{
    char buffer[1024];
    if constexpr (sizeof...(args) == 0) {
        std::snprintf(buffer, sizeof(buffer), "%s", fmt);
    } else {
        std::snprintf(buffer, sizeof(buffer), fmt, std::forward<Args>(args)...);
    }

    if (m_config.logFormat == LogFormat::Text) {
        const int length = fprintf(stdout, "%s", buffer);
        if (m_logFile.is_open() && length > 0) {
            m_logFile.write(buffer, length);
        }
    } else {
        fprintf(stderr, "%s", buffer);
    }
}

template <std::size_t N, typename... Args>
void Parser::OutputVerbose(const char (&fmt)[N], Args&&... args)
{
    if (m_config.verboseOutput) {
        Output(fmt, std::forward<Args>(args)...);
    }
}

} // namespace vnova::analyzer
