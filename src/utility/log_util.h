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

#ifndef VN_UTILITY_LOG_UTIL_H_
#define VN_UTILITY_LOG_UTIL_H_

#include <cstdio>
#include <string>
#include <utility>

namespace vnova::utility {

template <typename... Args>
void log(const char* level, const bool flush, const char* file, const int line, const char* fmt,
         Args&&... args)
{
    std::string prepended = "[lcevc_stream_analyzer] ";
    prepended += file;
    prepended += ":";
    prepended += std::to_string(line);
    prepended += " ";
    prepended += level;
    prepended += " ";
    prepended += fmt;
    prepended += "\n";

    if constexpr (sizeof...(args) == 0) {
        std::fprintf(stderr, "%s", prepended.c_str());
    } else {
        std::fprintf(stderr, prepended.c_str(), std::forward<Args>(args)...);
    }
    if (flush) {
        std::fflush(stderr);
    }
}

} // namespace vnova::utility

// NOLINTBEGIN(readability-identifier-naming)
#define VNLOG_DEBUG(fmt, ...) \
    vnova::utility::log("DEBUG", false, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VNLOG_INFO(fmt, ...) \
    vnova::utility::log(" INFO", false, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VNLOG_WARN(fmt, ...) \
    vnova::utility::log(" WARN", false, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define VNLOG_ERROR(fmt, ...) \
    vnova::utility::log("ERROR", true, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
// NOLINTEND(readability-identifier-naming)

#endif // VN_UTILITY_LOG_UTIL_H_
