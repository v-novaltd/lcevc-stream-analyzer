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

#include "extractor_bin.h"

#include "helper/nal_unit.h"
#include "utility/log_util.h"

#include <cstring>

using namespace vnova::utility;
using namespace vnova::helper;

namespace vnova::analyzer {

ExtractorBin::ExtractorBin(const std::filesystem::path& url, InputType type)
    : m_payload()
{
    if (type == InputType::BIN) {
        m_rawStream = true;
    }

    if (!m_reader.initialise(url)) {
        VNLOG_ERROR("Failed to initialise bin reader: %s", url.c_str());
        return;
    }
    m_initialized = true;
}

ExtractorBin::~ExtractorBin() { m_reader.release(); }

bool ExtractorBin::next(std::vector<LCEVCFrame>& lcevcFrames, BaseFrameQueue& /* frameQueue */)
{
    if (!m_reader.isValid()) {
        return false;
    }

    helper::bin::LCEVCBinReadResult result = m_reader.readBlock(m_block);
    if (result == helper::bin::LCEVCBinReadResult::NoMore) {
        return false;
    }
    if (result != helper::bin::LCEVCBinReadResult::Success) {
        VNLOG_ERROR("Failed to read LCEVC BIN block");
        return false;
    }
    if (helper::bin::LCEVCBinReader::parseBlockAsLCEVCPayload(m_block, m_payload)) {
        uint8_t startCodeSize = 0;
        if (helper::matchAnnexBStartCode(m_payload.data, m_payload.dataSize, startCodeSize) == false) {
            VNLOG_ERROR("Neither a 3 or 4 byte AnnexB start code");
            return false;
        };
        m_fourBytePrefix = (startCodeSize == 4);

        LCEVCFrame lcevc;
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
