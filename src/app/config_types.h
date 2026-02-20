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
#ifndef VN_SRC_CONFIG_TYPES_H_
#define VN_SRC_CONFIG_TYPES_H_

#include <cstdint>

namespace vnova::analyzer {

enum class Subcommand
{
    ANALYZE, // Analyze each frame printing various LCEVC related stats.
    EXTRACT, // Extract LCEVC enhancement layer to file.
    SIMULATE_BUFFER, // Print per-frame buffer fill level and generate output file for plotting fill level over time
    DUMP_AU // Dump every AU to binary files, print NAL units within each NAL
};

enum class ReturnCode : int
{
    Success = 0,
    ArgParseFail = 1,
    ExtractorFactoryFail = 2,
    SinkFactoryFail = 3,
    ParseFail = 4,
    ExtractionFail = 5,
    DecodeFail = 6,
    FileFail = 7,
    BufferSimulationFail = 8,
    PTSValidationFail = 9,
    PTSOrderingFail = 10,
};

enum class ExtractOutputType
{
    Bin,             // LCEVC is written in LCEVC bin format.
    Raw,             // LCEVC is written as raw LCEVC NALUs.
    LengthDelimited, // LCEVC is delimited by 4-byte uint lengths.
    Unknown
};
constexpr const char* toString(const ExtractOutputType type)
{
    switch (type) {
        case ExtractOutputType::Bin: return "Bin";
        case ExtractOutputType::Raw: return "Raw";
        case ExtractOutputType::LengthDelimited: return "LengthDelimited";
        case ExtractOutputType::Unknown: return "Unknown";
        default: return "[FAILED TO CONVERT TO STRING]";
    }
}

enum class LogFormat
{
    TEXT, // Human-readable unstructured text.
    JSON, // Structured JSON.
};
constexpr const char* toString(const LogFormat type)
{
    switch (type) {
        case LogFormat::TEXT: return "Text";
        case LogFormat::JSON: return "JSON";
        default: return "[FAILED TO CONVERT TO STRING]";
    }
}

} // namespace vnova::analyzer

#endif // VN_SRC_CONFIG_TYPES_H_
