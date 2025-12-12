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
#ifndef VN_EXTRACTOR_EXTRACTOR_DEMUXER_H_
#define VN_EXTRACTOR_EXTRACTOR_DEMUXER_H_

#include "config.h"
#include "extractor.h"
#include "helper/frame_queue.h"

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

#include <memory>
#include <string>

namespace vnova::analyzer {
enum NalUnitType : uint32_t
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

class ExtractorDemuxer : public Extractor
{
public:
    ExtractorDemuxer(const std::string& url, InputType type, const Config& config);
    ~ExtractorDemuxer();

    bool next(std::vector<LCEVC>& lcevcFrames, FrameQueue& frameBuffer) override;
    bool flush(FrameQueue& frameBuffer, std::vector<LCEVC>& lcevcFrames) override;

private:
    virtual bool processNALUnit(const utility::DataBuffer& nalUnit, LCEVC& lcevc,
                                utility::BaseType::Enum& baseType) = 0;

    bool extractLCEVCMp4(std::vector<LCEVC>& lcevcFrames);
    bool extractFromSeparateTrack(std::vector<LCEVC>& lcevcFrames, const int streamPID);
    bool extractLCEVCWebm(std::vector<LCEVC>& lcevcFrames);
    bool extractLCEVCData(std::vector<LCEVC>& lcevcFrames, const int streamPID);

    bool extractFrameType(const AVStream& stream, FrameQueue& frameBuffer, std::vector<LCEVC>& lcevcFrames);
    bool populateFrameInfo(AVCodecContext& codecCtx, FrameQueue& frameBuffer,
                           std::vector<LCEVC>& lcevcFrames);
    bool processNALFrameData(int64_t decodingIndex, FrameInfo& info, std::vector<LCEVC>& lcevcFrames);
    bool handleNALUnit(const std::vector<uint8_t>& nalUnit, std::vector<LCEVC>& lcevcFrames);
    void addLCEVCData(LCEVC& lcevc, std::vector<LCEVC>& lcevcFrames);
    void determineStreamTypes();
    bool validateLCEVCAnnexB();
    static bool parseLvcCAtom(const uint8_t* data, size_t size, LCEVCDecoderConfiguration& config);

    InputType m_inputType;

    const Config& m_config;
    AVPacket* m_packet = nullptr;
    AVFormatContext* m_formatContext = nullptr;

    utility::BaseType::Enum m_baseType = utility::BaseType::Enum::Unknown;
    utility::DataBuffer m_nalUnit;

    AVFrame* m_frame = nullptr;
    std::map<int, AVCodecContext*> m_codecContextMap;
    std::map<int64_t, LCEVC> m_lcevcDataMap;

    bool m_isFlush = false;
    int64_t m_maxReorder = 0;
    int64_t m_dts = 0;
    int64_t m_pts = 0;
    int m_decodingIndex = 0;
    int m_decodingCounter = 0;
    int64_t m_packetCount = 0;

    int m_lcevcStreamIndex = -1;
    bool m_isLcevcFound = false;
    int64_t m_pos = -1;
    LCEVC m_lcevcAccumulator;
};

} // namespace vnova::analyzer

#endif // VN_EXTRACTOR_EXTRACTOR_DEMUXER_H_
