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
SrvUnmarshallQueryFileInfoParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PUSHORT          pusFid,
    PSMB_INFO_LEVEL* ppSmbInfoLevel
    );

static
NTSTATUS
SrvQueryFileInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildQueryFileInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvMarshallFileStreamInfo(
    PBYTE   pFileStreamInfo,
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

NTSTATUS
SrvProcessTrans2QueryFileInformation(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    switch (pTrans2State->stage)
    {
        case SRV_TRANS2_STAGE_SMB_V1_INITIAL:

            ntStatus = SrvUnmarshallQueryFileInfoParams(
                            pTrans2State->pParameters,
                            pTrans2State->pRequestHeader->parameterCount,
                            &pTrans2State->usFid,
                            &pTrans2State->pSmbInfoLevel);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvConnectionFindSession_SMB_V1(
                            pCtxSmb1,
                            pConnection,
                            pSmbRequest->pHeader->uid,
                            &pTrans2State->pSession);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvSessionFindTree_SMB_V1(
                            pCtxSmb1,
                            pTrans2State->pSession,
                            pSmbRequest->pHeader->tid,
                            &pTrans2State->pTree);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvTreeFindFile_SMB_V1(
                            pCtxSmb1,
                            pTrans2State->pTree,
                            pTrans2State->usFid,
                            &pTrans2State->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_ATTEMPT_IO;

            // intentional fall through

        case SRV_TRANS2_STAGE_SMB_V1_ATTEMPT_IO:

            ntStatus = SrvQueryFileInfo(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_TRANS2_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildQueryFileInfoResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_TRANS2_STAGE_SMB_V1_DONE:

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

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
SrvQueryFileInfo(
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

            ntStatus = SrvQueryFileBasicInfo(pExecContext);

            break;

        case SMB_QUERY_FILE_STANDARD_INFO :

            ntStatus = SrvQueryFileStandardInfo(pExecContext);

            break;

        case SMB_QUERY_FILE_EA_INFO :

            ntStatus = SrvQueryFileEAInfo(pExecContext);

            break;

        case SMB_QUERY_FILE_STREAM_INFO :

            ntStatus = SrvQueryFileStreamInfo(pExecContext);

            break;

        case SMB_QUERY_FILE_ALL_INFO :

            ntStatus = SrvQueryFileAllInfo(pExecContext);

            break;

        case SMB_INFO_STANDARD :
        case SMB_INFO_QUERY_EA_SIZE :
        case SMB_INFO_QUERY_EAS_FROM_LIST :
        case SMB_INFO_QUERY_ALL_EAS :
        case SMB_INFO_IS_NAME_VALID :
        case SMB_QUERY_FILE_NAME_INFO :
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

    return ntStatus;
}

static
NTSTATUS
SrvBuildQueryFileInfoResponse(
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

            ntStatus = SrvBuildQueryFileBasicInfoResponse(pExecContext);

            break;

        case SMB_QUERY_FILE_STANDARD_INFO :

            ntStatus = SrvBuildQueryFileStandardInfoResponse(pExecContext);

            break;

        case SMB_QUERY_FILE_EA_INFO :

            ntStatus = SrvBuildQueryFileEAInfoResponse(pExecContext);

            break;

        case SMB_QUERY_FILE_STREAM_INFO :

            ntStatus = SrvBuildQueryFileStreamInfoResponse(pExecContext);

            break;

        case SMB_QUERY_FILE_ALL_INFO :

            ntStatus = SrvBuildQueryFileAllInfoResponse(pExecContext);

            break;

        case SMB_INFO_STANDARD :
        case SMB_INFO_QUERY_EA_SIZE :
        case SMB_INFO_QUERY_EAS_FROM_LIST :
        case SMB_INFO_QUERY_ALL_EAS :
        case SMB_INFO_IS_NAME_VALID :
        case SMB_QUERY_FILE_NAME_INFO :
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

    return ntStatus;
}

NTSTATUS
SrvQueryFileBasicInfo(
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

NTSTATUS
SrvBuildQueryFileBasicInfoResponse(
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

NTSTATUS
SrvQueryFileStandardInfo(
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

NTSTATUS
SrvBuildQueryFileStandardInfoResponse(
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

NTSTATUS
SrvQueryFileEAInfo(
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

NTSTATUS
SrvBuildQueryFileEAInfoResponse(
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

NTSTATUS
SrvQueryFileStreamInfo(
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

NTSTATUS
SrvBuildQueryFileStreamInfoResponse(
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

    ntStatus = SrvMarshallFileStreamInfo(
                    pTrans2State->pData2,
                    pTrans2State->usBytesAllocated,
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


NTSTATUS
SrvQueryFileAllInfo(
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
                                            FileAllInformation);
                    switch (ntStatus)
                    {
                        case STATUS_SUCCESS:

                            bContinue = FALSE;

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

NTSTATUS
SrvBuildQueryFileAllInfoResponse(
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
                    pTrans2State->usBytesAllocated,
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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
