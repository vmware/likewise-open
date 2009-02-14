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
SrvUnmarshallQueryPathInfoParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PSMB_INFO_LEVEL* ppSmbInfoLevel,
    PWSTR*           ppwszFilename
    );

static
NTSTATUS
SrvBuildQueryPathInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    SMB_INFO_LEVEL      smbInfoLevel,
    PWSTR               pwszFilename,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvBuildQueryFileBasicInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PWSTR               pwszFilepath,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvBuildQueryFileStandardInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PWSTR               pwszFilepath,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessTrans2QueryPathInformation(
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
    PSMB_INFO_LEVEL pSmbInfoLevel = NULL; // Do not free
    PWSTR    pwszFilename = NULL; // Do not free
    PSMB_PACKET pSmbResponse = NULL;

    ntStatus = SrvUnmarshallQueryPathInfoParams(
                    pParameters,
                    pRequestHeader->parameterCount,
                    &pSmbInfoLevel,
                    &pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildQueryPathInfoResponse(
                    pConnection,
                    pSmbRequest,
                    *pSmbInfoLevel,
                    pwszFilename,
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
SrvUnmarshallQueryPathInfoParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PSMB_INFO_LEVEL* ppSmbInfoLevel,
    PWSTR*           ppwszFilename
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_INFO_LEVEL pSmbInfoLevel = NULL;
    PWSTR    pwszFilename = NULL;
    PBYTE    pDataCursor = pParams;

    // Info level
    if (ulBytesAvailable < sizeof(SMB_INFO_LEVEL))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSmbInfoLevel = (PSMB_INFO_LEVEL)pDataCursor;
    pDataCursor += sizeof(SMB_INFO_LEVEL);
    ulBytesAvailable -= sizeof(SMB_INFO_LEVEL);

    // Reserved field
    if (ulBytesAvailable < sizeof(ULONG))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pDataCursor += sizeof(ULONG);
    ulBytesAvailable -= sizeof(ULONG);

    // Filename
    if (ulBytesAvailable < sizeof(wchar16_t))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pwszFilename = (PWSTR)pDataCursor;

    *ppSmbInfoLevel = pSmbInfoLevel;
    *ppwszFilename = pwszFilename;

cleanup:

    return ntStatus;

error:

    *ppSmbInfoLevel = NULL;
    *ppwszFilename = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvBuildQueryPathInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    SMB_INFO_LEVEL      smbInfoLevel,
    PWSTR               pwszFilename,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_SESSION pSession = NULL;
    PSMB_SRV_TREE    pTree = NULL;
    PSMB_PACKET pSmbResponse = NULL;
    PWSTR       pwszFilepath = NULL;

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

    ntStatus = SrvBuildFilePath(
                    pTree->pShareInfo->pwszPath,
                    pwszFilename,
                    &pwszFilepath);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (smbInfoLevel)
    {
        case SMB_INFO_STANDARD :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_INFO_QUERY_EA_SIZE :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_INFO_QUERY_EAS_FROM_LIST :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_INFO_QUERY_ALL_EAS :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_INFO_IS_NAME_VALID :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_QUERY_FILE_BASIC_INFO :

            ntStatus = SrvBuildQueryFileBasicInfoResponse(
                            pConnection,
                            pSmbRequest,
                            pwszFilepath,
                            &pSmbResponse);

            break;

        case SMB_QUERY_FILE_STANDARD_INFO :

            ntStatus = SrvBuildQueryFileStandardInfoResponse(
                            pConnection,
                            pSmbRequest,
                            pwszFilepath,
                            &pSmbResponse);

            break;

        case SMB_QUERY_FILE_EA_INFO :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_QUERY_FILE_NAME_INFO :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_QUERY_FILE_ALL_INFO :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_QUERY_FILE_ALT_NAME_INFO :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_QUERY_FILE_STREAM_INFO :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_QUERY_FILE_COMPRESSION_INFO :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_QUERY_FILE_UNIX_BASIC :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_QUERY_FILE_UNIX_LINK :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;


        default:

            ntStatus = STATUS_DATA_ERROR;

            break;
    }
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

    SMB_SAFE_FREE_MEMORY(pwszFilepath);

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
SrvBuildQueryFileBasicInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PWSTR               pwszFilepath,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PIO_ASYNC_CONTROL_BLOCK pAsyncControlBlock = NULL;
    PIO_CREATE_SECURITY_CONTEXT pSecurityContext = NULL;
    PVOID               pSecurityDescriptor = NULL;
    PVOID               pSecurityQOS = NULL;
    IO_FILE_NAME        filename = {0};
    FILE_BASIC_INFORMATION fileBasicInfo = {0};
    TRANS2_FILE_BASIC_INFORMATION fileBasicInfoPacked = {0};
    USHORT              usParam = 0;
    PUSHORT             pSetup = NULL;
    BYTE                setupCount = 0;
    USHORT              usDataOffset = 0;
    USHORT              usParameterOffset = 0;
    USHORT              usNumPackageBytesUsed = 0;

    filename.FileName = pwszFilepath;

    ntStatus = IoCreateFile(
                    &hFile,
                    pAsyncControlBlock,
                    &ioStatusBlock,
                    pSecurityContext,
                    &filename,
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

    ntStatus = IoQueryInformationFile(
                    hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileBasicInfo,
                    sizeof(fileBasicInfo),
                    FileBasicInformation);
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

    fileBasicInfoPacked.ChangeTime = fileBasicInfo.ChangeTime;
    fileBasicInfoPacked.CreationTime = fileBasicInfo.CreationTime;
    fileBasicInfoPacked.FileAttributes = fileBasicInfo.FileAttributes;
    fileBasicInfoPacked.LastAccessTime = fileBasicInfo.LastAccessTime;
    fileBasicInfoPacked.LastWriteTime = fileBasicInfo.LastWriteTime;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    (PBYTE)&usParam,
                    sizeof(usParam),
                    (PBYTE)&fileBasicInfoPacked,
                    sizeof(fileBasicInfoPacked),
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (hFile)
    {
        IoCloseFile(hFile);
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
SrvBuildQueryFileStandardInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PWSTR               pwszFilepath,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PIO_ASYNC_CONTROL_BLOCK pAsyncControlBlock = NULL;
    PIO_CREATE_SECURITY_CONTEXT pSecurityContext = NULL;
    PVOID               pSecurityDescriptor = NULL;
    PVOID               pSecurityQOS = NULL;
    IO_FILE_NAME        filename = {0};
    FILE_STANDARD_INFORMATION fileStandardInfo = {0};
    TRANS2_FILE_STANDARD_INFORMATION fileStandardInfoPacked = {0};
    USHORT              usParam = 0;
    PUSHORT             pSetup = NULL;
    BYTE                setupCount = 0;
    USHORT              usDataOffset = 0;
    USHORT              usParameterOffset = 0;
    USHORT              usNumPackageBytesUsed = 0;

    filename.FileName = pwszFilepath;

    ntStatus = IoCreateFile(
                    &hFile,
                    pAsyncControlBlock,
                    &ioStatusBlock,
                    pSecurityContext,
                    &filename,
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

    ntStatus = IoQueryInformationFile(
                    hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileStandardInfo,
                    sizeof(fileStandardInfo),
                    FileStandardInformation);
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

    fileStandardInfoPacked.AllocationSize = fileStandardInfo.AllocationSize;
    fileStandardInfoPacked.EndOfFile = fileStandardInfo.EndOfFile;
    fileStandardInfoPacked.NumberOfLinks = fileStandardInfo.NumberOfLinks;
    fileStandardInfoPacked.bDeletePending = fileStandardInfo.DeletePending;
    fileStandardInfoPacked.bDirectory = fileStandardInfo.Directory;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    (PBYTE)&usParam,
                    sizeof(usParam),
                    (PBYTE)&fileStandardInfoPacked,
                    sizeof(fileStandardInfoPacked),
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (hFile)
    {
        IoCloseFile(hFile);
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

