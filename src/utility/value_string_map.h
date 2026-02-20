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
#ifndef VN_UTILITY_VALUE_STRING_MAP_H_
#define VN_UTILITY_VALUE_STRING_MAP_H_

#include <string>
#include <vector>

namespace vnova::utility {
// Like std::pair<T, std::string>, but the items are called "value" and "name"
// instead of "first" and "second".
template <typename T>
struct ValueStringPair
{
    static_assert(std::is_trivially_copyable_v<T>);
    ~ValueStringPair() = default;
    ValueStringPair(ValueStringPair&& rhs) noexcept = default;
    ValueStringPair& operator=(ValueStringPair&& rhs) noexcept
    {
        if (this != &rhs) {
            value = rhs.value;
            name = std::move(rhs.name);
        }
        return *this;
    }
    ValueStringPair(const ValueStringPair& rhs) = default;
    ValueStringPair& operator=(const ValueStringPair& rhs) = default;
    ValueStringPair(T value, const char* name)
        : value(value)
        , name(name)
    {}
    T value;
    std::string name;
};

// A map from values to strings, implemented as a std::vector. Helper functions
// make it easy to construct, search, and iterate.
template <typename T>
class ValueStringMap
{
    static_assert(std::is_trivially_copyable_v<T>);
    using StringPairVec = std::vector<ValueStringPair<T>>;

public:
    ValueStringMap(T value, const char* name) { m_pairs.push_back(ValueStringPair(value, name)); }

    ValueStringMap& operator()(T value, const char* name)
    {
        m_pairs.push_back(ValueStringPair(value, name));
        return *this;
    }

    bool FindValue(T& res, const std::string& name, T failedReturn) const
    {
        for (typename StringPairVec::const_iterator it = m_pairs.begin(); it != m_pairs.end(); ++it) {
            if (IEquals((*it).name, name)) {
                res = (*it).value;
                return true;
            }
        }

        res = failedReturn;
        return false;
    }

    std::vector<std::string> ListStrings() const
    {
        std::vector<std::string> strings;
        for (const ValueStringPair<T>& entry : m_pairs) {
            strings.push_back(entry.name);
        }
        return strings;
    }

private:
    StringPairVec m_pairs;

    static bool IEquals(const std::string_view a, const std::string_view b)
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
};

} // namespace vnova::utility

#endif // VN_UTILITY_VALUE_STRING_MAP_H_
