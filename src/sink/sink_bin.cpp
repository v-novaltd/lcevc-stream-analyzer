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
#include "sink_bin.h"

#include "config.h"
#include "extractor/extractor.h"
#include "extractor/extractor_demuxer.h"
#include "utility/nal_header.h"

#include <array>

using namespace vnova::utility;

namespace vnova::analyzer {
static constexpr std::array<uint8_t, 4> kAnnexBStartCode{0x00, 0x00, 0x00, 0x01};

SinkBin::SinkBin(const Config& config)
    : Sink(config)
{}

bool SinkBin::initialise()
{
    const auto& outputPath = config.outputPath;
    if (!writer.initialise(outputPath)) {
        VNLog::Error("Failed to initialise bin writer: %s\n", outputPath.c_str());
        return false;
    }

    return true;
}

void SinkBin::release() { writer.release(); }

void SinkBin::ConvertToAnnexB(std::vector<uint8_t>& data)
{
    uint32_t startCodeSize = 0;
    if (matchAnnexBStartCode(data.data(), data.size(), startCodeSize)) {
        // Already Annex B; leave untouched
        return;
    }

    // Otherwise, assume length-prefixed LCEVC; replace 4-byte length with Annex B prefix
    if (data.size() >= 4) {
        data.erase(data.begin(), data.begin() + 4);
        data.insert(data.begin(), kAnnexBStartCode.begin(), kAnnexBStartCode.end());
    }
}

bool SinkBin::write(const LCEVC& lcevc)
{
    if (!writer.isValid()) {
        return false;
    }

    std::vector<uint8_t> data = lcevc.data;
    ConvertToAnnexB(data);

    LCEVCBinLCEVCPayload payload{};
    payload.presentationIndex = lcevc.pts;
    payload.decodeIndex = lcevc.dts;
    payload.data = data.data();
    payload.dataSize = static_cast<uint32_t>(data.size());

    return writer.writeLCEVCPayload(payload);
}

} // namespace vnova::analyzer
