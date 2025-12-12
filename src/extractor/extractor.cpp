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
#include "extractor.h"

#include "extractor/extractor_bin.h"
#include "extractor/extractor_demuxer.h"
#include "extractor/extractor_lcevc.h"
#include "extractor/extractor_mp4.h"
#include "extractor/extractor_sei.h"
#include "utility/log_util.h"

#include <memory>

namespace vnova::analyzer {
std::unique_ptr<Extractor> Extractor::factory(const Config& config)
{
    std::unique_ptr<Extractor> extractor;

    const auto inputType = config.inputType;
    const std::string& sourcePath = config.inputPath;

    switch (inputType) {
        case InputType::Unknown: break;
        case InputType::ELEMENTARY: [[fallthrough]];
        case InputType::TS: {
            extractor = std::make_unique<ExtractorSEI>(sourcePath, inputType, config);
            break;
        }
        case InputType::WEBM: [[fallthrough]];
        case InputType::MP4: {
            extractor = std::make_unique<ExtractorMP4>(sourcePath, inputType, config);
            break;
        }
        case InputType::BIN: {
            extractor = std::make_unique<ExtractorBin>(sourcePath, inputType);
            break;
        }
        case InputType::LCEVC: {
            extractor = std::make_unique<ExtractorLCEVC>(sourcePath);
            break;
        }
    }

    if (!extractor) {
        VNLog::Error("Can't make extractor, unrecognised input type specified\n");
        return nullptr;
    }

    if (!extractor->isInitialized()) {
        return nullptr;
    }
    return extractor;
}

} // namespace vnova::analyzer
