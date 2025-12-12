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
#include "extractor_demuxer.h"

#include "libavformat/avformat.h"
#include "utility/base_type.h"
#include "utility/byte_order.h"
#include "utility/log_util.h"
#include "utility/nal_header.h"
#include "utility/nal_unit.h"

#include <array>
#include <cstddef>

using namespace vnova::utility;

namespace vnova::analyzer {
static constexpr int kLCEVCCodec = 54;
static constexpr int kBlockAdditionalLcevcId = 5;
static constexpr int64_t kInvalidPts = (std::numeric_limits<int64_t>::max)();

template <typename T>
bool readBE(const uint8_t*& data, size_t& size, T& value)
{
    if (size < sizeof(T)) {
        return false;
    }

    T rawValue;
    std::memcpy(&rawValue, data, sizeof(T));
    value = ByteOrder<T>::toHost(rawValue);

    data += sizeof(T);
    size -= sizeof(T);
    return true;
}

void fixMisclassifiedStreams(AVFormatContext& formatContext)
{
    for (unsigned int i = 0; i < formatContext.nb_streams; ++i) {
        AVStream* stream = formatContext.streams[i];
        AVCodecParameters* codecPar = stream->codecpar;

        if (codecPar->codec_tag == kLCEVCCodec) {
            codecPar->codec_id = AV_CODEC_ID_NONE;
            codecPar->codec_type = AVMEDIA_TYPE_VIDEO;
        }
    }
}

bool ExtractorDemuxer::parseLvcCAtom(const uint8_t* data, size_t size, LCEVCDecoderConfiguration& config)
{
    size_t offset = 0;
    size_t length = size;
    if (length < 15) {
        return false;
    }

    const uint8_t* ptr = nullptr;

    config.configurationVersion = data[offset++];
    config.lcevcProfileIndication = data[offset++];
    config.lcevcLevelIndication = data[offset++];

    uint8_t byte = data[offset++];
    config.chromaFormatIdc = (byte >> 6) & 0x03;
    if (config.chromaFormatIdc > 3) {
        return false;
    }

    config.bitDepthLuma = ((byte >> 3) & 0x07) + 8;
    config.bitDepthChroma = (byte & 0x07) + 8;

    byte = data[offset++];
    config.lengthSize = ((byte >> 6) & 0x03) + 1;

    ptr = data + offset;
    readBE(ptr, size, config.picWidthInLumaSamples);
    offset += 4;

    ptr = data + offset;
    readBE(ptr, size, config.picHeightInLumaSamples);
    offset += 4;

    byte = data[offset++];
    config.scInStream = (byte >> 7) & 0x01;
    config.gcInStream = (byte >> 6) & 0x01;
    config.aiInStream = (byte >> 5) & 0x01;

    config.numOfArrays = data[offset++];

    for (uint8_t j = 0; j < config.numOfArrays; ++j) {
        if (offset + 2 > length) {
            return false;
        }

        byte = data[offset++];
        uint8_t nalUnitType = byte & 0x3F;

        uint16_t numOfNalus = 0;
        ptr = data + offset;
        readBE(ptr, size, numOfNalus);
        offset += 2;

        NALArray array;
        array.nalUnitType = nalUnitType;

        for (int i = 0; i < numOfNalus; ++i) {
            if (offset + 2 > length) {
                return false;
            }

            uint16_t nalUnitLength = 0;
            ptr = data + offset;
            readBE(ptr, size, nalUnitLength);
            offset += 2;
            if (offset + nalUnitLength > length) {
                return false;
            }

            NALUnit unit;
            unit.nalUnitLength = nalUnitLength;
            unit.nalUnit.insert(unit.nalUnit.end(), data + offset, data + offset + nalUnitLength);
            offset += nalUnitLength;

            array.nalUnits.push_back(unit);
        }

        config.arrays.push_back(array);
    }

    return true;
}

ExtractorDemuxer::ExtractorDemuxer(const std::string& url, InputType type, const Config& config)
    : m_inputType(type)
    , m_config(config)
    , m_packet(av_packet_alloc())
{
    if (!m_packet) {
        VNLog::Error("Error: Failed to allocate AVPacket\n");
        return;
    }

    const AVInputFormat* fmt = nullptr;
    switch (type) {
        case InputType::MP4: {
            fmt = av_find_input_format("mp4");
            if (fmt == nullptr) {
                VNLog::Info("Failed to turn input_type into a valid input format for mp4.");
            }
        } break;
        case InputType::ELEMENTARY: {
            switch (config.baseType) {
                case vnova::utility::BaseType::Enum::H264: {
                    fmt = av_find_input_format("h264");
                } break;
                case vnova::utility::BaseType::Enum::HEVC: {
                    fmt = av_find_input_format("hevc");
                } break;
                case vnova::utility::BaseType::Enum::VVC: {
                    fmt = av_find_input_format("vvc");
                } break;
                default: {
                    if (fmt == nullptr) {
                        VNLog::Info("Failed to turn base_type into a valid input format for es.");
                    }
                    break;
                }
            }
        } break;
        case InputType::TS: {
            fmt = av_find_input_format("mpegts");
            if (fmt == nullptr) {
                VNLog::Info("Failed to turn input_type into a valid input format for ts.");
            }
        } break;
        default: break;
    }

    if (avformat_open_input(&m_formatContext, url.c_str(), fmt, nullptr) != 0) {
        VNLog::Error("Could not open input file \n");
        return;
    }
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        VNLog::Error("Could not find stream info \n");
        return;
    }
    m_frame = av_frame_alloc();
    if (!m_frame) {
        VNLog::Error("Error: Failed to allocate AVFrame \n");
        return;
    }

    fixMisclassifiedStreams(*m_formatContext);
    determineStreamTypes();
    m_nalUnit.reserve(32768);
    durationSec = (double)m_formatContext->duration / AV_TIME_BASE;
    bInitialized = true;
}

ExtractorDemuxer::~ExtractorDemuxer()
{
    if (m_packet) {
        av_packet_free(&m_packet);
    }
    if (m_formatContext) {
        avformat_close_input(&m_formatContext);
    }
    for (auto& [streamIndex, codecCtx] : m_codecContextMap) {
        (void)streamIndex;
        avcodec_free_context(&codecCtx);
    }
    av_frame_free(&m_frame);
    m_nalUnit.clear();
}

static bool baseVideoCodec(AVCodecID codecID)
{
    switch (codecID) {
        case AV_CODEC_ID_H264:
        case AV_CODEC_ID_HEVC:
        case AV_CODEC_ID_VP9:
        case AV_CODEC_ID_VP8:
        case AV_CODEC_ID_AV1:
        case AV_CODEC_ID_VVC: return true;
        default: return false;
    }
}

void ExtractorDemuxer::determineStreamTypes()
{
    for (unsigned int i = 0; i < m_formatContext->nb_streams; ++i) {
        AVCodecParameters* codecPar = m_formatContext->streams[i]->codecpar;
        AVCodecID codecID = codecPar->codec_id;

        if (codecPar->codec_type == AVMEDIA_TYPE_VIDEO) {
            baseFps = codecPar->framerate.num;
            m_maxReorder = codecPar->video_delay;
        }

        if (codecID == AV_CODEC_ID_LCEVC || codecPar->codec_tag == kLCEVCCodec) {
            m_lcevcStreamIndex = static_cast<int>(i);
        } else if (baseVideoCodec(codecID)) {
            bBaseStream = true;
        }
    }
    if (m_config.inputType == InputType::ELEMENTARY) {
        bRawStream = true;
    }
    if (m_inputType == InputType::MP4 && m_lcevcStreamIndex != -1 && bBaseStream) {
        AVCodecParameters* codecPar = m_formatContext->streams[m_lcevcStreamIndex]->codecpar;
        LCEVCDecoderConfiguration config = {};
        if (parseLvcCAtom(codecPar->extradata, codecPar->extradata_size, config)) {
            lvccProfile = config.lcevcProfileIndication;
            lvccLevel = config.lcevcLevelIndication;
            bLvccPresent = true;
        } else {
            VNLog::Info("InValid LVCC Atom found.\n");
        }
    }
}

void ExtractorDemuxer::addLCEVCData(LCEVC& lcevc, std::vector<LCEVC>& lcevcFrames)
{
    lcevc.maxReorderFrames = 32;
    if (m_inputType == InputType::ELEMENTARY) {
        lcevc.dts = m_dts++;
        m_lcevcDataMap[m_decodingIndex++] = std::move(lcevc);
    } else if (m_packet) {
        lcevc.pts = m_packet->pts;
        lcevc.dts = m_packet->dts;
        lcevcFrames.push_back(std::move(lcevc));
    }
}

bool ExtractorDemuxer::handleNALUnit(const std::vector<uint8_t>& nalUnit, std::vector<LCEVC>& lcevcFrames)
{
    LCEVC lcevc;
    if (nalUnit.empty()) {
        return false;
    }
    if (processNALUnit(nalUnit, lcevc, m_baseType)) {
        addLCEVCData(lcevc, lcevcFrames);
        return true;
    }
    return false;
}

bool ExtractorDemuxer::extractLCEVCData(std::vector<LCEVC>& lcevcFrames, const int streamPID)
{
    m_nalUnit.clear();
    bool bInNAL = false;
    const uint8_t* dataPtr = m_packet->data;
    size_t packetSize = m_packet->size;

    const int desiredPID = m_config.tsPID;
    if (desiredPID != -1 && streamPID != desiredPID) {
        return false;
    }

    while (packetSize > 0) {
        const uint8_t* readPtr = dataPtr;

        uint32_t startCodeSize = 0;
        if (matchAnnexBStartCode(readPtr, packetSize, startCodeSize)) {
            if (bInNAL && !m_nalUnit.empty()) {
                bool foundLCEVC = handleNALUnit(m_nalUnit, lcevcFrames);
                if (m_inputType == InputType::TS && foundLCEVC) {
                    return true;
                }
                m_nalUnit.clear();
            }
            bInNAL = true;

            m_nalUnit.insert(m_nalUnit.end(), readPtr, readPtr + startCodeSize);
            readPtr += startCodeSize;
        } else {
            m_nalUnit.push_back(*readPtr);
            ++readPtr;
        }

        size_t offset = readPtr - dataPtr;
        dataPtr += offset;
        packetSize -= offset;
    }
    // Last NAL unit
    if (bInNAL && !m_nalUnit.empty()) {
        bool foundLCEVC = handleNALUnit(m_nalUnit, lcevcFrames);
        if (m_inputType == InputType::TS && foundLCEVC) {
            return true;
        }
        m_nalUnit.clear();
    }
    return bRawStream;
}

bool ExtractorDemuxer::extractLCEVCMp4(std::vector<LCEVC>& lcevcFrames)
{
    const uint8_t* ptr = m_packet->data;
    const uint8_t* data = ptr;
    size_t packetSize = m_packet->size;

    LCEVC lcevc;
    while (packetSize > 0) {
        uint32_t headerSize = 0;
        if (!readBE(ptr, packetSize, headerSize)) {
            return false;
        }

        m_nalUnit.assign(ptr, ptr + headerSize);
        if (processNALUnit(m_nalUnit, lcevc, m_baseType)) {
            if (bFourBytePrefix) {
                lcevc.data.insert(lcevc.data.begin(), data, data + 4);
            }
            addLCEVCData(lcevc, lcevcFrames);
            return true;
        }

        packetSize -= headerSize;
        data += (headerSize + 4);
        ptr = data;
    }
    if (processNALUnit(m_nalUnit, lcevc, m_baseType)) {
        if (bFourBytePrefix) {
            lcevc.data.insert(lcevc.data.begin(), data, data + 4);
        }
        addLCEVCData(lcevc, lcevcFrames);
        return true;
    }
    return false;
}

bool ExtractorDemuxer::extractLCEVCWebm(std::vector<LCEVC>& lcevcFrames)
{
    for (int i = 0; i < m_packet->side_data_elems; i++) {
        if (m_packet->side_data[i].type != AV_PKT_DATA_MATROSKA_BLOCKADDITIONAL) {
            continue;
        }
        const uint8_t* blockData = m_packet->side_data[i].data;
        size_t size = m_packet->side_data[i].size;
        uint64_t id = 0;

        if (!readBE(blockData, size, id)) {
            continue;
        }

        LCEVC lcevc;
        if (id == kBlockAdditionalLcevcId) {
            lcevc.data.assign(blockData, blockData + size);
            addLCEVCData(lcevc, lcevcFrames);
            return true;
        }
    }
    return false;
}

bool ExtractorDemuxer::validateLCEVCAnnexB()
{
    const uint8_t* data = m_packet->data;
    size_t size = m_packet->size;

    const bool hasAnnexBLcevc = parseAnnexBHeader(data, size, CodecType::LCEVC);

    if (m_inputType == InputType::MP4) {
        if (hasAnnexBLcevc) {
            VNLog::Error("Invalid format: MP4 input contains Annex B start codes "
                         "instead of length-prefixed NAL units.\n");
            bError = true;
            return false;
        }
        bFourBytePrefix = true;
    }
    return true;
}

bool ExtractorDemuxer::extractFromSeparateTrack(std::vector<LCEVC>& lcevcFrames, const int streamPID)
{
    const int desiredPID = m_config.tsPID;
    if (m_inputType == InputType::TS && (streamPID != desiredPID && desiredPID != -1)) {
        return false;
    }

    if (!validateLCEVCAnnexB()) {
        return false;
    }

    if (m_packet->pos != m_pos && !m_lcevcAccumulator.data.empty()) {
        lcevcFrames.push_back(std::move(m_lcevcAccumulator));
        m_lcevcAccumulator = LCEVC();
    }

    if (m_packet->pts != AV_NOPTS_VALUE && m_packet->pts != AV_NOPTS_VALUE) {
        m_lcevcAccumulator.pts = m_packet->pts;
        m_lcevcAccumulator.dts = m_packet->dts;
    }

    m_pos = m_packet->pos;
    m_lcevcAccumulator.maxReorderFrames = 32;
    m_lcevcAccumulator.data.insert(m_lcevcAccumulator.data.end(), m_packet->data,
                                   m_packet->data + m_packet->size);
    return true;
}

bool ExtractorDemuxer::processNALFrameData(int64_t decodingIndex, FrameInfo& info,
                                           std::vector<LCEVC>& lcevcFrames)
{
    auto it = m_lcevcDataMap.find(decodingIndex);
    if (it == m_lcevcDataMap.end()) {
        VNLog::Error("No LCEVC data found for decoding index: %" PRId64, decodingIndex);
        bError = true;
        return false;
    }

    LCEVC& lcevc = it->second;

    info.pts = m_maxReorder + m_pts++;
    lcevc.pts = info.pts;
    lcevc.frameType = info.frameType;
    lcevc.frameSize = static_cast<int64_t>(info.frameSize);

    auto lcevcIt = m_lcevcDataMap.find(m_decodingCounter);
    if (lcevcIt != m_lcevcDataMap.end()) {
        LCEVC& lcevcOutput = lcevcIt->second;
        if (lcevcOutput.pts != kInvalidPts) {
            lcevcFrames.push_back(lcevcOutput);
            m_lcevcDataMap.erase(lcevcIt);
            m_decodingCounter++;
        }
    }

    return true;
}

bool ExtractorDemuxer::populateFrameInfo(AVCodecContext& codecCtx, FrameQueue& framebuffer,
                                         std::vector<LCEVC>& lcevcFrames)
{
    if (!m_frame) {
        return false;
    }

    if (int ret = avcodec_receive_frame(&codecCtx, m_frame);
        ret == AVERROR_EOF || ret == AVERROR(EAGAIN) || ret < 0) {
        return false;
    }

    FrameInfo info;
    info.frameNum = codecCtx.frame_num;
    info.frameSize = static_cast<uint64_t>(codecCtx.width) * codecCtx.height;
    switch (codecCtx.codec_id) {
        case AV_CODEC_ID_VP9:
        case AV_CODEC_ID_AV1: [[fallthrough]];
        case AV_CODEC_ID_VP8: {
            info.frameType =
                (m_packet->flags & AV_PKT_FLAG_KEY) ? FrameType::KeyFrame : FrameType::InterFrame;
            break;
        }
        default:
            switch (m_frame->pict_type) {
                case AV_PICTURE_TYPE_I: info.frameType = FrameType::I; break;
                case AV_PICTURE_TYPE_P: info.frameType = FrameType::P; break;
                case AV_PICTURE_TYPE_B: info.frameType = FrameType::B; break;
                default: info.frameType = FrameType::Unknown; break;
            }
            break;
    }

    baseFrameCount++;

    if (getRawStream()) {
        return processNALFrameData(m_frame->pts, info, lcevcFrames);
    }

    info.pts = m_frame->pts;
    framebuffer.enqueue(info);
    return true;
}

bool ExtractorDemuxer::extractFrameType(const AVStream& stream, FrameQueue& framebuffer,
                                        std::vector<LCEVC>& lcevcFrames)
{
    int streamIndex = stream.index;
    AVCodecContext*& codecCtx = m_codecContextMap[streamIndex];

    if (!codecCtx) {
        const AVCodec* codec = avcodec_find_decoder(stream.codecpar->codec_id);
        if (!codec) {
            VNLog::Error("Decoder not found for stream %d\n", streamIndex);
            return false;
        }

        codecCtx = avcodec_alloc_context3(codec);
        if (!codecCtx || avcodec_parameters_to_context(codecCtx, stream.codecpar) < 0 ||
            avcodec_open2(codecCtx, codec, nullptr) < 0) {
            VNLog::Error("Failed to init codec context\n");
            avcodec_free_context(&codecCtx);
            return false;
        }
    }

    if (avcodec_send_packet(codecCtx, m_packet) < 0) {
        return false;
    }

    bool bSuccess = false;
    while (true) {
        if (!populateFrameInfo(*codecCtx, framebuffer, lcevcFrames)) {
            break;
        }
        bSuccess = true;
    }
    return bSuccess;
}

bool ExtractorDemuxer::flush(FrameQueue& frameBuffer, std::vector<LCEVC>& lcevcFrames)
{
    bool hasFlushedFrames = false;
    for (const auto& [streamIndex, codecCtx] : m_codecContextMap) {
        if (!m_isFlush) {
            if (avcodec_send_packet(codecCtx, nullptr) < 0) {
                m_isFlush = true;
                continue;
            }
            m_isFlush = true;
        }

        while (populateFrameInfo(*codecCtx, frameBuffer, lcevcFrames)) {
            hasFlushedFrames = true;
        }
    }
    // Flush remaining entries from lcevcDataMap
    while (!m_lcevcDataMap.empty()) {
        auto mapIt = m_lcevcDataMap.find(m_decodingCounter);
        if (mapIt != m_lcevcDataMap.end()) {
            LCEVC& lcevc = mapIt->second;
            lcevcFrames.push_back(lcevc);
            m_lcevcDataMap.erase(mapIt);

            m_decodingCounter++;
            hasFlushedFrames = true;
        }
    }
    return hasFlushedFrames;
}

bool ExtractorDemuxer::next(std::vector<LCEVC>& lcevcFrames, FrameQueue& frameBuffer)
{
    const int desiredPID = m_config.tsPID;

    while (av_read_frame(m_formatContext, m_packet) >= 0) {
        int currentStreamIndex = m_packet->stream_index;
        const AVStream* currentStream = m_formatContext->streams[currentStreamIndex];
        std::string codecID = avcodec_get_name(currentStream->codecpar->codec_id);
        int streamPID = m_formatContext->streams[currentStreamIndex]->id;

        totalPacketSize += m_packet->size;

        if (m_lcevcStreamIndex == -1 && currentStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_baseType = utility::BaseType::fromString(codecID);
            switch (m_inputType) {
                case InputType::MP4: {
                    m_isLcevcFound |= extractLCEVCMp4(lcevcFrames);
                    break;
                }
                case InputType::WEBM: {
                    m_isLcevcFound |= extractLCEVCWebm(lcevcFrames);
                    break;
                }
                case InputType::ELEMENTARY: [[fallthrough]];
                case InputType::TS: {
                    m_isLcevcFound |= extractLCEVCData(lcevcFrames, streamPID);
                    break;
                }
                default:
                    VNLog::Error("Unsupported input type: %d\n", static_cast<int>(m_inputType));
                    m_isLcevcFound |= false;
                    break;
            }
        } else if (currentStreamIndex == m_lcevcStreamIndex) {
            m_isLcevcFound |= extractFromSeparateTrack(lcevcFrames, streamPID);
        }

        if (currentStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
            m_packet->stream_index != m_lcevcStreamIndex && bBaseStream) {
            if (getRawStream()) {
                m_packet->pts = m_packetCount++;
            }
            if (baseFps == 0) {
                baseFps = currentStream->avg_frame_rate.num / currentStream->avg_frame_rate.den;
            }
            extractFrameType(*currentStream, frameBuffer, lcevcFrames);
        }

        if (m_isLcevcFound) {
            av_packet_unref(m_packet);
            return true;
        }
    }

    if (m_isLcevcFound == false) {
        if (m_inputType == InputType::TS) {
            VNLog::Error("*** Demux - PID: 0x%X - LCEVC data not found in packet***\n", desiredPID);
        } else {
            VNLog::Error("Failed to find lcevc in packet\n");
        }
        av_packet_unref(m_packet);
        bError = true;
        return false;
    }

    if (m_lcevcAccumulator.data.empty() == false) {
        lcevcFrames.push_back(std::move(m_lcevcAccumulator));
        m_lcevcAccumulator = LCEVC();
        return true;
    }

    return false;
}

} // namespace vnova::analyzer
