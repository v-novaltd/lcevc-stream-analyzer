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
#include "sink_bin.h"

#include "app/config.h"
#include "helper/nal_unit.h"
#include "utility/log_util.h"

#include <array>

using namespace vnova::utility;
using namespace vnova::helper;

namespace vnova::analyzer {

SinkBin::SinkBin(const Config& config)
    : Sink(config)
{}

bool SinkBin::initialise()
{
    if (const auto& binOutputPath = config.extractOutputPath; writer.initialise(binOutputPath) == false) {
        VNLOG_ERROR("Failed to initialise bin writer: %s", binOutputPath.c_str());
        return false;
    }

    return true;
}

void SinkBin::release() { writer.release(); }

void SinkBin::ConvertToAnnexB(std::vector<uint8_t>& data)
{
    uint8_t startCodeSize = 0;
    if (helper::matchAnnexBStartCode(data.data(), data.size(), startCodeSize)) {
        // Already Annex B; leave untouched
        return;
    }

    // Otherwise, assume length-prefixed LCEVC; replace 4-byte length with Annex B prefix
    if (data.size() >= 4) {
        for (size_t idx = 0; idx < 4; idx++) {
            data.at(idx) = static_cast<uint8_t>(helper::kFourByteStartCode.at(idx));
        }
    }
}

bool SinkBin::write(const LCEVCFrame& lcevc)
{
    if (!writer.isValid()) {
        return false;
    }

    std::vector<uint8_t> data = lcevc.data;
    ConvertToAnnexB(data);

    VNLOG_INFO("Writing Annex-B formatted LCEVC frame %zu with size %zu bytes to file", m_index,
               data.size());
    m_index++;

    helper::bin::LCEVCBinLCEVCPayload payload{};
    payload.presentationIndex = lcevc.pts;
    payload.decodeIndex = lcevc.dts;
    payload.data = data.data();
    payload.dataSize = static_cast<uint32_t>(data.size());

    return writer.writeLCEVCPayload(payload);
}

} // namespace vnova::analyzer
