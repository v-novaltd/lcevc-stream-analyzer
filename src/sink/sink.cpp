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
#include "sink.h"

#include "app/config.h"
#include "sink_bin.h"
#include "sink_null.h"
#include "sink_raw.h"
#include "utility/log_util.h"

namespace vnova::analyzer {
Sink::Sink(const Config& config)
    : config(config)
{}

std::unique_ptr<Sink> Sink::factory(const Config& config)
{
    // Always null sink if EXTRACT command not provided.
    const auto outputType = (config.subcommand.at(Subcommand::EXTRACT) == true)
                                ? config.extractOutputType
                                : ExtractOutputType::Unknown;
    std::unique_ptr<Sink> sink;

    switch (outputType) {
        case ExtractOutputType::Unknown: sink = std::make_unique<SinkNull>(config); break;
        case ExtractOutputType::Bin: sink = std::make_unique<SinkBin>(config); break;
        case ExtractOutputType::Raw: sink = std::make_unique<SinkRaw>(config, false); break;
        case ExtractOutputType::LengthDelimited:
            sink = std::make_unique<SinkRaw>(config, true);
            break;
    }

    if (!sink) {
        VNLOG_ERROR("Can't make sink, unrecognised output type specified");
        return nullptr;
    }

    if (!sink->initialise()) {
        VNLOG_ERROR("Failed to initialise sink");
        return nullptr;
    }

    return sink;
}

} // namespace vnova::analyzer
