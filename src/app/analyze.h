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

#include "extractor/extractor.h"
#include "helper/extracted_frame.h"
#include "helper/frame_timestamp.h"
#include "helper/reorder.h"
#include "parser/parser.h"

namespace vnova::analyzer::analyze {

bool parseFrames(helper::Reorder& buffer, Parser& parser, const Extractor& extractor,
                 helper::BaseFrameQueue& frameQueue, ParsedFrameList& parsedFrames);
void configureFlags(Extractor const & extractor, helper::Reorder& buffer, Parser& parser);
bool summarize(FILE* file, Extractor const & extractor, Parser& parser,
               const helper::timestamp::TimestampStats& pts);

} // namespace vnova::analyzer::analyze
