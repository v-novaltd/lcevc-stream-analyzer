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
#ifndef VN_SINK_SINK_H_
#define VN_SINK_SINK_H_

#include "extractor/extractor.h"

#include <memory>
#include <string>

namespace vnova::analyzer {
struct Config;

class Sink
{
public:
    explicit Sink(const Config& config);
    Sink(const Sink&) = delete;
    Sink& operator=(const Sink&) = delete;
    virtual ~Sink() = default;

    virtual bool initialise() = 0;
    virtual void release() = 0;
    virtual bool write(const helper::LCEVCFrame& lcevc) = 0;

    static std::unique_ptr<Sink> factory(const Config& config);

protected:
    const Config& config;
    size_t m_index = 0;
};

} // namespace vnova::analyzer

#endif // VN_SINK_SINK_H_
