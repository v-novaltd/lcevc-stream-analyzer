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
#include "reorder.h"

namespace vnova::analyzer {
void Reorder::enqueue(const LCEVC& lcevc)
{
    m_data.push(lcevc);
    m_requiredQueueSize = lcevc.maxReorderFrames;
}

bool Reorder::dequeue(LCEVC& lcevc, FrameQueue& frameBuffer)
{
    if (m_requiredQueueSize == 0 || (!m_endOfStream && m_data.size() < m_requiredQueueSize) ||
        m_data.empty() || (!getRawstream() && frameBuffer.data.empty() && getBasestream())) {
        return false;
    }

    FrameInfo info;
    if (!getRawstream() && getBasestream()) {
        bool matchFound = false;
        LCEVC lcevcItem = m_data.top();

        while (!frameBuffer.data.empty()) {
            info = frameBuffer.data.front();
            frameBuffer.data.pop();

            if (lcevcItem.pts == info.pts) {
                lcevcItem.frameType = info.frameType;
                lcevcItem.frameSize = static_cast<int64_t>(info.frameSize);

                lcevc = std::move(lcevcItem);

                m_data.pop();
                matchFound = true;
                break;
            }
        }

        if (!matchFound) {
            return false;
        }
    } else {
        lcevc = m_data.top();
        m_data.pop();
    }

    if (m_hasOutput && lcevc.pts <= m_lastPTS) {
        VNLog::Error("ERROR: lcevc frame with PTS:%" PRId64
                     " is out of order. Please check file is not corrupted\n",
                     lcevc.pts);
    }

    m_lastPTS = lcevc.pts;
    m_hasOutput = true;

    return true;
}

} // namespace vnova::analyzer
