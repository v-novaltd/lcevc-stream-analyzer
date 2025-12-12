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

#ifndef VN_UTILITY_LOG_UTIL_H_
#define VN_UTILITY_LOG_UTIL_H_

#include <cstddef>
#include <cstdio>
#include <string_view>
#include <utility>

namespace vnova {
struct VNLog
{
    template <std::size_t N, typename... Args>
    static void Debug(const char (&fmt)[N], Args&&... args)
    {
        if constexpr (sizeof...(args) == 0) {
            std::fwrite(fmt, 1, N - 1, stderr);
        } else {
            std::fprintf(stderr, fmt, std::forward<Args>(args)...);
        }
    }

    static void Debug(std::string_view msg) { std::fwrite(msg.data(), 1, msg.size(), stderr); }

    template <std::size_t N, typename... Args>
    static void Info(const char (&fmt)[N], Args&&... args)
    {
        if constexpr (sizeof...(args) == 0) {
            std::fwrite(fmt, 1, N - 1, stderr);
        } else {
            std::fprintf(stderr, fmt, std::forward<Args>(args)...);
        }
    }

    static void Info(std::string_view msg) { std::fwrite(msg.data(), 1, msg.size(), stderr); }

    template <std::size_t N, typename... Args>
    static void Error(const char (&fmt)[N], Args&&... args)
    {
        if constexpr (sizeof...(args) == 0) {
            std::fwrite(fmt, 1, N - 1, stderr);
        } else {
            std::fprintf(stderr, fmt, std::forward<Args>(args)...);
        }

        std::fflush(stderr);
    }

    static void Error(std::string_view msg)
    {
        std::fwrite(msg.data(), 1, msg.size(), stderr);
        std::fflush(stderr);
    }
};

} // namespace vnova

#endif // VN_UTILITY_LOG_UTIL_H_
