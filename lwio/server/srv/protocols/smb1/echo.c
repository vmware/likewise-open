/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "includes.h"

static
VOID
SrvLogEchoState_SMB_V1(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    );

static
NTSTATUS
SrvMarshallEchoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    PBYTE             pEchoBlob,
    USHORT            usEchoBlobLength
    );

NTSTATUS
SrvProcessEchoAndX(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
    // ULONG ulOffset         = pSmbRequest->usHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
    PECHO_REQUEST_HEADER pEchoHeader = NULL; // Do not Free
    PBYTE                pEchoBlob   = NULL; // Do Not Free

    ntStatus = WireUnmarshallEchoRequest(
                    pBuffer,
                    ulBytesAvailable,
                    &pEchoHeader,
                    &pEchoBlob);
    BAIL_ON_NT_STATUS(ntStatus);

    SRV_LOG_CALL_DEBUG(
            pExecContext->pLogContext,
            SMB_PROTOCOL_VERSION_1,
            pCtxSmb1->pRequests[pCtxSmb1->iMsg].pHeader->command,
            &SrvLogEchoState_SMB_V1,
            pEchoHeader,
            pEchoBlob);

    if (!pEchoHeader->echoCount)
    {
        // If echo count is zero, no response is sent
        goto cleanup;
    }

    // Always send back only one echo response
    ntStatus = SrvMarshallEchoResponse(
                        pExecContext,
                        pEchoBlob,
                        pEchoHeader->byteCount);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
VOID
SrvLogEchoState_SMB_V1(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PECHO_REQUEST_HEADER pEchoHeader = NULL; // Do not Free
    PBYTE                pEchoBlob   = NULL; // Do Not Free
    PSTR  pszHexString = NULL;
    ULONG ulLen = 0;
    va_list msgList;

    va_start(msgList, ulLine);

    pEchoHeader = va_arg(msgList, PECHO_REQUEST_HEADER);
    pEchoBlob   = va_arg(msgList, PBYTE);

    if (pEchoHeader)
    {
        if (pEchoHeader->byteCount)
        {
            ntStatus = SrvGetHexDump(
                            pEchoBlob,
                            pEchoHeader->byteCount,
                            SrvLogContextGetMaxLogLength(pLogContext),
                            &pszHexString,
                            &ulLen);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (logLevel >= LWIO_LOG_LEVEL_DEBUG)
        {
            LWIO_LOG_ALWAYS_CUSTOM(
                    logLevel,
                    "[%s() %s:%u] Delete directory state: EchoCount(%u),EchoBlob[%u/%u bytes](%s)",
                    LWIO_SAFE_LOG_STRING(pszFunction),
                    LWIO_SAFE_LOG_STRING(pszFile),
                    ulLine,
                    pEchoHeader->echoCount,
                    ulLen,
                    pEchoHeader->byteCount,
                    LWIO_SAFE_LOG_STRING(pszHexString));
        }
        else
        {
            LWIO_LOG_ALWAYS_CUSTOM(
                    logLevel,
                    "Delete directory state: EchoCount(%u),EchoBlob[%u/%u bytes](%s)",
                    pEchoHeader->echoCount,
                    ulLen,
                    pEchoHeader->byteCount,
                    LWIO_SAFE_LOG_STRING(pszHexString));
        }
    }

error:

    va_end(msgList);

    SRV_SAFE_FREE_MEMORY(pszHexString);

    return;
}

static
NTSTATUS
SrvMarshallEchoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    PBYTE             pEchoBlob,
    USHORT            usEchoBlobLength
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PECHO_RESPONSE_HEADER      pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    USHORT usBytesUsed          = 0;
    ULONG ulTotalBytesUsed     = 0;
    PCSTR    pMinEchoBlob = "lwio";

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_ECHO,
                        STATUS_SUCCESS,
                        TRUE,
                        pSmbRequest->pHeader->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pSmbRequest->pHeader->uid,
                        pSmbRequest->pHeader->mid,
                        FALSE,
                        &pSmbResponse->pHeader,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    else
    {
        ntStatus = SrvMarshalHeaderAndX_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_ECHO,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 1;

    if (ulBytesAvailable < sizeof(ECHO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PECHO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(ECHO_RESPONSE_HEADER);
    ulOffset         += sizeof(ECHO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(ECHO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(ECHO_RESPONSE_HEADER);

    ntStatus = WireMarshallEchoResponseData(
                    pOutBuffer,
                    ulBytesAvailable,
                    (usEchoBlobLength > 4 ? pEchoBlob : (PBYTE)pMinEchoBlob),
                    (usEchoBlobLength > 4 ? usEchoBlobLength : strlen(pMinEchoBlob)),
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponseHeader->byteCount = (USHORT)usBytesUsed;

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->pAndXHeader = NULL;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

