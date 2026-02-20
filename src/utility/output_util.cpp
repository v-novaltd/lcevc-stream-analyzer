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

#include "utility/output_util.h"

#include <cstdarg>
#include <cstdio>
#include <string>

namespace vnova::analyzer::output {

VNAttributeFormat(printf, 3, 4) void writeToStdAndFile(FILE* stdStream, std::ofstream* logFile,
                                                       const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    writeToStdAndFile(stdStream, logFile, fmt, args);
    va_end(args);
}

VNAttributeFormat(printf, 3, 0) void writeToStdAndFile(FILE* stdStream, std::ofstream* logFile,
                                                       const char* fmt, va_list args)
{
    va_list argsLength;
    va_copy(argsLength, args);
    int length = std::vsnprintf(nullptr, 0, fmt, argsLength);
    va_end(argsLength);
    if (length < 0) {
        return;
    }
    length += 1; // include NUL terminator

    std::string buffer;
    buffer.resize(static_cast<size_t>(length), '\0');

    va_list argsFormat;
    va_copy(argsFormat, args);
    const int written = std::vsnprintf(buffer.data(), buffer.size(), fmt, argsFormat);
    va_end(argsFormat);
    if (written <= 0) {
        return;
    }

    if (stdStream != nullptr) {
        (void)std::fwrite(buffer.data(), 1, static_cast<size_t>(written), stdStream);
    }

    if ((logFile != nullptr) && logFile->is_open()) {
        logFile->write(buffer.data(), written);
    }
}

} // namespace vnova::analyzer::output
