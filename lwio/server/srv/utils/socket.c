/*
 * Copyright Likewise Software    2004-2009
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        string.c
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        Utilities
 *
 *        Strings
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
inline
BOOLEAN
SrvSocketCompareIPV4Address(
    const struct sockaddr* pAddress1,
    const struct sockaddr* pAddress2
    );

#ifdef AF_INET6

static
inline
BOOLEAN
SrvSocketIsIPV4MappedAddress(
    const struct sockaddr* pAddress
    );

static
inline
VOID
SrvSocketBuildMappedIPV4Address(
    const struct sockaddr* pSrcAddress,
    struct sockaddr* pDstAddress
    );

static
inline
BOOLEAN
SrvSocketCompareIPV6Address(
    const struct sockaddr* pAddress1,
    const struct sockaddr* pAddress2
    );

#endif /* AF_INET6 */

NTSTATUS
SrvSocketAddressToStringW(
    struct sockaddr* pSocketAddress,
    PWSTR*           ppwszAddress
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    CHAR     szBuffer[SRV_SOCKET_ADDRESS_STRING_MAX_SIZE];
    PWSTR    pwszAddress = NULL;

    if (!pSocketAddress)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvSocketAddressToString(
                        pSocketAddress,
                        &szBuffer[0],
                        sizeof(szBuffer));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMbsToWc16s(szBuffer, &pwszAddress);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppwszAddress = pwszAddress;

cleanup:

    return ntStatus;

error:

    *ppwszAddress = NULL;

    goto cleanup;
}

NTSTATUS
SrvSocketAddressToString(
    struct sockaddr* pSocketAddress, /* IN     */
    PSTR             pszAddress,     /*    OUT */
    ULONG            ulAddressLength /* IN     */
    )
{
    NTSTATUS ntStatus  = STATUS_SUCCESS;
    PVOID pAddressPart = NULL;

    switch (pSocketAddress->sa_family)
    {
        case AF_INET:
            pAddressPart = &((struct sockaddr_in*)pSocketAddress)->sin_addr;
            break;
#ifdef AF_INET6
        case AF_INET6:
            pAddressPart = &((struct sockaddr_in6*)pSocketAddress)->sin6_addr;
            break;
#endif
        default:
           ntStatus = STATUS_NOT_SUPPORTED;
           BAIL_ON_NT_STATUS(ntStatus);
    }

    if (!inet_ntop( pSocketAddress->sa_family,
                    pAddressPart,
                    pszAddress,
                    ulAddressLength))
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    // Terminate output buffer
    if (ulAddressLength > 0)
    {
        pszAddress[0] = 0;
    }

    goto cleanup;
}

NTSTATUS
SrvSocketGetAddrInfoW(
    PCWSTR            pwszClientname,
    struct addrinfo** ppAddrInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    struct addrinfo* pAddrInfo = NULL;
    PSTR   pszClientname = NULL;

    ntStatus = SrvWc16sToMbs(pwszClientname, &pszClientname);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSocketGetAddrInfoA(pszClientname, &pAddrInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppAddrInfo = pAddrInfo;

cleanup:

    SRV_SAFE_FREE_MEMORY(pszClientname);

    return ntStatus;

error:

    *ppAddrInfo = NULL;

    if (pAddrInfo)
    {
        freeaddrinfo(pAddrInfo);
    }

    goto cleanup;
}

NTSTATUS
SrvSocketGetAddrInfoA(
    PCSTR             pszClientname,
    struct addrinfo** ppAddrInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    struct   addrinfo  hints     = {0};
    struct   addrinfo* pAddrInfo = NULL;

    if (IsNullOrEmptyString(pszClientname))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    hints.ai_family    = AF_UNSPEC;
    hints.ai_socktype  = SOCK_STREAM;
    hints.ai_flags     = AI_PASSIVE;
    hints.ai_protocol  = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr      = NULL;
    hints.ai_next      = NULL;

    if (getaddrinfo(pszClientname, NULL, &hints, &pAddrInfo) != 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppAddrInfo = pAddrInfo;

cleanup:

    return ntStatus;

error:

    *ppAddrInfo = NULL;

    if (pAddrInfo)
    {
        freeaddrinfo(pAddrInfo);
    }

    goto cleanup;
}

NTSTATUS
SrvSocketCompareAddress(
    const struct sockaddr* pAddress1,
    SOCKLEN_T              addrLength1,
    const struct sockaddr* pAddress2,
    SOCKLEN_T              addrLength2,
    PBOOLEAN               pbMatch
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bMatch   = FALSE;
#ifdef AF_INET6
    struct sockaddr mappedAddr1 = {0};
    struct sockaddr mappedAddr2 = {0};
#endif
    const struct sockaddr* pAddress1Ref = pAddress1;
    const struct sockaddr* pAddress2Ref = pAddress2;

    if (!pAddress1 || !pAddress2 || !addrLength1 || !addrLength2)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

#ifdef AF_INET6

    if (SrvSocketIsIPV4MappedAddress(pAddress1))
    {
        SrvSocketBuildMappedIPV4Address(pAddress1, &mappedAddr1);

        pAddress1Ref = &mappedAddr1;
    }

    if (SrvSocketIsIPV4MappedAddress(pAddress2))
    {
        SrvSocketBuildMappedIPV4Address(pAddress2, &mappedAddr2);

        pAddress2Ref = &mappedAddr2;
    }

#endif

    if (pAddress1Ref->sa_family == pAddress2Ref->sa_family)
    {
        switch (pAddress1Ref->sa_family)
        {
            case AF_INET:

                bMatch = SrvSocketCompareIPV4Address(pAddress1Ref, pAddress2Ref);

                break;

#ifdef AF_INET6

            case AF_INET6:

                bMatch = SrvSocketCompareIPV6Address(pAddress1Ref, pAddress2Ref);

                break;

#endif /* AF_INET6 */

            default:

                ntStatus = STATUS_NOT_SUPPORTED;
                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }
    }

    *pbMatch = bMatch;

cleanup:

    return ntStatus;

error:

    *pbMatch = FALSE;

    goto cleanup;
}

static
inline
BOOLEAN
SrvSocketCompareIPV4Address(
    const struct sockaddr* pAddress1,
    const struct sockaddr* pAddress2
    )
{
    union
    {
        const struct sockaddr* pGenericAddr;
        struct sockaddr_in*    pIPV4Addr;
    } addr1 =
    {
        .pGenericAddr = pAddress1
    };
    union
    {
        const struct sockaddr* pGenericAddr;
        struct sockaddr_in*    pIPV4Addr;
    } addr2 =
    {
        .pGenericAddr = pAddress2
    };

    return ((addr1.pIPV4Addr->sin_family == addr2.pIPV4Addr->sin_family) &&
            ((addr1.pIPV4Addr->sin_addr.s_addr == INADDR_ANY) ||
             (addr2.pIPV4Addr->sin_addr.s_addr == INADDR_ANY) ||
             (addr1.pIPV4Addr->sin_addr.s_addr ==
                             addr2.pIPV4Addr->sin_addr.s_addr)) &&
            ((addr1.pIPV4Addr->sin_port == 0) ||
             (addr2.pIPV4Addr->sin_port == 0) ||
             (addr1.pIPV4Addr->sin_port == addr2.pIPV4Addr->sin_port)));
}

#ifdef AF_INET6

static
inline
BOOLEAN
SrvSocketIsIPV4MappedAddress(
    const struct sockaddr* pAddress
    )
{
    BOOLEAN bIsMapped = FALSE;
    union
    {
        const struct sockaddr* pGenericAddr;
        struct sockaddr_in6*   pIPV6Addr;
    } addrInfo =
    {
        .pGenericAddr = pAddress
    };

    if ((pAddress->sa_family == AF_INET6) &&
        IN6_IS_ADDR_V4MAPPED(&addrInfo.pIPV6Addr->sin6_addr))
    {
        bIsMapped = TRUE;
    }

    return bIsMapped;
}

static
inline
VOID
SrvSocketBuildMappedIPV4Address(
    const struct sockaddr* pSrcAddress,
    struct sockaddr*       pDstAddress
    )
{
    union
    {
        const struct sockaddr* pGenericAddr;
        struct sockaddr_in6*   pIPV6Addr;
    } srcAddrInfo =
    {
        .pGenericAddr = pSrcAddress
    };
    union
    {
        const struct sockaddr* pGenericAddr;
        struct sockaddr_in*    pIPV4Addr;
    } dstAddrInfo =
    {
        .pGenericAddr = pDstAddress
    };

    dstAddrInfo.pIPV4Addr->sin_port = srcAddrInfo.pIPV6Addr->sin6_port;
    // An IPV6 address is 16 bytes.
    // We want to skip 12 bytes and get just 4 bytes for the IPV4 address
    memcpy( &dstAddrInfo.pIPV4Addr->sin_addr.s_addr,
            &srcAddrInfo.pIPV6Addr->sin6_addr.s6_addr[12],
            sizeof(dstAddrInfo.pIPV4Addr->sin_addr.s_addr));
}

static
inline
BOOLEAN
SrvSocketCompareIPV6Address(
    const struct sockaddr* pAddress1,
    const struct sockaddr* pAddress2
    )
{
    union
    {
        const struct sockaddr* pGenericAddr;
        struct sockaddr_in6*   pIPV6Addr;
    } addr1 =
    {
        .pGenericAddr = pAddress1
    };
    union
    {
        const struct sockaddr* pGenericAddr;
        struct sockaddr_in6*   pIPV6Addr;
    } addr2 =
    {
        .pGenericAddr = pAddress2
    };

    // TODO: include flow info and scope ids in the comparison?

    return ((addr1.pIPV6Addr->sin6_family == addr2.pIPV6Addr->sin6_family) &&
            (!memcmp(   &addr1.pIPV6Addr->sin6_addr,
                        &in6addr_any,
                        sizeof(in6addr_any)) ||
             !memcmp(   &addr2.pIPV6Addr->sin6_addr,
                        &in6addr_any,
                        sizeof(in6addr_any)) ||
             !memcmp(   &addr1.pIPV6Addr->sin6_addr,
                        &addr2.pIPV6Addr->sin6_addr,
                        sizeof(addr1.pIPV6Addr->sin6_addr))) &&
            ((addr1.pIPV6Addr->sin6_port == 0) ||
             (addr2.pIPV6Addr->sin6_port == 0) ||
             (addr1.pIPV6Addr->sin6_port == addr2.pIPV6Addr->sin6_port)));
}

#endif /* AF_INET6 */




