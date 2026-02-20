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

#ifndef VN_PARSER_PARSED_FRAME_H_
#define VN_PARSER_PARSED_FRAME_H_

#include "helper/extracted_frame.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace vnova::analyzer {

struct ParsedFrame
{
    size_t index = 0;
    helper::LCEVCFrame lcevc{};
    std::optional<helper::BaseFrame> base = std::nullopt;
    int64_t lcevcWireSize = -1;
    int64_t baseWireSize = -1;
    int64_t combinedWireSize = -1;
};

class ParsedFrameList
{
public:
    // NOLINTBEGIN(readability-identifier-naming) - keep std::vector-like API
    void push_back(const ParsedFrame& frame) { m_frames.push_back(frame); }
    size_t size() const { return m_frames.size(); };
    const ParsedFrame& at(const size_t index) const { return m_frames.at(index); };
    // NOLINTEND(readability-identifier-naming)

    std::optional<std::vector<int64_t>> GetPts() const;
    std::vector<int64_t> GetLcevcBytes() const;
    std::vector<int64_t> GetBaseBytes() const;
    std::vector<int64_t> GetCombinedBytes() const;
    std::optional<bool> HasBase() const;

private:
    std::vector<ParsedFrame> m_frames;
};

} // namespace vnova::analyzer

#endif // VN_PARSER_PARSED_FRAME_H_
