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
#ifndef VN_PARSED_TYPES_H_
#define VN_PARSED_TYPES_H_

#include <optional>
#include <string>
// section
//
#include <json.hpp>

#include <array>
#include <cstdint>

// NOLINTBEGIN(readability-identifier-naming,readability-identifier-length)

namespace vnova::analyzer {

template <typename T>
static T FromValue(uint8_t value)
{
    auto testEnum = static_cast<T>(value);
    if (nlohmann::json(testEnum).is_null()) {
        return T::UNKNOWN;
    }

    return testEnum;
}

template <typename T>
static std::string ToString(T value)
{
    const nlohmann::json json = nlohmann::json(value);
    return json.is_null() ? "UNKNOWN" : json.dump();
}

enum class BlockType : int8_t
{
    UNKNOWN = -1,
    SEQUENCE_CONFIG = 0,
    GLOBAL_CONFIG = 1,
    PICTURE_CONFIG = 2,
    ENCODED_DATA = 3,
    ENCODED_TILED_DATA = 4,
    ADDITIONAL_INFO = 5,
    FILLER = 6,
};
NLOHMANN_JSON_SERIALIZE_ENUM(BlockType, {{BlockType::UNKNOWN, nullptr}, // default if no match
                                         {BlockType::SEQUENCE_CONFIG, "SEQUENCE_CONFIG"},
                                         {BlockType::GLOBAL_CONFIG, "GLOBAL_CONFIG"},
                                         {BlockType::PICTURE_CONFIG, "PICTURE_CONFIG"},
                                         {BlockType::ENCODED_DATA, "ENCODED_DATA"},
                                         {BlockType::ENCODED_TILED_DATA, "ENCODED_TILED_DATA"},
                                         {BlockType::ADDITIONAL_INFO, "ADDITIONAL_INFO"},
                                         {BlockType::FILLER, "FILLER"}})

enum class PayloadSizeType : uint8_t
{
    SIZE_0_BYTES = 0,
    SIZE_1_BYTES = 1,
    SIZE_2_BYTES = 2,
    SIZE_3_BYTES = 3,
    SIZE_4_BYTES = 4,
    SIZE_5_BYTES = 5,
    // 6 RESERVED
    CUSTOM = 7,
};
NLOHMANN_JSON_SERIALIZE_ENUM(PayloadSizeType, {{PayloadSizeType::SIZE_0_BYTES, "SIZE_0_BYTES"},
                                               {PayloadSizeType::SIZE_1_BYTES, "SIZE_1_BYTES"},
                                               {PayloadSizeType::SIZE_2_BYTES, "SIZE_2_BYTES"},
                                               {PayloadSizeType::SIZE_3_BYTES, "SIZE_3_BYTES"},
                                               {PayloadSizeType::SIZE_4_BYTES, "SIZE_4_BYTES"},
                                               {PayloadSizeType::SIZE_5_BYTES, "SIZE_5_BYTES"},
                                               {PayloadSizeType::CUSTOM, "CUSTOM"}})

enum class ChromaSamplingType : int8_t
{
    UNKNOWN = -1,
    MONOCHROME = 0,
    CHROMA_420 = 1,
    CHROMA_422 = 2,
    CHROMA_444 = 3,
};
NLOHMANN_JSON_SERIALIZE_ENUM(ChromaSamplingType, {{ChromaSamplingType::UNKNOWN, nullptr},
                                                  {ChromaSamplingType::MONOCHROME, "MONOCHROME"},
                                                  {ChromaSamplingType::CHROMA_420, "CHROMA_420"},
                                                  {ChromaSamplingType::CHROMA_422, "CHROMA_422"},
                                                  {ChromaSamplingType::CHROMA_444, "CHROMA_444"}})

enum class BitDepthType : int8_t
{
    UNKNOWN = -1,
    DEPTH_8 = 0,
    DEPTH_10 = 1,
    DEPTH_12 = 2,
    DEPTH_14 = 3,
};
NLOHMANN_JSON_SERIALIZE_ENUM(BitDepthType, {{BitDepthType::UNKNOWN, nullptr},
                                            {BitDepthType::DEPTH_8, "DEPTH_8"},
                                            {BitDepthType::DEPTH_10, "DEPTH_10"},
                                            {BitDepthType::DEPTH_12, "DEPTH_12"},
                                            {BitDepthType::DEPTH_14, "DEPTH_14"}})

enum class UpsampleType : int8_t
{
    UNKNOWN = -1,
    NEAREST = 0,
    LINEAR = 1,
    CUBIC = 2,
    MODIFIED_CUBIC = 3,
    ADAPTIVE_CUBIC = 4,
};
NLOHMANN_JSON_SERIALIZE_ENUM(UpsampleType, {{UpsampleType::UNKNOWN, nullptr},
                                            {UpsampleType::NEAREST, "NEAREST"},
                                            {UpsampleType::LINEAR, "LINEAR"},
                                            {UpsampleType::CUBIC, "CUBIC"},
                                            {UpsampleType::MODIFIED_CUBIC, "MODIFIED_CUBIC"},
                                            {UpsampleType::ADAPTIVE_CUBIC, "ADAPTIVE_CUBIC"}})

enum class ScalingMode : int8_t
{
    UNKNOWN = -1,
    SCALING_0D = 0,
    SCALING_1D = 1,
    SCALING_2D = 2,
};
NLOHMANN_JSON_SERIALIZE_ENUM(ScalingMode, {{ScalingMode::UNKNOWN, nullptr},
                                           {ScalingMode::SCALING_0D, "SCALING_0D"},
                                           {ScalingMode::SCALING_1D, "SCALING_1D"},
                                           {ScalingMode::SCALING_2D, "SCALING_2D"}})

enum class TilingMode : int8_t
{
    UNKNOWN = -1,
    NONE = 0,
    TILE_512X256 = 1,
    TILE_1024X512 = 2,
    CUSTOM = 3,
};
NLOHMANN_JSON_SERIALIZE_ENUM(TilingMode, {{TilingMode::UNKNOWN, nullptr},
                                          {TilingMode::NONE, "NONE"},
                                          {TilingMode::TILE_512X256, "TILE_512X256"},
                                          {TilingMode::TILE_1024X512, "TILE_1024X512"},
                                          {TilingMode::CUSTOM, "CUSTOM"}})

enum class UserDataMode : int8_t
{
    UNKNOWN = -1,
    DISABLED = 0,
    WITH_2_BITS = 1,
    WITH_6_BITS = 2,
};
NLOHMANN_JSON_SERIALIZE_ENUM(UserDataMode, {{UserDataMode::UNKNOWN, nullptr},
                                            {UserDataMode::DISABLED, "DISABLED"},
                                            {UserDataMode::WITH_2_BITS, "WITH_2_BITS"},
                                            {UserDataMode::WITH_6_BITS, "WITH_6_BITS"}})

enum class PlaneMode : int8_t
{
    UNKNOWN = -1,
    Y = 0,
    YUV = 1,
};
NLOHMANN_JSON_SERIALIZE_ENUM(PlaneMode,
                             {{PlaneMode::UNKNOWN, nullptr}, {PlaneMode::Y, "Y"}, {PlaneMode::YUV, "YUV"}})

enum class TransformType : int8_t
{
    UNKNOWN = -1,
    TRANSFORM_2X2 = 0,
    TRANSFORM_4X4 = 1,
};
NLOHMANN_JSON_SERIALIZE_ENUM(TransformType, {{TransformType::UNKNOWN, nullptr},
                                             {TransformType::TRANSFORM_2X2, "TRANSFORM_2X2"},
                                             {TransformType::TRANSFORM_4X4, "TRANSFORM_4X4"}})

enum class TiledSizeCompressionType : int8_t
{
    UNKNOWN = -1,
    NONE = 0,
    PREFIX_CODING = 1,
    PREFIX_CODING_ON_DIFFERENCES = 2,
};
NLOHMANN_JSON_SERIALIZE_ENUM(TiledSizeCompressionType,
                             {{TiledSizeCompressionType::UNKNOWN, nullptr},
                              {TiledSizeCompressionType::NONE, "NONE"},
                              {TiledSizeCompressionType::PREFIX_CODING, "PREFIX_CODING"},
                              {TiledSizeCompressionType::PREFIX_CODING_ON_DIFFERENCES,
                               "PREFIX_CODING_ON_DIFFERENCES"}})

enum class EntropyEnabledPerTileType : uint8_t
{
    DISABLED = 0,
    RUN_LENGTH_ENCODING = 1,
};
NLOHMANN_JSON_SERIALIZE_ENUM(EntropyEnabledPerTileType,
                             {{EntropyEnabledPerTileType::DISABLED, "DISABLED"},
                              {EntropyEnabledPerTileType::RUN_LENGTH_ENCODING,
                               "RUN_LENGTH_ENCODING"}})

enum class PictureType : uint8_t
{
    FRAME = 0,
    FIELD = 1,
};
NLOHMANN_JSON_SERIALIZE_ENUM(PictureType, {{PictureType::FRAME, "FRAME"}, {PictureType::FIELD, "FIELD"}})

enum class FieldType : uint8_t
{
    TOP = 0,
    BOTTOM = 1,
};
NLOHMANN_JSON_SERIALIZE_ENUM(FieldType, {{FieldType::TOP, "TOP"}, {FieldType::BOTTOM, "BOTTOM"}})

enum class SublevelType : uint8_t
{
    SUBLEVEL_0 = 0,
    SUBLEVEL_1 = 1,
};
NLOHMANN_JSON_SERIALIZE_ENUM(SublevelType, {{SublevelType::SUBLEVEL_0, "SUBLEVEL_0"},
                                            {SublevelType::SUBLEVEL_1, "SUBLEVEL_1"}})

enum class DitherType : int8_t
{
    UNKNOWN = -1,
    NONE = 0,
    UNIFORM = 1,
    // 2 RESERVED
    // 3 RESERVED
};
NLOHMANN_JSON_SERIALIZE_ENUM(DitherType, {{DitherType::UNKNOWN, nullptr},
                                          {DitherType::NONE, "NONE"},
                                          {DitherType::UNIFORM, "UNIFORM"}})

enum class QuantMatrixMode : int8_t
{
    UNKNOWN = -1,
    USE_PREVIOUS = 0,
    USE_DEFAULT = 1,
    CUSTOM_BOTH = 2,
    CUSTOM_SUBLAYER_2 = 3,
    CUSTOM_SUBLAYER_1 = 4,
    CUSTOM_BOTH_UNIQUE = 5,
};
NLOHMANN_JSON_SERIALIZE_ENUM(QuantMatrixMode,
                             {{QuantMatrixMode::UNKNOWN, nullptr},
                              {QuantMatrixMode::USE_PREVIOUS, "USE_PREVIOUS"},
                              {QuantMatrixMode::USE_DEFAULT, "USE_DEFAULT"},
                              {QuantMatrixMode::CUSTOM_BOTH, "CUSTOM_BOTH"},
                              {QuantMatrixMode::CUSTOM_SUBLAYER_2, "CUSTOM_SUBLAYER_2"},
                              {QuantMatrixMode::CUSTOM_SUBLAYER_1, "CUSTOM_SUBLAYER_1"},
                              {QuantMatrixMode::CUSTOM_BOTH_UNIQUE, "CUSTOM_BOTH_UNIQUE"}})

enum class AdditionalInfoType : int8_t
{
    UNKNOWN = -1,
    SEI_PAYLOAD = 0,
    VUI_PARAMETERS = 1,
    S_FILTER = 23,
    BASE_HASH = 24,
    HDR = 25,
};
NLOHMANN_JSON_SERIALIZE_ENUM(AdditionalInfoType,
                             {{AdditionalInfoType::UNKNOWN, nullptr},
                              {AdditionalInfoType::SEI_PAYLOAD, "SEI_PAYLOAD"},
                              {AdditionalInfoType::VUI_PARAMETERS, "VUI_PARAMETERS"},
                              {AdditionalInfoType::S_FILTER, "S_FILTER"},
                              {AdditionalInfoType::BASE_HASH, "BASE_HASH"},
                              {AdditionalInfoType::HDR, "HDR"}})

enum class SEIPayloadType : int8_t
{
    UNKNOWN = -1,
    MASTERING_DISPLAY_COLOUR_VOLUME = 1,
    CONTENT_LIGHT_LEVEL_INFO = 2,
    USER_DATA_REGISTERED = 4,
};
NLOHMANN_JSON_SERIALIZE_ENUM(
    SEIPayloadType,
    {{SEIPayloadType::UNKNOWN, nullptr},
     {SEIPayloadType::MASTERING_DISPLAY_COLOUR_VOLUME, "MASTERING_DISPLAY_COLOUR_VOLUME"},
     {SEIPayloadType::CONTENT_LIGHT_LEVEL_INFO, "CONTENT_LIGHT_LEVEL_INFO"},
     {SEIPayloadType::USER_DATA_REGISTERED, "USER_DATA_REGISTERED"}})

enum class SFilterMode : int8_t
{
    UNKNOWN = -1,
    DISABLED = 0,
    IN_LOOP = 1,
    OUT_OF_LOOP = 2,
};
NLOHMANN_JSON_SERIALIZE_ENUM(SFilterMode, {{SFilterMode::UNKNOWN, nullptr},
                                           {SFilterMode::DISABLED, "DISABLED"},
                                           {SFilterMode::IN_LOOP, "IN_LOOP"},
                                           {SFilterMode::OUT_OF_LOOP, "OUT_OF_LOOP"}})

enum class EncodedDataSubLayer : int8_t
{
    UNKNOWN = -1,
    SUBLAYER_1 = 1,
    SUBLAYER_2 = 2,
    TEMPORAL = 3,
};
NLOHMANN_JSON_SERIALIZE_ENUM(EncodedDataSubLayer, {{EncodedDataSubLayer::UNKNOWN, nullptr},
                                                   {EncodedDataSubLayer::SUBLAYER_1, "SUBLAYER_1"},
                                                   {EncodedDataSubLayer::SUBLAYER_2, "SUBLAYER_2"},
                                                   {EncodedDataSubLayer::TEMPORAL, "TEMPORAL"}})

constexpr std::array<EncodedDataSubLayer, 2> EncodedDataSubLayerList = {
    EncodedDataSubLayer::SUBLAYER_1, EncodedDataSubLayer::SUBLAYER_2};

enum class EncodedDataMode : int8_t
{
    UNKNOWN = -1,
    DISABLED = 0,
    RLE = 1,
    PREFIX = 2,
};
NLOHMANN_JSON_SERIALIZE_ENUM(EncodedDataMode, {{EncodedDataMode::UNKNOWN, nullptr},
                                               {EncodedDataMode::DISABLED, "DISABLED"},
                                               {EncodedDataMode::RLE, "RLE"},
                                               {EncodedDataMode::PREFIX, "PREFIX"}})

enum class ProfileType : int8_t
{
    UNKNOWN = -1,
    MAIN = 0,
    MAIN_444 = 1,
    // 2-14 RESERVED
    EXTENDED = 15,
};
NLOHMANN_JSON_SERIALIZE_ENUM(ProfileType, {{ProfileType::UNKNOWN, nullptr},
                                           {ProfileType::MAIN, "MAIN"},
                                           {ProfileType::MAIN_444, "MAIN_444"},
                                           {ProfileType::EXTENDED, "EXTENDED"}})

enum class LevelType : int8_t
{
    UNKNOWN = -1,
    LEVEL_1 = 1,
    LEVEL_2 = 2,
    LEVEL_3 = 3,
    LEVEL_4 = 4,
    // 5-14 RESERVED
    EXTENDED = 15,
};
NLOHMANN_JSON_SERIALIZE_ENUM(LevelType, {{LevelType::UNKNOWN, nullptr},
                                         {LevelType::LEVEL_1, "LEVEL_1"},
                                         {LevelType::LEVEL_2, "LEVEL_2"},
                                         {LevelType::LEVEL_3, "LEVEL_3"},
                                         {LevelType::LEVEL_4, "LEVEL_4"},
                                         {LevelType::EXTENDED, "EXTENDED"}})

struct BaseConfig
{
    BlockType type = BlockType::UNKNOWN;
    PayloadSizeType size_type = PayloadSizeType::SIZE_0_BYTES;
    uint32_t size = 0;
};

struct SequenceConfigExt
{
    uint8_t extended_profile_idc = 0;
    uint8_t extended_level_idc = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SequenceConfigExt, extended_profile_idc, extended_level_idc);

struct SequenceConfigWindow
{
    uint32_t conf_win_left_offset = 0;
    uint32_t conf_win_right_offset = 0;
    uint32_t conf_win_top_offset = 0;
    uint32_t conf_win_bottom_offset = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SequenceConfigWindow, conf_win_left_offset, conf_win_right_offset,
                                   conf_win_top_offset, conf_win_bottom_offset);

struct SequenceConfig : public BaseConfig
{
    explicit SequenceConfig(const BaseConfig& base)
        : BaseConfig(base)
    {}

    ProfileType profile_idc = ProfileType::UNKNOWN;
    LevelType level_idc = LevelType::UNKNOWN;
    SublevelType sublevel_idc = SublevelType::SUBLEVEL_0;
    bool conformance_window_flag = false;

    std::optional<SequenceConfigExt> extended = std::nullopt;
    std::optional<SequenceConfigWindow> conf_win = std::nullopt;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SequenceConfig, type, size_type, size, profile_idc, level_idc,
                                   sublevel_idc, conformance_window_flag, extended, conf_win);

struct GlobalConfigCompressionType
{
    EntropyEnabledPerTileType compression_type_entropy_enabled_per_tile_flag =
        EntropyEnabledPerTileType::DISABLED;
    TiledSizeCompressionType compression_type_size_per_tile = TiledSizeCompressionType::UNKNOWN;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GlobalConfigCompressionType,
                                   compression_type_entropy_enabled_per_tile_flag,
                                   compression_type_size_per_tile);

struct GlobalConfig : public BaseConfig
{
    explicit GlobalConfig(const BaseConfig& base)
        : BaseConfig(base)
    {}

    bool processed_planes_type_flag = false;
    uint8_t resolution_type = 0;
    TransformType transform_type = TransformType::UNKNOWN;
    ChromaSamplingType chroma_sampling_type = ChromaSamplingType::UNKNOWN;
    BitDepthType base_depth_type = BitDepthType::UNKNOWN;
    BitDepthType enhancement_depth_type = BitDepthType::UNKNOWN;
    bool temporal_step_width_modifier_signalled_flag = false;
    bool predicted_residual_mode_flag = false;
    bool temporal_tile_intra_signalling_enabled_flag = false;
    bool temporal_enabled_flag = false;
    UpsampleType upsample_type = UpsampleType::UNKNOWN;
    bool level1_filtering_signalled_flag = false;
    ScalingMode scaling_mode_level1 = ScalingMode::UNKNOWN;
    ScalingMode scaling_mode_level2 = ScalingMode::UNKNOWN;
    TilingMode tile_dimensions_type = TilingMode::UNKNOWN;
    UserDataMode user_data_enabled = UserDataMode::UNKNOWN;
    bool level1_depth_flag = false;
    bool chroma_step_width_flag = false;
    PlaneMode planes_type = PlaneMode::Y;
    uint8_t temporal_step_width_modifier = 48;
    std::optional<std::array<uint16_t, 4>> adaptive_cubic_kernel_coeffs = std::nullopt;
    std::optional<std::array<uint8_t, 2>> level1_filtering = std::nullopt;
    std::optional<std::array<uint16_t, 2>> custom_tile = std::nullopt;
    std::optional<GlobalConfigCompressionType> compression_type;
    std::optional<std::array<uint16_t, 2>> custom_resolution = std::nullopt;
    uint8_t chroma_step_width_multiplier = 64;

    // x_ members are outside the structure spec but are useful anyway. They should still be serialized in the JSON output.
    std::array<uint16_t, 2> x_resolution = {0, 0};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    GlobalConfig, type, size_type, size, processed_planes_type_flag, resolution_type,
    transform_type, chroma_sampling_type, base_depth_type, enhancement_depth_type,
    temporal_step_width_modifier_signalled_flag, temporal_step_width_modifier,
    predicted_residual_mode_flag, temporal_tile_intra_signalling_enabled_flag,
    temporal_enabled_flag, upsample_type, level1_filtering_signalled_flag, scaling_mode_level1,
    scaling_mode_level2, tile_dimensions_type, user_data_enabled, level1_depth_flag,
    chroma_step_width_flag, planes_type, adaptive_cubic_kernel_coeffs, level1_filtering,
    custom_tile, compression_type, custom_resolution, chroma_step_width_multiplier, x_resolution);

struct PictureConfigSublayer1
{
    uint16_t step_width_sublayer1 = 0;
    bool level1_filtering_enabled_flag = false;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PictureConfigSublayer1, step_width_sublayer1,
                                   level1_filtering_enabled_flag);

struct PictureConfigDequantOffset
{
    bool dequant_offset_mode_flag = false;
    uint8_t dequant_offset = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PictureConfigDequantOffset, dequant_offset_mode_flag, dequant_offset);

struct PictureConfigDithering
{
    DitherType dithering_type = DitherType::UNKNOWN;
    std::optional<uint8_t> dithering_strength = std::nullopt;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PictureConfigDithering, dithering_type, dithering_strength);

struct PictureConfig : public BaseConfig
{
    explicit PictureConfig(const BaseConfig& base)
        : BaseConfig(base)
    {}

    bool no_enhancement_bit_flag = false;

    QuantMatrixMode quant_matrix_mode = QuantMatrixMode::UNKNOWN;
    bool dequant_offset_signalled_flag = false;
    PictureType picture_type_bit_flag = PictureType::FRAME;
    bool temporal_refresh_bit_flag = false;
    bool step_width_sublayer1_enabled_flag = false;
    std::optional<uint16_t> step_width_sublayer2 = std::nullopt;
    bool dithering_control_flag = false;

    bool temporal_signalling_present_flag = false;

    std::optional<FieldType> field_type_bit_flag = std::nullopt;
    std::optional<PictureConfigSublayer1> step_width_sublayer1 = std::nullopt;
    std::optional<std::vector<uint8_t>> qm_coefficient_0 = std::nullopt;
    std::optional<std::vector<uint8_t>> qm_coefficient_1 = std::nullopt;
    std::optional<PictureConfigDequantOffset> dequant_offset = std::nullopt;
    std::optional<PictureConfigDithering> dithering = std::nullopt;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PictureConfig, type, size_type, size, no_enhancement_bit_flag,
                                   quant_matrix_mode, dequant_offset_signalled_flag,
                                   picture_type_bit_flag, temporal_refresh_bit_flag,
                                   step_width_sublayer1_enabled_flag, step_width_sublayer2,
                                   dithering_control_flag, temporal_signalling_present_flag,
                                   field_type_bit_flag, step_width_sublayer1, qm_coefficient_0,
                                   qm_coefficient_1, dequant_offset, dithering);

struct EncodedDataLayer
{
    int64_t plane = 0;
    std::optional<EncodedDataSubLayer> sublayer = std::nullopt;
    std::optional<int64_t> coefficient_group = std::nullopt;

    bool entropy_enabled_flag = false;
    bool rle_only_flag = false;

    int64_t size = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EncodedDataLayer, plane, sublayer, coefficient_group, size,
                                   entropy_enabled_flag, rle_only_flag);

struct EncodedData : public BaseConfig
{
    explicit EncodedData(const BaseConfig& base)
        : BaseConfig(base)
    {}
    std::vector<EncodedDataLayer> layers;

    int64_t x_plane_count = 0;
    int64_t x_coefficient_group_count = 0;
    int64_t x_surface_header_bytes = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EncodedData, type, size_type, size, layers, x_plane_count,
                                   x_coefficient_group_count, x_surface_header_bytes);

struct EncodedTileDataLayer : public EncodedDataLayer
{
    std::optional<int64_t> tile = std::nullopt;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EncodedTileDataLayer, plane, sublayer, coefficient_group, tile,
                                   size, entropy_enabled_flag, rle_only_flag);

struct EncodedTileData : public BaseConfig
{
    explicit EncodedTileData(const BaseConfig& base)
        : BaseConfig(base)
    {}

    std::vector<EncodedTileDataLayer> layers;

    int64_t x_plane_count = 0;
    int64_t x_coefficient_group_count = 0;
    int64_t x_surface_header_bytes = 0;
    std::array<int64_t, 2> x_tile_dimensions = {0, 0};
    int64_t x_level1_tile_count = 0;
    int64_t x_level2_tile_count = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EncodedTileData, type, size_type, size, layers, x_plane_count,
                                   x_coefficient_group_count, x_surface_header_bytes,
                                   x_tile_dimensions, x_level1_tile_count, x_level2_tile_count);

struct AdditionalInfoSFilter
{
    SFilterMode s_filter_mode = SFilterMode::UNKNOWN;
    uint8_t s_filter_strength = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AdditionalInfoSFilter, s_filter_mode, s_filter_strength);

struct AdditionalInfoSEIMasteringDisplayColourVolume
{
    std::array<uint16_t, 3> display_primaries_x = {0, 0, 0};
    std::array<uint16_t, 3> display_primaries_y = {0, 0, 0};
    uint16_t white_point_x = 0;
    uint16_t white_point_y = 0;
    uint32_t max_display_mastering_luminance = 0;
    uint32_t min_display_mastering_luminance = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AdditionalInfoSEIMasteringDisplayColourVolume, display_primaries_x,
                                   display_primaries_y, white_point_x, white_point_y,
                                   max_display_mastering_luminance, min_display_mastering_luminance);

struct AdditionalInfoSEIContentLightLevelInfo
{
    uint16_t max_content_light_level = 0;
    uint16_t max_picture_average_light_level = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AdditionalInfoSEIContentLightLevelInfo, max_content_light_level,
                                   max_picture_average_light_level);

struct AdditionalInfoSEIVNovaPayload
{
    uint8_t bitstream_version = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AdditionalInfoSEIVNovaPayload, bitstream_version);

struct AdditionalInfoSEIUserDataRegistered
{
    uint8_t t35_country_code = 0;
    std::optional<std::array<uint8_t, 3>> t35_remaining_code = std::nullopt;
    std::optional<AdditionalInfoSEIVNovaPayload> v_nova_payload = std::nullopt;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AdditionalInfoSEIUserDataRegistered, t35_country_code,
                                   t35_remaining_code, v_nova_payload);

struct AdditionalInfoSEI
{
    SEIPayloadType sei_payload_type = SEIPayloadType::UNKNOWN;
    std::optional<AdditionalInfoSEIMasteringDisplayColourVolume> mastering_display_colour_volume =
        std::nullopt;
    std::optional<AdditionalInfoSEIContentLightLevelInfo> content_light_level_info = std::nullopt;
    std::optional<AdditionalInfoSEIUserDataRegistered> user_data_registered = std::nullopt;
    std::optional<std::string> sei_error = std::nullopt;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AdditionalInfoSEI, sei_payload_type, mastering_display_colour_volume,
                                   content_light_level_info, user_data_registered, sei_error);

struct AdditionalInfoVUI
{
    bool aspect_ratio_info_present = false;
    std::optional<uint8_t> aspect_ratio_idc = std::nullopt;
    std::optional<uint16_t> sar_width = std::nullopt;
    std::optional<uint16_t> sar_height = std::nullopt;
    bool overscan_info_present = false;
    std::optional<bool> overscan_appropriate_flag = std::nullopt;
    bool video_signal_type_present = false;
    std::optional<uint8_t> video_format = std::nullopt;
    std::optional<bool> video_full_range = std::nullopt;
    std::optional<bool> colour_description_present = std::nullopt;
    std::optional<uint8_t> colour_primaries = std::nullopt;
    std::optional<uint8_t> transfer_characteristics = std::nullopt;
    std::optional<uint8_t> matrix_coefficients = std::nullopt;
    bool chroma_loc_info_present = false;
    std::optional<uint32_t> chroma_sample_loc_type_top_field = std::nullopt;
    std::optional<uint32_t> chroma_sample_loc_type_bottom_field = std::nullopt;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AdditionalInfoVUI, aspect_ratio_info_present, aspect_ratio_idc,
                                   sar_width, sar_height, overscan_info_present,
                                   overscan_appropriate_flag, video_signal_type_present,
                                   video_format, video_full_range, colour_description_present,
                                   colour_primaries, transfer_characteristics, matrix_coefficients,
                                   chroma_loc_info_present, chroma_sample_loc_type_top_field,
                                   chroma_sample_loc_type_bottom_field);

struct AdditionalInfoHDR
{
    bool tonemapper_location = false;
    uint8_t tone_mapper_type = 0;
    bool tonemapper_data_present = false;
    bool deinterlacer_enabled = false;
    std::optional<uint8_t> tone_mapper_type_extended = std::nullopt;
    std::optional<uint8_t> deinterlacer_type = std::nullopt;
    std::optional<bool> top_field_first = std::nullopt;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AdditionalInfoHDR, tonemapper_location, tone_mapper_type,
                                   tonemapper_data_present, deinterlacer_enabled,
                                   tone_mapper_type_extended, deinterlacer_type, top_field_first);

struct AdditionalInfo : public BaseConfig
{
    explicit AdditionalInfo(const BaseConfig& base)
        : BaseConfig(base)
    {}

    AdditionalInfoType info_type = AdditionalInfoType::UNKNOWN;
    std::optional<AdditionalInfoSFilter> s_filter = std::nullopt;
    std::optional<uint64_t> base_hash = std::nullopt;
    std::optional<AdditionalInfoSEI> sei = std::nullopt;
    std::optional<AdditionalInfoVUI> vui = std::nullopt;
    std::optional<AdditionalInfoHDR> hdr = std::nullopt;
    std::optional<uint32_t> unhandled_info_type = std::nullopt;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AdditionalInfo, type, size_type, size, info_type, s_filter,
                                   base_hash, sei, vui, hdr, unhandled_info_type);

struct FrameBase
{
    uint64_t index = 0;
    std::string frame_type;
    int64_t dts = 0;
    int64_t pts = 0;
    int32_t height = 0;
    int32_t width = 0;
    int64_t base_size = 0;
    int64_t combined_size = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FrameBase, index, frame_type, dts, pts, height, width, base_size,
                                   combined_size);

struct FrameLCEVC
{
    std::string frame_type;
    int64_t payload_size = 0;
    int64_t raw_size = 0;
    int64_t wire_size = 0;
    std::optional<nlohmann::ordered_json> blocks = std::nullopt;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FrameLCEVC, frame_type, payload_size, raw_size, wire_size, blocks);

struct Frame
{
    FrameBase base;
    FrameLCEVC lcevc;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Frame, base, lcevc);

struct SummaryLCEVC
{
    int64_t frame_count = 0;
    int64_t layer_size = 0;
    int64_t level_1_size = 0.0;
    int64_t level_2_size = 0.0;
    int64_t temporal_size = 0.0;
    std::optional<double> layer_bitrate = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SummaryLCEVC, frame_count, layer_size, level_1_size,
                                   level_2_size, temporal_size, layer_bitrate);

struct SummaryBase
{
    int64_t frame_count = 0;
    int64_t layer_size = 0;
    double lcevc_base_ratio = 0;
    std::optional<double> layer_bitrate = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SummaryBase, frame_count, layer_size, lcevc_base_ratio, layer_bitrate);

struct SummaryPTS
{
    bool consistent = false;
    size_t interval_count = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SummaryPTS, consistent, interval_count);

struct Summary
{
    SummaryPTS pts;
    SummaryLCEVC lcevc;
    std::optional<SummaryBase> base;
    std::optional<double> framerate = 0;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Summary, framerate, lcevc, base, pts);

struct Root
{
    std::array<uint8_t, 3> version = {0, 0, 0};
    Summary summary;
    std::vector<Frame> frames;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Root, version, summary, frames);

} // namespace vnova::analyzer

// NOLINTEND(readability-identifier-naming,readability-identifier-length)

#endif
