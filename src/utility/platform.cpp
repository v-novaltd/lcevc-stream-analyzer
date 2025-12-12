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
#include "platform.h"

#include <cstdint>
#include <cstdio>

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace vnova::utility::file {
#if defined(_WIN32) || defined(_WIN64)
uint64_t tellPosition(FILE* file) { return static_cast<uint64_t>(_ftelli64(file)); }

void seekToPosition(FILE* file, long long offset, int32_t origin)
{
    _fseeki64(file, offset, origin);
}
#else
uint64_t tellPosition(FILE* file) { return static_cast<uint64_t>(ftello(file)); }

void seekToPosition(FILE* file, long long offset, int32_t origin) { fseeko(file, offset, origin); }
#endif

uint64_t getFileSize(FILE* file)
{
    uint64_t cur = tellPosition(file);
    seekToPosition(file, 0, SEEK_END);
    uint64_t size = tellPosition(file);
    seekToPosition(file, static_cast<long long>(cur), SEEK_SET);
    return size;
}

} // namespace vnova::utility::file
