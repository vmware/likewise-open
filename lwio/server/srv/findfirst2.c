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

typedef struct _SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER
{
    ULONG     NextEntryOffset;
    ULONG     FileIndex;
    LONG64    CreationTime;
    LONG64    LastAccessTime;
    LONG64    LastWriteTime;
    LONG64    ChangeTime;
    LONG64    EndOfFile;
    LONG64    AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    ULONG     FileNameLength;
    ULONG     EaSize;
    UCHAR     ShortNameLength;
    UCHAR     Reserved;
    wchar16_t ShortName[12];
} __attribute__((__packed__)) SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER, *PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER;

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
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    );

static
NTSTATUS
SrvMarshallBothDirInfoResponse(
    USHORT  usBytesAvailable,
    PBYTE   pFileInfo,
    ULONG   ulBytesAllocated,
    PUSHORT pusSearchCount,
    PUSHORT pusSearchResultLen,
    PBYTE*  ppData
    );

static
NTSTATUS
SrvBuildFindFirst2FileUnixResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
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
    PSMB_PACKET pSmbResponse = NULL;

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
SrvBuildFindFirst2NamesInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
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
    USHORT              usSearchAttrs,
    USHORT              usSearchCount,
    USHORT              usFlags,
    ULONG               ulSearchStorageType,
    PWSTR               pwszSearchPattern,
    PSMB_PACKET         pSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_SESSION pSession = NULL;
    PSMB_SRV_TREE pTree = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    IO_FILE_SPEC ioFileSpec;
    PFILE_BOTH_DIR_INFORMATION pFileInfo = NULL;
    SMB_FIND_FIRST2_RESPONSE_PARAMETERS responseParams = {0};
    BOOLEAN bReturnSingleEntry = FALSE;
    BOOLEAN bRestartScan = FALSE;
    USHORT              usBytesAllocated = 0;
    wchar16_t           wszBackSlash;
    USHORT              usDataOffset = 0;
    USHORT              usParameterOffset = 0;
    USHORT              usNumPackageBytesUsed = 0;
    ULONG               usBytesAvailable = 0;
    USHORT              usSearchResultLen = 0;
    PBYTE               pData = NULL;
    IO_FILE_HANDLE      hFile = NULL;
    IO_FILE_NAME        fileName = {0};
    PVOID               pSecurityDescriptor = NULL;
    PVOID               pSecurityQOS = NULL;
    PIO_CREATE_SECURITY_CONTEXT pSecurityContext = NULL;
    HANDLE              hSearchSpace = NULL;
    PUSHORT             pSetup = NULL;
    BYTE                setupCount = 0;

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

    wcstowc16s(&wszBackSlash, L"\\", 1);

    if (pwszSearchPattern && *pwszSearchPattern == wszBackSlash)
        pwszSearchPattern++;

    ioFileSpec.Type = IO_FILE_SPEC_TYPE_UNKNOWN;
    // ioFileSpec.Options = IO_NAME_OPTION_CASE_SENSITIVE;
    RtlUnicodeStringInit(
        &ioFileSpec.FileName,
        pwszSearchPattern);

    usBytesAllocated = sizeof(FILE_BOTH_DIR_INFORMATION) + 256 * sizeof(wchar16_t);

    ntStatus = SMBAllocateMemory(
                    usBytesAllocated,
                    (PVOID*)&pFileInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        ntStatus = IoQueryDirectoryFile(
                        hFile,
                        NULL,
                        &ioStatusBlock,
                        pFileInfo,
                        usBytesAllocated,
                        FileBothDirectoryInformation,
                        bReturnSingleEntry,
                        &ioFileSpec,
                        bRestartScan);
        if (ntStatus == STATUS_SUCCESS)
        {
            break;
        }
        else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            USHORT usNewSize = usBytesAllocated + 256 * sizeof(wchar16_t);

            ntStatus = SMBReallocMemory(
                            pFileInfo,
                            (PVOID*)&pFileInfo,
                            usNewSize);
            BAIL_ON_NT_STATUS(ntStatus);

            usBytesAllocated = usNewSize;

            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while (TRUE);

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

    ntStatus = SrvMarshallBothDirInfoResponse(
                    usBytesAvailable,
                    (PBYTE)pFileInfo,
                    usBytesAllocated,
                    &responseParams.usSearchCount,
                    &usSearchResultLen,
                    &pData);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvFinderCreateSearchSpace(
                    pSession->hFinderRepository,
                    hFile,
                    (PBYTE)pFileInfo,
                    usBytesAllocated,
                    FileBothDirectoryInformation,
                    usSearchCount,
                    &hSearchSpace,
                    &responseParams.usSearchId);
    BAIL_ON_NT_STATUS(ntStatus);

    hFile = NULL;
    pFileInfo = NULL;

    if (usFlags & SMB_FIND_CLOSE_AFTER_REQUEST)
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

cleanup:

    if (hSearchSpace)
    {
        SrvFinderReleaseSearchSpace(hSearchSpace);

        if (usFlags & SMB_FIND_CLOSE_AFTER_REQUEST)
        {
            SrvFinderCloseSearchSpace(
                pSession->hFinderRepository,
                responseParams.usSearchId);
        }
    }

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    SMB_SAFE_FREE_MEMORY(pFileInfo);
    SMB_SAFE_FREE_MEMORY(pData);

    return ntStatus;

error:

    if (ntStatus == STATUS_NO_MORE_MATCHES)
    {
        ntStatus = STATUS_NO_SUCH_FILE;
    }

    goto cleanup;
}

static
NTSTATUS
SrvMarshallBothDirInfoResponse(
    USHORT  usBytesAvailable,
    PBYTE   pFileInfo,
    ULONG   ulBytesAllocated,
    PUSHORT pusSearchCount,
    PUSHORT pusSearchResultLen,
    PBYTE*  ppData
    )
{
    NTSTATUS ntStatus = 0;
    USHORT   usBytesRequired = 0;
    PFILE_BOTH_DIR_INFORMATION pFileInfoCursor = NULL;
    PBYTE    pData = NULL;
    PBYTE    pDataCursor = NULL;
    USHORT   usSearchCount = 0;
    USHORT   iSearchCount = 0;
    USHORT   usOffset = 0;

    pFileInfoCursor = (PFILE_BOTH_DIR_INFORMATION)pFileInfo;
    while (pFileInfoCursor && (usBytesAvailable > 0))
    {
        USHORT usInfoBytesRequired = 0;

        usInfoBytesRequired = sizeof(SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER) +
                              (wc16slen(pFileInfoCursor->FileName) + 1) * sizeof(wchar16_t);

        if (usBytesAvailable < usInfoBytesRequired)
        {
            break;
        }

        usSearchCount++;
        usBytesAvailable -= usInfoBytesRequired;
        usBytesRequired += usInfoBytesRequired;

        if (pFileInfoCursor->NextEntryOffset)
        {
            pFileInfoCursor = (PFILE_BOTH_DIR_INFORMATION)(((PBYTE)pFileInfo) + pFileInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileInfoCursor = NULL;
        }
    }

    ntStatus = SMBAllocateMemory(
                    usBytesRequired,
                    (PVOID*)&pData);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pData;
    pFileInfoCursor = (PFILE_BOTH_DIR_INFORMATION)pFileInfo;
    for (; iSearchCount < usSearchCount; iSearchCount++)
    {
        PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER pInfoHeader = NULL;
        USHORT usFileNameLen = 0;

        pInfoHeader = (PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER)pDataCursor;

        pInfoHeader->NextEntryOffset = usOffset;
        pInfoHeader->FileIndex = pFileInfoCursor->FileIndex;
        pInfoHeader->CreationTime = pFileInfoCursor->CreationTime;
        pInfoHeader->LastAccessTime = pFileInfoCursor->LastAccessTime;
        pInfoHeader->LastWriteTime = pFileInfoCursor->LastWriteTime;
        pInfoHeader->ChangeTime = pFileInfoCursor->ChangeTime;
        pInfoHeader->EndOfFile = pFileInfoCursor->EndOfFile;
        pInfoHeader->AllocationSize = pFileInfoCursor->AllocationSize;
        pInfoHeader->FileAttributes = pFileInfoCursor->FileAttributes;
        pInfoHeader->FileNameLength = pFileInfoCursor->FileNameLength;
        pInfoHeader->ShortNameLength = pFileInfoCursor->ShortNameLength;
        memcpy(pInfoHeader->ShortName, pFileInfoCursor->ShortName, sizeof(pInfoHeader->ShortName));

        pDataCursor += sizeof(SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER);
        usOffset += sizeof(SMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER);

        usFileNameLen = wc16slen(pFileInfoCursor->FileName);

        if (usFileNameLen)
        {
            memcpy(pDataCursor, pFileInfoCursor->FileName, usFileNameLen * sizeof(wchar16_t));
        }

        pDataCursor += (usFileNameLen + 1) * sizeof(wchar16_t);
        usOffset += (usFileNameLen + 1) * sizeof(wchar16_t);

        pFileInfoCursor = (PFILE_BOTH_DIR_INFORMATION)(((PBYTE)pFileInfo) + pFileInfoCursor->NextEntryOffset);
    }

    *pusSearchCount = usSearchCount;
    *pusSearchResultLen = usBytesRequired;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *pusSearchCount = 0;
    *pusSearchResultLen = 0;
    *ppData = NULL;

    SMB_SAFE_FREE_MEMORY(pData);

    goto cleanup;
}

static
NTSTATUS
SrvBuildFindFirst2FileUnixResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
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
