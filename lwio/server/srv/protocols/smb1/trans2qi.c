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
} __attribute__((__packed__)) SMB_FILE_STREAM_INFO_RESPONSE_HEADER,
                             *PSMB_FILE_STREAM_INFO_RESPONSE_HEADER;

static
NTSTATUS
SrvMarshallFileStreamInfo(
    PBYTE   pFileStreamInfo,
    USHORT  usDataLength,
    USHORT  usBytesAvailable,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    );

static
NTSTATUS
SrvMarshallFileAllInfo(
    PBYTE   pFileAllInfo,
    USHORT  usBytesAvailable,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    );

static
NTSTATUS
SrvMarshallFileNameInfo(
    PLWIO_SRV_TREE pTree,
    PBYTE          pInfoBuffer,
    USHORT         usBytesAvailable,
    PBYTE*         ppData,
    PUSHORT        pusDataLen
    );

static
NTSTATUS
SrvQueryBasicInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildQueryBasicInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvQueryStandardInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildQueryStandardInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvQueryEAInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildQueryEAInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvQueryStreamInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildQueryStreamInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvQueryAllInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildQueryAllInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvQueryNameInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildQueryNameInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvQueryAltNameInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildQueryAltNameInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvQueryInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    switch (*pTrans2State->pSmbInfoLevel)
    {
        case SMB_QUERY_FILE_BASIC_INFO :
        case SMB_QUERY_FILE_BASIC_INFO_ALIAS :

            ntStatus = SrvQueryBasicInfo(pExecContext);

            break;

        case SMB_QUERY_FILE_STANDARD_INFO :
        case SMB_QUERY_FILE_STANDARD_INFO_ALIAS :

            ntStatus = SrvQueryStandardInfo(pExecContext);

            break;

        case SMB_QUERY_FILE_EA_INFO :
        case SMB_QUERY_FILE_EA_INFO_ALIAS :

            ntStatus = SrvQueryEAInfo(pExecContext);

            break;

        case SMB_QUERY_FILE_STREAM_INFO :
        case SMB_QUERY_FILE_STREAM_INFO_ALIAS :

            ntStatus = SrvQueryStreamInfo(pExecContext);

            break;

        case SMB_QUERY_FILE_ALL_INFO :
        case SMB_QUERY_FILE_ALL_INFO_ALIAS :

            ntStatus = SrvQueryAllInfo(pExecContext);

            break;

        case SMB_QUERY_FILE_NAME_INFO :
        case SMB_QUERY_FILE_NAME_INFO_ALIAS :

            ntStatus = SrvQueryNameInfo(pExecContext);

            break;

        case SMB_QUERY_FILE_ALT_NAME_INFO :
        case SMB_QUERY_FILE_ALT_NAME_INFO_ALIAS :

            ntStatus = SrvQueryAltNameInfo(pExecContext);

            break;

        case SMB_INFO_STANDARD :
        case SMB_INFO_QUERY_EA_SIZE :
        case SMB_INFO_QUERY_EAS_FROM_LIST :
        case SMB_INFO_QUERY_ALL_EAS :
        case SMB_INFO_IS_NAME_VALID :
        case SMB_QUERY_FILE_COMPRESSION_INFO :
        case SMB_QUERY_FILE_COMPRESSION_INFO_ALIAS :
        case SMB_QUERY_FILE_UNIX_BASIC :
        case SMB_QUERY_FILE_UNIX_LINK :
        case SMB_QUERY_FILE_INTERNAL_INFO :
        case SMB_QUERY_FILE_ACCESS_INFO :
        case SMB_QUERY_FILE_POSITION_INFO :
        case SMB_QUERY_FILE_MODE_INFO :
        case SMB_QUERY_FILE_ALIGNMENT_INFO :
        case SMB_QUERY_FILE_NETWORK_OPEN_INFO :
        case SMB_QUERY_FILE_ATTRIBUTE_TAG_INFO :

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        default:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

            break;
    }

    return ntStatus;
}

NTSTATUS
SrvBuildQueryInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    switch (*pTrans2State->pSmbInfoLevel)
    {
        case SMB_QUERY_FILE_BASIC_INFO :
        case SMB_QUERY_FILE_BASIC_INFO_ALIAS :

            ntStatus = SrvBuildQueryBasicInfoResponse(pExecContext);

            break;

        case SMB_QUERY_FILE_STANDARD_INFO :
        case SMB_QUERY_FILE_STANDARD_INFO_ALIAS :

            ntStatus = SrvBuildQueryStandardInfoResponse(pExecContext);

            break;

        case SMB_QUERY_FILE_EA_INFO :
        case SMB_QUERY_FILE_EA_INFO_ALIAS :

            ntStatus = SrvBuildQueryEAInfoResponse(pExecContext);

            break;

        case SMB_QUERY_FILE_STREAM_INFO :
        case SMB_QUERY_FILE_STREAM_INFO_ALIAS :

            ntStatus = SrvBuildQueryStreamInfoResponse(pExecContext);

            break;

        case SMB_QUERY_FILE_ALL_INFO :
        case SMB_QUERY_FILE_ALL_INFO_ALIAS :

            ntStatus = SrvBuildQueryAllInfoResponse(pExecContext);

            break;

        case SMB_QUERY_FILE_NAME_INFO :
        case SMB_QUERY_FILE_NAME_INFO_ALIAS :

            ntStatus = SrvBuildQueryNameInfoResponse(pExecContext);

            break;

        case SMB_QUERY_FILE_ALT_NAME_INFO :
        case SMB_QUERY_FILE_ALT_NAME_INFO_ALIAS :

            ntStatus = SrvBuildQueryAltNameInfoResponse(pExecContext);

            break;

        case SMB_INFO_STANDARD :
        case SMB_INFO_QUERY_EA_SIZE :
        case SMB_INFO_QUERY_EAS_FROM_LIST :
        case SMB_INFO_QUERY_ALL_EAS :
        case SMB_INFO_IS_NAME_VALID :
        case SMB_QUERY_FILE_COMPRESSION_INFO :
        case SMB_QUERY_FILE_COMPRESSION_INFO_ALIAS :
        case SMB_QUERY_FILE_UNIX_BASIC :
        case SMB_QUERY_FILE_UNIX_LINK :
        case SMB_QUERY_FILE_INTERNAL_INFO :
        case SMB_QUERY_FILE_ACCESS_INFO :
        case SMB_QUERY_FILE_POSITION_INFO :
        case SMB_QUERY_FILE_MODE_INFO :
        case SMB_QUERY_FILE_ALIGNMENT_INFO :
        case SMB_QUERY_FILE_NETWORK_OPEN_INFO :
        case SMB_QUERY_FILE_ATTRIBUTE_TAG_INFO :

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        default:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvQueryBasicInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    ntStatus = pTrans2State->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pTrans2State->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_BASIC_INFORMATION),
                        (PVOID*)&pTrans2State->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pTrans2State->usBytesAllocated = sizeof(FILE_BASIC_INFORMATION);

        SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

        ntStatus = IoQueryInformationFile(
                        (pTrans2State->pFile ? pTrans2State->pFile->hFile :
                                               pTrans2State->hFile),
                        pTrans2State->pAcb,
                        &pTrans2State->ioStatusBlock,
                        pTrans2State->pData2,
                        pTrans2State->usBytesAllocated,
                        FileBasicInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildQueryBasicInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE   pOutBuffer        = pSmbResponse->pBuffer;
    ULONG   ulBytesAvailable  = pSmbResponse->ulBytesAvailable;
    ULONG   ulOffset          = 0;
    USHORT  usBytesUsed       = 0;
    ULONG   ulTotalBytesUsed  = 0;
    USHORT  usParam           = 0;
    PUSHORT pSetup            = NULL;
    BYTE    setupCount        = 0;
    USHORT  usDataOffset      = 0;
    USHORT  usParameterOffset = 0;
    PFILE_BASIC_INFORMATION       pFileBasicInfo      = NULL;
    TRANS2_FILE_BASIC_INFORMATION fileBasicInfoPacked = {0};
    PSRV_TRANS2_STATE_SMB_V1      pTrans2State        = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    pFileBasicInfo = (PFILE_BASIC_INFORMATION)pTrans2State->pData2;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION2,
                        STATUS_SUCCESS,
                        TRUE,
                        pTrans2State->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pTrans2State->pSession->uid,
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

    fileBasicInfoPacked.ChangeTime     = pFileBasicInfo->ChangeTime;
    fileBasicInfoPacked.CreationTime   = pFileBasicInfo->CreationTime;
    fileBasicInfoPacked.FileAttributes = pFileBasicInfo->FileAttributes;
    fileBasicInfoPacked.LastAccessTime = pFileBasicInfo->LastAccessTime;
    fileBasicInfoPacked.LastWriteTime  = pFileBasicInfo->LastWriteTime;

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

static
NTSTATUS
SrvQueryStandardInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    ntStatus = pTrans2State->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pTrans2State->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_STANDARD_INFORMATION),
                        (PVOID*)&pTrans2State->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pTrans2State->usBytesAllocated = sizeof(FILE_STANDARD_INFORMATION);

        SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

        ntStatus = IoQueryInformationFile(
                        (pTrans2State->pFile ? pTrans2State->pFile->hFile :
                                               pTrans2State->hFile),
                        pTrans2State->pAcb,
                        &pTrans2State->ioStatusBlock,
                        pTrans2State->pData2,
                        pTrans2State->usBytesAllocated,
                        FileStandardInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildQueryStandardInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE   pOutBuffer        = pSmbResponse->pBuffer;
    ULONG   ulBytesAvailable  = pSmbResponse->ulBytesAvailable;
    ULONG   ulOffset          = 0;
    USHORT  usBytesUsed       = 0;
    ULONG   ulTotalBytesUsed  = 0;
    USHORT  usParam           = 0;
    PUSHORT pSetup            = NULL;
    BYTE    setupCount        = 0;
    USHORT  usDataOffset      = 0;
    USHORT  usParameterOffset = 0;
    PFILE_STANDARD_INFORMATION       pFileStandardInfo      = NULL;
    TRANS2_FILE_STANDARD_INFORMATION fileStandardInfoPacked = {0};
    PSRV_TRANS2_STATE_SMB_V1         pTrans2State           = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;
    pFileStandardInfo = (PFILE_STANDARD_INFORMATION)pTrans2State->pData2;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION2,
                        STATUS_SUCCESS,
                        TRUE,
                        pTrans2State->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pTrans2State->pSession->uid,
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

    fileStandardInfoPacked.AllocationSize = pFileStandardInfo->AllocationSize;
    fileStandardInfoPacked.EndOfFile      = pFileStandardInfo->EndOfFile;
    fileStandardInfoPacked.NumberOfLinks  = pFileStandardInfo->NumberOfLinks;
    fileStandardInfoPacked.bDeletePending = pFileStandardInfo->DeletePending;
    fileStandardInfoPacked.bDirectory     = pFileStandardInfo->Directory;

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

static
NTSTATUS
SrvQueryEAInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    ntStatus = pTrans2State->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pTrans2State->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_EA_INFORMATION),
                        (PVOID*)&pTrans2State->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pTrans2State->usBytesAllocated = sizeof(FILE_EA_INFORMATION);

        SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

        ntStatus = IoQueryInformationFile(
                        (pTrans2State->pFile ? pTrans2State->pFile->hFile :
                                               pTrans2State->hFile),
                        pTrans2State->pAcb,
                        &pTrans2State->ioStatusBlock,
                        pTrans2State->pData2,
                        pTrans2State->usBytesAllocated,
                        FileEaInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildQueryEAInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE   pOutBuffer        = pSmbResponse->pBuffer;
    ULONG   ulBytesAvailable  = pSmbResponse->ulBytesAvailable;
    ULONG   ulOffset          = 0;
    USHORT  usBytesUsed       = 0;
    ULONG   ulTotalBytesUsed  = 0;
    USHORT  usParam           = 0;
    PUSHORT pSetup            = NULL;
    BYTE    setupCount        = 0;
    USHORT  usDataOffset      = 0;
    USHORT  usParameterOffset = 0;
    PSRV_TRANS2_STATE_SMB_V1 pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION2,
                        STATUS_SUCCESS,
                        TRUE,
                        pCtxSmb1->pTree->tid,
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
                    (PBYTE)&usParam,
                    sizeof(usParam),
                    pTrans2State->pData2,
                    pTrans2State->usBytesAllocated,
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

static
NTSTATUS
SrvQueryStreamInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    BOOLEAN                    bContinue    = TRUE;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    do
    {
        ntStatus = pTrans2State->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_BUFFER_TOO_SMALL:

                {
                    USHORT usNewSize =  0;

                    if (pTrans2State->usBytesAllocated >
                                    pTrans2State->pRequestHeader->maxDataCount)
                    {
                        bContinue = FALSE;
                    }
                    else if (!pTrans2State->usBytesAllocated)
                    {
                        usNewSize = pTrans2State->usBytesAllocated +
                                        sizeof(FILE_STREAM_INFORMATION) +
                                        256 * sizeof(wchar16_t);
                    }
                    else
                    {
                        usNewSize = pTrans2State->usBytesAllocated +
                                        256 * sizeof(wchar16_t);
                    }

                    ntStatus = SMBReallocMemory(
                                    pTrans2State->pData2,
                                    (PVOID*)&pTrans2State->pData2,
                                    usNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pTrans2State->usBytesAllocated = usNewSize;

                    SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

                    ntStatus = IoQueryInformationFile(
                                            (pTrans2State->pFile ?
                                                    pTrans2State->pFile->hFile :
                                                    pTrans2State->hFile),
                                            pTrans2State->pAcb,
                                            &pTrans2State->ioStatusBlock,
                                            pTrans2State->pData2,
                                            pTrans2State->usBytesAllocated,
                                            FileStreamInformation);
                    switch (ntStatus)
                    {
                        case STATUS_SUCCESS:

                            bContinue = FALSE;

                            pTrans2State->usBytesUsed =
                                pTrans2State->ioStatusBlock.BytesTransferred;

                            // intentional fall through

                        case STATUS_BUFFER_TOO_SMALL:

                            // synchronous completion
                            SrvReleaseTrans2StateAsync(pTrans2State);

                            break;

                        default:

                            BAIL_ON_NT_STATUS(ntStatus);
                    }
                }

                break;

            case STATUS_SUCCESS:

                if (!pTrans2State->pData2)
                {
                    pTrans2State->ioStatusBlock.Status =
                                            STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    bContinue = FALSE;

                    pTrans2State->usBytesUsed =
                                pTrans2State->ioStatusBlock.BytesTransferred;
                }

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

    } while (bContinue);

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildQueryStreamInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE   pOutBuffer        = pSmbResponse->pBuffer;
    ULONG   ulBytesAvailable  = pSmbResponse->ulBytesAvailable;
    ULONG   ulOffset          = 0;
    USHORT  usBytesUsed       = 0;
    ULONG   ulTotalBytesUsed  = 0;
    USHORT  usParam           = 0;
    PUSHORT pSetup            = NULL;
    BYTE    setupCount        = 0;
    USHORT  usDataOffset      = 0;
    USHORT  usParameterOffset = 0;
    PBYTE   pData             = NULL;
    USHORT  usDataLen         = 0;
    PSRV_TRANS2_STATE_SMB_V1 pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION2,
                        STATUS_SUCCESS,
                        TRUE,
                        pCtxSmb1->pTree->tid,
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

    if (pTrans2State->usBytesUsed > 0)
    {
        ntStatus = SrvMarshallFileStreamInfo(
                        pTrans2State->pData2,
                        pTrans2State->usBytesUsed,
                        pTrans2State->pRequestHeader->maxDataCount,
                        &pData,
                        &usDataLen);
        BAIL_ON_NT_STATUS(ntStatus);
    }

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
    USHORT  usDataLength,
    USHORT  usBytesAvailable,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    PBYTE    pDataCursor = NULL;
    USHORT   usBytesAvailable1 = usBytesAvailable;
    USHORT   usBytesRequired = 0;
    USHORT   iInfoCount = 0;
    USHORT   usInfoCount = 0;
    USHORT   usOffset = 0;
    PFILE_STREAM_INFORMATION pFileStreamInfoCursor = NULL;
    PSMB_FILE_STREAM_INFO_RESPONSE_HEADER pInfoHeaderPrev = NULL;
    PSMB_FILE_STREAM_INFO_RESPONSE_HEADER pInfoHeaderCur = NULL;

    pFileStreamInfoCursor = (PFILE_STREAM_INFORMATION)pFileStreamInfo;
    while (pFileStreamInfoCursor && (usBytesAvailable1 > 0))
    {
        USHORT usInfoBytesRequired = 0;

        usInfoBytesRequired += sizeof(SMB_FILE_STREAM_INFO_RESPONSE_HEADER);
        usInfoBytesRequired += pFileStreamInfoCursor->StreamNameLength;

        /* Null terminate all streams names but the last. */
        if (pFileStreamInfoCursor->NextEntryOffset != 0)
        {
            usInfoBytesRequired += sizeof(wchar16_t);

            if (usInfoBytesRequired % 8)
            {
                usInfoBytesRequired += 8 - (usInfoBytesRequired % 8);
            }
        }

        if (usBytesAvailable1 < usInfoBytesRequired)
        {
            break;
        }

        usInfoCount++;

        usBytesAvailable1 -= usInfoBytesRequired;
        usBytesRequired   += usInfoBytesRequired;

        if (pFileStreamInfoCursor->NextEntryOffset)
        {
            pFileStreamInfoCursor =
                (PFILE_STREAM_INFORMATION)(((PBYTE)pFileStreamInfoCursor) +
                                        pFileStreamInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileStreamInfoCursor = NULL;
        }
    }

    ntStatus = SrvAllocateMemory(usBytesRequired, (PVOID*)&pData);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pData;
    pFileStreamInfoCursor = (PFILE_STREAM_INFORMATION)pFileStreamInfo;

    for (; iInfoCount < usInfoCount; iInfoCount++)
    {
        pInfoHeaderPrev = pInfoHeaderCur;
        pInfoHeaderCur = (PSMB_FILE_STREAM_INFO_RESPONSE_HEADER)pDataCursor;

        /* Update next entry offset for previous entry. */
        if (pInfoHeaderPrev != NULL)
        {
            pInfoHeaderPrev->ulNextEntryOffset = usOffset;
        }

        /* Reset the offset to 0 since it's relative. */
        usOffset = 0;

        /* Add the header info. */
        pInfoHeaderCur->ulNextEntryOffset = 0;
        pInfoHeaderCur->llStreamAllocationSize =
                            pFileStreamInfoCursor->StreamAllocationSize;
        pInfoHeaderCur->llStreamSize = pFileStreamInfoCursor->StreamSize;
        pInfoHeaderCur->ulStreamNameLength =
                            pFileStreamInfoCursor->StreamNameLength;

        pDataCursor += sizeof(SMB_FILE_STREAM_INFO_RESPONSE_HEADER);
        usOffset    += sizeof(SMB_FILE_STREAM_INFO_RESPONSE_HEADER);

        memcpy( pDataCursor,
                pFileStreamInfoCursor->StreamName,
                pFileStreamInfoCursor->StreamNameLength);

        pDataCursor += pFileStreamInfoCursor->StreamNameLength;
        usOffset    += pFileStreamInfoCursor->StreamNameLength;

        /* Null terminate all streams names but the last. */
        if (pFileStreamInfoCursor->NextEntryOffset != 0)
        {
            pDataCursor += sizeof(wchar16_t);
            usOffset    += sizeof(wchar16_t);

            if (usOffset % 8)
            {
               USHORT usAlign = 8 - (usOffset % 8);

               pDataCursor += usAlign;
               usOffset    += usAlign;
            }
        }

        pFileStreamInfoCursor =
                    (PFILE_STREAM_INFORMATION)(((PBYTE)pFileStreamInfoCursor) +
                                    pFileStreamInfoCursor->NextEntryOffset);
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

static
NTSTATUS
SrvQueryAllInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    BOOLEAN                    bContinue    = TRUE;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    do
    {
        ntStatus = pTrans2State->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_BUFFER_TOO_SMALL:

                {
                    USHORT usNewSize =  0;

                    if (!pTrans2State->usBytesAllocated)
                    {
                        usNewSize = pTrans2State->usBytesAllocated +
                                        sizeof(FILE_ALL_INFORMATION) +
                                        256 * sizeof(wchar16_t);
                    }
                    else
                    {
                        usNewSize = pTrans2State->usBytesAllocated +
                                        256 * sizeof(wchar16_t);
                    }

                    ntStatus = SMBReallocMemory(
                                    pTrans2State->pData2,
                                    (PVOID*)&pTrans2State->pData2,
                                    usNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pTrans2State->usBytesAllocated = usNewSize;

                    SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

                    ntStatus = IoQueryInformationFile(
                                            (pTrans2State->pFile ?
                                                    pTrans2State->pFile->hFile :
                                                    pTrans2State->hFile),
                                            pTrans2State->pAcb,
                                            &pTrans2State->ioStatusBlock,
                                            pTrans2State->pData2,
                                            pTrans2State->usBytesAllocated,
                                            FileAllInformation);
                    switch (ntStatus)
                    {
                        case STATUS_SUCCESS:

                            bContinue = FALSE;

                            pTrans2State->usBytesUsed =
                                pTrans2State->ioStatusBlock.BytesTransferred;

                            // intentional fall through

                        case STATUS_BUFFER_TOO_SMALL:

                            // synchronous completion
                            SrvReleaseTrans2StateAsync(pTrans2State);

                            break;

                        default:

                            BAIL_ON_NT_STATUS(ntStatus);
                    }
                }

                break;

            case STATUS_SUCCESS:

                if (!pTrans2State->pData2)
                {
                    pTrans2State->ioStatusBlock.Status =
                                            STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    bContinue = FALSE;

                    pTrans2State->usBytesUsed =
                                pTrans2State->ioStatusBlock.BytesTransferred;
                }

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

    } while (bContinue);

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildQueryAllInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE   pOutBuffer        = pSmbResponse->pBuffer;
    ULONG   ulBytesAvailable  = pSmbResponse->ulBytesAvailable;
    ULONG   ulOffset          = 0;
    USHORT  usBytesUsed       = 0;
    ULONG   ulTotalBytesUsed  = 0;
    USHORT  usParam           = 0;
    PUSHORT pSetup            = NULL;
    BYTE    setupCount        = 0;
    USHORT  usDataOffset      = 0;
    USHORT  usParameterOffset = 0;
    PFILE_ALL_INFORMATION       pFileAllInfo      = NULL;
    PSRV_TRANS2_STATE_SMB_V1    pTrans2State      = NULL;
    PBYTE   pData             = NULL;
    USHORT  usDataLen         = 0;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;
    pFileAllInfo = (PFILE_ALL_INFORMATION)pTrans2State->pData2;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION2,
                        STATUS_SUCCESS,
                        TRUE,
                        pTrans2State->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pTrans2State->pSession->uid,
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

    ntStatus = SrvMarshallFileAllInfo(
                    pTrans2State->pData2,
                    pTrans2State->usBytesUsed,
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
SrvMarshallFileAllInfo(
    PBYTE   pInfoBuffer,
    USHORT  usBytesAvailable,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE    pData = NULL;
    USHORT   usBytesRequired = 0;
    PFILE_ALL_INFORMATION pFileAllInfo = (PFILE_ALL_INFORMATION)pInfoBuffer;
    PTRANS2_FILE_ALL_INFORMATION pFileAllInfoPacked = NULL;

    usBytesRequired = sizeof(*pFileAllInfoPacked) +
                      pFileAllInfo->NameInformation.FileNameLength;

    ntStatus = SrvAllocateMemory(usBytesRequired, (PVOID*)&pData);
    BAIL_ON_NT_STATUS(ntStatus);

    pFileAllInfoPacked = (PTRANS2_FILE_ALL_INFORMATION)pData;

    pFileAllInfoPacked->ChangeTime     = pFileAllInfo->BasicInformation.ChangeTime;
    pFileAllInfoPacked->CreationTime   = pFileAllInfo->BasicInformation.CreationTime;
    pFileAllInfoPacked->FileAttributes = pFileAllInfo->BasicInformation.FileAttributes;
    pFileAllInfoPacked->LastAccessTime = pFileAllInfo->BasicInformation.LastAccessTime;
    pFileAllInfoPacked->LastWriteTime  = pFileAllInfo->BasicInformation.LastWriteTime;

    pFileAllInfoPacked->AllocationSize = pFileAllInfo->StandardInformation.AllocationSize;
    pFileAllInfoPacked->EndOfFile      = pFileAllInfo->StandardInformation.EndOfFile;
    pFileAllInfoPacked->NumberOfLinks  = pFileAllInfo->StandardInformation.NumberOfLinks;
    pFileAllInfoPacked->bDeletePending = pFileAllInfo->StandardInformation.DeletePending;
    pFileAllInfoPacked->bDirectory     = pFileAllInfo->StandardInformation.Directory;

    pFileAllInfoPacked->EaSize         = pFileAllInfo->EaInformation.EaSize;

    pFileAllInfoPacked->FileNameLength = pFileAllInfo->NameInformation.FileNameLength;
    memcpy(pFileAllInfoPacked->FileName,
           pFileAllInfo->NameInformation.FileName,
           pFileAllInfo->NameInformation.FileNameLength);

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

static
NTSTATUS
SrvQueryNameInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    BOOLEAN                    bContinue    = TRUE;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    do
    {
        ntStatus = pTrans2State->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_BUFFER_TOO_SMALL:

                {
                    USHORT usNewSize =  0;

                    if (!pTrans2State->usBytesAllocated)
                    {
                        usNewSize = pTrans2State->usBytesAllocated +
                                        sizeof(FILE_NAME_INFORMATION) +
                                        256 * sizeof(wchar16_t);
                    }
                    else
                    {
                        usNewSize = pTrans2State->usBytesAllocated +
                                        256 * sizeof(wchar16_t);
                    }

                    ntStatus = SMBReallocMemory(
                                    pTrans2State->pData2,
                                    (PVOID*)&pTrans2State->pData2,
                                    usNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pTrans2State->usBytesAllocated = usNewSize;

                    SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

                    ntStatus = IoQueryInformationFile(
                                            (pTrans2State->pFile ?
                                                    pTrans2State->pFile->hFile :
                                                    pTrans2State->hFile),
                                            pTrans2State->pAcb,
                                            &pTrans2State->ioStatusBlock,
                                            pTrans2State->pData2,
                                            pTrans2State->usBytesAllocated,
                                            FileNameInformation);
                    switch (ntStatus)
                    {
                        case STATUS_SUCCESS:

                            bContinue = FALSE;

                            pTrans2State->usBytesUsed =
                                   pTrans2State->ioStatusBlock.BytesTransferred;

                            // intentional fall through

                        case STATUS_BUFFER_TOO_SMALL:

                            // synchronous completion
                            SrvReleaseTrans2StateAsync(pTrans2State);

                            break;

                        default:

                            BAIL_ON_NT_STATUS(ntStatus);
                    }
                }

                break;

            case STATUS_SUCCESS:

                if (!pTrans2State->pData2)
                {
                    pTrans2State->ioStatusBlock.Status =
                                            STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    bContinue = FALSE;

                    pTrans2State->usBytesUsed =
                           pTrans2State->ioStatusBlock.BytesTransferred;
                }

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

    } while (bContinue);

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildQueryNameInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE   pOutBuffer        = pSmbResponse->pBuffer;
    ULONG   ulBytesAvailable  = pSmbResponse->ulBytesAvailable;
    ULONG   ulOffset          = 0;
    USHORT  usBytesUsed       = 0;
    ULONG   ulTotalBytesUsed  = 0;
    USHORT  usParam           = 0;
    PUSHORT pSetup            = NULL;
    BYTE    setupCount        = 0;
    USHORT  usDataOffset      = 0;
    USHORT  usParameterOffset = 0;
    PFILE_NAME_INFORMATION  pFileNameInfo = NULL;
    PSRV_TRANS2_STATE_SMB_V1 pTrans2State   = NULL;
    PBYTE   pData             = NULL;
    USHORT  usDataLen         = 0;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;
    pFileNameInfo = (PFILE_NAME_INFORMATION)pTrans2State->pData2;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION2,
                        STATUS_SUCCESS,
                        TRUE,
                        pTrans2State->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pTrans2State->pSession->uid,
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

    ntStatus = SrvMarshallFileNameInfo(
                    pTrans2State->pTree,
                    pTrans2State->pData2,
                    pTrans2State->usBytesUsed,
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
SrvMarshallFileNameInfo(
    PLWIO_SRV_TREE pTree,
    PBYTE          pInfoBuffer,
    USHORT         usBytesAvailable,
    PBYTE*         ppData,
    PUSHORT        pusDataLen
    )
{
    NTSTATUS ntStatus        = STATUS_SUCCESS;
    PBYTE    pData           = NULL;
    USHORT   usBytesRequired = 0;
    BOOLEAN  bInLock         = FALSE;
    PWSTR     pwszTreePath   = NULL; // Do not free
    PFILE_NAME_INFORMATION pFileNameInfo =
                                        (PFILE_NAME_INFORMATION)pInfoBuffer;
    PTRANS2_FILE_NAME_INFORMATION pFileNameInfoPacked = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->mutex);

    ntStatus = SrvGetTreeRelativePath(
                    pTree->pShareInfo->pwszPath,
                    &pwszTreePath);
    BAIL_ON_NT_STATUS(ntStatus);

    if (STATUS_SUCCESS != SrvMatchPathPrefix(
                                pFileNameInfo->FileName,
                                pFileNameInfo->FileNameLength/sizeof(wchar16_t),
                                pwszTreePath))
    {
        ntStatus = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ULONG ulPrefixLength = wc16slen(pwszTreePath) * sizeof(wchar16_t);

        usBytesRequired = sizeof(TRANS2_FILE_NAME_INFORMATION) +
                              pFileNameInfo->FileNameLength - ulPrefixLength;

        ntStatus = SrvAllocateMemory(usBytesRequired, (PVOID*)&pData);
        BAIL_ON_NT_STATUS(ntStatus);

        pFileNameInfoPacked = (PTRANS2_FILE_NAME_INFORMATION)pData;

        pFileNameInfoPacked->ulFileNameLength =
                    pFileNameInfo->FileNameLength - ulPrefixLength;

        memcpy((PBYTE)pFileNameInfoPacked->FileName,
               (PBYTE)pFileNameInfo->FileName + ulPrefixLength,
               pFileNameInfoPacked->ulFileNameLength);
    }

    *ppData = pData;
    *pusDataLen = usBytesRequired;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

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

static
NTSTATUS
SrvQueryAltNameInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    BOOLEAN                    bContinue    = TRUE;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    do
    {
        ntStatus = pTrans2State->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_BUFFER_TOO_SMALL:

                {
                    USHORT usNewSize =  0;

                    if (!pTrans2State->usBytesAllocated)
                    {
                        usNewSize = pTrans2State->usBytesAllocated +
                                        sizeof(FILE_NAME_INFORMATION) +
                                        256 * sizeof(wchar16_t);
                    }
                    else
                    {
                        usNewSize = pTrans2State->usBytesAllocated +
                                        256 * sizeof(wchar16_t);
                    }

                    ntStatus = SMBReallocMemory(
                                    pTrans2State->pData2,
                                    (PVOID*)&pTrans2State->pData2,
                                    usNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pTrans2State->usBytesAllocated = usNewSize;

                    SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

                    ntStatus = IoQueryInformationFile(
                                            (pTrans2State->pFile ?
                                                    pTrans2State->pFile->hFile :
                                                    pTrans2State->hFile),
                                            pTrans2State->pAcb,
                                            &pTrans2State->ioStatusBlock,
                                            pTrans2State->pData2,
                                            pTrans2State->usBytesAllocated,
                                            FileAlternateNameInformation);
                    switch (ntStatus)
                    {
                        case STATUS_SUCCESS:

                            bContinue = FALSE;

                            pTrans2State->usBytesUsed =
                                pTrans2State->ioStatusBlock.BytesTransferred;

                            // intentional fall through

                        case STATUS_BUFFER_TOO_SMALL:

                            // synchronous completion
                            SrvReleaseTrans2StateAsync(pTrans2State);

                            break;

                        default:

                            BAIL_ON_NT_STATUS(ntStatus);
                    }
                }

                break;

            case STATUS_SUCCESS:

                if (!pTrans2State->pData2)
                {
                    pTrans2State->ioStatusBlock.Status =
                                            STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    bContinue = FALSE;

                    pTrans2State->usBytesUsed =
                        pTrans2State->ioStatusBlock.BytesTransferred;
                }

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

    } while (bContinue);

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildQueryAltNameInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    return SrvBuildQueryNameInfoResponse(pExecContext);
}
