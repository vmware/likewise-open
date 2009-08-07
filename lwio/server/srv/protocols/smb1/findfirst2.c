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
SrvUnmarshallFindFirst2Params(
    PBYTE           pParams,
    USHORT          usBytesAvailable,
    USHORT          usParameterOffset,
    PUSHORT         pusSearchAttrs,
    PUSHORT         pusSearchCount,
    PUSHORT         pusFlags,
    PSMB_INFO_LEVEL pSmbInfoLevel,
    PULONG          pulSearchStorageType,
    PWSTR*          ppwszSearchPattern
    );

static
NTSTATUS
SrvBuildFindFirst2Response(
    PSRV_EXEC_CONTEXT   pExecContext,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    SMB_INFO_LEVEL      infoLevel,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    USHORT              usMaxDataCount
    );

NTSTATUS
SrvProcessTrans2FindFirst2(
    IN  PSRV_EXEC_CONTEXT           pExecContext,
    IN  PTRANSACTION_REQUEST_HEADER pRequestHeader,
    IN  PUSHORT                     pSetup,
    IN  PUSHORT                     pByteCount,
    IN  PBYTE                       pParameters,
    IN  PBYTE                       pData
    )
{
    NTSTATUS ntStatus = 0;
    USHORT         usSearchAttrs = 0;
    USHORT         usSearchCount = 0;
    USHORT         usFlags = 0;
    SMB_INFO_LEVEL infoLevel = 0;
    ULONG          ulSearchStorageType = 0;
    PWSTR          pwszSearchPattern = NULL; // Do not free

    ntStatus = SrvUnmarshallFindFirst2Params(
                    pParameters,
                    pRequestHeader->parameterCount,
                    pRequestHeader->parameterOffset,
                    &usSearchAttrs,
                    &usSearchCount,
                    &usFlags,
                    &infoLevel,
                    &ulSearchStorageType,
                    &pwszSearchPattern);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildFindFirst2Response(
                    pExecContext,
                    usSearchAttrs,
                    usSearchCount,
                    usFlags,
                    infoLevel,
                    ulSearchStorageType,
                    pwszSearchPattern,
                    pRequestHeader->maxDataCount);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvUnmarshallFindFirst2Params(
    PBYTE           pParams,
    USHORT          usBytesAvailable,
    USHORT          usParameterOffset,
    PUSHORT         pusSearchAttrs,
    PUSHORT         pusSearchCount,
    PUSHORT         pusFlags,
    PSMB_INFO_LEVEL pInfoLevel,
    PULONG          pulSearchStorageType,
    PWSTR*          ppwszSearchPattern
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pDataCursor = pParams;
    USHORT   usSearchAttrs = 0;
    USHORT   usSearchCount = 0;
    USHORT   usFlags = 0;
    SMB_INFO_LEVEL infoLevel = 0;
    ULONG    ulSearchStorageType = 0;
    PWSTR    pwszSearchPattern = NULL;
    USHORT   usAlignment = 0;

    // TODO: Is this necessary?
    //
    usAlignment = usParameterOffset % 2;

    if (usBytesAvailable < usAlignment)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pDataCursor += usAlignment;
    usParameterOffset += usAlignment;
    usBytesAvailable -= usAlignment;

    if (usBytesAvailable < sizeof(usSearchAttrs))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usSearchAttrs = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usSearchAttrs);
    usParameterOffset += sizeof(usSearchAttrs);
    usBytesAvailable -= sizeof(usSearchAttrs);

    if (usBytesAvailable < sizeof(usSearchCount))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usSearchCount = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usSearchCount);
    usParameterOffset += sizeof(usSearchCount);
    usBytesAvailable -= sizeof(usSearchCount);

    if (usBytesAvailable < sizeof(usFlags))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usFlags = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usFlags);
    usParameterOffset += sizeof(usFlags);
    usBytesAvailable -= sizeof(usFlags);

    if (usBytesAvailable < sizeof(infoLevel))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    infoLevel = *((PSMB_INFO_LEVEL)pDataCursor);
    pDataCursor += sizeof(SMB_INFO_LEVEL);
    usParameterOffset += sizeof(SMB_INFO_LEVEL);
    usBytesAvailable -= sizeof(SMB_INFO_LEVEL);

    if (usBytesAvailable < sizeof(ulSearchStorageType))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulSearchStorageType = *((PULONG)pDataCursor);
    pDataCursor += sizeof(ulSearchStorageType);
    usParameterOffset += sizeof(ulSearchStorageType);
    usBytesAvailable -= sizeof(ulSearchStorageType);

    if (usBytesAvailable)
    {
        pwszSearchPattern = (PWSTR)pDataCursor;
    }

    *pusSearchAttrs = usSearchAttrs;
    *pusSearchCount = usSearchCount;
    *pusFlags = usFlags;
    *pInfoLevel = infoLevel;
    *pulSearchStorageType = ulSearchStorageType;
    *ppwszSearchPattern = pwszSearchPattern;

cleanup:

    return ntStatus;

error:

    *pusSearchAttrs = 0;
    *pusSearchCount = 0;
    *pusFlags = 0;
    *pInfoLevel = 0;
    *pulSearchStorageType = 0;
    *ppwszSearchPattern = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvBuildFindFirst2Response(
    PSRV_EXEC_CONTEXT   pExecContext,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    SMB_INFO_LEVEL      infoLevel,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    USHORT              usMaxDataCount
    )
{
    NTSTATUS          ntStatus = 0;
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
    PLWIO_SRV_TREE    pTree = NULL;
    SMB_FIND_FIRST2_RESPONSE_PARAMETERS responseParams = {0};
    BOOLEAN   bReturnSingleEntry = FALSE;
    BOOLEAN   bRestartScan = FALSE;
    wchar16_t wszBackSlash[] = {'\\', 0};
    USHORT    usDataOffset = 0;
    USHORT    usParameterOffset = 0;
    ULONG     usBytesAvailable = 0;
    USHORT    usSearchResultLen = 0;
    USHORT    usSearchId = 0;
    PBYTE     pData = NULL;
    HANDLE    hSearchSpace = NULL;
    PUSHORT   pSetup = NULL;
    BYTE      setupCount = 0;
    BOOLEAN   bEndOfSearch = FALSE;
    BOOLEAN   bInLock = FALSE;
    PWSTR     pwszFilesystemPath = NULL;
    PWSTR     pwszSearchPattern2 = NULL;

    ntStatus = SrvConnectionFindSession_SMB_V1(
                    pCtxSmb1,
                    pConnection,
                    pSmbRequest->pHeader->uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionFindTree_SMB_V1(
                    pCtxSmb1,
                    pSession,
                    pSmbRequest->pHeader->tid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pwszSearchPattern && *pwszSearchPattern == wszBackSlash[0])
    {
        pwszSearchPattern++;
    }

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->pShareInfo->mutex);

    ntStatus = SrvFinderBuildSearchPath(
                    pTree->pShareInfo->pwszPath,
                    pwszSearchPattern,
                    &pwszFilesystemPath,
                    &pwszSearchPattern2);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    ntStatus = SrvFinderCreateSearchSpace(
                    pSession->pIoSecurityContext,
                    pSession->hFinderRepository,
                    pwszFilesystemPath,
                    pwszSearchPattern2,
                    usSearchAttrs,
                    ulSearchStorageType,
                    infoLevel,
                    (pSmbRequest->pHeader->flags2 & FLAG2_KNOWS_LONG_NAMES ? TRUE : FALSE),
                    &hSearchSpace,
                    &usSearchId);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_TRANSACTION2,
                    STATUS_SUCCESS,
                    TRUE,
                    pSmbRequest->pHeader->tid,
                    pSmbRequest->pHeader->pid,
                    pCtxSmb1->pSession->uid,
                    pSmbRequest->pHeader->mid,
                    pConnection->serverProperties.bRequireSecuritySignatures,
                    &pSmbResponse->pHeader,
                    &pSmbResponse->pAndXHeader,
                    &pSmbResponse->usHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    pSmbResponse->pHeader->wordCount = 10 + setupCount;

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

    usBytesAvailable =
        SMB_MIN(usMaxDataCount,
                pConnection->serverProperties.MaxBufferSize - usBytesUsed);

    ntStatus = SrvFinderGetSearchResults(
                    hSearchSpace,
                    bReturnSingleEntry,
                    bRestartScan,
                    usSearchCount,
                    usBytesAvailable,
                    usDataOffset,
                    &pData,
                    &usSearchResultLen,
                    &responseParams.usSearchCount,
                    &bEndOfSearch);
    BAIL_ON_NT_STATUS(ntStatus);

    responseParams.usSearchId = usSearchId;

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
        SrvFinderReleaseSearchSpace(hSearchSpace);

        if ((usFlags & SMB_FIND_CLOSE_AFTER_REQUEST) ||
            (bEndOfSearch && (usFlags & SMB_FIND_CLOSE_IF_EOS)))
        {
            SrvFinderCloseSearchSpace(
                pSession->hFinderRepository,
                usSearchId);
        }
    }

    if (pTree)
    {
        LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pData)
    {
        SrvFreeMemory(pData);
    }
    if (pwszFilesystemPath)
    {
        SrvFreeMemory(pwszFilesystemPath);
    }
    if (pwszSearchPattern2)
    {
        SrvFreeMemory(pwszSearchPattern2);
    }

    return ntStatus;

error:

    if (ntStatus == STATUS_NO_MORE_MATCHES)
    {
        ntStatus = STATUS_NO_SUCH_FILE;
    }

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->pAndXHeader = NULL;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

