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
#include "extractor_sei.h"

#include "utility/log_util.h"
#include "utility/nal_header.h"
#include "utility/nal_unit.h"

#include <string>

using namespace vnova::utility;

namespace vnova::analyzer {
static constexpr uint8_t kRegisterUserDataSEIType = 4;
static constexpr uint32_t kLCEVCITUSize = 4;
static constexpr uint8_t kLCEVCITUHeader[kLCEVCITUSize] = {0xb4, 0x00, 0x50, 0x00};
// LCEVC NAL detection is handled by CodecType::LCEVC parsing in nal_header

bool isSEINALUnit(BaseType::Enum baseType, const uint8_t* data, size_t length, uint32_t& headerSize,
                  bool& bError)
{
    bError = false;
    headerSize = 0;

    ParsedNALHeader hdr;
    if (!parseAnnexBHeader(data, length, baseType, hdr)) {
        return false;
    }

    headerSize = hdr.headerSize;
    return isSEI(hdr);
}

bool isLCEVCITUPayload(const uint8_t* data)
{
    return memcmp(data, kLCEVCITUHeader, kLCEVCITUSize) == 0;
}

ExtractorSEI::ExtractorSEI(const std::string& url, InputType type, const Config& config)
    : ExtractorDemuxer(url, type, config)
{}

bool ExtractorSEI::processNALUnit(const utility::DataBuffer& nalUnit, LCEVC& lcevc,
                                  utility::BaseType::Enum& baseType)
{
    const uint8_t* nalUnitData = nalUnit.data();
    size_t nalUnitLength = nalUnit.size();
    uint32_t headerLength = 0;

    if (isSEINALUnit(baseType, nalUnitData, nalUnitLength, headerLength, bError)) {
        // Ignore header
        nalUnitData += headerLength;
        nalUnitLength -= headerLength;

        // Unencapsulate the data into a destination buffer. The destination
        // is required to be as big as encapsulated data, unencapsulation will
        // never expand the number of bytes only contract.
        utility::DataBuffer data(nalUnitLength, 0);

        const uint32_t dataLength = nalUnencapsulate(data.data(), nalUnitData, nalUnitLength);
        VNAssert(dataLength <= nalUnitLength);

        const uint8_t* dataPtr = data.data();
        const uint8_t* dataEnd = dataPtr + dataLength;

        // There can be multiple payloads within a single NAL unit so process them
        // all.
        while (dataPtr < dataEnd) {
            const uint8_t payloadType = *dataPtr++;
            VNAssert(dataPtr < dataEnd);

            size_t payloadSize = 0;

            while (*dataPtr == 0xFF) {
                payloadSize += 255;
                dataPtr++;
            }
            payloadSize += *dataPtr++;

            if ((payloadType == kRegisterUserDataSEIType) && (payloadSize >= kLCEVCITUSize) &&
                isLCEVCITUPayload(dataPtr)) {
                // Found LCEVC data, now store it without the ITU T.35 header.
                lcevc.data.assign(dataPtr + kLCEVCITUSize, dataPtr + payloadSize);

                // Only 1 LCEVC payload per SEI NAL unit.
                return true;
            }

            dataPtr += payloadSize;
        }
    } else {
        if (parseAnnexBHeader(nalUnit, CodecType::LCEVC)) {
            lcevc.data.assign(nalUnitData, nalUnitData + nalUnitLength);
            return true;
        }
    }
    return false;
}

} // namespace vnova::analyzer
