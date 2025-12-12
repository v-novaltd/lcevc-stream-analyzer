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

#include "base_type.h"

#include "string_util.h"

namespace vnova::utility {
BaseType::Enum BaseType::fromString(std::string_view str)
{
    if (str == "h264" || str == "H264") {
        return Enum::H264;
    }
    if (str == "hevc" || str == "HEVC") {
        return Enum::HEVC;
    }
    if (str == "vvc" || str == "VVC") {
        return Enum::VVC;
    }
    if (str == "vp8" || str == "VP8") {
        return Enum::VP8;
    }
    if (str == "vp9" || str == "VP9") {
        return Enum::VP9;
    }
    if (str == "av1" || str == "AV1") {
        return Enum::AV1;
    }
    if (str == "vc6" || str == "VC6") {
        return Enum::VC6;
    }
    if (str == "unknown") {
        return Enum::Unknown;
    }
    return BaseType::Enum::Invalid;
}

const char* BaseType::ToString(Enum type)
{
    switch (type) {
        case Enum::H264: return "H264";
        case Enum::HEVC: return "HEVC";
        case Enum::VVC: return "VVC";
        case Enum::VP8: return "VP8";
        case Enum::VP9: return "VP9";
        case Enum::AV1: return "AV1";
        case Enum::VC6: return "VC6";
        case Enum::Unknown: return "unknown";
        case Enum::Invalid: return "invalid";
    }

    return "Unrecognized value";
}

} // namespace vnova::utility
