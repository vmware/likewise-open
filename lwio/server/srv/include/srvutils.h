/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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

// Logging

#define _SRV_LOG_CALL_IF(level, pLogCtx, protoVer, usOpcode, pfnLogger, ...) \
    if (pLogCtx)                                                             \
    {                                                                        \
        LWIO_LOG_LEVEL maxLogLevel = SrvLogContextGetLevel(                  \
                                        pLogCtx,                             \
                                        protoVer,                            \
                                        usOpcode);                           \
        if (maxLogLevel >= (level))                                          \
        {                                                                    \
            (pfnLogger)(pLogCtx,                                             \
                        (level),                                             \
                        __FUNCTION__,                                        \
                        LWIO_SAFE_LOG_STRING(SrvPathGetFileName(__FILE__)),  \
                        __LINE__,                                            \
                        ## __VA_ARGS__);                                     \
        }                                                                    \
    }

#define SRV_LOG_CALL_ALWAYS(pLogCtx, protoVer, usOpcode, pfnLogger, ...) \
        _SRV_LOG_CALL_IF(LWIO_LOG_LEVEL_ALWAYS, \
                            pLogCtx,   \
                            protoVer,  \
                            usOpcode,  \
                            pfnLogger, \
                            ## __VA_ARGS__)

#define SRV_LOG_CALL_ERROR(pLogCtx, protoVer, usOpcode, pfnLogger, ...) \
        _SRV_LOG_CALL_IF(LWIO_LOG_LEVEL_ERROR, \
                            pLogCtx,   \
                            protoVer,  \
                            usOpcode,  \
                            pfnLogger, \
                            ## __VA_ARGS__)

#define SRV_LOG_CALL_WARNING(pLogCtx, protoVer, usOpcode, pfnLogger, ...) \
        _SRV_LOG_CALL_IF(LWIO_LOG_LEVEL_WARNING, \
                            pLogCtx,   \
                            protoVer,  \
                            usOpcode,  \
                            pfnLogger, \
                            ## __VA_ARGS__)

#define SRV_LOG_CALL_INFO(pLogCtx, protoVer, usOpcode, pfnLogger, ...) \
        _SRV_LOG_CALL_IF(LWIO_LOG_LEVEL_INFO, \
                            pLogCtx,   \
                            protoVer,  \
                            usOpcode,  \
                            pfnLogger, \
                            ## __VA_ARGS__)

#define SRV_LOG_CALL_VERBOSE(pLogCtx, protoVer, usOpcode, pfnLogger, ...) \
        _SRV_LOG_CALL_IF(LWIO_LOG_LEVEL_VERBOSE, \
                            pLogCtx,   \
                            protoVer,  \
                            usOpcode,  \
                            pfnLogger, \
                            ## __VA_ARGS__)

#define SRV_LOG_CALL_DEBUG(pLogCtx, protoVer, usOpcode, pfnLogger, ...) \
        _SRV_LOG_CALL_IF(LWIO_LOG_LEVEL_DEBUG,  \
                            pLogCtx,   \
                            protoVer,  \
                            usOpcode,  \
                            pfnLogger, \
                            ## __VA_ARGS__)

#define _SRV_LOG_IF(level, pLogCtx, protoVer, usOpcode, szFmt, ...)         \
    if (pLogCtx)                                                            \
    {                                                                       \
        LWIO_LOG_LEVEL maxLogLevel = SrvLogContextGetLevel(                 \
                                        pLogCtx,                            \
                                        protoVer,                           \
                                        usOpcode);                          \
        if (maxLogLevel >= (level))                                         \
        {                                                                   \
            LW_RTL_LOG_AT_LEVEL((level), "srv", szFmt, ## __VA_ARGS__);     \
        }                                                                   \
    }

#define SRV_LOG_ALWAYS(pLogCtx, protoVer, usOpcode, szFmt, ...) \
        _SRV_LOG_IF(LWIO_LOG_LEVEL_ALWAYS, \
                    pLogCtx,  \
                    protoVer, \
                    usOpcode, \
                    szFmt,    \
                    ## __VA_ARGS__)

#define SRV_LOG_ERROR(pLogCtx, protoVer, usOpcode, szFmt, ...) \
        _SRV_LOG_IF(LWIO_LOG_LEVEL_ERROR, \
                    pLogCtx,  \
                    protoVer, \
                    usOpcode, \
                    szFmt,    \
                    ## __VA_ARGS__)

#define SRV_LOG_WARNING(pLogCtx, protoVer, usOpcode, szFmt, ...) \
        _SRV_LOG_IF(LWIO_LOG_LEVEL_WARNING, \
                    pLogCtx,  \
                    protoVer, \
                    usOpcode, \
                    szFmt,    \
                    ## __VA_ARGS__)

#define SRV_LOG_INFO(pLogCtx, protoVer, usOpcode, szFmt, ...) \
        _SRV_LOG_IF(LWIO_LOG_LEVEL_INFO, \
                    pLogCtx,  \
                    protoVer, \
                    usOpcode, \
                    szFmt,    \
                    ## __VA_ARGS__)

#define SRV_LOG_VERBOSE(pLogCtx, protoVer, usOpcode, szFmt, ...) \
        _SRV_LOG_IF(LWIO_LOG_LEVEL_VERBOSE, \
                    pLogCtx,  \
                    protoVer, \
                    usOpcode, \
                    szFmt,    \
                    ## __VA_ARGS__)

#define SRV_LOG_DEBUG(pLogCtx, protoVer, usOpcode, szFmt, ...) \
        _SRV_LOG_IF(LWIO_LOG_LEVEL_DEBUG, \
                    pLogCtx,  \
                    protoVer, \
                    usOpcode, \
                    szFmt,    \
                    ## __VA_ARGS__)

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

typedef struct _SRV_LOG_CONTEXT* PSRV_LOG_CONTEXT;

typedef VOID (*PFN_SRV_LOG_HANDLER)(
                    PSRV_LOG_CONTEXT  pLogContext,
                    LWIO_LOG_LEVEL    logLevel,
                    PCSTR             pszFunction,
                    PCSTR             pszFile,
                    ULONG             ulLine,
                    ...
                    );

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
SrvBuildFilePathString(
    IN PWSTR pwszPrefix,
    IN PWSTR pwszSuffix,
    OUT PWSTR* ppwszFilename
    );

NTSTATUS
SrvBuildFilePath(
    IN PWSTR pwszPrefix,
    IN PWSTR pwszSuffix,
    OUT PUNICODE_STRING pFilename
    );

NTSTATUS
SrvGetParentPath(
    IN PUNICODE_STRING pPath,
    OUT PUNICODE_STRING pParentPath
    );

PCSTR
SrvPathGetFileName(
    PCSTR  pszPath
    );

NTSTATUS
SrvMatchPathPrefix(
    PWSTR pwszPath,
    ULONG ulPathLength,
    PWSTR pwszPrefix
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
SrvUnicodeStringToMbs(
    IN PUNICODE_STRING pString,
    OUT PSTR* ppszString
    );

NTSTATUS
SrvAllocateStringW(
    IN  PWSTR  pwszInputString,
    OUT PWSTR* ppwszOutputString
    );

NTSTATUS
SrvAllocateString(
    IN  PSTR  pszInputString,
    OUT PSTR* ppszOutputString
    );

NTSTATUS
SrvAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    );

NTSTATUS
SrvInitializeUnicodeString(
    IN PCWSTR pwszInputString,
    OUT PUNICODE_STRING pOutputString
    );

NTSTATUS
SrvAllocateUnicodeString(
    IN PUNICODE_STRING pInputString,
    OUT PUNICODE_STRING pOutputString
    );

NTSTATUS
SrvAllocateUnicodeStringW(
    IN PCWSTR pwszInputString,
    OUT PUNICODE_STRING pOutputString
    );

VOID
SrvFreeUnicodeString(
    IN PUNICODE_STRING pString
    );

#define SRV_FREE_UNICODE_STRING(pString) \
    do { \
        if ((pString)->Buffer) \
        { \
            SrvFreeUnicodeString(pString); \
        } \
    } while (0)

NTSTATUS
SrvGetHexDump(
    PBYTE  pBuffer,
    ULONG  ulBufLen,
    ULONG  ulMaxLength,
    PSTR*  ppszHexString,
    PULONG pulHexStringLength
    );

VOID
SrvProdConsFreeContents(
    PSMB_PROD_CONS_QUEUE pQueue
    );

NTSTATUS
SrvSocketAddressToStringW(
    const struct sockaddr* pSocketAddress,
    PWSTR*           ppwszAddress
    );

NTSTATUS
SrvSocketAddressToString(
    const struct sockaddr* pSocketAddress, /* IN     */
    PSTR             pszAddress,     /*    OUT */
    ULONG            ulAddressLength /* IN     */
    );

NTSTATUS
SrvSocketStringToAddressA(
    PCSTR            pszAddress,
    struct sockaddr** ppSocketAddress,
    SOCKLEN_TYPE*     pAddressLength
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
    SOCKLEN_TYPE           addrLength1,
    const struct sockaddr* pAddress2,
    SOCKLEN_TYPE           addrLength2,
    PBOOLEAN               pbMatch
    );

NTSTATUS
SrvLogContextCreate(
    PSRV_LOG_CONTEXT* ppLogContext
    );

NTSTATUS
SrvLogContextUpdateFilter(
    PSRV_LOG_CONTEXT pLogContext,
    const struct sockaddr* pClientAddress,
    SOCKLEN_TYPE     ulClientAddressLength
    );

LWIO_LOG_LEVEL
SrvLogContextGetLevel(
    PSRV_LOG_CONTEXT pLogContext,
    ULONG            protocolVer,
    USHORT           usOpcode
    );

ULONG
SrvLogContextGetMaxLogLength(
    PSRV_LOG_CONTEXT pLogContext
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

