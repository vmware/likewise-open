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

typedef struct _SMB_FILE_STREAM_INFO_RESPONSE_HEADER
{
    ULONG  ulNextEntryOffset;
    ULONG  ulStreamNameLength;
    LONG64 llStreamSize;
    LONG64 llStreamAllocationSize;
} __attribute__((__packed__)) SMB_FILE_STREAM_INFO_RESPONSE_HEADER, *PSMB_FILE_STREAM_INFO_RESPONSE_HEADER;

static
NTSTATUS
SrvUnmarshallQueryFileInfoParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PUSHORT          pusFid,
    PSMB_INFO_LEVEL* ppSmbInfoLevel
    );

static
NTSTATUS
SrvBuildQueryFileInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    USHORT            usFid,
    SMB_INFO_LEVEL    smbInfoLevel
    );

static
NTSTATUS
SrvMarshallFileStreamInfo(
    PBYTE   pFileStreamInfo,
    USHORT  usBytesAvailable,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    );

NTSTATUS
SrvProcessTrans2QueryFileInformation(
    PSRV_EXEC_CONTEXT           pExecContext,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PBYTE                       pParameters,
    PBYTE                       pData
    )
{
    NTSTATUS ntStatus = 0;
    USHORT usFid = 0;
    PSMB_INFO_LEVEL pSmbInfoLevel = NULL; // Do not free

    ntStatus = SrvUnmarshallQueryFileInfoParams(
                    pParameters,
                    pRequestHeader->parameterCount,
                    &usFid,
                    &pSmbInfoLevel);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildQueryFileInfoResponse(
                    pExecContext,
                    usFid,
                    *pSmbInfoLevel);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvUnmarshallQueryFileInfoParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PUSHORT          pusFid,
    PSMB_INFO_LEVEL* ppSmbInfoLevel
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_INFO_LEVEL pSmbInfoLevel = NULL;
    USHORT   usFid = 0;
    PBYTE    pDataCursor = pParams;

    // FID
    if (ulBytesAvailable < sizeof(USHORT))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usFid = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(USHORT);
    ulBytesAvailable -= sizeof(USHORT);

    // Info level
    if (ulBytesAvailable < sizeof(SMB_INFO_LEVEL))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSmbInfoLevel = (PSMB_INFO_LEVEL)pDataCursor;
    pDataCursor += sizeof(SMB_INFO_LEVEL);
    ulBytesAvailable -= sizeof(SMB_INFO_LEVEL);

    *pusFid = usFid;
    *ppSmbInfoLevel = pSmbInfoLevel;

cleanup:

    return ntStatus;

error:

    *pusFid = 0;
    *ppSmbInfoLevel = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvBuildQueryFileInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    USHORT            usFid,
    SMB_INFO_LEVEL    smbInfoLevel
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE    pTree = NULL;
    PLWIO_SRV_FILE    pFile = NULL;

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

    ntStatus = SrvTreeFindFile_SMB_V1(
                    pCtxSmb1,
                    pTree,
                    usFid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (smbInfoLevel)
    {
        case SMB_QUERY_FILE_BASIC_INFO :

            ntStatus = SrvBuildQueryFileBasicInfoResponse(
                            pExecContext,
                            pFile->hFile);

            break;

        case SMB_QUERY_FILE_STANDARD_INFO :

            ntStatus = SrvBuildQueryFileStandardInfoResponse(
                            pExecContext,
                            pFile->hFile);

            break;

        case SMB_QUERY_FILE_EA_INFO :

            ntStatus = SrvBuildQueryFileEAInfoResponse(
                            pExecContext,
                            pFile->hFile);

            break;

        case SMB_QUERY_FILE_STREAM_INFO :

            ntStatus = SrvBuildQueryFileStreamInfoResponse(
                            pExecContext,
                            pFile->hFile);

            break;


        case SMB_INFO_STANDARD :
        case SMB_INFO_QUERY_EA_SIZE :
        case SMB_INFO_QUERY_EAS_FROM_LIST :
        case SMB_INFO_QUERY_ALL_EAS :
        case SMB_INFO_IS_NAME_VALID :
        case SMB_QUERY_FILE_NAME_INFO :
        case SMB_QUERY_FILE_ALL_INFO :
        case SMB_QUERY_FILE_ALT_NAME_INFO :
        case SMB_QUERY_FILE_COMPRESSION_INFO :
        case SMB_QUERY_FILE_UNIX_BASIC :
        case SMB_QUERY_FILE_UNIX_LINK :

            ntStatus = STATUS_NOT_SUPPORTED;

            break;


        default:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

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

    goto cleanup;
}


NTSTATUS
SrvBuildQueryFileBasicInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    IO_FILE_HANDLE    hFile
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
    IO_STATUS_BLOCK ioStatusBlock = {0};
    FILE_BASIC_INFORMATION fileBasicInfo = {0};
    TRANS2_FILE_BASIC_INFORMATION fileBasicInfoPacked = {0};
    USHORT              usParam = 0;
    PUSHORT             pSetup = NULL;
    BYTE                setupCount = 0;
    USHORT              usDataOffset = 0;
    USHORT              usParameterOffset = 0;

    ntStatus = IoQueryInformationFile(
                    hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileBasicInfo,
                    sizeof(fileBasicInfo),
                    FileBasicInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_TRANSACTION2,
                    STATUS_SUCCESS,
                    TRUE,
                    pCtxSmb1->pTree->tid,
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

    fileBasicInfoPacked.ChangeTime = fileBasicInfo.ChangeTime;
    fileBasicInfoPacked.CreationTime = fileBasicInfo.CreationTime;
    fileBasicInfoPacked.FileAttributes = fileBasicInfo.FileAttributes;
    fileBasicInfoPacked.LastAccessTime = fileBasicInfo.LastAccessTime;
    fileBasicInfoPacked.LastWriteTime = fileBasicInfo.LastWriteTime;

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    (PBYTE)&usParam,
                    sizeof(usParam),
                    (PBYTE)&fileBasicInfoPacked,
                    sizeof(fileBasicInfoPacked),
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

NTSTATUS
SrvBuildQueryFileStandardInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    IO_FILE_HANDLE    hFile
    )
{
    NTSTATUS                   ntStatus = 0;
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
    IO_STATUS_BLOCK ioStatusBlock = {0};
    FILE_STANDARD_INFORMATION fileStandardInfo = {0};
    TRANS2_FILE_STANDARD_INFORMATION fileStandardInfoPacked = {0};
    USHORT              usParam = 0;
    PUSHORT             pSetup = NULL;
    BYTE                setupCount = 0;
    USHORT              usDataOffset = 0;
    USHORT              usParameterOffset = 0;

    ntStatus = IoQueryInformationFile(
                    hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileStandardInfo,
                    sizeof(fileStandardInfo),
                    FileStandardInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_TRANSACTION2,
                    STATUS_SUCCESS,
                    TRUE,
                    pCtxSmb1->pTree->tid,
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

    fileStandardInfoPacked.AllocationSize = fileStandardInfo.AllocationSize;
    fileStandardInfoPacked.EndOfFile = fileStandardInfo.EndOfFile;
    fileStandardInfoPacked.NumberOfLinks = fileStandardInfo.NumberOfLinks;
    fileStandardInfoPacked.bDeletePending = fileStandardInfo.DeletePending;
    fileStandardInfoPacked.bDirectory = fileStandardInfo.Directory;

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    (PBYTE)&usParam,
                    sizeof(usParam),
                    (PBYTE)&fileStandardInfoPacked,
                    sizeof(fileStandardInfoPacked),
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

NTSTATUS
SrvBuildQueryFileEAInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    IO_FILE_HANDLE    hFile
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
    IO_STATUS_BLOCK ioStatusBlock = {0};
    FILE_EA_INFORMATION fileEAInfo = {0};
    USHORT              usParam = 0;
    PUSHORT             pSetup = NULL;
    BYTE                setupCount = 0;
    USHORT              usDataOffset = 0;
    USHORT              usParameterOffset = 0;

    ntStatus = IoQueryInformationFile(
                    hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileEAInfo,
                    sizeof(fileEAInfo),
                    FileEaInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_TRANSACTION2,
                    STATUS_SUCCESS,
                    TRUE,
                    pCtxSmb1->pTree->tid,
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
                    (PBYTE)&usParam,
                    sizeof(usParam),
                    (PBYTE)&fileEAInfo,
                    sizeof(fileEAInfo),
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

NTSTATUS
SrvBuildQueryFileStreamInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    IO_FILE_HANDLE    hFile
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
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PBYTE   pFileStreamInfo = NULL;
    USHORT  usBytesAllocated = 0;
    USHORT  usParam = 0;
    PUSHORT pSetup = NULL;
    BYTE    setupCount = 0;
    USHORT  usDataOffset = 0;
    USHORT  usParameterOffset = 0;
    PBYTE   pData = NULL;
    USHORT  usDataLen = 0;

    usBytesAllocated = sizeof(FILE_STREAM_INFORMATION) + 256 * sizeof(wchar16_t);

    ntStatus = SrvAllocateMemory(
                    usBytesAllocated,
                    (PVOID*)&pFileStreamInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        ntStatus = IoQueryInformationFile(
                        hFile,
                        NULL,
                        &ioStatusBlock,
                        pFileStreamInfo,
                        usBytesAllocated,
                        FileStreamInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        if (ntStatus == STATUS_SUCCESS)
        {
            break;
        }
        else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            USHORT usNewSize = usBytesAllocated + 256 * sizeof(wchar16_t);

            ntStatus = SMBReallocMemory(
                            pFileStreamInfo,
                            (PVOID*)&pFileStreamInfo,
                            usNewSize);
            BAIL_ON_NT_STATUS(ntStatus);

            usBytesAllocated = usNewSize;

            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while (TRUE);

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_TRANSACTION2,
                    STATUS_SUCCESS,
                    TRUE,
                    pCtxSmb1->pTree->tid,
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

    ntStatus = SrvMarshallFileStreamInfo(
                    pFileStreamInfo,
                    usBytesAllocated,
                    &pData,
                    &usDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    (PBYTE)&usParam,
                    sizeof(usParam),
                    pData,
                    usDataLen,
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

    if (pFileStreamInfo)
    {
        SrvFreeMemory(pFileStreamInfo);
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

static
NTSTATUS
SrvMarshallFileStreamInfo(
    PBYTE   pFileStreamInfo,
    USHORT  usBytesAvailable,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    PBYTE    pDataCursor = NULL;
    USHORT   usBytesRequired = 0;
    USHORT   iInfoCount = 0;
    USHORT   usInfoCount = 0;
    USHORT   usOffset = 0;
    PFILE_STREAM_INFORMATION pFileStreamInfoCursor = NULL;

    pFileStreamInfoCursor = (PFILE_STREAM_INFORMATION)pFileStreamInfo;
    while (pFileStreamInfoCursor && (usBytesAvailable > 0))
    {
        USHORT usInfoBytesRequired = 0;

        usInfoBytesRequired = sizeof(SMB_FILE_STREAM_INFO_RESPONSE_HEADER);
        usInfoBytesRequired += wc16slen(pFileStreamInfoCursor->StreamName) * sizeof(wchar16_t);
        usInfoBytesRequired += sizeof(wchar16_t);

        if (usBytesAvailable < usInfoBytesRequired)
        {
            break;
        }

        usInfoCount++;

        usBytesAvailable -= usInfoBytesRequired;
        usBytesRequired += usInfoBytesRequired;

        if (pFileStreamInfoCursor->NextEntryOffset)
        {
            pFileStreamInfoCursor = (PFILE_STREAM_INFORMATION)(((PBYTE)pFileStreamInfo) + pFileStreamInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileStreamInfoCursor = NULL;
        }
    }

    ntStatus = SrvAllocateMemory(
                    usBytesRequired,
                    (PVOID*)&pData);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pData;
    pFileStreamInfoCursor = (PFILE_STREAM_INFORMATION)pFileStreamInfo;
    for (; iInfoCount < usInfoCount; iInfoCount++)
    {
        PSMB_FILE_STREAM_INFO_RESPONSE_HEADER pInfoHeader = NULL;
        USHORT usStreamNameLen = 0;

        pInfoHeader = (PSMB_FILE_STREAM_INFO_RESPONSE_HEADER)pDataCursor;

        pInfoHeader->ulNextEntryOffset = usOffset;
        pInfoHeader->llStreamAllocationSize = pFileStreamInfoCursor->StreamAllocationSize;
        pInfoHeader->llStreamSize = pFileStreamInfoCursor->StreamSize;
        pInfoHeader->ulStreamNameLength = pFileStreamInfoCursor->StreamNameLength * sizeof(wchar16_t);

        pDataCursor += sizeof(SMB_FILE_STREAM_INFO_RESPONSE_HEADER);
        usOffset += sizeof(SMB_FILE_STREAM_INFO_RESPONSE_HEADER);

        usStreamNameLen = wc16slen(pFileStreamInfoCursor->StreamName);
        if (usStreamNameLen)
        {
            memcpy(pDataCursor, (PBYTE)pFileStreamInfoCursor->StreamName, usStreamNameLen * sizeof(wchar16_t));
            pDataCursor += usStreamNameLen * sizeof(wchar16_t);
            usOffset += usStreamNameLen * sizeof(wchar16_t);
        }

        pDataCursor += sizeof(wchar16_t);
        usOffset += sizeof(wchar16_t);

        pFileStreamInfoCursor = (PFILE_STREAM_INFORMATION)(((PBYTE)pFileStreamInfo) + pFileStreamInfoCursor->NextEntryOffset);
    }

    *ppData = pData;
    *pusDataLen = usBytesRequired;

cleanup:

    return ntStatus;

error:

    *ppData = NULL;
    *pusDataLen = 0;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}
