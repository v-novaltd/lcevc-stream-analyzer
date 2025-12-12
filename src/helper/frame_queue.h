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
#ifndef VN_HELPER_FRAME_QUEUE_H_
#define VN_HELPER_FRAME_QUEUE_H_

#include <cstdint>
#include <limits>
#include <queue>

namespace vnova::analyzer {
enum class FrameType
{
    Unknown,
    I,
    P,
    B,
    KeyFrame,
    InterFrame
};

struct FrameInfo
{
    int64_t pts = (std::numeric_limits<int64_t>::max)();
    int64_t frameNum = -1;
    FrameType frameType = FrameType::Unknown;
    uint64_t frameSize = (std::numeric_limits<uint64_t>::max)();
};
class FrameQueue
{
public:
    void enqueue(const FrameInfo& info) { data.push(info); }

    std::queue<FrameInfo> data;
};

} // namespace vnova::analyzer

#endif // VN_HELPER_FRAME_QUEUE_H_
