/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        statistics.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Reference Statistics Logging Module (SRV)
 *
 *        Logging functions
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
LwioSrvStatPopMessage_inlock(
    HANDLE   hContext,            /* IN              */
    ULONG    ulOpCode,            /* IN              */
    ULONG    ulResponseLength,    /* IN              */
    NTSTATUS msgStatus            /* IN              */
    );

static
PSRV_STAT_MESSAGE_CONTEXT
LwioSrvStatReverseMessageStack(
    PSRV_STAT_MESSAGE_CONTEXT pMsgContext
    );

static
NTSTATUS
LwioSrvStatLogStatistics_inlock(
    PSRV_STAT_REQUEST_CONTEXT pStatContext
    );

static
NTSTATUS
LwioSrvStatLogContextHeader(
    PSRV_STAT_REQUEST_CONTEXT pStatContext,
    PSTR*                     ppszBuffer,
    PULONG                    pulTotalLength,
    PULONG                    pulBytesUsed
    );

static
NTSTATUS
LwioSrvStatLogMessageContext(
    PSRV_STAT_MESSAGE_CONTEXT pMessageContext,
    PSTR*                     ppszBuffer,
    PULONG                    pulTotalLength,
    PULONG                    pulBytesUsed
    );

static
NTSTATUS
LwioSrvStatLogContextTrailer(
    PSRV_STAT_REQUEST_CONTEXT pStatContext,
    PSTR*                     ppszBuffer,
    PULONG                    pulTotalLength,
    PULONG                    pulBytesUsed
    );

static
NTSTATUS
LwioSrvStatLogToString(
    PCSTR                   pszPrefix,
    PCSTR                   pszName,
    PSRV_STAT_HANDLER_VALUE pValue,
    PCSTR                   pszSuffix,
    PSTR*                   ppszBuffer,
    PULONG                  pulTotalLength,
    PULONG                  pulBytesUsed
    );

static
NTSTATUS
LwioSrvStatSocketAddressToString(
    struct sockaddr* pSocketAddress, /* IN     */
    PSTR             pszAddress,     /*    OUT */
    ULONG            ulAddressLength /* IN     */
    );

static
VOID
LwioSrvStatFreeContext(
    PSRV_STAT_REQUEST_CONTEXT pStatContext
    );

static
VOID
LwioSrvStatFreeMessageStack(
    PSRV_STAT_MESSAGE_CONTEXT pMsgContext
    );

static
VOID
LwioSrvStatFreeMessageContext(
    PSRV_STAT_MESSAGE_CONTEXT pMsgContext
    );

NTSTATUS
LwioSrvStatCreateRequestContext(
    PSRV_STAT_CONNECTION_INFO  pConnection,        /* IN              */
    SRV_STAT_SMB_VERSION       protocolVersion,    /* IN              */
    ULONG                      ulRequestLength,    /* IN              */
    PHANDLE                    phContext           /*    OUT          */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = NULL;

    BAIL_ON_INVALID_POINTER(pConnection);
    BAIL_ON_INVALID_POINTER(phContext);

    ntStatus = RTL_ALLOCATE(
                    &pContext,
                    SRV_STAT_REQUEST_CONTEXT,
                    sizeof(SRV_STAT_REQUEST_CONTEXT));
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pContext->mutex, NULL);
    pContext->pMutex = &pContext->mutex;

    pContext->protocolVersion = protocolVersion;
    pContext->ulRequestLength = ulRequestLength;

    pContext->connInfo.ulResourceId  = pConnection->ulResourceId;
    pContext->connInfo.clientAddress = pConnection->clientAddress;
    pContext->connInfo.clientAddrLen = pConnection->clientAddrLen;
    pContext->connInfo.serverAddress = pConnection->serverAddress;
    pContext->connInfo.serverAddrLen = pConnection->serverAddrLen;

    ntStatus = LwioSrvStatGetCurrentNTTime(&pContext->llRequestStartTime);
    BAIL_ON_NT_STATUS(ntStatus);

    *phContext = (HANDLE)pContext;

cleanup:

    return ntStatus;

error:

    if (phContext)
    {
        *phContext = NULL;
    }

    if (pContext)
    {
        LwioSrvStatCloseRequestContext(pContext);
    }

    goto cleanup;
}

NTSTATUS
LwioSrvStatPushMessage(
    HANDLE hContext,     /* IN              */
    ULONG  ulOpcode,     /* IN              */
    ULONG  ulMessageLen  /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;
    PSRV_STAT_MESSAGE_CONTEXT pMsgContext = NULL;
    BOOLEAN  bInLock = FALSE;

    BAIL_ON_INVALID_POINTER(pContext);

    SRV_STAT_HANDLER_LOCK_MUTEX(bInLock, &pContext->mutex);

    if (pContext->pCurrentMessage)
    {
        ntStatus = LwioSrvStatPopMessage_inlock(
                        hContext,
                        pContext->pCurrentMessage->ulOpcode,
                        0,
                        STATUS_SUCCESS);
        BAIL_ON_NT_STATUS(ntStatus);

        pContext->pCurrentMessage = NULL;
    }

    ntStatus = RTL_ALLOCATE(
                    &pMsgContext,
                    SRV_STAT_MESSAGE_CONTEXT,
                    sizeof(SRV_STAT_MESSAGE_CONTEXT));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwioSrvStatGetCurrentNTTime(&pMsgContext->llMsgStartTime);
    BAIL_ON_NT_STATUS(ntStatus);

    pMsgContext->ulOpcode = ulOpcode;
    pMsgContext->ulMessageRequestLength = ulMessageLen;

    if (!pContext->pMessageStack)
    {
        pContext->pMessageStack = pMsgContext;
    }
    else
    {
        pMsgContext->pNext = pContext->pMessageStack;
        pContext->pMessageStack = pMsgContext;
    }
    pContext->pCurrentMessage = pContext->pMessageStack;

cleanup:

    SRV_STAT_HANDLER_UNLOCK_MUTEX(bInLock, &pContext->mutex);

    return ntStatus;

error:

    if (pMsgContext)
    {
        LwioSrvStatFreeMessageContext(pMsgContext);
    }

    goto cleanup;
}

NTSTATUS
LwioSrvStatSetSubOpcode(
    HANDLE hContext,            /* IN              */
    ULONG  ulSubOpcode          /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;
    BOOLEAN  bInLock = FALSE;

    BAIL_ON_INVALID_POINTER(pContext);

    SRV_STAT_HANDLER_LOCK_MUTEX(bInLock, &pContext->mutex);

    if (!pContext->pCurrentMessage)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pContext->pCurrentMessage->ulSubOpcode = ulSubOpcode;

    LwSetFlag(pContext->pCurrentMessage->ulFlags, SRV_STAT_FLAG_SUB_OPCODE_SET);

cleanup:

    SRV_STAT_HANDLER_UNLOCK_MUTEX(bInLock, &pContext->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
LwioSrvStatSetIOCTL(
    HANDLE hContext,            /* IN              */
    ULONG  ulIoCtlCode          /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;
    BOOLEAN  bInLock = FALSE;

    BAIL_ON_INVALID_POINTER(pContext);

    SRV_STAT_HANDLER_LOCK_MUTEX(bInLock, &pContext->mutex);

    if (!pContext->pCurrentMessage)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pContext->pCurrentMessage->ulIOCTLcode = ulIoCtlCode;

    LwSetFlag(pContext->pCurrentMessage->ulFlags, SRV_STAT_FLAG_IOCTL_SET);

cleanup:

    SRV_STAT_HANDLER_UNLOCK_MUTEX(bInLock, &pContext->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
LwioSrvStatSetSessionInfo(
    HANDLE                 hContext,            /* IN              */
    PSRV_STAT_SESSION_INFO pSessionInfo         /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;
    BOOLEAN  bInLock = FALSE;

    BAIL_ON_INVALID_POINTER(pContext);

    SRV_STAT_HANDLER_LOCK_MUTEX(bInLock, &pContext->mutex);

    pContext->sessionInfo.ulUid        = pSessionInfo->ulUid;
    pContext->sessionInfo.ulGid        = pSessionInfo->ulGid;
    pContext->sessionInfo.ullSessionId = pSessionInfo->ullSessionId;

#if 0
    RTL_FREE(&pContext->sessionInfo.pwszUserPrincipal);
    if (pSessionInfo->pwszUserPrincipal)
    {
        ntStatus = RtlCStringDuplicate(
                        &pContext->sessionInfo.pwszUserPrincipal,
                        pSessionInfo->pwszUserPrincipal);
        BAIL_ON_NT_STATUS(ntStatus);
    }
#endif

cleanup:

    SRV_STAT_HANDLER_UNLOCK_MUTEX(bInLock, &pContext->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
LwioSrvStatPopMessage(
    HANDLE   hContext,            /* IN              */
    ULONG    ulOpCode,            /* IN              */
    ULONG    ulResponseLength,    /* IN              */
    NTSTATUS msgStatus            /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;
    BOOLEAN  bInLock  = FALSE;

    BAIL_ON_INVALID_POINTER(pContext);

    SRV_STAT_HANDLER_LOCK_MUTEX(bInLock, &pContext->mutex);

    ntStatus = LwioSrvStatPopMessage_inlock(
                    hContext,
                    ulOpCode,
                    ulResponseLength,
                    msgStatus);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SRV_STAT_HANDLER_UNLOCK_MUTEX(bInLock, &pContext->mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
LwioSrvStatPopMessage_inlock(
    HANDLE   hContext,            /* IN              */
    ULONG    ulOpCode,            /* IN              */
    ULONG    ulResponseLength,    /* IN              */
    NTSTATUS msgStatus            /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;

    BAIL_ON_INVALID_POINTER(pContext);

    if (!pContext->pCurrentMessage)
    {
        ntStatus = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pContext->pCurrentMessage->ulOpcode != ulOpCode)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LwioSrvStatGetCurrentNTTime(
                        &pContext->pCurrentMessage->llMsgEndTime);
    BAIL_ON_NT_STATUS(ntStatus);

    pContext->pCurrentMessage->ulMessageResponseLength = ulResponseLength;
    pContext->pCurrentMessage->responseStatus          = msgStatus;

    pContext->pCurrentMessage = NULL;

error:

    return ntStatus;
}

NTSTATUS
LwioSrvStatSetResponseInfo(
    HANDLE hContext,            /* IN              */
    ULONG  ulResponseLength     /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;
    BOOLEAN  bInLock  = FALSE;

    BAIL_ON_INVALID_POINTER(pContext);

    SRV_STAT_HANDLER_LOCK_MUTEX(bInLock, &pContext->mutex);

    pContext->ulResponseLength = ulResponseLength;

cleanup:

    SRV_STAT_HANDLER_UNLOCK_MUTEX(bInLock, &pContext->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
LwioSrvStatCloseRequestContext(
    HANDLE hContext            /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;
    BOOLEAN  bInLock = FALSE;

    BAIL_ON_INVALID_POINTER(pContext);

    SRV_STAT_HANDLER_LOCK_MUTEX(bInLock, &pContext->mutex);

    ntStatus = LwioSrvStatGetCurrentNTTime(&pContext->llRequestEndTime);
    BAIL_ON_NT_STATUS(ntStatus);

    pContext->pMessageStack = LwioSrvStatReverseMessageStack(
                                        pContext->pMessageStack);

    ntStatus = LwioSrvStatLogStatistics_inlock(pContext);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SRV_STAT_HANDLER_UNLOCK_MUTEX(bInLock, &pContext->mutex);

    if (pContext)
    {
        LwioSrvStatFreeContext(pContext);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
PSRV_STAT_MESSAGE_CONTEXT
LwioSrvStatReverseMessageStack(
    PSRV_STAT_MESSAGE_CONTEXT pMsgContext
    )
{
    PSRV_STAT_MESSAGE_CONTEXT pP = NULL;
    PSRV_STAT_MESSAGE_CONTEXT pQ = pMsgContext;
    PSRV_STAT_MESSAGE_CONTEXT pR = NULL;

    while (pQ)
    {
        pR = pQ->pNext;
        pQ->pNext = pP;
        pP = pQ;
        pQ = pR;
    }

    return pP;
}

static
NTSTATUS
LwioSrvStatLogStatistics_inlock(
    PSRV_STAT_REQUEST_CONTEXT pStatContext
    )
{
    NTSTATUS ntStatus         = STATUS_SUCCESS;
    PSTR     pszBuffer        = NULL;
    ULONG    ulTotalLength    = 0;
    ULONG    ulBytesUsed      = 0;
    PSRV_STAT_MESSAGE_CONTEXT pCursor = pStatContext->pMessageStack;

    if (gSrvStatHandlerGlobals.pLogger)
    {
        ntStatus = RTL_ALLOCATE(&pszBuffer, CHAR, 256);
        BAIL_ON_NT_STATUS(ntStatus);

        ulTotalLength = 256;

        ntStatus = LwioSrvStatLogContextHeader(
                        pStatContext,
                        &pszBuffer,
                        &ulTotalLength,
                        &ulBytesUsed);
        BAIL_ON_NT_STATUS(ntStatus);

        while (pCursor)
        {
            ntStatus = LwioSrvStatLogMessageContext(
                            pCursor,
                            &pszBuffer,
                            &ulTotalLength,
                            &ulBytesUsed);
            BAIL_ON_NT_STATUS(ntStatus);

            pCursor = pCursor->pNext;
        }

        ntStatus = LwioSrvStatLogContextTrailer(
                        pStatContext,
                        &pszBuffer,
                        &ulTotalLength,
                        &ulBytesUsed);
        BAIL_ON_NT_STATUS(ntStatus);

        LwioSrvStatLogMessage(gSrvStatHandlerGlobals.pLogger, "%s", pszBuffer);
    }

cleanup:

    RTL_FREE(&pszBuffer);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
LwioSrvStatLogContextHeader(
    PSRV_STAT_REQUEST_CONTEXT pStatContext,
    PSTR*                     ppszBuffer,
    PULONG                    pulTotalLength,
    PULONG                    pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_STAT_HANDLER_VALUE v;
    LONG64 llInterval = 0;

    v.valueType = SRV_STAT_HANDLER_VALUE_TYPE_PULONG;
    v.val.pulValue = &pStatContext->connInfo.ulResourceId;

    ntStatus = LwioSrvStatLogToString(
                    "(",
                    "connection-id",
                    &v,
                    NULL,
                    ppszBuffer,
                    pulTotalLength,
                    pulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    v.valueType = SRV_STAT_HANDLER_VALUE_TYPE_PSOCKADDR;
    v.val.pSockAddr = &pStatContext->connInfo.clientAddress;

    ntStatus = LwioSrvStatLogToString(
                    ",",
                    "client-address",
                    &v,
                    NULL,
                    ppszBuffer,
                    pulTotalLength,
                    pulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    v.val.pSockAddr = &pStatContext->connInfo.serverAddress;

    ntStatus = LwioSrvStatLogToString(
                    ",",
                    "server-address",
                    &v,
                    NULL,
                    ppszBuffer,
                    pulTotalLength,
                    pulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    v.valueType = SRV_STAT_HANDLER_VALUE_TYPE_PULONG;
    v.val.pulValue = &pStatContext->ulRequestLength;

    ntStatus = LwioSrvStatLogToString(
                    ",",
                    "request-length",
                    &v,
                    NULL,
                    ppszBuffer,
                    pulTotalLength,
                    pulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    v.val.pulValue = &pStatContext->ulResponseLength;

    ntStatus = LwioSrvStatLogToString(
                    ",",
                    "response-length",
                    &v,
                    NULL,
                    ppszBuffer,
                    pulTotalLength,
                    pulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    v.val.pulValue = &pStatContext->protocolVersion;

    ntStatus = LwioSrvStatLogToString(
                    ",",
                    "protocol-version",
                    &v,
                    NULL,
                    ppszBuffer,
                    pulTotalLength,
                    pulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    v.val.pulValue = &pStatContext->sessionInfo.ulUid;

    ntStatus = LwioSrvStatLogToString(
                    ",",
                    "user-id",
                    &v,
                    NULL,
                    ppszBuffer,
                    pulTotalLength,
                    pulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    llInterval = (pStatContext->llRequestEndTime -
                    pStatContext->llRequestStartTime) * 100;

    v.valueType = SRV_STAT_HANDLER_VALUE_TYPE_PLONG64;
    v.val.pllValue = &llInterval;

    ntStatus = LwioSrvStatLogToString(
                    ",",
                    "process-time-ns",
                    &v,
                    NULL,
                    ppszBuffer,
                    pulTotalLength,
                    pulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
LwioSrvStatLogMessageContext(
    PSRV_STAT_MESSAGE_CONTEXT pMessageContext,
    PSTR*                     ppszBuffer,
    PULONG                    pulTotalLength,
    PULONG                    pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_STAT_HANDLER_VALUE v;
    LONG64 llInterval = 0;

    v.valueType = SRV_STAT_HANDLER_VALUE_TYPE_PULONG;
    v.val.pulValue = &pMessageContext->ulOpcode;

    ntStatus = LwioSrvStatLogToString(
                    ",(",
                    "opcode",
                    &v,
                    NULL,
                    ppszBuffer,
                    pulTotalLength,
                    pulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    if (LwIsSetFlag(pMessageContext->ulFlags, SRV_STAT_FLAG_SUB_OPCODE_SET))
    {
        v.val.pulValue = &pMessageContext->ulSubOpcode;

        ntStatus = LwioSrvStatLogToString(
                        ",",
                        "sub-opcode",
                        &v,
                        NULL,
                        ppszBuffer,
                        pulTotalLength,
                        pulBytesUsed);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (LwIsSetFlag(pMessageContext->ulFlags, SRV_STAT_FLAG_IOCTL_SET))
    {
        v.val.pulValue = &pMessageContext->ulIOCTLcode;

        ntStatus = LwioSrvStatLogToString(
                        ",",
                        "ioctl-code",
                        &v,
                        NULL,
                        ppszBuffer,
                        pulTotalLength,
                        pulBytesUsed);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    v.val.pulValue = &pMessageContext->ulMessageRequestLength;

    ntStatus = LwioSrvStatLogToString(
                    ",",
                    "request-length",
                    &v,
                    NULL,
                    ppszBuffer,
                    pulTotalLength,
                    pulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    v.val.pulValue = &pMessageContext->ulMessageResponseLength;

    ntStatus = LwioSrvStatLogToString(
                    ",",
                    "response-length",
                    &v,
                    NULL,
                    ppszBuffer,
                    pulTotalLength,
                    pulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    v.valueType = SRV_STAT_HANDLER_VALUE_TYPE_PLONG;
    v.val.plValue = &pMessageContext->responseStatus;

    ntStatus = LwioSrvStatLogToString(
                    ",",
                    "status-code",
                    &v,
                    NULL,
                    ppszBuffer,
                    pulTotalLength,
                    pulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    llInterval = (pMessageContext->llMsgEndTime -
                    pMessageContext->llMsgStartTime) * 100;

    v.valueType = SRV_STAT_HANDLER_VALUE_TYPE_PLONG64;
    v.val.pllValue = &llInterval;

    ntStatus = LwioSrvStatLogToString(
                    ",",
                    "process-time-ns",
                    &v,
                    ")",
                    ppszBuffer,
                    pulTotalLength,
                    pulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
LwioSrvStatLogContextTrailer(
    PSRV_STAT_REQUEST_CONTEXT pStatContext,
    PSTR*                     ppszBuffer,
    PULONG                    pulTotalLength,
    PULONG                    pulBytesUsed
    )
{
    SRV_STAT_HANDLER_VALUE v =
    {
        .valueType    = SRV_STAT_HANDLER_VALUE_TYPE_PSTR,
        .val.pszValue = ""
    };

    return LwioSrvStatLogToString(
                    NULL,
                    ")",
                    &v,
                    NULL,
                    ppszBuffer,
                    pulTotalLength,
                    pulBytesUsed);
}

static
NTSTATUS
LwioSrvStatLogToString(
    PCSTR                   pszPrefix,
    PCSTR                   pszName,
    PSRV_STAT_HANDLER_VALUE pValue,
    PCSTR                   pszSuffix,
    PSTR*                   ppszBuffer,
    PULONG                  pulTotalLength,
    PULONG                  pulBytesUsed
    )
{
    NTSTATUS ntStatus      = STATUS_SUCCESS;

    do
    {
        PSTR   pszCursor  = *ppszBuffer + *pulBytesUsed;
        size_t nBytesAvbl = *pulTotalLength - *pulBytesUsed;
        size_t nWritten   = 0;

        switch (pValue->valueType)
        {
            case SRV_STAT_HANDLER_VALUE_TYPE_PLONG:

                nWritten = snprintf(
                                pszCursor,
                                nBytesAvbl,
                                "%s%s=%d%s",
                                (pszPrefix ? pszPrefix : ""),
                                pszName,
                                *pValue->val.plValue,
                                (pszSuffix ? pszSuffix : ""));

                break;

            case SRV_STAT_HANDLER_VALUE_TYPE_PULONG:

                nWritten = snprintf(
                                pszCursor,
                                nBytesAvbl,
                                "%s%s=%u%s",
                                (pszPrefix ? pszPrefix : ""),
                                pszName,
                                *pValue->val.pulValue,
                                (pszSuffix ? pszSuffix : ""));

                break;

            case SRV_STAT_HANDLER_VALUE_TYPE_PLONG64:

                nWritten = snprintf(
                                pszCursor,
                                nBytesAvbl,
                                "%s%s=%lld%s",
                                (pszPrefix ? pszPrefix : ""),
                                pszName,
                                *((long long *)pValue->val.pllValue),
                                (pszSuffix ? pszSuffix : ""));

                break;

            case SRV_STAT_HANDLER_VALUE_TYPE_PSTR:

                nWritten = snprintf(
                                pszCursor,
                                nBytesAvbl,
                                "%s%s=%s%s",
                                (pszPrefix ? pszPrefix : ""),
                                pszName,
                                pValue->val.pszValue,
                                (pszSuffix ? pszSuffix : ""));

                break;

            case SRV_STAT_HANDLER_VALUE_TYPE_PSOCKADDR:
                {
                    CHAR  szAddr[LWIO_SRV_STAT_SOCKET_ADDRESS_STRING_MAX_SIZE] = {0};

                    ntStatus = LwioSrvStatSocketAddressToString(
                                        pValue->val.pSockAddr,
                                        &szAddr[0],
                                        sizeof(szAddr));
                    BAIL_ON_NT_STATUS(ntStatus);

                    nWritten = snprintf(
                                    pszCursor,
                                    nBytesAvbl,
                                    "%s%s=%s%s",
                                    (pszPrefix ? pszPrefix : ""),
                                    pszName,
                                    &szAddr[0],
                                    (pszSuffix ? pszSuffix : ""));
                }

                break;

            default:

                ntStatus = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

        if (nWritten < 0)
        {
            ntStatus = LwErrnoToNtStatus(errno);
            if (ntStatus == STATUS_SUCCESS)
            {
                ntStatus = STATUS_INTERNAL_ERROR;
            }
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else if (nWritten >= nBytesAvbl) // not enough space
        {
            PSTR  pszBufferNew = NULL;
            PSTR  pszBuffer    = *ppszBuffer;
            ULONG ulIncrement  = 256;

            ntStatus = RTL_ALLOCATE(
                            &pszBufferNew,
                            CHAR,
                            *pulTotalLength + ulIncrement);
            BAIL_ON_NT_STATUS(ntStatus);

            memcpy(pszBufferNew, pszBuffer, *pulBytesUsed);

            if (*ppszBuffer)
            {
                RTL_FREE(ppszBuffer);
            }

            *ppszBuffer      = pszBufferNew;
            *pulTotalLength += ulIncrement;
        }
        else
        {
            *pulBytesUsed      += nWritten;
            // pszCursor        += nWritten;

            break;
        }

    } while (1);

error:

    return ntStatus;
}

static
NTSTATUS
LwioSrvStatSocketAddressToString(
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

static
VOID
LwioSrvStatFreeContext(
    PSRV_STAT_REQUEST_CONTEXT pStatContext
    )
{
    if (pStatContext->pMutex)
    {
        pthread_mutex_destroy(&pStatContext->mutex);
    }

    RTL_FREE(&pStatContext->sessionInfo.pwszUserPrincipal);

    if (pStatContext->pMessageStack)
    {
        LwioSrvStatFreeMessageStack(pStatContext->pMessageStack);
    }

    RTL_FREE(&pStatContext);
}

static
VOID
LwioSrvStatFreeMessageStack(
    PSRV_STAT_MESSAGE_CONTEXT pMsgContext
    )
{
    while (pMsgContext)
    {
        PSRV_STAT_MESSAGE_CONTEXT pItem = pMsgContext;

        pMsgContext = pMsgContext->pNext;

        LwioSrvStatFreeMessageContext(pItem);
    }
}

static
VOID
LwioSrvStatFreeMessageContext(
    PSRV_STAT_MESSAGE_CONTEXT pMsgContext
    )
{
    RTL_FREE(&pMsgContext);
}
