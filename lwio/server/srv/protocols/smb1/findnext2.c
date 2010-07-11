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
NTSTATUS
SrvUnmarshallFindNext2Params(
    PBYTE           pParameters,
    USHORT          usParameterCount,
    USHORT          usParameterOffset,
    PUSHORT         pusSearchId,
    PUSHORT         pusSearchCount,
    PUSHORT         pusFlags,
    SMB_INFO_LEVEL* pInfoLevel,
    PULONG          pulResumeHandle,
    PWSTR*          ppwszResumeFilename
    );

static
VOID
SrvLogFindNext2Params_SMB_V1(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    );

static
NTSTATUS
SrvBuildFindNext2Response(
    PSRV_EXEC_CONTEXT pExecContext,
    USHORT            usSearchId,
    USHORT            usSearchCount,
    USHORT            usFlags,
    SMB_INFO_LEVEL    infoLevel,
    ULONG             ulResumeHandle,
    PWSTR             pwszResumeFilename,
    USHORT            usMaxDataCount
    );

NTSTATUS
SrvProcessTrans2FindNext2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    USHORT         usSearchId = 0;
    USHORT         usSearchCount = 0;
    ULONG          ulResumeHandle = 0;
    USHORT         usFlags = 0;
    SMB_INFO_LEVEL infoLevel = 0;
    PWSTR          pwszResumeFilename = NULL; // Do not free
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    ntStatus = SrvUnmarshallFindNext2Params(
                    pTrans2State->pParameters,
                    pTrans2State->pRequestHeader->parameterCount,
                    pTrans2State->pRequestHeader->parameterOffset,
                    &usSearchId,
                    &usSearchCount,
                    &usFlags,
                    &infoLevel,
                    &ulResumeHandle,
                    &pwszResumeFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    SRV_LOG_CALL_DEBUG(
            pExecContext->pLogContext,
            SMB_PROTOCOL_VERSION_1,
            pCtxSmb1->pRequests[pCtxSmb1->iMsg].pHeader->command,
            &SrvLogFindNext2Params_SMB_V1,
            &usSearchId,
            &usSearchCount,
            &usFlags,
            &infoLevel,
            &ulResumeHandle,
            pwszResumeFilename);

    ntStatus = SrvBuildFindNext2Response(
                    pExecContext,
                    usSearchId,
                    usSearchCount,
                    usFlags,
                    infoLevel,
                    ulResumeHandle,
                    pwszResumeFilename,
                    pTrans2State->pRequestHeader->maxDataCount);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvUnmarshallFindNext2Params(
    PBYTE           pParameters,
    USHORT          usBytesAvailable,
    USHORT          usParameterOffset,
    PUSHORT         pusSearchId,
    PUSHORT         pusSearchCount,
    PUSHORT         pusFlags,
    SMB_INFO_LEVEL* pInfoLevel,
    PULONG          pulResumeHandle,
    PWSTR*          ppwszResumeFilename
    )
{
    NTSTATUS       ntStatus = 0;
    PBYTE          pDataCursor = pParameters;
    USHORT         usAlignment = 0;
    USHORT         usSearchId = 0;
    USHORT         usSearchCount = 0;
    USHORT         usFlags = 0;
    SMB_INFO_LEVEL infoLevel = 0;
    ULONG          ulResumeHandle = 0;
    PWSTR          pwszResumeFilename = NULL;

    // TODO: Is this necessary?
    //
    usAlignment = usParameterOffset % 2;

    if (usBytesAvailable < usAlignment)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usSearchId = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usSearchId);

    if (usBytesAvailable < sizeof(usSearchCount))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usSearchCount = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usSearchCount);

    if (usBytesAvailable < sizeof(infoLevel))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    infoLevel = *((SMB_INFO_LEVEL*)pDataCursor);
    pDataCursor += sizeof(infoLevel);

    if (usBytesAvailable < sizeof(ulResumeHandle))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulResumeHandle = *((PULONG)pDataCursor);
    pDataCursor += sizeof(ulResumeHandle);

    if (usBytesAvailable < sizeof(usFlags))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usFlags = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usFlags);

    if (!usBytesAvailable)
    {
        if (!(usFlags & SMB_FIND_CONTINUE_SEARCH))
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }
    else
    {
        pwszResumeFilename = (PWSTR)pDataCursor;

        if (wc16slen(pwszResumeFilename) > 256 * sizeof(wchar16_t))
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    *pusSearchId = usSearchId;
    *pusSearchCount = usSearchCount;
    *pusFlags = usFlags;
    *pulResumeHandle = ulResumeHandle;
    *ppwszResumeFilename = pwszResumeFilename;

cleanup:

    return ntStatus;

error:

    *pusSearchId = 0;
    *pusSearchCount = 0;
    *pusFlags = 0;
    *pulResumeHandle = 0;
    *ppwszResumeFilename = NULL;

    goto cleanup;
}

static
VOID
SrvLogFindNext2Params_SMB_V1(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSHORT         pusSearchId    = 0;
    PUSHORT         pusSearchCount = 0;
    PUSHORT         pusFlags       = 0;
    PSMB_INFO_LEVEL pinfoLevel     = 0;
    PULONG          pulResumeHandle = 0;
    PWSTR           pwszResumeFilename   = NULL; // Do not free
    PSTR            pszResumeFilename = NULL;
    va_list         msgList;

    va_start(msgList, ulLine);
    pusSearchId    = (PUSHORT)va_arg(msgList, PUSHORT);
    pusSearchCount = (PUSHORT)va_arg(msgList, PUSHORT);
    pusFlags       = (PUSHORT)va_arg(msgList, PUSHORT);
    pinfoLevel     = (PSMB_INFO_LEVEL)va_arg(msgList, PSMB_INFO_LEVEL);
    pulResumeHandle = (PULONG)va_arg(msgList, PULONG);
    pwszResumeFilename = (PWSTR)va_arg(msgList, PWSTR);

    if (pwszResumeFilename)
    {
        ntStatus = SrvWc16sToMbs(pwszResumeFilename, &pszResumeFilename);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (logLevel >= LWIO_LOG_LEVEL_DEBUG)
    {
        LWIO_LOG_ALWAYS_CUSTOM(
                logLevel,
                "[%s() %s:%u] FindFirst2 Parameters: SearchId(%u),SearchCount(%u),Flags(0x%x),InfoLevel(%u),ResumeHandle(%u),ResumeFilename(%s)",
                LWIO_SAFE_LOG_STRING(pszFunction),
                LWIO_SAFE_LOG_STRING(pszFile),
                ulLine,
                *pusSearchId,
                *pusSearchCount,
                *pusFlags,
                *pinfoLevel,
                *pulResumeHandle,
                LWIO_SAFE_LOG_STRING(pszResumeFilename));
    }
    else
    {
        LWIO_LOG_ALWAYS_CUSTOM(
                logLevel,
                "FindFirst2 Parameters: SearchId(%u),SearchCount(%u),Flags(0x%x),InfoLevel(%u),ResumeHandle(%u),ResumeFilename(%s)",
                *pusSearchId,
                *pusSearchCount,
                *pusFlags,
                *pinfoLevel,
                *pulResumeHandle,
                LWIO_SAFE_LOG_STRING(pszResumeFilename));
    }

error:

    va_end(msgList);

    SRV_SAFE_FREE_MEMORY(pszResumeFilename);

    return;
}


static
NTSTATUS
SrvBuildFindNext2Response(
    PSRV_EXEC_CONTEXT pExecContext,
    USHORT            usSearchId,
    USHORT            usSearchCount,
    USHORT            usFlags,
    SMB_INFO_LEVEL    infoLevel,
    ULONG             ulResumeHandle,
    PWSTR             pwszResumeFilename,
    USHORT            usMaxDataCount
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    USHORT usBytesUsed          = 0;
    ULONG ulTotalBytesUsed     = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    HANDLE           hSearchSpace = NULL;
    BOOLEAN          bEndOfSearch = FALSE;
    BOOLEAN          bReturnSingleEntry = FALSE;
    BOOLEAN          bRestartScan = FALSE;
    PBYTE            pData = 0;
    USHORT           usSearchResultLen = 0;
    USHORT           usDataOffset = 0;
    USHORT           usParameterOffset = 0;
    PUSHORT          pSetup = NULL;
    BYTE             setupCount = 0;
    SMB_FIND_NEXT2_RESPONSE_PARAMETERS responseParams = {0};

    ntStatus = SrvConnectionFindSession_SMB_V1(
                    pCtxSmb1,
                    pConnection,
                    pSmbRequest->pHeader->uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSetStatSessionInfo(pExecContext, pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvFinderGetSearchSpace(
                    pSession->hFinderRepository,
                    usSearchId,
                    &hSearchSpace);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION2,
                        STATUS_SUCCESS,
                        TRUE,
                        pSmbRequest->pHeader->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pCtxSmb1->pSession->uid,
                        pSmbRequest->pHeader->mid,
                        pConnection->serverProperties.bRequireSecuritySignatures,
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
                        COM_TRANSACTION2,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 10 + setupCount;

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    (PBYTE)&responseParams,
                    sizeof(responseParams),
                    NULL,
                    0,
                    &usDataOffset,
                    &usParameterOffset,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvFinderGetSearchResults(
                    hSearchSpace,
                    bReturnSingleEntry,
                    bRestartScan,
                    usSearchCount,
                    usMaxDataCount,
                    usDataOffset,
                    &pData,
                    &usSearchResultLen,
                    &responseParams.usSearchCount,
                    &bEndOfSearch);

    if (ntStatus == STATUS_NO_MORE_MATCHES)
    {
        ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    if (bEndOfSearch || (usFlags & SMB_FIND_CLOSE_AFTER_REQUEST))
    {
        responseParams.usEndOfSearch = 1;
    }

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    (PBYTE)&responseParams,
                    sizeof(responseParams),
                    pData,
                    usSearchResultLen,
                    &usDataOffset,
                    &usParameterOffset,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    if (hSearchSpace)
    {
        if ((usFlags & SMB_FIND_CLOSE_AFTER_REQUEST) ||
            (bEndOfSearch && (usFlags & SMB_FIND_CLOSE_IF_EOS)))
        {
            SrvFinderCloseSearchSpace(
                    pSession->hFinderRepository,
                    usSearchId);
        }

        SrvFinderReleaseSearchSpace(hSearchSpace);
    }

    if (pData)
    {
        SrvFreeMemory(pData);
    }

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


