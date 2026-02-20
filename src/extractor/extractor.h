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
#ifndef VN_EXTRACTOR_EXTRACTOR_H_
#define VN_EXTRACTOR_EXTRACTOR_H_

#include "app/config.h"
#include "helper/extracted_frame.h"

#include <cstdint>
#include <optional>
#include <unordered_map>

namespace vnova::analyzer {

double normaliseFps(const double fps);

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
    virtual bool next(std::vector<helper::LCEVCFrame>& lcevcFrames, helper::BaseFrameQueue& frameQueue) = 0;
    virtual bool flush(helper::BaseFrameQueue& /*frameQueue*/, std::vector<helper::LCEVCFrame>& /*lcevcFrames*/)
    {
        return false;
    }

    uint64_t getTotalPacketSize() const noexcept;
    std::optional<int64_t> getRemainingSizeForDts(const int64_t dts) const noexcept;

    bool getBaseStream() const noexcept { return m_hasBaseVideoStream; }
    bool getBaseStreamSizeCountable() const noexcept { return m_baseVideoStreamIsSizeCountable; }
    bool getError() const noexcept { return m_error; }
    bool getInitialized() const noexcept { return m_initialized; }
    bool getLvccPresent() const noexcept { return m_lvccPresent; }
    bool getRawStream() const noexcept { return m_rawStream; }
    bool getFourBytePrefix() const noexcept { return m_fourBytePrefix; }
    double getDurationSec() const noexcept { return m_durationSec; }
    std::optional<double> getBaseFramerate() const noexcept { return m_baseFramerate; }
    void setBaseFramerate(double baseFramerate) noexcept
    {
        m_baseFramerate = normaliseFps(baseFramerate);
    }
    std::optional<double> getBaseTimebase() const noexcept
    {
        if (m_rawStream) {
            if (m_baseFramerate.has_value() == false) {
                return std::nullopt;
            }
            return 1.0 / m_baseFramerate.value();
        }
        return m_baseTimebase;
    }
    void setBaseTimebase(double baseTimebase) noexcept { m_baseTimebase = baseTimebase; }
    uint64_t getBaseFrameCount() const noexcept { return m_decodedBaseFrameCount; }
    uint8_t getLvccLevel() const noexcept { return m_lvccLevel; }
    uint8_t getLvccProfile() const noexcept { return m_lvccProfile; }

protected:
    bool m_hasBaseVideoStream = false;
    bool m_baseVideoStreamIsSizeCountable = false;
    bool m_error = false;
    bool m_fourBytePrefix = false;
    bool m_initialized = false;
    bool m_lvccPresent = false;
    bool m_rawStream = false;
    double m_durationSec = 0;
    std::optional<double> m_baseFramerate = std::nullopt;
    std::optional<double> m_baseTimebase = std::nullopt;
    uint64_t m_decodedBaseFrameCount = 0;

    // Sum of size of all packets for each DTS (size of both any base and LCEVC data).
    std::unordered_map<int64_t, int64_t> m_totalSizeForDts;

    // Start with packet size, subtract size of 1+ LCEVC frames in packet, remainder is base frame size.
    std::unordered_map<int64_t, int64_t> m_remainingSizeForDts;

    uint8_t m_lvccLevel = 0;
    uint8_t m_lvccProfile = 0;
};

} // namespace vnova::analyzer

#endif // VN_EXTRACTOR_EXTRACTOR_H_
