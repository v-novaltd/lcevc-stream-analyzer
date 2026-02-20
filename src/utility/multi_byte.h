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
#ifndef VN_UTILITY_MULTI_BYTE_H_
#define VN_UTILITY_MULTI_BYTE_H_

#include <cstdint>
#include <stdexcept>

namespace vnova::utility {
/*
 * MultiByte sequence helper class
 *
 * Inputs/outputs multibyte sequence given a function that can input/output
 * a single byte to/from bytestream
 *
 * According to DIS requirement produces/expects big endian bytestream
 *
 */

class MultiByte
{
public:
    template <typename ReadFn, typename... Args>
    static uint32_t Decode(ReadFn readFn, Args... args)
    {
        uint8_t byte = 0;
        uint32_t result = 0;
        uint32_t bytesRead = 0;

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
        do {
            byte = readFn(args...);
            result = (result << 7) | uint32_t(byte & 0x7F);
            ++bytesRead;
            const bool overflowOnLastByte = bytesRead == 5 && (byte & 0x70);
            if (bytesRead > 5 || overflowOnLastByte) {
                throw std::overflow_error("MultiByte decode overflows 32 bits");
            }
        } while (byte & 0x80);

        return result;
    }
};

} // namespace vnova::utility

#endif // VN_UTILITY_MULTI_BYTE_H_
