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

#ifndef VN_HELPER_OUTPUT_UTIL_H_
#define VN_HELPER_OUTPUT_UTIL_H_

#include "utility/format_attribute.h"

#include <cstdarg>
#include <fstream>

#define VN_STR_0 "* %-15s | "
#define VN_STR_1 "\t%-20s | "
#define VN_STR_2 "\t\t%-25s | "
#define VN_STR_3 "\t\t\t%-25s | "

namespace vnova::analyzer::output {

VNAttributeFormat(printf, 3, 4) void writeToStdAndFile(FILE* stdStream, std::ofstream* logFile,
                                                       const char* fmt, ...);

// To handle already-captured vararg list.
VNAttributeFormat(printf, 3, 0) void writeToStdAndFile(FILE* stdStream, std::ofstream* logFile,
                                                       const char* fmt, va_list args);

} // namespace vnova::analyzer::output

#endif // VN_HELPER_OUTPUT_UTIL_H_
