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
 *        srvutils.h
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        Utility Functions
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __SRV_UTILS_H__
#define __SRV_UTILS_H__

#define SRV_SAFE_FREE_MEMORY(pMemory) \
    if (pMemory) { SrvFreeMemory(pMemory); }

#define SRV_SAFE_FREE_MEMORY_AND_RESET(pMemory) \
    if (pMemory) { SrvFreeMemory(pMemory); (pMemory) = NULL; }

#ifdef AF_INET6
#define SRV_SOCKET_ADDRESS_STRING_MAX_SIZE \
    (LW_MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN) + 1)
#else
#define SRV_SOCKET_ADDRESS_STRING_MAX_SIZE \
    (INET_ADDRSTRLEN + 1)
#endif

typedef VOID (*PFN_PROD_CONS_QUEUE_FREE_ITEM)(PVOID pItem);

typedef struct _SMB_PROD_CONS_QUEUE
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    LWIO_QUEUE       queue;

    ULONG           ulNumMaxItems;
    ULONG           ulNumItems;

    PFN_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem;

    pthread_cond_t  event;
    pthread_cond_t* pEvent;

} SMB_PROD_CONS_QUEUE, *PSMB_PROD_CONS_QUEUE;


typedef struct _SRV_HOST_INFO
{
    LONG  refcount;

    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    PSTR  pszHostname;
    PSTR  pszDomain;

    BOOLEAN bIsJoined;

} SRV_HOST_INFO, *PSRV_HOST_INFO;

typedef struct _SRV_LOG_CONTEXT* PSRV_LOG_CONTEXT;

NTSTATUS
SrvUtilsInitialize(
    VOID
    );

NTSTATUS
SrvAllocateMemory(
    IN  size_t size,
    OUT PVOID* ppMemory
    );

NTSTATUS
SrvReallocMemory(
    IN  PVOID  pMemory,
    IN  size_t size,
    OUT PVOID* ppNewMemory
    );

VOID
SrvFreeMemory(
    IN PVOID pMemory
    );

NTSTATUS
SrvAcquireHostInfo(
    PSRV_HOST_INFO  pOrigHostInfo,
    PSRV_HOST_INFO* ppNewHostInfo
    );

VOID
SrvReleaseHostInfo(
    PSRV_HOST_INFO pHostinfo
    );

NTSTATUS
SrvBuildFilePath(
    PWSTR  pwszPrefix,
    PWSTR  pwszSuffix,
    PWSTR* ppwszFilename
    );

NTSTATUS
SrvGetParentPath(
    PWSTR  pwszPath,
    PWSTR* ppwszParentPath
    );

NTSTATUS
SrvMatchPathPrefix(
    PWSTR pwszPath,
    ULONG ulPathLength,
    PWSTR pwszPrefix
    );

NTSTATUS
SrvProdConsInit(
    ULONG                         ulNumMaxItems,
    PFN_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem,
    PSMB_PROD_CONS_QUEUE*         ppQueue
    );

NTSTATUS
SrvProdConsInitContents(
    PSMB_PROD_CONS_QUEUE          pQueue,
    ULONG                         ulNumMaxItems,
    PFN_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem
    );

NTSTATUS
SrvProdConsEnqueue(
    PSMB_PROD_CONS_QUEUE pQueue,
    PVOID                pItem
    );

NTSTATUS
SrvProdConsDequeue(
    PSMB_PROD_CONS_QUEUE pQueue,
    PVOID*               ppItem
    );

NTSTATUS
SrvProdConsTimedDequeue(
    PSMB_PROD_CONS_QUEUE pQueue,
    struct timespec*     pTimespec,
    PVOID*               ppItem
    );

VOID
SrvProdConsFree(
    PSMB_PROD_CONS_QUEUE pQueue
    );
NTSTATUS
SrvMbsToWc16s(
    IN  PCSTR  pszString,
    OUT PWSTR* ppwszString
    );

NTSTATUS
SrvWc16sToMbs(
    IN  PCWSTR pwszString,
    OUT PSTR*  ppszString
    );

NTSTATUS
SrvAllocateStringW(
    IN  PWSTR  pwszInputString,
    OUT PWSTR* ppwszOutputString
    );

NTSTATUS
SrvAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    );

VOID
SrvProdConsFreeContents(
    PSMB_PROD_CONS_QUEUE pQueue
    );

NTSTATUS
SrvSocketAddressToStringW(
    struct sockaddr* pSocketAddress,
    PWSTR*           ppwszAddress
    );

NTSTATUS
SrvSocketAddressToString(
    struct sockaddr* pSocketAddress, /* IN     */
    PSTR             pszAddress,     /*    OUT */
    ULONG            ulAddressLength /* IN     */
    );

NTSTATUS
SrvSocketStringToAddressA(
    PCSTR            pszAddress,
    struct sockaddr* pSocketAddress,
    SOCKLEN_T*       pAddressLength
    );

NTSTATUS
SrvSocketGetAddrInfoW(
    PCWSTR            pwszClientname,
    struct addrinfo** ppAddrInfo
    );

NTSTATUS
SrvSocketGetAddrInfoA(
    PCSTR             pszClientname,
    struct addrinfo** ppAddrInfo
    );

NTSTATUS
SrvSocketCompareAddress(
    const struct sockaddr* pAddress1,
    SOCKLEN_T              addrLength1,
    const struct sockaddr* pAddress2,
    SOCKLEN_T              addrLength2,
    PBOOLEAN               pbMatch
    );

NTSTATUS
SrvLogContextCreate(
    PSRV_LOG_CONTEXT* ppLogContext
    );

NTSTATUS
SrvLogContextGetLevel(
    PSRV_LOG_CONTEXT     pLogContext,
    struct sockaddr*     pClientAddress,
    SOCKLEN_T            ulClientAddressLength,
    ULONG                protocolVer,
    USHORT               usOpcode,
    LWIO_LOG_LEVEL*      pLogLevel
    );

VOID
SrvLogContextFree(
    PSRV_LOG_CONTEXT pLogContext
    );

NTSTATUS
SrvUtilsShutdown(
    VOID
    );

#endif /* __SRV_UTILS_H__ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
