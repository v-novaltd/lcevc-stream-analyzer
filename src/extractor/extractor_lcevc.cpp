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
#include "extractor_lcevc.h"

#include "utility/log_util.h"

#include <cstddef>

using namespace vnova::utility;

namespace vnova::analyzer {

static constexpr bool isStartCode4(const uint8_t* array) noexcept
{
    return array[0] == 0x00 && array[1] == 0x00 && array[2] == 0x00 && array[3] == 0x01;
}
static constexpr bool isStartCode3(const uint8_t* array) noexcept
{
    return array[0] == 0x00 && array[1] == 0x00 && array[2] == 0x01;
}

ExtractorLCEVC::ExtractorLCEVC(const std::string& url)
{
    // Raw stream of LCEVC NAL units
    bRawStream = true;

    // Read entire file into memory and index NAL start codes
    io::FileIORead file(url);
    if (!file.isValid()) {
        VNLog::Error("Failed to open LCEVC input: %s\n", url.c_str());
        return;
    }

    const auto sz = file.size();
    m_buffer.resize(sz);

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    if (file.read(reinterpret_cast<std::byte*>(m_buffer.data()), sz) != sz) {
        VNLog::Error("Failed to read LCEVC input\n");
        return;
    }

    indexAnnexB();

    // Normalize to 4-byte prefix for consistent parsing
    bFourBytePrefix = true;
    bInitialized = true;
}

void ExtractorLCEVC::indexAnnexB()
{
    const size_t size = m_buffer.size();
    size_t index = 0;
    while (index + 3 < size) {
        const uint8_t* array = &m_buffer[index];
        if (index + 4 <= size && isStartCode4(array)) {
            m_starts.push_back(index);
            index += 4;
            continue;
        }
        if (isStartCode3(array)) {
            m_starts.push_back(index);
            index += 3;
            continue;
        }
        ++index;
    }
}

bool ExtractorLCEVC::next(std::vector<LCEVC>& lcevcFrames, FrameQueue& /*frameBuffer*/)
{
    if (m_index >= m_starts.size()) {
        return false;
    }

    const auto start = m_starts[m_index];
    const size_t end = (m_index + 1 < m_starts.size()) ? m_starts[m_index + 1] : m_buffer.size();
    if (end <= start) {
        ++m_index;
        return (m_index < m_starts.size());
    }

    LCEVC out;
    // Determine actual start-code length at this NAL
    size_t codeLen = 0;
    if (start + 4 <= m_buffer.size() && isStartCode4(&m_buffer[start])) {
        codeLen = 4;
    } else if (start + 3 <= m_buffer.size() && isStartCode3(&m_buffer[start])) {
        codeLen = 3;
    }

    if (codeLen == 3) {
        // Normalize to 4-byte start code by inserting an extra 0x00
        out.data.reserve(1 + (end - start));
        out.data.emplace_back(static_cast<uint8_t>(0x00));
        out.data.insert(out.data.end(), m_buffer.begin() + static_cast<int64_t>(start),
                        m_buffer.begin() + static_cast<int64_t>(end));
    } else {
        // Already 4-byte or unrecognized pattern, copy as is
        out.data.assign(m_buffer.begin() + static_cast<int64_t>(start),
                        m_buffer.begin() + static_cast<int64_t>(end));
    }

    totalPacketSize += out.data.size();

    out.pts = m_counter;
    out.dts = m_counter;
    // Ensure the reorder buffer dequeues immediately for raw streams
    out.maxReorderFrames = 1;

    lcevcFrames.push_back(std::move(out));
    ++m_counter;
    ++m_index;
    return true;
}

} // namespace vnova::analyzer
