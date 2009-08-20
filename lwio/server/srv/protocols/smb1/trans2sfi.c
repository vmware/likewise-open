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
SrvUnmarshallSetFileInfoParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PUSHORT          pusFid,
    PSMB_INFO_LEVEL* ppSmbInfoLevel
    );

static
NTSTATUS
SrvSetFileInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetFileBasicInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetFileDispositionInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetFileAllocationInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetEndOfFileInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildSetFileInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvProcessTrans2SetFileInformation(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
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

            ntStatus = SrvUnmarshallSetFileInfoParams(
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

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_IO_COMPLETE;

            ntStatus = SrvSetFileInfoResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            // intentional fall through

        case SRV_TRANS2_STAGE_SMB_V1_IO_COMPLETE:

            ntStatus = pTrans2State->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_TRANS2_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildSetFileInfoResponse(pExecContext);
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

error:

    return ntStatus;
}

static
NTSTATUS
SrvUnmarshallSetFileInfoParams(
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
SrvSetFileInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    switch (*pTrans2State->pSmbInfoLevel)
    {
        case SMB_SET_FILE_BASIC_INFO :

            ntStatus = SrvSetFileBasicInfo(pExecContext);

            break;

        case SMB_SET_FILE_DISPOSITION_INFO :

            ntStatus = SrvSetFileDispositionInfo(pExecContext);

            break;

        case SMB_SET_FILE_ALLOCATION_INFO :

            ntStatus = SrvSetFileAllocationInfo(pExecContext);

            break;

        case SMB_SET_FILE_END_OF_FILE_INFO :

            ntStatus = SrvSetEndOfFileInfo(pExecContext);

            break;

        case SMB_INFO_STANDARD :
        case SMB_INFO_QUERY_EA_SIZE :
        case SMB_SET_FILE_UNIX_BASIC :
        case SMB_SET_FILE_UNIX_LINK :
        case SMB_SET_FILE_UNIX_HLINK :

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
SrvSetFileBasicInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus       = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol   = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1       = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State   = NULL;
    PFILE_BASIC_INFORMATION    pFileBasicInfo = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    if (pTrans2State->pRequestHeader->dataCount < sizeof(FILE_BASIC_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileBasicInfo = (PFILE_BASIC_INFORMATION)pTrans2State->pData;

    SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

    ntStatus = IoSetInformationFile(
                    pTrans2State->pFile->hFile,
                    pTrans2State->pAcb,
                    &pTrans2State->ioStatusBlock,
                    pFileBasicInfo,
                    sizeof(FILE_BASIC_INFORMATION),
                    FileBasicInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously

error:

    return ntStatus;
}

static
NTSTATUS
SrvSetFileDispositionInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                      ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT    pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1      pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1      pTrans2State = NULL;
    PFILE_DISPOSITION_INFORMATION pFileDispositionInfo = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    if (pTrans2State->pRequestHeader->dataCount  < sizeof(FILE_DISPOSITION_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileDispositionInfo = (PFILE_DISPOSITION_INFORMATION)pTrans2State->pData;

    SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

    ntStatus = IoSetInformationFile(
                    pTrans2State->pFile->hFile,
                    pTrans2State->pAcb,
                    &pTrans2State->ioStatusBlock,
                    pFileDispositionInfo,
                    sizeof(FILE_DISPOSITION_INFORMATION),
                    FileDispositionInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously

error:

    return ntStatus;
}

static
NTSTATUS
SrvSetFileAllocationInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1     pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1     pTrans2State = NULL;
    PFILE_ALLOCATION_INFORMATION pFileAllocationInfo = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    if (pTrans2State->pRequestHeader->dataCount < sizeof(FILE_ALLOCATION_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileAllocationInfo = (PFILE_ALLOCATION_INFORMATION)pTrans2State->pData;

    SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

    ntStatus = IoSetInformationFile(
                    pTrans2State->pFile->hFile,
                    pTrans2State->pAcb,
                    &pTrans2State->ioStatusBlock,
                    pFileAllocationInfo,
                    sizeof(FILE_ALLOCATION_INFORMATION),
                    FileAllocationInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously

error:

    return ntStatus;
}

static
NTSTATUS
SrvSetEndOfFileInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                      ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT    pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1      pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1      pTrans2State = NULL;
    PFILE_END_OF_FILE_INFORMATION pFileEofInfo = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    if (pTrans2State->pRequestHeader->dataCount < sizeof(FILE_END_OF_FILE_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileEofInfo = (PFILE_END_OF_FILE_INFORMATION)pTrans2State->pData;

    SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

    ntStatus = IoSetInformationFile(
                    pTrans2State->pFile->hFile,
                    pTrans2State->pAcb,
                    &pTrans2State->ioStatusBlock,
                    pFileEofInfo,
                    sizeof(FILE_END_OF_FILE_INFORMATION),
                    FileEndOfFileInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildSetFileInfoResponse(
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
    PUSHORT pSetup            = NULL;
    BYTE    setupCount        = 0;
    USHORT  usParams          = 0;
    USHORT  usDataOffset      = 0;
    USHORT  usParameterOffset = 0;

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
                    (PBYTE)&usParams,
                    sizeof(usParams),
                    NULL,
                    0,
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
