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

typedef struct _SMB_FIND_FIRST2_RESPONSE_PARAMETERS
{
    USHORT usSearchId;
    USHORT usSearchCount;
    USHORT usEndOfSearch;
    USHORT usEaErrorOffset;
    USHORT usLastNameOffset;

} __attribute__((__packed__)) SMB_FIND_FIRST2_RESPONSE_PARAMETERS, *PSMB_FIND_FIRST2_RESPONSE_PARAMETERS;

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
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    SMB_INFO_LEVEL      infoLevel,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessTrans2FindFirst2(
    PSMB_SRV_CONNECTION         pConnection,
    PSMB_PACKET                 pSmbRequest,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSMB_PACKET*                ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    USHORT         usSearchAttrs = 0;
    USHORT         usSearchCount = 0;
    USHORT         usFlags = 0;
    SMB_INFO_LEVEL infoLevel = 0;
    ULONG          ulSearchStorageType = 0;
    PWSTR          pwszSearchPattern = NULL; // Do not free
    PSMB_PACKET    pSmbResponse = NULL;

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
                    pConnection,
                    pSmbRequest,
                    usSearchAttrs,
                    usSearchCount,
                    usFlags,
                    infoLevel,
                    ulSearchStorageType,
                    pwszSearchPattern,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

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
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pDataCursor += usAlignment;
    usParameterOffset += usAlignment;
    usBytesAvailable -= usAlignment;

    if (usBytesAvailable < sizeof(usSearchAttrs))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usSearchAttrs = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usSearchAttrs);
    usParameterOffset += sizeof(usSearchAttrs);
    usBytesAvailable -= sizeof(usSearchAttrs);

    if (usBytesAvailable < sizeof(usSearchCount))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usSearchCount = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usSearchCount);
    usParameterOffset += sizeof(usSearchCount);
    usBytesAvailable -= sizeof(usSearchCount);

    if (usBytesAvailable < sizeof(usFlags))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usFlags = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usFlags);
    usParameterOffset += sizeof(usFlags);
    usBytesAvailable -= sizeof(usFlags);

    if (usBytesAvailable < sizeof(infoLevel))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    infoLevel = *((PSMB_INFO_LEVEL)pDataCursor);
    pDataCursor += sizeof(SMB_INFO_LEVEL);
    usParameterOffset += sizeof(SMB_INFO_LEVEL);
    usBytesAvailable -= sizeof(SMB_INFO_LEVEL);

    if (usBytesAvailable < sizeof(ulSearchStorageType))
    {
        ntStatus = STATUS_DATA_ERROR;
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
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    SMB_INFO_LEVEL      infoLevel,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_SESSION pSession = NULL;
    PSMB_SRV_TREE pTree = NULL;
    SMB_FIND_FIRST2_RESPONSE_PARAMETERS responseParams = {0};
    BOOLEAN   bReturnSingleEntry = FALSE;
    BOOLEAN   bRestartScan = FALSE;
    wchar16_t wszBackSlash[2];
    wchar16_t wszStar[2];
    USHORT    usDataOffset = 0;
    USHORT    usParameterOffset = 0;
    USHORT    usNumPackageBytesUsed = 0;
    ULONG     usBytesAvailable = 0;
    USHORT    usSearchResultLen = 0;
    USHORT    usSearchId = 0;
    PBYTE     pData = NULL;
    HANDLE    hSearchSpace = NULL;
    PUSHORT   pSetup = NULL;
    BYTE      setupCount = 0;
    BOOLEAN   bEndOfSearch = FALSE;
    BOOLEAN   bInLock = FALSE;
    PSMB_PACKET pSmbResponse = NULL;

    ntStatus = SrvConnectionFindSession(
                    pConnection,
                    pSmbRequest->pSMBHeader->uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionFindTree(
                    pSession,
                    pSmbRequest->pSMBHeader->tid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    wcstowc16s(&wszBackSlash[0], L"\\", 1);
    wcstowc16s(&wszStar[0], L"*", 1);

    if (pwszSearchPattern && *pwszSearchPattern == wszBackSlash[0])
        pwszSearchPattern++;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pTree->pShareInfo->mutex);

    ntStatus = SrvFinderCreateSearchSpace(
                    pSession->hFinderRepository,
                    pTree->pShareInfo->pwszPath,
                    (pwszSearchPattern && *pwszSearchPattern ? pwszSearchPattern : &wszStar[0]),
                    usSearchAttrs,
                    ulSearchStorageType,
                    infoLevel,
                    (pSmbRequest->pSMBHeader->flags2 & FLAG2_KNOWS_LONG_NAMES ? TRUE : FALSE),
                    &hSearchSpace,
                    &usSearchId);
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_TRANSACTION2,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 10 + setupCount;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    (PBYTE)&responseParams,
                    sizeof(responseParams),
                    NULL,
                    0,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    usBytesAvailable = pSmbResponse->bufferLen - usNumPackageBytesUsed;

    ntStatus = SrvFinderGetSearchResults(
                    hSearchSpace,
                    bReturnSingleEntry,
                    bRestartScan,
                    usSearchCount,
                    usBytesAvailable,
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
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    (PBYTE)&responseParams,
                    sizeof(responseParams),
                    pData,
                    usSearchResultLen,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketUpdateAndXOffset(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

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
        SMB_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    SMB_SAFE_FREE_MEMORY(pData);

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    if (ntStatus == STATUS_NO_MORE_MATCHES)
    {
        ntStatus = STATUS_NO_SUCH_FILE;
    }

    goto cleanup;
}


