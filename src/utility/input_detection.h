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

/* Copyright (c) V-Nova International Limited 2025. All rights reserved. */
#ifndef VN_UTILITY_INPUT_DETECTION_H_
#define VN_UTILITY_INPUT_DETECTION_H_

#include "config_types.h"
#include "utility/base_type.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace vnova::analyzer {

enum class InputType;

struct DetectedInputFormat
{
    DetectedInputFormat() = default;

    InputType inputType = InputType::Unknown;
    utility::BaseType::Enum baseType = utility::BaseType::Enum::Invalid;
    bool isLikelyAnnexB = false;
    bool isLikelyAvcc = false;
};

// Attempts to detect the input format from a file on disk by examining the first few kilobytes.
// Returns true if detection ran (i.e. the file could be opened and read).
bool detectInputFormatFromFile(const std::string& path, DetectedInputFormat& outFormat);

// Exposed for tests and in-memory probing.
DetectedInputFormat detectInputFormatFromMemory(const uint8_t* data, size_t size);

} // namespace vnova::analyzer

#endif // VN_UTILITY_INPUT_DETECTION_H_
