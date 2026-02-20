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

/* Copyright (c) V-Nova International Limited 2025. All rights reserved. */
#include "helper/input_detection.h"

#include "app/config_types.h"
#include "helper/base_type.h"
#include "helper/nal_unit.h"
#include "helper/stream_reader.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <vector>

namespace vnova::helper {
namespace {
    using namespace vnova::helper;
    constexpr size_t kMaxProbeBytes = 4096;

    struct ElementaryProbeResult
    {
        CodecType codec = CodecType::UNKNOWN;
        bool isAnnexB = false;
        bool isAvcc = false;
        size_t startCodeHits = 0;
    };

    struct CodecEvidence
    {
        int h264 = 0;
        int hevc = 0;
        int vvc = 0;
        int lcevc = 0;
    };

    bool hasBinMagic(const uint8_t* data, size_t size)
    {
        static constexpr std::array<uint8_t, 8> kMagic = {0x6C, 0x63, 0x65, 0x76,
                                                          0x63, 0x62, 0x69, 0x6E};
        return size >= kMagic.size() && std::equal(kMagic.begin(), kMagic.end(), data);
    }

    bool looksLikeMp4Ftyp(const uint8_t* data, size_t size)
    {
        if (size < 12) {
            return false;
        }

        StreamReader reader;
        reader.Reset(data, size);
        auto boxSize = static_cast<uint32_t>(reader.ReadValue<uint32_t>());
        if (boxSize < 8 || boxSize > size + 16) {
            return false;
        }

        return data[4] == 'f' && data[5] == 't' && data[6] == 'y' && data[7] == 'p';
    }

    bool looksLikeMpegTs(const uint8_t* data, size_t size)
    {
        constexpr size_t packet_size = 188;
        constexpr size_t needed = packet_size * 3;

        if (size < needed) {
            return false;
        }

        return data[0] == 0x47 && data[packet_size] == 0x47 && data[packet_size * 2] == 0x47;
    }

    bool looksLikeEbml(const uint8_t* data, size_t size)
    {
        if (size < 4) {
            return false;
        }

        return data[0] == 0x1A && data[1] == 0x45 && data[2] == 0xDF && data[3] == 0xA3;
    }

    void accumulateCodecEvidence(const uint8_t* nal, size_t nalSize, CodecEvidence& evidence, bool& anyRecognised)
    {
        if (!nalSize) {
            return;
        }

        const std::array<CodecType, 4> kCodecs = {CodecType::LCEVC, CodecType::VVC, CodecType::HEVC,
                                                  CodecType::H264};

        for (auto codec : kCodecs) {
            ParsedNALHeader header{};
            if (!parseAnnexBHeader(nal, nalSize, codec, header) &&
                !parseLengthPrefixedHeader(nal, nalSize, codec, header)) {
                continue;
            }

            switch (header.codec) {
                case CodecType::H264:
                    evidence.h264++;
                    if (isVPS(header) || isSPS(header) || isPPS(header) || isSEI(header) || isAUD(header)) {
                        evidence.h264 += 2;
                    }
                    break;
                case CodecType::HEVC:
                    evidence.hevc++;
                    if (isVPS(header) || isSPS(header) || isPPS(header) || isSEI(header) || isAUD(header)) {
                        evidence.hevc += 2;
                    }
                    break;
                case CodecType::VVC:
                    evidence.vvc++;
                    if (isVPS(header) || isSPS(header) || isPPS(header) || isSEI(header) || isAUD(header)) {
                        evidence.vvc += 2;
                    }
                    break;
                case CodecType::LCEVC:
                    evidence.lcevc++;
                    if (isLcevcIDR(header) || isLcevcNonIDR(header)) {
                        evidence.lcevc += 2;
                    }
                    break;
                default: break;
            }
            anyRecognised = true;
        }
    }

    CodecType pickCodec(const CodecEvidence& evidence)
    {
        struct Score
        {
            CodecType codec;
            int value;
        };
        const std::array<Score, 4> scores = {{{CodecType::H264, evidence.h264},
                                              {CodecType::HEVC, evidence.hevc},
                                              {CodecType::VVC, evidence.vvc},
                                              {CodecType::LCEVC, evidence.lcevc}}};

        const auto best = // NOLINT(readability-qualified-auto)
            std::max_element(scores.begin(), scores.end(),
                             [](const Score& a, const Score& b) { return a.value < b.value; });

        if (best != scores.end() && best->value > 0) {
            return best->codec;
        }
        return CodecType::UNKNOWN;
    }

    bool looksLikeAvccStream(const uint8_t* data, size_t size, CodecEvidence& evidence)
    {
        if (size < 8) {
            return false;
        }

        size_t offset = 0;
        int inspectedNals = 0;
        bool recognised = false;

        while (offset + 4 <= size && inspectedNals < 4) {
            StreamReader reader;
            reader.Reset(data + offset, size - offset);
            const auto len = reader.ReadValue<uint32_t>();
            if (len == 0 || len > size - offset - 4) {
                return false;
            }

            const uint8_t* nal = data + offset + 4;
            size_t nalSize = len;
            bool valid = false;
            accumulateCodecEvidence(nal, nalSize, evidence, valid);
            if (!valid) {
                return false;
            }

            recognised = true;
            offset += 4 + len;
            inspectedNals++;
        }

        const int minNals = (size >= 32) ? 2 : 1;
        return recognised && inspectedNals >= minNals;
    }

    std::vector<size_t> findAnnexbStartcodes(const uint8_t* data, size_t size, size_t maxCodes = 32)
    {
        std::vector<size_t> positions;
        if (size < 3) {
            return positions;
        }

        for (size_t i = 0; i + 2 < size && positions.size() < maxCodes;) {
            uint8_t startSize = 0;
            if (matchAnnexBStartCode(data + i, size - i, startSize)) {
                positions.push_back(i);
                i += (startSize > 0) ? startSize : 1;
                continue;
            }
            ++i;
        }

        return positions;
    }

    ElementaryProbeResult probeAnnexbStream(const uint8_t* data, size_t size)
    {
        ElementaryProbeResult result;
        auto startcodes = findAnnexbStartcodes(data, size);
        if (startcodes.empty()) {
            return result;
        }

        CodecEvidence evidence;
        size_t examined = 0;

        for (size_t idx = 0; idx < startcodes.size(); ++idx) {
            size_t start = startcodes[idx];
            size_t next = (idx + 1 < startcodes.size()) ? startcodes[idx + 1] : size;
            size_t nalSize = (next > start) ? next - start : 0;
            if (nalSize == 0) {
                continue;
            }

            bool valid = false;
            accumulateCodecEvidence(data + start, nalSize, evidence, valid);
            if (valid) {
                examined++;
            }

            if (examined >= 8) {
                break;
            }
        }

        result.startCodeHits = startcodes.size();
        if (examined >= 1) {
            result.isAnnexB = true;
        }

        result.codec = pickCodec(evidence);
        return result;
    }

    ElementaryProbeResult detectElementaryStream(const uint8_t* data, size_t size)
    {
        ElementaryProbeResult result;
        CodecEvidence evidence;

        if (looksLikeAvccStream(data, size, evidence)) {
            result.codec = pickCodec(evidence);
            result.isAvcc = result.codec != CodecType::UNKNOWN;
            return result;
        }

        return probeAnnexbStream(data, size);
    }

    void applyElementaryDetection(const ElementaryProbeResult& probe, DetectedInputFormat& detected)
    {
        switch (probe.codec) {
            case CodecType::H264:
                detected.inputType = InputType::ES;
                detected.baseType = BaseType::H264;
                break;
            case CodecType::HEVC:
                detected.inputType = InputType::ES;
                detected.baseType = BaseType::HEVC;
                break;
            case CodecType::VVC:
                detected.inputType = InputType::ES;
                detected.baseType = BaseType::VVC;
                break;
            case CodecType::LCEVC:
                detected.inputType = InputType::LCEVC;
                detected.baseType = BaseType::UNKNOWN;
                break;
            default: break;
        }

        detected.isLikelyAnnexB = probe.isAnnexB;
        detected.isLikelyAvcc = probe.isAvcc;
    }

} // namespace

DetectedInputFormat detectInputFormatFromMemory(const uint8_t* data, size_t size)
{
    DetectedInputFormat detected;
    if (!data || size == 0) {
        return detected;
    }

    if (hasBinMagic(data, size)) {
        detected.inputType = InputType::BIN;
        detected.baseType = BaseType::UNKNOWN;
        return detected;
    }

    if (looksLikeMp4Ftyp(data, size)) {
        detected.inputType = InputType::MP4;
        detected.baseType = BaseType::UNKNOWN;
        return detected;
    }

    if (looksLikeMpegTs(data, size)) {
        detected.inputType = InputType::TS;
        detected.baseType = BaseType::UNKNOWN;
        return detected;
    }

    if (looksLikeEbml(data, size)) {
        detected.inputType = InputType::WEBM;
        detected.baseType = BaseType::UNKNOWN;
        return detected;
    }

    auto probe = detectElementaryStream(data, size);
    applyElementaryDetection(probe, detected);
    if (detected.inputType == InputType::Unknown && probe.isAnnexB) {
        // Default unknown Annex B to elementary; if no base type was found but the codec
        // evidence pointed to LCEVC, promote to LCEVC raw stream.
        if (probe.codec == CodecType::LCEVC) {
            detected.inputType = InputType::LCEVC;
            detected.baseType = BaseType::UNKNOWN;
        } else {
            detected.inputType = InputType::ES;
            detected.baseType = BaseType::UNKNOWN;
        }
    }
    if (detected.inputType == InputType::Unknown && probe.isAvcc) {
        detected.inputType = InputType::ES;
        detected.baseType = BaseType::UNKNOWN;
        detected.isLikelyAvcc = true;
    }
    return detected;
}

bool detectInputFormatFromFile(const std::filesystem::path& path, DetectedInputFormat& outFormat)
{
    outFormat = DetectedInputFormat();
    if (path.empty()) {
        return false;
    }

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        return false;
    }

    std::vector<char> raw(kMaxProbeBytes);
    ifs.read(raw.data(), static_cast<std::streamsize>(raw.size()));
    const std::streamsize readCount = ifs.gcount();

    if (readCount <= 0) {
        return true;
    }

    std::vector<uint8_t> buffer(raw.begin(), raw.begin() + readCount);
    outFormat = detectInputFormatFromMemory(buffer.data(), buffer.size());
    return true;
}

} // namespace vnova::helper
