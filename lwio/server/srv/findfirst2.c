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

static
NTSTATUS
SrvBuildFindFirst2StdInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    );

static
NTSTATUS
SrvBuildFindFirst2QueryEASizeResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    );

static
NTSTATUS
SrvBuildFindFirst2QueryEASFromListResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    );

static
NTSTATUS
SrvBuildFindFirst2DirInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    );

static
NTSTATUS
SrvBuildFindFirst2FullDirInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    );

static
NTSTATUS
SrvBuildFindFirst2NamesInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    );

static
NTSTATUS
SrvBuildFindFirst2BothDirInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    );

static
NTSTATUS
SrvBuildFindFirst2FileUnixResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
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
    PSMB_PACKET pSmbResponse = NULL;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    IO_FILE_NAME fileName = {0};
    PVOID        pSecurityDescriptor = NULL;
    PVOID        pSecurityQOS = NULL;
    PIO_CREATE_SECURITY_CONTEXT pSecurityContext = NULL;

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

    fileName.FileName = pTree->pShareInfo->pwszPath;

    ntStatus = IoCreateFile(
                    &hFile,
                    NULL,
                    &ioStatusBlock,
                    pSecurityContext,
                    &fileName,
                    pSecurityDescriptor,
                    pSecurityQOS,
                    GENERIC_READ,
                    0,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ,
                    FILE_OPEN,
                    0,
                    NULL, /* EA Buffer */
                    0,    /* EA Length */
                    NULL  /* ECP List  */
                    );
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

            ntStatus = SrvBuildFindFirst2StdInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            usSearchAttrs,
                            usSearchCount,
                            usFlags,
                            ulSearchStorageType,
                            pwszSearchPattern,
                            pSmbResponse);

            break;

        case SMB_INFO_QUERY_EA_SIZE:

            ntStatus = SrvBuildFindFirst2QueryEASizeResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            usSearchAttrs,
                            usSearchCount,
                            usFlags,
                            ulSearchStorageType,
                            pwszSearchPattern,
                            pSmbResponse);

            break;

        case SMB_INFO_QUERY_EAS_FROM_LIST:

            ntStatus = SrvBuildFindFirst2QueryEASFromListResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            usSearchAttrs,
                            usSearchCount,
                            usFlags,
                            ulSearchStorageType,
                            pwszSearchPattern,
                            pSmbResponse);

            break;

        case SMB_FIND_FILE_DIRECTORY_INFO:

            ntStatus = SrvBuildFindFirst2DirInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            usSearchAttrs,
                            usSearchCount,
                            usFlags,
                            ulSearchStorageType,
                            pwszSearchPattern,
                            pSmbResponse);

            break;

        case SMB_FIND_FILE_FULL_DIRECTORY_INFO:

            ntStatus = SrvBuildFindFirst2FullDirInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            usSearchAttrs,
                            usSearchCount,
                            usFlags,
                            ulSearchStorageType,
                            pwszSearchPattern,
                            pSmbResponse);

            break;

        case SMB_FIND_FILE_NAMES_INFO:

            ntStatus = SrvBuildFindFirst2NamesInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            usSearchAttrs,
                            usSearchCount,
                            usFlags,
                            ulSearchStorageType,
                            pwszSearchPattern,
                            pSmbResponse);

            break;

        case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:

            ntStatus = SrvBuildFindFirst2BothDirInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            usSearchAttrs,
                            usSearchCount,
                            usFlags,
                            ulSearchStorageType,
                            pwszSearchPattern,
                            pSmbResponse);

            break;

        case SMB_FIND_FILE_UNIX:

            ntStatus = SrvBuildFindFirst2FileUnixResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            usSearchAttrs,
                            usSearchCount,
                            usFlags,
                            ulSearchStorageType,
                            pwszSearchPattern,
                            pSmbResponse);

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

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
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
SrvBuildFindFirst2StdInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvBuildFindFirst2QueryEASizeResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvBuildFindFirst2QueryEASFromListResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvBuildFindFirst2DirInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvBuildFindFirst2FullDirInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    IO_FILE_SPEC ioFileSpec;
    FILE_BOTH_DIR_INFORMATION fileInfo = {0};
    BOOLEAN bReturnSingleEntry = FALSE;
    BOOLEAN bRestartScan = FALSE;

    ntStatus = IoQueryDirectoryFile(
                    hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileInfo,
                    sizeof(fileInfo),
                    FileBothDirectoryInformation,
                    bReturnSingleEntry,
                    &ioFileSpec,
                    bRestartScan);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildFindFirst2NamesInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvBuildFindFirst2BothDirInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvBuildFindFirst2FileUnixResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    )
{
    return STATUS_NOT_SUPPORTED;
}
