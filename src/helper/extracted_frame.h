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
#ifndef VN_HELPER_FRAME_QUEUE_H_
#define VN_HELPER_FRAME_QUEUE_H_

#include "utility/types_util.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <queue>

namespace vnova::helper {

enum class FrameType : uint8_t
{
    Unknown,
    I,
    P,
    B,
    KeyFrame,
    InterFrame,
};

constexpr const char* toString(FrameType type)
{
    switch (type) {
        case FrameType::I: return "I";
        case FrameType::P: return "P";
        case FrameType::B: return "B";
        case FrameType::KeyFrame: return "KEY";
        case FrameType::InterFrame: return "INT";
        default: return " ? ";
    }
}

struct BaseFrame
{
    int64_t decodeIndex;

    int64_t dts;
    int64_t pts;
    int64_t index;
    FrameType type;
    int32_t width;
    int32_t height;
};
static constexpr BaseFrame DEFAULT_FRAME_INFO = {-1, -1, -1, -1, FrameType::Unknown, -1, -1};

using BaseFrameQueue = std::queue<BaseFrame>;

constexpr int64_t kInvalidPts = std::numeric_limits<int64_t>::min();
constexpr int64_t kInvalidDts = std::numeric_limits<int64_t>::min();

struct LCEVCFrame
{
    uint8_t maxReorderFrames = 0;

    utility::DataBuffer data;
    int64_t pts = kInvalidPts;
    int64_t dts = kInvalidDts;

    int64_t lcevcWireSize = 0;
    int64_t remainingWireSize = 0;
    int64_t totalWireSize = 0;
};

struct LCEVCWithBase
{
    LCEVCFrame lcevc;
    std::optional<BaseFrame> base;
};
} // namespace vnova::helper

#endif // VN_HELPER_FRAME_QUEUE_H_
