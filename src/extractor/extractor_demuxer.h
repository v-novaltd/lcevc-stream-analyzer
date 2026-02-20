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
#ifndef VN_EXTRACTOR_EXTRACTOR_DEMUXER_H_
#define VN_EXTRACTOR_EXTRACTOR_DEMUXER_H_

#include "app/config.h"
#include "extractor.h"
#include "helper/extracted_frame.h"

#if defined(_MSC_VER)
#pragma warning(push)
// ffmpeg headers perform narrow conversions internally; suppress noisy 4244 warnings on MSVC.
#pragma warning(disable : 4244)
#endif

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <map>
#include <set>

namespace vnova::analyzer {

constexpr int kLCEVCCodec = 54;
constexpr int kBlockAdditionalLcevcId = 5;

enum class NalUnitType : uint32_t
{
    H264_NAL_UNIT_SEI = 6,
    HEVC_NAL_UNIT_PREFIX_SEI = 39,
    HEVC_NAL_UNIT_SUFFIX_SEI = 40,
    VVC_NAL_UNIT_PREFIX_SEI = 23,
    VVC_NAL_UNIT_SUFFIX_SEI = 24,
};

struct NALUnit
{
    uint16_t nalUnitLength;
    std::vector<uint8_t> nalUnit;
};

struct NALArray
{
    uint8_t nalUnitType;
    std::vector<NALUnit> nalUnits;
};

struct LCEVCDecoderConfiguration
{
    uint8_t configurationVersion;
    uint8_t lcevcProfileIndication;
    uint8_t lcevcLevelIndication;
    uint8_t chromaFormatIdc;
    uint8_t bitDepthLuma;
    uint8_t bitDepthChroma;
    uint8_t lengthSize;
    uint32_t picWidthInLumaSamples;
    uint32_t picHeightInLumaSamples;
    bool scInStream;
    bool gcInStream;
    bool aiInStream;
    uint8_t numOfArrays;
    std::vector<NALArray> arrays;
};

struct DemuxedFrame
{
    helper::LCEVCFrame lcevc;

    int64_t decodeIndex = 0;

    // Only finished frames are released to the lcevcFrames vector which links extraction to parsing.
    // A frame is considered finished if it has a valid PTS (even synthesized for ES handling).
    // This is because PTS is used to link an LCEVC frame to a base frame for e.g. size accounting.
    bool finished = false;
};
using DemuxedFrameMap = std::unordered_map<int64_t, DemuxedFrame>;

class ExtractorDemuxer : public Extractor
{
public:
    ExtractorDemuxer(const std::filesystem::path& url, helper::InputType type, const Config& config);
    ~ExtractorDemuxer() override;

    bool next(std::vector<helper::LCEVCFrame>& lcevcFrames, helper::BaseFrameQueue& frameQueue) override;
    bool flush(helper::BaseFrameQueue& frameQueue, std::vector<helper::LCEVCFrame>& lcevcFrames) override;

private:
    virtual std::optional<helper::LCEVCFrame> parseNalIsLcevc(const utility::DataBuffer& nalUnit,
                                                              helper::BaseType& baseType) = 0;
    bool extractFromSingleTrack(const char* streamCodecName, const int streamPID);
    bool extractFromSeparateTrack(const int streamPID);
    bool extractLcevcMp4();
    bool extractLcevcWebm();
    bool extractLcevcStream(const int streamPID);

    bool processBaseFrame(const AVStream& stream, helper::BaseFrameQueue& frameQueue);
    bool populateFrameInfo(AVCodecContext& codecCtx, helper::BaseFrameQueue& frameQueue);
    void provideFrameInfoToLcevc(int64_t decodingIndex, const helper::BaseFrame& info);
    bool handlePacketNalUnit(const utility::DataBuffer& nalBuffer);

    [[nodiscard]] bool storeLcevcFrame(helper::LCEVCFrame& lcevc);
    bool releaseLcevcFrame(std::vector<helper::LCEVCFrame>& lcevcFrames);

    void determineStreamTypes();
    bool validateMp4IsNotAnnexB();

    helper::InputType m_inputType;

    const Config& m_config;
    AVPacket* m_packet = nullptr;
    AVFormatContext* m_formatContext = nullptr;

    helper::BaseType m_baseType = helper::BaseType::UNKNOWN;

    std::map<int, AVCodecContext*> m_codecContextMap;
    DemuxedFrameMap m_demuxedFrameMap;
    std::map<int64_t, helper::BaseFrame> m_preParsedFrameInfo;

    std::set<int64_t> m_releasedPtsTracker;

    bool m_isFlush = false;

    uint8_t m_maxReorder = 16; // H.264, HEVC, VVC max is 16

    int64_t m_lcevcDecodeIndex = 0;
    std::map<int, int64_t> m_baseDecodeIndex;

    int64_t m_mapSearchIndex = 0;

    std::optional<int> m_lcevcStreamIndex = std::nullopt;

    // Any LCEVC data found in the stream for any frame - true once a single LCEVC packet seen.
    bool m_isLcevcFound = false;

    helper::LCEVCFrame m_lcevcAccumulator;
};

} // namespace vnova::analyzer

#endif // VN_EXTRACTOR_EXTRACTOR_DEMUXER_H_
