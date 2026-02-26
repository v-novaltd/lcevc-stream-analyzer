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
#include "app/config.h"
#include "utility/platform.h"
// platform.h first to avoid min/max macro Windows issue.

#include "extractor_demuxer.h"
#include "helper/base_type.h"
#include "helper/nal_unit.h"
#include "utility/byte_order.h"
#include "utility/log_util.h"
#include "utility/span.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <optional>

using namespace vnova::utility;
using namespace vnova::helper;

namespace vnova::analyzer {

namespace {

    void avFrameDeleter(AVFrame* frame)
    {
        if (frame) {
            av_frame_free(&frame);
        }
    }
    using AVFramePtr = std::unique_ptr<AVFrame, decltype(&avFrameDeleter)>;

    class PacketMeta
    {
    public:
        struct Data
        {
            int64_t decodeIndex;
            int flags;
            int64_t dts;
        };

        static void Free(void* /*opaque*/, uint8_t* data) { av_free(data); }

        static int Attach(AVPacket* pkt, const int64_t decodeIndex)
        {
            auto* meta = static_cast<Data*>(av_mallocz(sizeof(Data)));
            if (!meta) {
                return AVERROR(ENOMEM);
            }

            meta->decodeIndex = decodeIndex;
            meta->flags = pkt->flags;
            meta->dts = pkt->dts;

            auto* data = static_cast<void*>(meta);
            AVBufferRef* ref =
                av_buffer_create(static_cast<uint8_t*>(data), sizeof(*meta), Free, nullptr, 0);
            if (!ref) {
                av_free(meta);
                return AVERROR(ENOMEM);
            }

            // Any previous opaque_ref should be replaced/unref'd.
            av_buffer_unref(&pkt->opaque_ref);
            pkt->opaque_ref = ref;
            return 0;
        }

        static std::optional<Data> Retrieve(const AVFrame* frame)
        {
            if (frame->opaque_ref) {
                // Horrible opaque cast from void* is needed here.
                // NOLINTNEXTLINE(bugprone-casting-through-void)
                const auto* meta = static_cast<Data*>(static_cast<void*>(frame->opaque_ref->data));
                return std::make_optional<Data>(*meta);
            }
            return std::nullopt;
        }
    };

    template <typename T>
    bool readBE(const uint8_t*& data, size_t& size, T& value)
    {
        if (size < sizeof(T)) {
            return false;
        }

        T rawValue;
        std::memcpy(&rawValue, data, sizeof(T));
        value = ByteOrder<T>::ToHost(rawValue);

        data += sizeof(T);
        size -= sizeof(T);
        return true;
    }

    void fixMisclassifiedStreams(AVFormatContext const & formatContext)
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

    std::optional<double> rationalToDouble(const AVRational& rational)
    {
        if (rational.den <= 0) {
            return std::nullopt;
        }
        return static_cast<double>(rational.num) / static_cast<double>(rational.den);
    }

    std::optional<double> validateFramerate(const AVRational& guessedFramerate)
    {
        const bool invalidGuess = guessedFramerate.num == 0 && guessedFramerate.den == 1;
        return invalidGuess ? std::nullopt : rationalToDouble(guessedFramerate);
    }

    std::optional<double> guessFramerate(AVFormatContext* context,
                                         const AVCodecParameters* codecPar, AVStream* stream)
    {
        if (auto framerate = validateFramerate(av_guess_frame_rate(context, stream, nullptr));
            framerate.has_value()) {
            return framerate;
        }

        if (auto framerate = validateFramerate(stream->avg_frame_rate); framerate.has_value()) {
            return framerate;
        }

        if (auto framerate = validateFramerate(codecPar->framerate); framerate.has_value()) {
            return framerate;
        }

        VNLOG_WARN("Failed to guess framerate");
        return std::nullopt;
    }

    std::optional<double> guessTimebase(const AVFormatContext*, const AVCodecParameters*,
                                        const AVStream* stream)
    {
        if (auto timebase = validateFramerate(stream->time_base); timebase.has_value()) {
            return timebase;
        }

        VNLOG_WARN("Failed to guess timebase");
        return std::nullopt;
    }

    bool parseLvcCAtom(const uint8_t* data, size_t size, LCEVCDecoderConfiguration& config)
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

    FrameType toFrameType(const AVPictureType& pic)
    {
        switch (pic) {
            case AVPictureType::AV_PICTURE_TYPE_I: return FrameType::I;
            case AVPictureType::AV_PICTURE_TYPE_B: return FrameType::B;
            case AVPictureType::AV_PICTURE_TYPE_P: return FrameType::P;
            default: return FrameType::Unknown;
        }
    }

    FrameType resolveFrameType(const AVCodecContext& codecCtx, const AVFrame* frame, const int flags)
    {
        switch (codecCtx.codec_id) {
            case AV_CODEC_ID_VP9:
            case AV_CODEC_ID_AV1: [[fallthrough]];
            case AV_CODEC_ID_VP8:
                return (flags & AV_PKT_FLAG_KEY) ? FrameType::KeyFrame : FrameType::InterFrame;
            default: return frame ? toFrameType(frame->pict_type) : FrameType::Unknown;
        }
    }

    void dumpAVPacket(const AVPacket* packet, const std::filesystem::path& dir)
    {
        constexpr const char* fmt = "dts%08" PRId64 ".pts%08" PRId64 ".bin";
        std::string filename;

        const size_t strLength =
            snprintf(nullptr, 0, fmt, packet->pts, packet->dts != kInvalidDts ? packet->dts : 0);
        filename.resize(strLength + 1);
        snprintf(filename.data(), filename.size(), fmt, packet->pts,
                 packet->dts != kInvalidDts ? packet->dts : 0);

        std::ofstream out(dir / filename, std::ios::binary);
        if (!out) {
            throw vnova::utility::file::FileError("Failed to open bin dump file");
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        out.write(reinterpret_cast<const char*>(packet->data), packet->size);
    }

    class NALScanResult
    {
    public:
        NALScanResult(const helper::BaseType& codec, const bool isAnnexB,
                      const helper::ParsedNALHeader& header, const size_t location)
            : m_codec(codec)
            , m_header(header)
            , m_location(location)
            , m_isAnnexB(isAnnexB)
        {}

        void Parse(const uint8_t* bufferData, const size_t bufferSize, const size_t nalSize)
        {
            // Ignore header
            const uint8_t* bufferLocation = bufferData + m_location;
            size_t bufferRemaining = bufferSize - m_location;
            size_t nalRemaining = nalSize;

            uint32_t seiHeaderSize = 0;
            if (isSEINALUnit(m_codec, bufferLocation, bufferRemaining, seiHeaderSize) == false) {
                m_isParsed = true;
                return;
            }

            bufferLocation += seiHeaderSize;

            nalRemaining -= seiHeaderSize;

            // Unencapsulate the data into a destination buffer. The destination
            // is required to be as big as encapsulated data, unencapsulation will
            // never expand the number of bytes only contract.
            utility::DataBuffer unencapsulatedNal(nalSize, 0);

            const uint32_t dataLength =
                helper::nalUnencapsulate(unencapsulatedNal.data(), bufferLocation, nalRemaining);
            VNAssert(dataLength <= nalSize);

            const uint8_t* dataPtr = unencapsulatedNal.data();
            const uint8_t* dataEnd = dataPtr + dataLength;

            while (dataPtr < dataEnd) {
                const auto payloadType = std::byte{*dataPtr++};

                size_t payloadSize = 0;
                while (*dataPtr == 0xFF) {
                    payloadSize += 0xFF;
                    dataPtr++;
                }
                payloadSize += *dataPtr++;

                m_seiPayloads += 1;

                if ((payloadType == helper::kRegisterUserDataSEIType.at(0)) &&
                    (payloadSize >= helper::kLCEVCITUHeader.size()) &&
                    helper::isLCEVCITUPayload(dataPtr)) {
                    m_lcevcSeiPayloads += 1;
                }

                dataPtr += payloadSize;
            }

            m_isParsed = true;
        }

        static size_t PrintAll(const uint8_t* bufferData, const size_t bufferSize,
                               std::vector<NALScanResult>& results, const std::vector<size_t>& offsets)
        {
            if (offsets.size() != results.size() + 1) {
                throw std::runtime_error("Offsets must be 1 longer than headers, where the size of "
                                         "the packet is the last value.");
            }

            size_t accumulatedSize = 0;

            for (size_t index = 0; index < results.size(); index++) {
                auto& result = results.at(index);
                const size_t size = offsets.at(index + 1) - offsets.at(index);
                result.Parse(bufferData, bufferSize, size);
                result.Print(index, size);

                accumulatedSize += size;
            }
            return accumulatedSize;
        }

    private:
        helper::BaseType m_codec;
        helper::ParsedNALHeader m_header;
        size_t m_location;
        bool m_isAnnexB;

        bool m_isParsed = false;

        size_t m_seiPayloads = 0;
        size_t m_lcevcSeiPayloads = 0;

        void Print(const size_t index, const size_t nalLength) const
        {
            if (m_isParsed) {
                VNLOG_INFO("    nal:%2d - %10s (%3d)   nal_length:%d is_annex_b:%d n_sei:%d "
                           "n_lcevc_sei:%d",
                           index, toString(m_header.nalUnitValue, m_codec), m_header.nalUnitValue,
                           nalLength, m_isAnnexB, m_seiPayloads, m_lcevcSeiPayloads);
            } else {
                VNLOG_ERROR("Tried to print unparsed NALScanResult");
            }
        }
    };

    void scanAnnexBNalUnits(const AVPacket* packet, int streamIndex, const helper::BaseType& codec)
    {
        const auto* location = packet->data;
        int remaining = packet->size;

        std::vector<NALScanResult> results;
        std::vector<size_t> locations;

        VNLOG_INFO("stream:%2d dts:%10d pts:%10d size: %10d", streamIndex, packet->dts, packet->pts,
                   packet->size);

        while (remaining) {
            if (helper::ParsedNALHeader header; parseAnnexBHeader(location, remaining, codec, header)) {
                results.emplace_back(codec, true, header, location - packet->data);
                locations.push_back(location - packet->data);

                location += header.headerSize;
                remaining -= static_cast<int>(header.headerSize);

            } else {
                location += 1;
                remaining -= 1;
            }
        }
        locations.push_back(packet->size);

        const size_t runningSize = NALScanResult::PrintAll(packet->data, packet->size, results, locations);
        if (runningSize != static_cast<size_t>(packet->size)) {
            VNLOG_ERROR("ERROR: Part of packet unaccounted for");
        }
    }

    bool isBaseType(AVCodecID codecID)
    {
        switch (codecID) {
            case AV_CODEC_ID_H264:
            case AV_CODEC_ID_HEVC:
            case AV_CODEC_ID_VVC:
            case AV_CODEC_ID_VP9:
            case AV_CODEC_ID_VP8:
            case AV_CODEC_ID_AV1: return true;
            default: return false;
        }
    }

    bool isBaseTypeSizeCountable(AVCodecID codecID)
    {
        switch (codecID) {
            case AV_CODEC_ID_H264:
            case AV_CODEC_ID_HEVC:
            case AV_CODEC_ID_VVC: return true;
            case AV_CODEC_ID_VP9:
            case AV_CODEC_ID_VP8:
            case AV_CODEC_ID_AV1:
            default: return false;
        }
    }

} // namespace

ExtractorDemuxer::ExtractorDemuxer(const std::filesystem::path& url, InputType type, const Config& config)
    : m_inputType(type)
    , m_config(config)
    , m_packet(av_packet_alloc())
{
    if (!m_packet) {
        VNLOG_ERROR("Failed to allocate AVPacket");
        m_error = true;
        return;
    }

    const AVInputFormat* fmt = nullptr;
    switch (type) {
        case InputType::MP4: {
            fmt = av_find_input_format("mp4");
            if (fmt == nullptr) {
                VNLOG_INFO("Failed to turn type into a valid input format for mp4");
                m_error = true;
            }
        } break;
        case InputType::ES: {
            switch (config.baseType) {
                case vnova::helper::BaseType::H264: {
                    fmt = av_find_input_format("h264");
                } break;
                case vnova::helper::BaseType::HEVC: {
                    fmt = av_find_input_format("hevc");
                } break;
                case vnova::helper::BaseType::VVC: {
                    fmt = av_find_input_format("vvc");
                } break;
                default: {
                    if (fmt == nullptr) {
                        VNLOG_INFO("Failed to turn type into a valid input format for es");
                        m_error = true;
                    }
                    break;
                }
            }
        } break;
        case InputType::TS: {
            fmt = av_find_input_format("mpegts");
            if (fmt == nullptr) {
                VNLOG_INFO("Failed to turn type into a valid input format for ts");
                m_error = true;
            }
        } break;
        default: break;
    }

    if (avformat_open_input(&m_formatContext, url.string().c_str(), fmt, nullptr) != 0) {
        VNLOG_ERROR("Could not open input file ");
        m_error = true;
        return;
    }
    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        VNLOG_ERROR("Could not find stream info ");
        m_error = true;
        return;
    }

    fixMisclassifiedStreams(*m_formatContext);

    if (determineStreamTypes() == false) {
        m_error = true;
    }
    m_durationSec = (double)m_formatContext->duration / AV_TIME_BASE;
    m_initialized = true;
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
}

bool ExtractorDemuxer::determineStreamTypes()
{
    for (unsigned int i = 0; i < m_formatContext->nb_streams; ++i) {
        AVStream* stream = m_formatContext->streams[i];
        const AVCodecParameters* codecPar = stream->codecpar;
        AVCodecID codecID = codecPar->codec_id;

        if (codecPar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (m_baseFramerate.has_value() == false) {
                m_baseFramerate = guessFramerate(m_formatContext, codecPar, stream);
            }
            if (m_baseTimebase.has_value() == false) {
                m_baseTimebase = guessTimebase(m_formatContext, codecPar, stream);
            }
            m_maxReorder = std::max(static_cast<uint8_t>(codecPar->video_delay), m_maxReorder);
        }

        if (codecID == AV_CODEC_ID_LCEVC || codecPar->codec_tag == kLCEVCCodec) {
            VNLOG_INFO("Stream index %d contains LCEVC data", stream->index);
            m_lcevcStreamIndex = stream->index;
        } else if (isBaseType(codecID)) {
            const auto* codecName = avcodec_get_name(codecID);
            VNLOG_INFO("Stream index %d contains %s video data", stream->index, codecName);
            const auto baseType = helper::baseTypeFromString(codecName);
            if (checkBaseTypeIsAllowed(baseType) == false) {
                m_error = true;
                return false;
            }

            // Guarding rather than assigning prevents subesquent non-video streams from putting this back to false.
            m_hasBaseVideoStream = true;
            m_baseVideoStreamIsSizeCountable = isBaseTypeSizeCountable(codecID);
        } else {
            VNLOG_INFO("Stream index %d contains %s data, skipping", stream->index,
                       avcodec_get_name(codecID));
        }
    }
    if (m_config.inputType == InputType::ES) {
        m_rawStream = true;
    }
    if (m_inputType == InputType::MP4 && m_lcevcStreamIndex.has_value() && m_hasBaseVideoStream) {
        const AVCodecParameters* codecPar = m_formatContext->streams[m_lcevcStreamIndex.value()]->codecpar;
        LCEVCDecoderConfiguration config = {};
        if (parseLvcCAtom(codecPar->extradata, codecPar->extradata_size, config)) {
            m_lvccProfile = config.lcevcProfileIndication;
            m_lvccLevel = config.lcevcLevelIndication;
            m_lvccPresent = true;
        } else {
            VNLOG_INFO("InValid LVCC Atom found");
            return false;
        }
    }
    return true;
}

bool ExtractorDemuxer::storeLcevcFrame(LCEVCFrame& lcevc)
{
    lcevc.maxReorderFrames = m_maxReorder;

    if (lcevc.lcevcWireSize <= 0) {
        lcevc.lcevcWireSize = static_cast<int64_t>(lcevc.data.size());
    }

    if ((m_remainingSizeForDts.count(lcevc.dts) > 0) &&
        (m_remainingSizeForDts[lcevc.dts] - lcevc.lcevcWireSize >= 0)) {
        m_remainingSizeForDts[lcevc.dts] -= lcevc.lcevcWireSize;

    } else if (m_baseVideoStreamIsSizeCountable) {
        // This check does not apply to non-size countable base streams (AV1, VP9)
        VNLOG_ERROR("Not sure how to account for lcevc bytes as there is no remaining size map "
                    "entry for dts %d, or subtracting %d from it would send it negative",
                    lcevc.dts, lcevc.lcevcWireSize);
        return false;
    }

    lcevc.totalWireSize = m_totalSizeForDts[lcevc.dts];

    // A demuxed frame is considered finished if it has a valid PTS. Until we have matched with a
    // base frame, the PTS remains invalid. The correct PTS for this LCEVC frame will be given to
    // the LCEVC frame in provideFrameInfoToLcevc(), and the demuxed frame will be marked finished
    // then.
    const bool finished = lcevc.pts != kInvalidPts;
    m_demuxedFrameMap[m_lcevcDecodeIndex] = {std::move(lcevc), m_lcevcDecodeIndex, finished};
    m_lcevcDecodeIndex += 1;

    return true;
}

bool ExtractorDemuxer::releaseLcevcFrame(std::vector<LCEVCFrame>& lcevcFrames)
{
    auto it = m_demuxedFrameMap.find(m_mapSearchIndex);
    if (it == m_demuxedFrameMap.end()) {
        return false;
    }

    DemuxedFrame& demuxedFrame = it->second;
    if (demuxedFrame.finished == false) {
        if (const auto preParsedFrameIt = m_preParsedFrameInfo.find(demuxedFrame.decodeIndex);
            preParsedFrameIt != m_preParsedFrameInfo.end()) {
            provideFrameInfoToLcevc(demuxedFrame.decodeIndex, preParsedFrameIt->second);
            m_preParsedFrameInfo.erase(preParsedFrameIt);
        } else {
            return false;
        }
    }

    if (m_releasedPtsTracker.count(demuxedFrame.lcevc.pts) > 0) {
        VNLOG_ERROR("Trying to release lcevc frame %d but a previous frame had same pts %d",
                    m_mapSearchIndex, demuxedFrame.lcevc.pts);
        m_error = true;
        return false;
    }
    m_releasedPtsTracker.insert(demuxedFrame.lcevc.pts);

    demuxedFrame.lcevc.totalWireSize = m_totalSizeForDts[demuxedFrame.lcevc.dts];
    lcevcFrames.push_back(demuxedFrame.lcevc);
    m_demuxedFrameMap.erase(it);
    m_mapSearchIndex += 1;
    return true;
}

bool ExtractorDemuxer::handlePacketNalUnit(const utility::DataBuffer& nalBuffer)
{
    if (nalBuffer.empty()) {
        return false;
    }
    if (std::optional<LCEVCFrame> lcevcOpt = parseNalIsLcevc(nalBuffer, m_baseType); lcevcOpt.has_value()) {
        auto& lcevc = lcevcOpt.value();
        lcevc.dts = m_packet->dts;
        return storeLcevcFrame(lcevc);
    }
    return false;
}

bool ExtractorDemuxer::extractLcevcStream(const int streamPID)
{
    utility::DataBuffer nalBuffer;
    nalBuffer.clear();

    bool nalStarted = false;
    bool foundLcevcNal = false;

    utility::Span data{m_packet->data, static_cast<size_t>(m_packet->size)};

    if (const int desiredPID = m_config.tsPID; desiredPID != -1 && streamPID != desiredPID) {
        return false;
    }

    while (data.Remaining() > 0) {
        if (uint8_t startCodeSize = 0;
            helper::matchAnnexBStartCode(data.Head(), data.Remaining(), startCodeSize)) {
            // It's the start of a new NAL unit - if we have one buffered, process it now. The
            // first pass through this loop we won't.

            if (nalStarted && handlePacketNalUnit(nalBuffer) && m_inputType == InputType::TS) {
                // We found LCEVC!
                foundLcevcNal = true;
            }

            // We did not find LCEVC, clear buffer and test next NAL.
            nalBuffer.clear();

            // Store the Annex B start code. We will then copy each byte in until the next start code, to process the NAL
            // header as one when we encounter the next NAL start, or the end of the packet.
            nalBuffer.insert(nalBuffer.end(), data.Head(), data.Head() + startCodeSize);
            data.Forward(startCodeSize);

            // Now we are in a NAL.
            nalStarted = true;

        } else {
            // Copy byte in to working nal unit.
            nalBuffer.push_back(*data.Head());
            data.Forward(1);
        }
    }

    // Process the last NAL unit in our packet.
    if (nalStarted && handlePacketNalUnit(nalBuffer) && m_inputType == InputType::TS) {
        foundLcevcNal = true;
    }

    if (foundLcevcNal == true) {
        return true;
    }

    return m_rawStream;
}

bool ExtractorDemuxer::extractLcevcMp4()
{
    const uint8_t* ptr = m_packet->data;
    const uint8_t* data = ptr;
    size_t packetSize = m_packet->size;

    utility::DataBuffer nalBuffer;
    nalBuffer.reserve(32768);

    while (packetSize > 0) {
        uint32_t payloadSize = 0;
        if (!readBE(ptr, packetSize, payloadSize)) {
            return false;
        }

        if (payloadSize > packetSize) {
            VNLOG_ERROR("Reported payloadSize (%d) >  remaining packetSize (%d)", payloadSize, packetSize);
            m_error = true;
            return false;
        }

        nalBuffer.assign(ptr, ptr + payloadSize);
        if (handlePacketNalUnit(nalBuffer)) {
            return true;
        }

        packetSize -= payloadSize;
        data += (payloadSize + 4);
        ptr = data;
    }
    if (handlePacketNalUnit(nalBuffer)) {
        return true;
    }
    return false;
}

bool ExtractorDemuxer::extractLcevcWebm()
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

        LCEVCFrame lcevc;
        lcevc.dts = m_packet->dts;

        if (id == kBlockAdditionalLcevcId) {
            lcevc.data.assign(blockData, blockData + size);
            return storeLcevcFrame(lcevc);
        }
    }
    return false;
}

bool ExtractorDemuxer::validateMp4IsNotAnnexB()
{
    const bool isMp4 = m_inputType == InputType::MP4;

    if (isMp4 && parseAnnexBHeader(m_packet->data, m_packet->size, helper::CodecType::LCEVC)) {
        VNLOG_ERROR("MP4 must use length-prefixed NAL units, not Annex B start codes.");
        m_error = true;
        return false;
    }

    m_fourBytePrefix = isMp4;

    return true;
}

bool ExtractorDemuxer::extractFromSingleTrack(const char* streamCodecName, const int streamPID)
{
    m_baseType = helper::baseTypeFromString(streamCodecName);
    switch (m_inputType) {
        case InputType::MP4: {
            return extractLcevcMp4();
        }
        case InputType::WEBM: {
            return extractLcevcWebm();
        }
        case InputType::ES: [[fallthrough]];
        case InputType::TS: {
            return extractLcevcStream(streamPID);
        }
        default:
            VNLOG_ERROR("Unsupported input type: %d", static_cast<int>(m_inputType));
            return false;
    }
}

bool ExtractorDemuxer::extractFromSeparateTrack(const int streamPID)
{
    if (m_inputType == InputType::TS && (streamPID != m_config.tsPID && m_config.tsPID != -1)) {
        return false;
    }

    if (validateMp4IsNotAnnexB() == false) {
        return false;
    }

    LCEVCFrame lcevc;
    lcevc.dts = m_packet->dts;

    if (m_packet->pts == kInvalidPts) {
        VNLOG_ERROR("Invalid pts in separate track - this is unexpected");
        m_error = true;
        return false;
    }
    lcevc.pts = m_packet->pts;

    lcevc.maxReorderFrames = 32;
    lcevc.data.insert(lcevc.data.end(), m_packet->data, m_packet->data + m_packet->size);

    const bool ret = storeLcevcFrame(lcevc);
    lcevc = LCEVCFrame();
    return ret;
}

void ExtractorDemuxer::provideFrameInfoToLcevc(int64_t decodingIndex, const BaseFrame& info)
{
    if (m_demuxedFrameMap.count(decodingIndex) == 0) {
        if (m_preParsedFrameInfo.count(decodingIndex) > 0) {
            VNLOG_ERROR("Already stored pre-parsed frame info for decoding index: %" PRId64, decodingIndex);
            m_error = true;
            return;
        }
        m_preParsedFrameInfo[decodingIndex] = info;
        return;
    }

    DemuxedFrame& demuxedFrame = m_demuxedFrameMap.at(decodingIndex);
    if (demuxedFrame.finished == false) {
        // Now we finally get to give the lcevc frame, which we associated via decodingIndex, its true PTS.
        demuxedFrame.lcevc.pts = info.pts;

        // This frame is finished.
        demuxedFrame.finished = true;

    } else if (demuxedFrame.lcevc.pts != info.pts) {
        // If we claim the frame is finished, then the LCEVC and base pts must match.
        VNLOG_ERROR(
            "Supposedly finished lcevc frame pts does not match associated base frame info pts");
        m_error = true;
        return;
    }
}

bool ExtractorDemuxer::populateFrameInfo(AVCodecContext& codecCtx, BaseFrameQueue& frameQueue)
{
    AVFramePtr frame(av_frame_alloc(), &avFrameDeleter);

    if (!frame) {
        VNLOG_ERROR("Failed to allocate AVFrame ");
        return false;
    }

    if (int ret = avcodec_receive_frame(&codecCtx, frame.get());
        ret == AVERROR_EOF || ret == AVERROR(EAGAIN) || ret < 0) {
        return false;
    }

    auto meta = PacketMeta::Retrieve(frame.get());
    if (meta.has_value() == false) {
        VNLOG_ERROR("Failed to decode packet metadata");
        return false;
    }

    const int64_t decodeIndex = meta->decodeIndex;
    const int64_t presentationIndex =
        frame->pts == AV_NOPTS_VALUE ? static_cast<int64_t>(m_decodedBaseFrameCount) : frame->pts;

    m_decodedBaseFrameCount += 1;

    BaseFrame info{decodeIndex,
                   meta->dts,
                   presentationIndex,
                   codecCtx.frame_num,
                   resolveFrameType(codecCtx, frame.get(), meta->flags),
                   codecCtx.width,
                   codecCtx.height};

    provideFrameInfoToLcevc(decodeIndex, info);
    frameQueue.push(info);

    return true;
}

bool ExtractorDemuxer::processBaseFrame(const AVStream& stream, BaseFrameQueue& frameQueue)
{
    const int streamIndex = stream.index;
    AVCodecContext*& codecCtx = m_codecContextMap[streamIndex];

    if (!codecCtx) {
        const AVCodec* codec = avcodec_find_decoder(stream.codecpar->codec_id);
        if (!codec) {
            VNLOG_ERROR("Decoder not found for stream %d", streamIndex);
            return false;
        }

        codecCtx = avcodec_alloc_context3(codec);

        // Push opaque user-defined metadata to each frame from one of the packets which form it.
        codecCtx->flags |= AV_CODEC_FLAG_COPY_OPAQUE;

        if (!codecCtx || avcodec_parameters_to_context(codecCtx, stream.codecpar) < 0 ||
            avcodec_open2(codecCtx, codec, nullptr) < 0) {
            VNLOG_ERROR("Failed to init codec context");
            avcodec_free_context(&codecCtx);
            return false;
        }
    }

    if (avcodec_send_packet(codecCtx, m_packet) < 0) {
        return false;
    }

    bool anyFramesProcessed = false;
    while (populateFrameInfo(*codecCtx, frameQueue)) {
        anyFramesProcessed = true;
    }
    return anyFramesProcessed;
}

bool ExtractorDemuxer::flush(BaseFrameQueue& frameQueue, std::vector<LCEVCFrame>& lcevcFrames)
{
    // Flush remaining entries from lcevcDataMap
    {
        bool hasReleasedFrames = true;
        while (hasReleasedFrames) {
            hasReleasedFrames = releaseLcevcFrame(lcevcFrames);
        }
    }

    bool hasFlushedFrames = false;
    for (const auto& [streamIndex, codecCtx] : m_codecContextMap) {
        if (!m_isFlush) {
            if (avcodec_send_packet(codecCtx, nullptr) < 0) {
                m_isFlush = true;
                continue;
            }
            m_isFlush = true;
        }

        while (populateFrameInfo(*codecCtx, frameQueue)) {
            hasFlushedFrames = true;
        }
    }

    // Flush remaining entries from lcevcDataMap
    {
        bool hasReleasedFrames = true;
        while (hasReleasedFrames) {
            hasReleasedFrames = releaseLcevcFrame(lcevcFrames);
        }
    }

    return hasFlushedFrames;
}

bool ExtractorDemuxer::next(std::vector<LCEVCFrame>& lcevcFrames, BaseFrameQueue& frameQueue)
{
    const int desiredPID = m_config.tsPID;

    // Flush one entries from lcevcDataMap, if there is one ready.
    releaseLcevcFrame(lcevcFrames);

    while (av_read_frame(m_formatContext, m_packet) >= 0) {
        const int streamIndex = m_packet->stream_index;
        const AVStream* stream = m_formatContext->streams[streamIndex];
        const int streamPID = stream->id;
        const char* streamCodecName = avcodec_get_name(stream->codecpar->codec_id);
        const bool streamContainsVideo =
            m_hasBaseVideoStream && stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO;
        const bool streamIsSeparate = (m_lcevcStreamIndex.has_value() == true);
        const bool streamIsSeparateAndIsLcevc =
            streamIsSeparate && (m_lcevcStreamIndex.value() == streamIndex);

        const bool streamIsBase = (streamIsSeparate == false) ||
                                  (streamIsSeparateAndIsLcevc == false && streamContainsVideo);

        if (m_config.dumpAuOutputPath.empty() == false) {
            std::filesystem::create_directories(m_config.dumpAuOutputPath);
            dumpAVPacket(m_packet, m_config.dumpAuOutputPath);

            scanAnnexBNalUnits(m_packet, streamIndex, m_config.baseType);
        }

        if (m_packet->dts == AV_NOPTS_VALUE) {
            if (m_rawStream == true) {
                // Synthesize a monotonically incrementing dts for raw streams.
                m_packet->dts = m_baseDecodeIndex[streamIndex];
            } else {
                VNLOG_ERROR("Invalid pts for non-ES packet at pos %d", m_packet->pos);
                av_packet_unref(m_packet);
                return true;
            }
        }

        PacketMeta::Attach(m_packet, m_baseDecodeIndex[streamIndex]);
        m_baseDecodeIndex[streamIndex] += 1;

        m_totalSizeForDts[m_packet->dts] += m_packet->size;
        m_remainingSizeForDts[m_packet->dts] += m_packet->size;

        if (streamIsSeparateAndIsLcevc) {
            m_isLcevcFound |= extractFromSeparateTrack(streamPID);
        } else if (streamContainsVideo) {
            m_isLcevcFound |= extractFromSingleTrack(streamCodecName, streamPID);
        } else {
            // Audio, other tracks, etc... Do nothing.
            av_packet_unref(m_packet);
            return true;
        }

        if (streamIsBase) {
            processBaseFrame(*stream, frameQueue);
        }

        // We processed a packet in this call to next and have at some point found LCEVC.
        // There is more to come, so return true.
        if (m_isLcevcFound == true) {
            av_packet_unref(m_packet);
            return true;
        }
    }

    if (m_isLcevcFound == false) {
        if (m_inputType == InputType::TS) {
            VNLOG_ERROR("Demux PID: %d - LCEVC data not found in packet", desiredPID);
        } else {
            VNLOG_ERROR("Failed to find LCEVC in packet");
        }
        av_packet_unref(m_packet);
        m_error = true;
        return false;
    }

    if (m_lcevcAccumulator.data.empty() == false) {
        bool ret = storeLcevcFrame(m_lcevcAccumulator);
        m_lcevcAccumulator = LCEVCFrame();
        return ret;
    }

    return false;
}

} // namespace vnova::analyzer
