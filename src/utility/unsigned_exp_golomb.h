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
#ifndef VN_UTILITY_UNSIGNED_EXP_GOLOMB_H_
#define VN_UTILITY_UNSIGNED_EXP_GOLOMB_H_

#include "utility/platform.h"

#include <cstdint>
namespace vnova::utility {
/*
 * UnsignedExpGolomb sequence helper class
 *
 * Inputs/outputs exp-golomb sequence given a function that can input/output
 * a single bit to/from bytestream
 *
 * This expects the read functions to feed a raw bitstream from MSB to
 * LSB.
 */
class UnsignedExpGolomb
{
public:
    template <typename ReadBitFn>
    static uint32_t Decode(ReadBitFn readBitFn)
    {
        // Read all the prefix zeros, number of zeros = number of bits to read - 1.
        bool bit = readBitFn();
        uint32_t readBitCount = 0;

        while (!bit) {
            readBitCount++;
            bit = readBitFn();
        }

        // Special case there were no leading zeros, so encoded value is 0.
        if (!readBitCount) {
            return 0;
        }

        // Perform reading into a uint32_t loading bits up into LSB so that
        // value has meaning.
        //
        // Do not need to adjust readBitCount, we already read the first
        // bit when reading the zeros as that broke us out of the loop.
        // As such we always start the value at 1.
        uint32_t value = 1u;
        while (readBitCount--) {
            value = (value << 1) | (readBitFn() ? 1u : 0u);
        }

        // Value binary is encoded + 1 (this is to handle 0 case).
        VNAssert(value > 0);
        return (value - 1);
    }
};

} // namespace vnova::utility

#endif // VN_UTILITY_UNSIGNED_EXP_GOLOMB_H_
