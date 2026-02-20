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
#ifndef VN_HELPER_REORDER_H_
#define VN_HELPER_REORDER_H_

#include "helper/extracted_frame.h"

#include <queue>

namespace vnova::helper {
class Reorder
{
    struct PTSMinOrder
    {
        bool operator()(const LCEVCFrame& l, const LCEVCFrame& r) const { return l.pts > r.pts; }
    };

public:
    void enqueue(const LCEVCFrame& lcevc);
    bool dequeue(LCEVCWithBase& lcevcWithBase, BaseFrameQueue& frameQueue);

    void setEndOfStream(bool endOfStream) noexcept { m_endOfStream = endOfStream; }
    void setRawstream(bool rawStream) noexcept { m_isRawStream = rawStream; }
    bool getRawstream() noexcept { return m_isRawStream; }
    void setBaseStream(bool isBaseStream) noexcept { m_isBaseStream = isBaseStream; }
    bool getBasestream() noexcept { return m_isBaseStream; }

private:
    std::priority_queue<LCEVCFrame, std::vector<LCEVCFrame>, PTSMinOrder> m_data;
    int64_t m_lastPTS = 0;
    bool m_hasOutput = false;
    bool m_endOfStream = false;
    uint8_t m_requiredQueueSize = 0;
    bool m_isRawStream = false;
    bool m_isBaseStream = false;
};

} // namespace vnova::helper

#endif // VN_HELPER_REORDER_H_
