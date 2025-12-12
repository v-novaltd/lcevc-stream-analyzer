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
#ifndef VN_UTILITY_FORMAT_ATTRIBUTE_H_
#define VN_UTILITY_FORMAT_ATTRIBUTE_H_

#if defined(_WIN32) || defined(_WIN64)

// for _Printf_format_string_
#include <sal.h>

#define VNAttributeFormat(archetype, stringIndex, firstToCheck)

// On Windows, disable custom format checking macros(handled by SAL)
#define VNPrintfFormatString _Printf_format_string_

#else

// For GCC/Clang: enable compile-time format checking via __attribute__
#define VNAttributeFormat(archetype, stringIndex, firstToCheck) \
    __attribute__((format(archetype, (stringIndex), (firstToCheck))))

#define VNPrintfFormatString

#endif

#endif // VN_UTILITY_FORMAT_ATTRIBUTE_H_
