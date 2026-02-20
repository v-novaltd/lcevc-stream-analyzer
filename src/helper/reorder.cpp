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
#include "reorder.h"

#include "utility/log_util.h"

#include <cinttypes>
#include <optional>

namespace vnova::helper {
void Reorder::enqueue(const LCEVCFrame& lcevc)
{
    m_data.push(lcevc);
    m_requiredQueueSize = lcevc.maxReorderFrames;
}

bool Reorder::dequeue(LCEVCWithBase& lcevcWithBase, BaseFrameQueue& frameQueue)
{
    if (m_requiredQueueSize == 0 || (!m_endOfStream && m_data.size() < m_requiredQueueSize) ||
        m_data.empty() || (!getRawstream() && frameQueue.empty() && getBasestream())) {
        return false;
    }

    if (getBasestream()) {
        bool matchFound = false;
        LCEVCFrame lcevcItem = m_data.top();

        while (!frameQueue.empty()) {
            BaseFrame info = frameQueue.front();
            frameQueue.pop();

            if (lcevcItem.pts == kInvalidPts) {
                VNLOG_ERROR("Invalid PTS for LCEVC frame");
                return false;
            }
            if (info.pts == kInvalidPts) {
                VNLOG_ERROR("Invalid PTS for base frame");
                return false;
            }

            if (lcevcItem.pts == info.pts) {
                lcevcWithBase.lcevc = std::move(lcevcItem);
                lcevcWithBase.base = info;

                m_data.pop();
                matchFound = true;
                break;
            }

            VNLOG_ERROR("Mismatched pts for head of lcevc queue %d and head of frame queue %d",
                        lcevcItem.pts, info.pts);
            return false;
        }

        if (!matchFound) {
            return false;
        }
    } else {
        lcevcWithBase.lcevc = m_data.top();
        lcevcWithBase.base = std::nullopt;
        m_data.pop();
    }

    if (m_hasOutput && lcevcWithBase.lcevc.pts <= m_lastPTS) {
        VNLOG_ERROR("LCEVC frame with PTS:%" PRId64
                    " is out of order. Please check file is not corrupted",
                    lcevcWithBase.lcevc.pts);
    }

    m_lastPTS = lcevcWithBase.lcevc.pts;
    m_hasOutput = true;

    return true;
}

} // namespace vnova::helper
