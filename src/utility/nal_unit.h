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
#ifndef VN_UTILITY_NAL_UNIT_H_
#define VN_UTILITY_NAL_UNIT_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vnova::utility {
namespace lcevc {
    static const uint32_t kHeaderSize = 5;
    static const uint32_t kPrefixHeaderSize = 6;

    struct NalUnitType
    {
        enum class Enum : int8_t
        {
            /* 0 - 27: Unspecified */
            NonIDR = 28,
            IDR = 29,
            /* 30: Reserved Level */
            /* 31: Unspecified */
            Invalid = -1,
        };

        static Enum FromValue(uint8_t value);
        static const char* ToString(Enum value);
    };

    // Reads the LCEVC NAL header, returning the type of NAL defined in the header.
    // If any part of the header is malformed returns LCEVCNalUnitType::Invalid.
    NalUnitType::Enum readNalHeader(const uint8_t* src, size_t length, uint32_t headerSize);
} // namespace lcevc

// Unencapsulates a payload, replacing any sequence of [0, 0, 3, 0/1/2/3] with
// [0, 0, 0/1/2/3]. dst may be nullptr to determine the length of the
// unencapsulated data. Returns number of bytes written
uint32_t nalUnencapsulate(uint8_t* dst, const uint8_t* src, size_t length);

} // namespace vnova::utility

#endif // VN_UTILITY_NAL_UNIT_H_
