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
#ifndef VN_UTILITY_BYTE_ORDER_H_
#define VN_UTILITY_BYTE_ORDER_H_

#include "utility/platform.h"

// ntoh, hton
#if defined(_WIN32) || defined(_WIN64)
#include <Winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#ifndef htonll
#define htonll(x)                                                 \
    ((((uint64_t)htonl((uint32_t)((x) >> 32))) & 0xFFFFFFFFULL) | \
     (((uint64_t)htonl((uint32_t)((x)&0xFFFFFFFF))) << 32))
#endif

#ifndef ntohll
#define ntohll(x)                                                 \
    ((((uint64_t)ntohl((uint32_t)((x) >> 32))) & 0xFFFFFFFFULL) | \
     (((uint64_t)ntohl((uint32_t)((x)&0xFFFFFFFF))) << 32))
#endif
#else
#if defined(__APPLE__)
#include <machine/endian.h>
#else
#include <endian.h>
#endif
#include <netinet/in.h>

#ifndef ntohll
#define ntohll be64toh
#endif
#ifndef htonll
#define htonll htobe64
#endif
#endif

namespace vnova::utility {
template <typename T>
struct ByteOrder
{
    static inline T toNetwork(T word);
    static inline T toHost(T word);
};

template <>
inline int64_t ByteOrder<int64_t>::toNetwork(int64_t hostBytes)
{
    return static_cast<int64_t>(htonll(static_cast<uint64_t>(hostBytes)));
}

template <>
inline uint64_t ByteOrder<uint64_t>::toNetwork(uint64_t hostBytes)
{
    return htonll(hostBytes);
}

template <>
inline uint32_t ByteOrder<uint32_t>::toNetwork(uint32_t hostBytes)
{
    return htonl(hostBytes);
}

template <>
inline uint16_t ByteOrder<uint16_t>::toNetwork(uint16_t hostBytes)
{
    return htons(hostBytes);
}

template <>
inline uint8_t ByteOrder<uint8_t>::toNetwork(uint8_t hostBytes)
{
    return hostBytes;
}

template <>
inline int64_t ByteOrder<int64_t>::toHost(int64_t hostBytes)
{
    return static_cast<int64_t>(ntohll(static_cast<uint64_t>(hostBytes)));
}

template <>
inline uint64_t ByteOrder<uint64_t>::toHost(uint64_t networkBytes)
{
    return ntohll(networkBytes);
}

template <>
inline uint32_t ByteOrder<uint32_t>::toHost(uint32_t networkBytes)
{
    return ntohl(networkBytes);
}

template <>
inline uint16_t ByteOrder<uint16_t>::toHost(uint16_t networkBytes)
{
    return ntohs(networkBytes);
}

template <>
inline uint8_t ByteOrder<uint8_t>::toHost(uint8_t networkBytes)
{
    return networkBytes;
}

} // namespace vnova::utility

#endif // VN_UTILITY_BYTE_ORDER_H_
