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
#ifndef VN_EXTRACTOR_EXTRACTOR_BIN_H_
#define VN_EXTRACTOR_EXTRACTOR_BIN_H_

#include "extractor.h"
#include "helper/extracted_frame.h"
#include "helper/lcevc_bin.h"

namespace vnova::analyzer {
class ExtractorBin : public Extractor
{
public:
    ExtractorBin(const std::filesystem::path& url, helper::InputType type);
    ~ExtractorBin();

    bool next(std::vector<helper::LCEVCFrame>& lcevcFrames, helper::BaseFrameQueue& frameQueue) override;

private:
    helper::bin::LCEVCBinReader m_reader;
    helper::bin::LCEVCBinBlock m_block;
    helper::bin::LCEVCBinLCEVCPayload m_payload;
};

} // namespace vnova::analyzer

#endif // VN_EXTRACTOR_EXTRACTOR_BIN_H_
