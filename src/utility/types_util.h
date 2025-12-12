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
#ifndef VN_UTILITY_TYPES_UTIL_H_
#define VN_UTILITY_TYPES_UTIL_H_

#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS

#include <cinttypes>
#include <cstdint>
#include <vector>

#if defined(_MSC_VER) && _MSC_VER <= 1700
// Manual fallback for old MSVC (VS2012 or earlier)
#ifndef PRId32
#define PRId32 "d"
#endif

#ifndef PRIu32
#define PRIu32 "u"
#endif

#ifndef PRId64
#define PRId64 "ld"
#endif

#ifndef PRIx64
#define PRIx64 "llx"
#endif

#ifndef PRIu64
#define PRIu64 "llu"
#endif
#endif

namespace vnova::utility {
template <typename T>
class Callback
{
public:
    T callback = nullptr;
    void* data = nullptr;
};

using DataBuffer = std::vector<uint8_t>;

#if __cplusplus >= 201703L
template <typename...>
inline constexpr auto AlwaysFalse = false;

template <auto...>
inline constexpr auto AlwaysFalseValue = false;
#endif
} // namespace vnova::utility

#endif // VN_UTILITY_TYPES_UTIL_H_
