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
NTSTATUS
SrvBuildFindNext2Response(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    USHORT              usSearchId,
    USHORT              usSearchCount,
    USHORT              usFlags,
    SMB_INFO_LEVEL      infoLevel,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessTrans2FindNext2(
    IN  PLWIO_SRV_CONNECTION         pConnection,
    IN  PSMB_PACKET                 pSmbRequest,
    IN  PTRANSACTION_REQUEST_HEADER pRequestHeader,
    IN  PUSHORT                     pSetup,
    IN  PUSHORT                     pByteCount,
    IN  PBYTE                       pParameters,
    IN  PBYTE                       pData,
    OUT PSMB_PACKET*                ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    USHORT         usSearchId = 0;
    USHORT         usSearchCount = 0;
    ULONG          ulResumeHandle = 0;
    USHORT         usFlags = 0;
    SMB_INFO_LEVEL infoLevel = 0;
    PWSTR          pwszResumeFilename = NULL; // Do not free
    PSMB_PACKET    pSmbResponse = NULL;

    ntStatus = SrvUnmarshallFindNext2Params(
                    pParameters,
                    pRequestHeader->parameterCount,
                    pRequestHeader->parameterOffset,
                    &usSearchId,
                    &usSearchCount,
                    &usFlags,
                    &infoLevel,
                    &ulResumeHandle,
                    &pwszResumeFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildFindNext2Response(
                    pConnection,
                    pSmbRequest,
                    usSearchId,
                    usSearchCount,
                    usFlags,
                    infoLevel,
                    ulResumeHandle,
                    pwszResumeFilename,
                    pRequestHeader->maxDataCount,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

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
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usSearchId = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usSearchId);

    if (usBytesAvailable < sizeof(usSearchCount))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usSearchCount = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usSearchCount);

    if (usBytesAvailable < sizeof(infoLevel))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    infoLevel = *((SMB_INFO_LEVEL*)pDataCursor);
    pDataCursor += sizeof(infoLevel);

    if (usBytesAvailable < sizeof(ulResumeHandle))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulResumeHandle = *((PULONG)pDataCursor);
    pDataCursor += sizeof(ulResumeHandle);

    if (usBytesAvailable < sizeof(usFlags))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usFlags = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(usFlags);

    if (!usBytesAvailable)
    {
        if (!(usFlags & SMB_FIND_CONTINUE_SEARCH))
        {
            ntStatus = STATUS_DATA_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }
    else
    {
        pwszResumeFilename = (PWSTR)pDataCursor;

        if (wc16slen(pwszResumeFilename) > 256 * sizeof(wchar16_t))
        {
            ntStatus = STATUS_DATA_ERROR;
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
NTSTATUS
SrvBuildFindNext2Response(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    USHORT              usSearchId,
    USHORT              usSearchCount,
    USHORT              usFlags,
    SMB_INFO_LEVEL      infoLevel,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    HANDLE           hSearchSpace = NULL;
    BOOLEAN          bEndOfSearch = FALSE;
    BOOLEAN          bReturnSingleEntry = FALSE;
    BOOLEAN          bRestartScan = FALSE;
    PBYTE            pData = 0;
    USHORT           usSearchResultLen = 0;
    USHORT           usDataOffset = 0;
    USHORT           usParameterOffset = 0;
    USHORT           usNumPackageBytesUsed = 0;
    ULONG            usBytesAvailable = 0;
    PUSHORT          pSetup = NULL;
    BYTE             setupCount = 0;
    SMB_FIND_NEXT2_RESPONSE_PARAMETERS responseParams = {0};
    PSMB_PACKET      pSmbResponse = NULL;

    ntStatus = SrvConnectionFindSession(
                    pConnection,
                    pSmbRequest->pSMBHeader->uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvFinderGetSearchSpace(
                    pSession->hFinderRepository,
                    usSearchId,
                    &hSearchSpace);
    BAIL_ON_NT_STATUS(ntStatus);

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

    usBytesAvailable = SMB_MIN(usMaxDataCount, pConnection->serverProperties.MaxBufferSize - usNumPackageBytesUsed);

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

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    if (ntStatus == STATUS_NO_MORE_MATCHES) {
        ntStatus = STATUS_NO_SUCH_FILE;
    }

    goto cleanup;
}


