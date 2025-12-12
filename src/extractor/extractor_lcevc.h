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
#ifndef VN_EXTRACTOR_EXTRACTOR_LCEVC_H_
#define VN_EXTRACTOR_EXTRACTOR_LCEVC_H_

#include "extractor.h"
#include "io/file_io.h"

#include <utility>

namespace vnova::analyzer {

class ExtractorLCEVC : public Extractor
{
public:
    explicit ExtractorLCEVC(const std::string& url);
    ~ExtractorLCEVC() override = default;

    bool next(std::vector<LCEVC>& lcevcFrames, FrameQueue& frameBuffer) override;

private:
    void indexAnnexB();

    std::vector<uint8_t> m_buffer;
    std::vector<size_t> m_starts;
    size_t m_index = 0;
    int64_t m_counter = 0;
};

} // namespace vnova::analyzer

#endif // VN_EXTRACTOR_EXTRACTOR_LCEVC_H_
