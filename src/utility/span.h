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
#ifndef VN_UTILITY_SPAN_H_
#define VN_UTILITY_SPAN_H_

#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace vnova::utility {

class Span
{
public:
    Span(const uint8_t* data, size_t size)
        : m_top(data)
        , m_capacity(size)
        , m_head(m_top)
        , m_remaining(m_capacity)
    {}

    const uint8_t* Head() const { return m_head; }
    size_t Remaining() const { return m_remaining; }
    size_t Offset() const { return m_head - m_top; }

    const uint8_t* Top() const { return m_top; }
    size_t Capacity() const { return m_capacity; }

    void Forward(const size_t offset)
    {
        if (static_cast<int64_t>(m_remaining) - static_cast<int64_t>(offset) < 0) {
            throw std::length_error("Out of range");
        }
        m_head += offset;
        m_remaining -= offset;
    }

    void Backward(const size_t offset)
    {
        if (static_cast<int64_t>(m_remaining) + static_cast<int64_t>(offset) >
            static_cast<int64_t>(m_capacity)) {
            throw std::length_error("Out of range");
        }

        m_head -= offset;
        m_remaining += offset;
    }

    void Rewind()
    {
        m_head = m_top;
        m_remaining = m_capacity;
    }

private:
    const uint8_t* m_top;
    size_t m_capacity;

    const uint8_t* m_head;
    size_t m_remaining;
};

} // namespace vnova::utility

#endif
