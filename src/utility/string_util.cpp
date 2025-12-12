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
#include "string_util.h"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace vnova::utility::string {
bool iEquals(const std::string_view a, const std::string_view b)
{
    if (a.length() != b.length()) {
        return false;
    }

    auto aIt = std::begin(a); // NOLINT(readability-qualified-auto) - Does not work on Windows
    auto bIt = std::begin(b); // NOLINT(readability-qualified-auto) - Does not work on Windows

    for (; aIt != std::end(a) && bIt != std::end(b); ++aIt, ++bIt) {
        if (std::toupper(*aIt) != std::toupper(*bIt)) {
            return false;
        }
    }

    return true;
}

std::string pathExtension(std::string_view path)
{
    size_t part = path.rfind('.');
    std::string res;

    if (part != std::string::npos) {
        res = path.substr(part + 1);
    }

    return res;
}

} // namespace vnova::utility::string
