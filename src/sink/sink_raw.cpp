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
#include "sink_raw.h"

#include "app/config.h"
#include "utility/file_io.h"
#include "utility/log_util.h"

namespace vnova::analyzer {
SinkRaw::SinkRaw(const Config& config, bool lengthDelimited)
    : Sink(config)
    , m_lengthDelimited(lengthDelimited)
{}

bool SinkRaw::initialise()
{
    const auto& binOutputPath = config.extractOutputPath;

    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    m_file = std::make_unique<utility::io::FileIOWrite>(binOutputPath);
    return (m_file != nullptr);
}

void SinkRaw::release() {}

bool SinkRaw::write(const helper::LCEVCFrame& lcevc)
{
    if (!m_file) {
        return false;
    }

    const auto size = static_cast<uint32_t>(lcevc.data.size());

    VNLOG_INFO("Writing raw formatted LCEVC frame %zu with size %zu bytes to file", m_index, size);
    m_index++;

    if (m_lengthDelimited) { // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        if (!m_file->write(reinterpret_cast<const std::byte*>(&size), sizeof(uint32_t))) {
            return false;
        }
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    if (!m_file->write(reinterpret_cast<const std::byte*>(lcevc.data.data()), size)) {
        return false;
    }

    return true;
}

} // namespace vnova::analyzer
