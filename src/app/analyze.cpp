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

#include "app/analyze.h"

#include "extractor/extractor.h"
#include "helper/extracted_frame.h"
#include "helper/frame_timestamp.h"
#include "helper/reorder.h"
#include "parser/parsed_types.h"
#include "parser/parser.h"
#include "utility/log_util.h"

using namespace vnova::helper;

namespace vnova::analyzer::analyze {

bool parseFrames(Reorder& buffer, Parser& parser, const Extractor& extractor,
                 BaseFrameQueue& frameQueue, ParsedFrameList& parsedFrames)
{
    LCEVCWithBase lcevcWithBase;
    while (buffer.dequeue(lcevcWithBase, frameQueue)) {
        auto& lcevc = lcevcWithBase.lcevc;
        const auto baseDts = lcevcWithBase.base.value_or(DEFAULT_FRAME_INFO).dts;
        lcevc.remainingWireSize = extractor.getRemainingSizeForDts(baseDts).value_or(0);

        if (auto parsedFrameOpt = parser.parse(lcevcWithBase); parsedFrameOpt.has_value()) {
            parsedFrames.push_back(parsedFrameOpt.value());

        } else {
            VNLOG_ERROR("Failed to decode LCEVC data");
            return false;
        }
    }

    return true;
}

void configureFlags(Extractor const & extractor, Reorder& buffer, Parser& parser)
{
    buffer.setRawstream(extractor.getRawStream());
    buffer.setBaseStream(extractor.getBaseStream());

    parser.setIsBaseStreamSizeCountable(extractor.getBaseStreamSizeCountable());

    parser.setLvccProfile(extractor.getLvccProfile());
    parser.setLvccLevel(extractor.getLvccLevel());
    parser.setLvccPresent(extractor.getLvccPresent());
}

bool summarize(FILE* file, Extractor const & extractor, Parser& parser, const timestamp::TimestampStats& pts)
{
    const uint64_t lcevcSize = parser.getLcevcLayerSize();

    if (lcevcSize == 0) {
        VNLOG_WARN("Cannot summarize if no LCEVC data was parsed");
        return false;
    }

    Summary summary;
    summary.framerate = extractor.getBaseFramerate();
    summary.lcevc.frame_count = static_cast<int64_t>(parser.getTotal());
    summary.lcevc.layer_size = static_cast<int64_t>(lcevcSize);
    summary.lcevc.level_1_size = static_cast<int64_t>(parser.getSublayer1Size());
    summary.lcevc.level_2_size = static_cast<int64_t>(parser.getSublayer2Size());
    summary.lcevc.temporal_size = static_cast<int64_t>(parser.getTemporalSize());

    const uint64_t totalPacketSize = extractor.getTotalPacketSize();
    if (const bool hasBase = totalPacketSize > 0; hasBase) {
        summary.base = SummaryBase();
        const uint64_t baseFrameCount = extractor.getBaseFrameCount();
        summary.base->frame_count = static_cast<int64_t>(baseFrameCount);
        summary.base->layer_size = static_cast<int64_t>(totalPacketSize - lcevcSize);

        if (summary.framerate.has_value()) {
            const double baseDuration = static_cast<double>(baseFrameCount) / summary.framerate.value();
            if (baseDuration > 0.0) {
                summary.base->layer_bitrate =
                    (static_cast<double>(summary.base->layer_size) * 8.0) / baseDuration;
            }
        }

        if (summary.base->layer_size > 0) {
            summary.base->lcevc_base_ratio =
                static_cast<double>(lcevcSize) / static_cast<double>(summary.base->layer_size);
        }
    }

    if (const double lcevcDuration =
            summary.framerate.has_value()
                ? static_cast<double>(summary.lcevc.frame_count) / summary.framerate.value()
                : extractor.getDurationSec();
        lcevcDuration > 0.0) {
        summary.lcevc.layer_bitrate = (static_cast<double>(lcevcSize) * 8.0) / lcevcDuration;
    }

    summary.pts = {pts.consistent, pts.intervalCount};

    parser.writeOut(file, summary);

    return true;
}

} // namespace vnova::analyzer::analyze
