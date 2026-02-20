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
#ifndef VN_UTILITY_BASE_TYPE_H_
#define VN_UTILITY_BASE_TYPE_H_

#include <cstdint>
#include <string_view>

namespace vnova::helper {

enum class BaseType : int8_t
{
    INVALID = -2,
    UNKNOWN = -1,
    H264 = 1,
    HEVC = 2,
    VVC = 3,
    VP8 = 4,
    VP9 = 5,
    AV1 = 6,
    VC6 = 7,
};

constexpr BaseType baseTypeFromString(std::string_view str)
{
    if (str == "h264" || str == "H264") {
        return BaseType::H264;
    }
    if (str == "hevc" || str == "HEVC") {
        return BaseType::HEVC;
    }
    if (str == "vvc" || str == "VVC") {
        return BaseType::VVC;
    }
    if (str == "vp8" || str == "VP8") {
        return BaseType::VP8;
    }
    if (str == "vp9" || str == "VP9") {
        return BaseType::VP9;
    }
    if (str == "av1" || str == "AV1") {
        return BaseType::AV1;
    }
    if (str == "vc6" || str == "VC6") {
        return BaseType::VC6;
    }
    if (str == "unknown") {
        return BaseType::UNKNOWN;
    }
    return BaseType::INVALID;
}

constexpr const char* toString(const BaseType type)
{
    switch (type) {
        case BaseType::H264: return "H264";
        case BaseType::HEVC: return "HEVC";
        case BaseType::VVC: return "VVC";
        case BaseType::VP8: return "VP8";
        case BaseType::VP9: return "VP9";
        case BaseType::AV1: return "AV1";
        case BaseType::VC6: return "VC6";
        case BaseType::UNKNOWN: return "UNKNOWN";
        case BaseType::INVALID: return "INVALID";
    }

    return "INVALID";
}

} // namespace vnova::helper

#endif // VN_UTILITY_BASE_TYPE_H_
