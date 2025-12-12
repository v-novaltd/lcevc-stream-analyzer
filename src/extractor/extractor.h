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
#ifndef VN_EXTRACTOR_EXTRACTOR_H_
#define VN_EXTRACTOR_EXTRACTOR_H_

#include "config.h"
#include "helper/frame_queue.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace vnova::analyzer {
struct LCEVC
{
    utility::DataBuffer data;
    int64_t pts = (std::numeric_limits<int64_t>::max)();
    int64_t dts = (std::numeric_limits<int64_t>::max)();
    uint8_t maxReorderFrames = 0;
    FrameType frameType = FrameType::Unknown;
    int64_t frameSize = 0;
};

// @brief Interface class used for extracting a block of raw LCEVC data from
//        a source.
class Extractor
{
public:
    Extractor() = default;
    Extractor(const Extractor&) = delete;
    Extractor& operator=(const Extractor&) = delete;
    Extractor(Extractor&&) = delete;
    Extractor& operator=(Extractor&&) = delete;
    virtual ~Extractor() = default;

    static std::unique_ptr<Extractor> factory(const Config& config);

    // @brief Reads a single frames worth of raw LCEVC data from the source
    // interface.
    virtual bool next(std::vector<LCEVC>& lcevcFrames, FrameQueue& frameBuffer) = 0;
    virtual bool flush(FrameQueue& /*frameBuffer*/, std::vector<LCEVC>& /*lcevcFrames*/)
    {
        return false;
    }

    bool hasError() const noexcept { return bError; }
    virtual bool getBaseStream() const noexcept { return bBaseStream; }
    virtual bool getRawStream() const noexcept { return bRawStream; }
    virtual bool getFourBytePrefix() const noexcept { return bFourBytePrefix; }
    virtual uint8_t getLvccProfile() const noexcept { return lvccProfile; }
    virtual uint8_t getLvccLevel() const noexcept { return lvccLevel; }
    virtual bool isInitialized() const noexcept { return bInitialized; }
    virtual bool hasLvccAtom() const noexcept { return bLvccPresent; }

    uint64_t baseFrameCount = 0;
    uint64_t totalPacketSize = 0;
    int64_t baseFps = 0;
    double durationSec = 0;

protected:
    bool bError = false;
    bool bInitialized = false;
    bool bFourBytePrefix = false;
    bool bBaseStream = false;
    bool bRawStream = false;
    bool bLvccPresent = false;
    uint8_t lvccProfile = 0;
    uint8_t lvccLevel = 0;
};

} // namespace vnova::analyzer

#endif // VN_EXTRACTOR_EXTRACTOR_H_
