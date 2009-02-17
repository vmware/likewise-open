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
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    USHORT              usSearchId,
    USHORT              usSearchCount,
    USHORT              usFlags,
    SMB_INFO_LEVEL      infoLevel,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvBuildFindNext2StdInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvBuildFindNext2QueryEASizeResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvBuildFindNext2QueryEASFromListResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvBuildFindNext2DirInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvBuildFindNext2FullDirInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvBuildFindNext2NamesInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvBuildFindNext2BothDirInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    );

static
NTSTATUS
SrvBuildFindNext2FileUnixResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    );

NTSTATUS
SrvProcessTrans2FindNext2(
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
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    USHORT              usSearchId,
    USHORT              usSearchCount,
    USHORT              usFlags,
    SMB_INFO_LEVEL      infoLevel,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_SESSION pSession = NULL;
    HANDLE           hSearchSpace = NULL;
    BOOLEAN          bEndOfSearch = FALSE;
    PSMB_PACKET pSmbResponse = NULL;

    ntStatus = SrvConnectionFindSession(
                    pConnection,
                    pSmbRequest->pSMBHeader->uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvFinderGetSearchSpace(
                    pSession,
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

    switch (infoLevel)
    {
        case SMB_INFO_STANDARD:

            ntStatus = SrvBuildFindNext2StdInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hSearchSpace,
                            usSearchCount,
                            usFlags,
                            ulResumeHandle,
                            pwszResumeFilename,
                            pSmbResponse,
                            &bEndOfSearch);

            break;

        case SMB_INFO_QUERY_EA_SIZE:

            ntStatus = SrvBuildFindNext2QueryEASizeResponse(
                            pConnection,
                            pSmbRequest,
                            hSearchSpace,
                            usSearchCount,
                            usFlags,
                            ulResumeHandle,
                            pwszResumeFilename,
                            pSmbResponse,
                            &bEndOfSearch);

            break;

        case SMB_INFO_QUERY_EAS_FROM_LIST:

            ntStatus = SrvBuildFindNext2QueryEASFromListResponse(
                            pConnection,
                            pSmbRequest,
                            hSearchSpace,
                            usSearchCount,
                            usFlags,
                            ulResumeHandle,
                            pwszResumeFilename,
                            pSmbResponse,
                            &bEndOfSearch);

            break;

        case SMB_FIND_FILE_DIRECTORY_INFO:

            ntStatus = SrvBuildFindNext2DirInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hSearchSpace,
                            usSearchCount,
                            usFlags,
                            ulResumeHandle,
                            pwszResumeFilename,
                            pSmbResponse,
                            &bEndOfSearch);

            break;

        case SMB_FIND_FILE_FULL_DIRECTORY_INFO:

            ntStatus = SrvBuildFindNext2FullDirInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hSearchSpace,
                            usSearchCount,
                            usFlags,
                            ulResumeHandle,
                            pwszResumeFilename,
                            pSmbResponse,
                            &bEndOfSearch);

            break;

        case SMB_FIND_FILE_NAMES_INFO:

            ntStatus = SrvBuildFindNext2NamesInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hSearchSpace,
                            usSearchCount,
                            usFlags,
                            ulResumeHandle,
                            pwszResumeFilename,
                            pSmbResponse,
                            &bEndOfSearch);

            break;

        case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:

            ntStatus = SrvBuildFindNext2BothDirInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hSearchSpace,
                            usSearchCount,
                            usFlags,
                            ulResumeHandle,
                            pwszResumeFilename,
                            pSmbResponse,
                            &bEndOfSearch);

            break;

        case SMB_FIND_FILE_UNIX:

            ntStatus = SrvBuildFindNext2FileUnixResponse(
                            pConnection,
                            pSmbRequest,
                            hSearchSpace,
                            usSearchCount,
                            usFlags,
                            ulResumeHandle,
                            pwszResumeFilename,
                            pSmbResponse,
                            &bEndOfSearch);

            break;

        default:

            ntStatus = STATUS_NOT_SUPPORTED;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

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
SrvBuildFindNext2StdInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFindNext2QueryEASizeResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFindNext2QueryEASFromListResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFindNext2DirInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFindNext2FullDirInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFindNext2NamesInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFindNext2BothDirInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFindNext2FileUnixResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    HANDLE              hSearchSpace,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulResumeHandle,
    PWSTR               pwszResumeFilename,
    PSMB_PACKET         pSmbResponse,
    PBOOLEAN            pbEndOfSearch
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

