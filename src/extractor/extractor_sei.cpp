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
#include "extractor_sei.h"

#include "helper/nal_unit.h"
#include "utility/platform.h"

#include <array>

using namespace vnova::utility;
using namespace vnova::helper;

namespace vnova::analyzer {

ExtractorSEI::ExtractorSEI(const std::filesystem::path& url, InputType type, const Config& config)
    : ExtractorDemuxer(url, type, config)
{}

std::optional<LCEVCFrame> ExtractorSEI::parseNalIsLcevc(const utility::DataBuffer& nalUnit,
                                                        helper::BaseType& baseType)
{
    LCEVCFrame lcevc;
    const uint8_t* nalUnitData = nalUnit.data();
    size_t nalUnitLength = nalUnit.size();

    if (uint32_t headerLength = 0; isSEINALUnit(baseType, nalUnitData, nalUnitLength, headerLength)) {
        // Ignore header
        nalUnitData += headerLength;
        nalUnitLength -= headerLength;

        // Unencapsulate the data into a destination buffer. The destination
        // is required to be as big as encapsulated data, unencapsulation will
        // never expand the number of bytes only contract.
        utility::DataBuffer data(nalUnitLength, 0);

        const uint32_t dataLength = helper::nalUnencapsulate(data.data(), nalUnitData, nalUnitLength);
        VNAssert(dataLength <= nalUnitLength);

        const uint8_t* dataPtr = data.data();
        const uint8_t* dataEnd = dataPtr + dataLength;

        // There can be multiple payloads within a single NAL unit so process them
        // all.
        while (dataPtr < dataEnd) {
            const auto payloadType = std::byte{*dataPtr++};
            VNAssert(dataPtr < dataEnd);

            size_t payloadSize = 0;

            while (*dataPtr == 0xFF) {
                payloadSize += 255;
                dataPtr++;
            }
            payloadSize += *dataPtr++;

            if ((payloadType == helper::kRegisterUserDataSEIType.at(0)) &&
                (payloadSize >= helper::kLCEVCITUHeader.size()) && helper::isLCEVCITUPayload(dataPtr)) {
                const auto nalWireSize = nalUnitLength + headerLength;

                // Found LCEVC data, now store it without the ITU T.35 header.
                lcevc.data.assign(dataPtr + helper::kLCEVCITUHeader.size(), dataPtr + payloadSize);
                lcevc.lcevcWireSize = static_cast<int64_t>(nalWireSize);

                // Only 1 LCEVC payload per SEI NAL unit.
                return lcevc;
            }

            dataPtr += payloadSize;
        }
    } else {
        if (parseAnnexBHeader(nalUnit, helper::CodecType::LCEVC)) {
            lcevc.data.assign(nalUnitData, nalUnitData + nalUnitLength);
            return lcevc;
        }
    }

    return std::nullopt;
}

} // namespace vnova::analyzer
