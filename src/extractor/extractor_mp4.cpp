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
#include "extractor_mp4.h"

#include "helper/nal_unit.h"
#include "utility/platform.h"

using namespace vnova::utility;
using namespace vnova::helper;

namespace vnova::analyzer {
namespace {
    bool isLengthPrefixedSEI(const uint8_t*& data, helper::BaseType baseType, size_t length)
    {
        helper::ParsedNALHeader hdr;
        if (!helper::parseLengthPrefixedHeader(data, length, baseType, hdr)) {
            return false;
        }
        data += hdr.headerSize; // advance past header bytes
        return isSEI(hdr);
    }
} // namespace

ExtractorMP4::ExtractorMP4(const std::filesystem::path& url, InputType type, const Config& config)
    : ExtractorDemuxer(url, type, config)
{}

std::optional<LCEVCFrame> ExtractorMP4::parseNalIsLcevc(const utility::DataBuffer& nalUnit,
                                                        helper::BaseType& baseType)
{
    LCEVCFrame lcevc;

    const uint8_t* nalUnitData = nalUnit.data();
    size_t nalUnitLength = nalUnit.size();

    const uint8_t* ptr = nalUnitData;
    if (isLengthPrefixedSEI(nalUnitData, baseType, nalUnitLength)) {
        nalUnitLength -= (nalUnitData - ptr);

        utility::DataBuffer data(nalUnitLength, 0);

        const uint32_t dataLength = helper::nalUnencapsulate(data.data(), nalUnitData, nalUnitLength);
        VNAssert(dataLength <= nalUnitLength);

        const uint8_t* dataPtr = data.data();
        const uint8_t* dataEnd = dataPtr + dataLength;

        while (dataPtr < dataEnd) {
            auto payloadType = std::byte{*dataPtr++};
            uint8_t byte = 0;
            size_t payloadSize = 0;

            do {
                byte = *dataPtr++;
                payloadSize += byte;
            } while (byte == 0xFF);

            if ((payloadType == helper::kRegisterUserDataSEIType.at(0)) &&
                (payloadSize >= helper::kLCEVCITUHeader.size()) &&
                memcmp(dataPtr, helper::kLCEVCITUHeader.data(), helper::kLCEVCITUHeader.size()) == 0) {
                lcevc.data.assign(dataPtr + helper::kLCEVCITUHeader.size(), dataPtr + payloadSize);
                return lcevc;
            }
            dataPtr += payloadSize;
        }
    } else {
        if (parseLengthPrefixedHeader(ptr, nalUnitLength, helper::CodecType::LCEVC)) {
            m_fourBytePrefix = true;
            lcevc.data.insert(lcevc.data.end(), ptr, ptr + nalUnitLength);
            return lcevc;
        }
    }
    return std::nullopt;
}

} // namespace vnova::analyzer
