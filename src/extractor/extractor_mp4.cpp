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
#include "extractor_mp4.h"

#include "utility/log_util.h"
#include "utility/nal_header.h"
#include "utility/nal_unit.h"

#include <string>

using namespace vnova::utility;

namespace vnova::analyzer {
static constexpr uint8_t kRegisterUserDataSEIType = 4;
static constexpr uint32_t kLCEVCITUSize = 4;
static constexpr uint8_t kLCEVCITUHeader[kLCEVCITUSize] = {0xb4, 0x00, 0x50, 0x00};

bool checkSEINALUnit(const uint8_t*& data, BaseType::Enum baseType, size_t length)
{
    ParsedNALHeader hdr;
    if (!parseLengthPrefixedHeader(data, length, baseType, hdr)) {
        return false;
    }
    data += hdr.headerSize; // advance past header bytes
    return isSEI(hdr);
}

ExtractorMP4::ExtractorMP4(const std::string& url, InputType type, const Config& config)
    : ExtractorDemuxer(url, type, config)
{}

bool ExtractorMP4::processNALUnit(const utility::DataBuffer& nalUnit, LCEVC& lcevc,
                                  utility::BaseType::Enum& baseType)
{
    const uint8_t* nalUnitData = nalUnit.data();
    size_t nalUnitLength = nalUnit.size();

    const uint8_t* ptr = nalUnitData;
    if (checkSEINALUnit(nalUnitData, baseType, nalUnitLength)) {
        nalUnitLength -= (nalUnitData - ptr);

        utility::DataBuffer data(nalUnitLength, 0);

        const uint32_t dataLength = nalUnencapsulate(data.data(), nalUnitData, nalUnitLength);
        VNAssert(dataLength <= nalUnitLength);

        const uint8_t* dataPtr = data.data();
        const uint8_t* dataEnd = dataPtr + dataLength;

        while (dataPtr < dataEnd) {
            uint32_t payloadType = 0;
            uint8_t byte = 0;
            size_t payloadSize = 0;

            do {
                byte = *dataPtr++;
                payloadType += byte;
            } while (byte == 0xFF);

            do {
                byte = *dataPtr++;
                payloadSize += byte;
            } while (byte == 0xFF);

            if ((payloadType == kRegisterUserDataSEIType) && (payloadSize >= kLCEVCITUSize) &&
                memcmp(dataPtr, kLCEVCITUHeader, kLCEVCITUSize) == 0) {
                lcevc.data.assign(dataPtr + kLCEVCITUSize, dataPtr + payloadSize);
                return true;
            }
            dataPtr += payloadSize;
        }
    } else {
        if (parseLengthPrefixedHeader(ptr, nalUnitLength, CodecType::LCEVC)) {
            lcevc.data.insert(lcevc.data.end(), ptr, ptr + nalUnitLength);
            bFourBytePrefix = true;
            return true;
        }
    }
    return false;
}

} // namespace vnova::analyzer
