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
#ifndef VN_UTILITY_ENUM_MAP_H_
#define VN_UTILITY_ENUM_MAP_H_

#include "value_string_map.h"

#include <map>

namespace vnova::utility {
template <typename E>
class EnumStringPair : public ValueStringPair<E>
{
    static_assert(std::is_enum_v<E>);
    using BaseClass = ValueStringPair<E>;

public:
    EnumStringPair(E value, const char* name)
        : BaseClass(value, name)
    {}
};

template <typename E>
class EnumMap : public ValueStringMap<E>
{
    static_assert(std::is_enum_v<E>);
    using BaseClass = ValueStringMap<E>;

public:
    EnumMap(E value, const char* name)
        : BaseClass(value, name)
    {}
    EnumMap& operator()(E value, const char* name)
    {
        BaseClass::operator()(value, name);
        return *this;
    }

    bool FindEnum(E& res, const std::string& name, E failedReturn) const
    {
        return BaseClass::FindValue(res, name, failedReturn);
    }

    std::map<std::string, E, std::less<>> NameToValueMap(const E fallback) const
    {
        E value;
        std::map<std::string, E, std::less<>> map;
        auto strings = BaseClass::ListStrings();
        for (const auto& string : strings) {
            FindEnum(value, string, fallback);
            map[string] = value;
        }
        return map;
    }
};

} // namespace vnova::utility

#endif // VN_UTILITY_ENUM_MAP_H_
