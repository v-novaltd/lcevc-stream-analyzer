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

#include "extractor_bin.h"

#include "config.h"
#include "extractor/extractor.h"

#include <array>

using namespace vnova::utility;

namespace vnova::analyzer {
static constexpr std::array<uint8_t, 4> kAnnexBStartCode = {0x00, 0x00, 0x00, 0x01};

ExtractorBin::ExtractorBin(const std::string& url, InputType type)
    : m_payload()
{
    if (type == InputType::BIN) {
        bRawStream = true;
    }

    if (!m_reader.initialise(url)) {
        VNLog::Error("Failed to initialise bin reader: %s\n", url.c_str());
        return;
    }
    bInitialized = true;
}

ExtractorBin::~ExtractorBin() { m_reader.release(); }

bool ExtractorBin::next(std::vector<LCEVC>& lcevcFrames, FrameQueue& /* frameBuffer */)
{
    if (!m_reader.isValid()) {
        return false;
    }

    LCEVCBinReadResult result = m_reader.readBlock(m_block);
    if (result == LCEVCBinReadResult::NoMore) {
        return false;
    }
    if (result != LCEVCBinReadResult::Success) {
        VNLog::Error("Failed to read LCEVC BIN block\n");
        return false;
    }
    if (utility::LCEVCBinReader::parseBlockAsLCEVCPayload(m_block, m_payload)) {
        if (std::memcmp(m_payload.data, kAnnexBStartCode.data(), kAnnexBStartCode.size()) == 0) {
            bFourBytePrefix = true;
        }

        LCEVC lcevc;
        lcevc.data.assign(m_payload.data, m_payload.data + m_payload.dataSize);
        lcevc.pts = m_payload.presentationIndex;
        lcevc.dts = m_payload.decodeIndex;
        lcevc.maxReorderFrames = 32;
        lcevcFrames.push_back(lcevc);
        return true;
    }
    return false;
}

} // namespace vnova::analyzer
