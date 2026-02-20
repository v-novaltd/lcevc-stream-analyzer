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
#ifndef VN_SRC_CONFIG_H_
#define VN_SRC_CONFIG_H_

#include "config_types.h"
#include "helper/base_type.h"
#include "helper/input_detection.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <unordered_map>

namespace vnova::analyzer {

struct Config
{
public:
    std::array<uint8_t, 3> version = {0, 0, 0};

    // Generic options
    std::filesystem::path inputPath;
    helper::InputType inputType = helper::InputType::Unknown;
    int32_t tsPID = -1;
    double inputFramerate = 0.0;
    helper::BaseType baseType = helper::BaseType::INVALID;
    uint64_t frameLimit = 0;

    std::unordered_map<Subcommand, bool> subcommand;

    // ANALYZE options
    bool analyzeVerboseOutput = false;
    std::filesystem::path analyzeLogPath;
    LogFormat analyzeLogFormat = LogFormat::TEXT;

    // EXTRACT options
    uint64_t extractFrameLimit = 0;
    std::filesystem::path extractOutputPath;
    ExtractOutputType extractOutputType = ExtractOutputType::Unknown;

    // DUMP_AU options
    std::filesystem::path dumpAuOutputPath;

    // SIMULATE_BUFFER options
    std::filesystem::path simulateBufferModelPath;
    std::filesystem::path simulateBufferOutputPath;
    double simulateBufferSampleInterval = 0.005;

    // Private config options, not modifiable from command line.
    bool ptsValidation = true;
};

std::optional<const Config> parseArgs(int argc, char* argv[]);

} // namespace vnova::analyzer

#endif // VN_SRC_CONFIG_H_
