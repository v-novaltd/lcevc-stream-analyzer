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
#include "extractor.h"

#include "extractor/extractor_bin.h"
#include "extractor/extractor_lcevc.h"
#include "extractor/extractor_mp4.h"
#include "extractor/extractor_sei.h"
#include "utility/log_util.h"
#include "utility/math_util.h"

#include <memory>
#include <numeric>

using namespace vnova::helper;

namespace vnova::analyzer {

double normaliseFps(const double fps)
{
    if (utility::math::approxEqual(fps, 23.976, 0.01)) {
        VNLOG_WARN("Interpreting baseFps ~23.976 as 24000/1001", fps);
        return (24000.0 / 1001.0);
    }
    if (utility::math::approxEqual(fps, 29.97, 0.01)) {
        VNLOG_WARN("Interpreting baseFps ~29.97 as 30000/1001", fps);
        return (30000.0 / 1001.0);
    }
    if (utility::math::approxEqual(fps, 59.94, 0.01)) {
        VNLOG_WARN("Interpreting baseFps ~59.94 as 60000/1001", fps);
        return (60000.0 / 1001.0);
    }
    return fps;
}

std::unique_ptr<Extractor> Extractor::factory(const Config& config)
{
    std::unique_ptr<Extractor> extractor;

    const auto inputType = config.inputType;
    const std::filesystem::path& sourcePath = config.inputPath;

    switch (inputType) {
        case InputType::Unknown: break;
        case InputType::ES: [[fallthrough]];
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
        VNLOG_ERROR("Can't make extractor, unrecognised input type specified");
        return nullptr;
    }

    if (!extractor->getInitialized()) {
        return nullptr;
    }
    return extractor;
}

uint64_t Extractor::getTotalPacketSize() const noexcept
{
    const int64_t sum = std::accumulate(
        std::begin(m_totalSizeForDts), std::end(m_totalSizeForDts), int64_t{0},
        [](const std::size_t accum, const auto& element) { return accum + element.second; });
    return sum > 0 ? static_cast<uint64_t>(sum) : 0;
}

std::optional<int64_t> Extractor::getRemainingSizeForDts(const int64_t dts) const noexcept
{
    if (m_remainingSizeForDts.count(dts) == 0) {
        return std::nullopt;
    }
    return m_remainingSizeForDts.at(dts);
}

} // namespace vnova::analyzer
