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
#ifndef VN_UTILITY_MATH_UTIL_H_
#define VN_UTILITY_MATH_UTIL_H_

#include "utility/platform.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <type_traits>
#include <vector>

namespace vnova::utility::math {

template <typename T, typename = typename std::enable_if_t<std::is_integral_v<T>>>
T ceilDiv(T a, T b)
{
    VNAssert(b != 0);
    if (b != 0) {
        return (a + b - 1) / b;
    }
    return 0;
}

template <typename T, typename = typename std::enable_if_t<std::is_floating_point_v<T>>>
bool approxEqual(T a, T b, T absoluteTol)
{
    VNAssert(absoluteTol > 0);
    return std::abs(a - b) < absoluteTol;
}

template <typename T>
std::vector<T> diff(const std::vector<T>& input)
{
    if (input.size() < 2) {
        return {};
    }

    std::vector<T> delta(input.size() - 1);
    std::transform(std::next(input.begin()), input.end(), input.begin(), delta.begin(),
                   [](T a, T b) { return std::abs(a - b); });

    return delta;
}

template <typename T>
std::map<T, size_t> countOccurrences(const std::vector<T>& input)
{
    std::map<T, size_t> map;
    for (const auto& elem : input) {
        map[elem] += 1;
    }
    return map;
}

template <typename A, typename B>
std::multimap<B, A> invertMap(const std::map<A, B>& map)
{
    std::multimap<B, A> inverted;

    for (const auto& elem : map) {
        inverted.insert(std::pair<B, A>(elem.second, elem.first));
    }

    return inverted;
}

} // namespace vnova::utility::math

#endif // VN_UTILITY_MATH_UTIL_H_
