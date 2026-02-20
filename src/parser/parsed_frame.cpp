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

#include "parser/parsed_frame.h"

#include "utility/log_util.h"

#include <cinttypes>

namespace vnova::analyzer {

std::optional<std::vector<int64_t>> ParsedFrameList::GetPts() const
{
    std::vector<int64_t> pts;
    pts.reserve(m_frames.size());

    for (const auto& frame : m_frames) {
        if (frame.base.has_value() == false) {
            pts.push_back(frame.lcevc.pts);

        } else {
            const int64_t basePts = frame.base.value().pts;
            if (basePts != frame.lcevc.pts) {
                VNLOG_ERROR("Frame %zu PTS mismatch: base=%" PRId64 ", lcevc=%" PRId64 "",
                            frame.index, basePts, frame.lcevc.pts);
                return std::nullopt;
            }

            pts.push_back(basePts);
        }
    }

    return pts;
}

std::vector<int64_t> ParsedFrameList::GetLcevcBytes() const
{
    std::vector<int64_t> sizes;
    sizes.reserve(m_frames.size());
    for (const auto& frame : m_frames) {
        sizes.push_back(frame.lcevcWireSize);
    }
    return sizes;
}

std::vector<int64_t> ParsedFrameList::GetBaseBytes() const
{
    std::vector<int64_t> sizes;
    sizes.reserve(m_frames.size());
    for (const auto& frame : m_frames) {
        sizes.push_back(frame.baseWireSize);
    }
    return sizes;
}

std::vector<int64_t> ParsedFrameList::GetCombinedBytes() const
{
    std::vector<int64_t> sizes;
    sizes.reserve(m_frames.size());
    for (const auto& frame : m_frames) {
        sizes.push_back(frame.combinedWireSize);
    }
    return sizes;
}

std::optional<bool> ParsedFrameList::HasBase() const
{
    if (m_frames.empty()) {
        return false;
    }

    const bool first = m_frames.front().base.has_value();
    for (const auto& frame : m_frames) {
        if (frame.base.has_value() != first) {
            VNLOG_ERROR("Only some frames have base info, it should be all or none");
            return std::nullopt;
        }
    }
    return first;
}

} // namespace vnova::analyzer
