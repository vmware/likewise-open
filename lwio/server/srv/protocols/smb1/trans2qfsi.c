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
SrvQueryFilesystemInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildQueryFilesystemInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvQueryFSAllocationInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFSAllocationInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvQueryFSInfoVolume(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFSInfoVolumeResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvMarshallFSInfoVolume(
    PBYTE   pVolumeInfo,
    USHORT  usBytesAllocated,
    USHORT  usDataOffset,
    USHORT  usMaxDataCount,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    );

static
NTSTATUS
SrvQueryFSVolumeInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFSVolumeInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvMarshallFSVolumeInfo(
    PBYTE   pVolumeInfo,
    USHORT  usBytesAllocated,
    USHORT  usMaxDataCount,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    );

static
NTSTATUS
SrvQueryFSSizeInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFSSizeInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvQueryFSAttributeInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFSAttributeInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvMarshallFSAttributeInfo(
    PBYTE   pVolumeInfo,
    USHORT  usBytesAllocated,
    USHORT  usMaxDataCount,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    );

NTSTATUS
SrvProcessTrans2QueryFilesystemInformation(
    PSRV_EXEC_CONTEXT       pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    BOOLEAN                    bInLock      = FALSE;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    switch (pTrans2State->stage)
    {
        case SRV_TRANS2_STAGE_SMB_V1_INITIAL:

            if ((pTrans2State->pRequestHeader->parameterCount != 2) &&
                (pTrans2State->pRequestHeader->parameterCount != 4))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

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

            LWIO_LOCK_RWMUTEX_SHARED(   bInLock,
                                        &pTrans2State->pTree->pShareInfo->mutex);

            ntStatus = SrvAllocateStringW(
                            pTrans2State->pTree->pShareInfo->pwszPath,
                            &pTrans2State->fileName.FileName);
            BAIL_ON_NT_STATUS(ntStatus);

            LWIO_UNLOCK_RWMUTEX(bInLock,
                                &pTrans2State->pTree->pShareInfo->mutex);

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_CREATE_FILE_COMPLETED;

            SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

            ntStatus = IoCreateFile(
                            &pTrans2State->hFile,
                            pTrans2State->pAcb,
                            &pTrans2State->ioStatusBlock,
                            pTrans2State->pSession->pIoSecurityContext,
                            &pTrans2State->fileName,
                            pTrans2State->pSecurityDescriptor,
                            pTrans2State->pSecurityQOS,
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

            SrvReleaseTrans2StateAsync(pTrans2State); // completed sync

            // intentional fall through

        case SRV_TRANS2_STAGE_SMB_V1_CREATE_FILE_COMPLETED:

            ntStatus = pTrans2State->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_ATTEMPT_IO;

            // intentional fall through

        case SRV_TRANS2_STAGE_SMB_V1_ATTEMPT_IO:

            ntStatus = SrvQueryFilesystemInfo(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_TRANS2_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildQueryFilesystemInfoResponse(pExecContext);
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

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTrans2State->pTree->pShareInfo->mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvQueryFilesystemInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    switch (*((PUSHORT)pTrans2State->pParameters))
    {
        case SMB_INFO_ALLOCATION:

            ntStatus = SrvQueryFSAllocationInfo(pExecContext);

            break;

        case SMB_INFO_VOLUME:

            ntStatus = SrvQueryFSInfoVolume(pExecContext);

            break;

        case SMB_QUERY_FS_VOLUME_INFO:

            ntStatus = SrvQueryFSVolumeInfo(pExecContext);

            break;

        case SMB_QUERY_FS_SIZE_INFO:

            ntStatus = SrvQueryFSSizeInfo(pExecContext);

            break;

        case SMB_QUERY_FS_ATTRIBUTE_INFO:

            ntStatus = SrvQueryFSAttributeInfo(pExecContext);

            break;

        case SMB_QUERY_FS_DEVICE_INFO:
        case SMB_QUERY_CIFS_UNIX_INFO:
        case SMB_QUERY_MAC_FS_INFO:

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
SrvBuildQueryFilesystemInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    switch (*((PUSHORT)pTrans2State->pParameters))
    {
        case SMB_INFO_ALLOCATION:

            ntStatus = SrvBuildFSAllocationInfoResponse(pExecContext);

            break;

        case SMB_INFO_VOLUME:

            ntStatus = SrvBuildFSInfoVolumeResponse(pExecContext);

            break;

        case SMB_QUERY_FS_VOLUME_INFO:

            ntStatus = SrvBuildFSVolumeInfoResponse(pExecContext);

            break;

        case SMB_QUERY_FS_SIZE_INFO:

            ntStatus = SrvBuildFSSizeInfoResponse(pExecContext);

            break;

        case SMB_QUERY_FS_ATTRIBUTE_INFO:

            ntStatus = SrvBuildFSAttributeInfoResponse(pExecContext);

            break;

        case SMB_QUERY_FS_DEVICE_INFO:
        case SMB_QUERY_CIFS_UNIX_INFO:
        case SMB_QUERY_MAC_FS_INFO:

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
SrvQueryFSAllocationInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    return SrvQueryFSSizeInfo(pExecContext);
}

static
NTSTATUS
SrvBuildFSAllocationInfoResponse(
    PSRV_EXEC_CONTEXT       pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE    pOutBuffer            = pSmbResponse->pBuffer;
    ULONG    ulBytesAvailable      = pSmbResponse->ulBytesAvailable;
    ULONG    ulOffset              = 0;
    USHORT   usBytesUsed           = 0;
    ULONG    ulTotalBytesUsed      = 0;
    PUSHORT  pSetup                = NULL;
    BYTE     setupCount            = 0;
    USHORT   usDataOffset          = 0;
    USHORT   usParameterOffset     = 0;
    SMB_FS_INFO_ALLOCATION   fsAllocInfo = {0};
    PFILE_FS_SIZE_INFORMATION pFSSizeInfo = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;
    pFSSizeInfo = (PFILE_FS_SIZE_INFORMATION)pTrans2State->pData2;

    // TODO: Resolve large integer values
    fsAllocInfo.ulFileSystemId = 0;
    fsAllocInfo.ulNumAllocationUnits =
            SMB_MIN(UINT32_MAX, pFSSizeInfo->TotalAllocationUnits);
    fsAllocInfo.ulNumSectorsPerAllocationUnit =
            pFSSizeInfo->SectorsPerAllocationUnit;
    fsAllocInfo.ulNumUnitsAvailable =
            SMB_MIN(UINT32_MAX, pFSSizeInfo->AvailableAllocationUnits);
    fsAllocInfo.usNumBytesPerSector =
            SMB_MIN(UINT16_MAX, pFSSizeInfo->BytesPerSector);

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

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
                    (PBYTE)&fsAllocInfo,
                    sizeof(fsAllocInfo),
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
SrvQueryFSInfoVolume(
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
                                        sizeof(FILE_FS_VOLUME_INFORMATION) +
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

                    ntStatus = IoQueryVolumeInformationFile(
                                    pTrans2State->hFile,
                                    pTrans2State->pAcb,
                                    &pTrans2State->ioStatusBlock,
                                    pTrans2State->pData2,
                                    pTrans2State->usBytesAllocated,
                                    FileFsVolumeInformation);
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

static
NTSTATUS
SrvBuildFSInfoVolumeResponse(
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
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    PBYTE    pOutBuffer        = pSmbResponse->pBuffer;
    ULONG    ulBytesAvailable  = pSmbResponse->ulBytesAvailable;
    ULONG    ulOffset          = 0;
    USHORT   usBytesUsed       = 0;
    ULONG    ulTotalBytesUsed  = 0;
    PBYTE    pData             = NULL;
    USHORT   usDataLen         = 0;
    PUSHORT  pSetup            = NULL;
    BYTE     setupCount        = 0;
    USHORT   usDataOffset      = 0;
    USHORT   usParameterOffset = 0;

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

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
                    pData,
                    usDataLen,
                    &usDataOffset,
                    &usParameterOffset,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshallFSInfoVolume(
                    pTrans2State->pData2,
                    pTrans2State->usBytesAllocated,
                    usDataOffset,
                    pTrans2State->pRequestHeader->maxDataCount,
                    &pData,
                    &usDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
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
SrvMarshallFSInfoVolume(
    PBYTE   pVolumeInfo,
    USHORT  usBytesAllocated,
    USHORT  usDataOffset,
    USHORT  usMaxDataCount,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    PBYTE    pDataCursor = NULL;
    USHORT   usBytesRequired = 0;
    PFILE_FS_VOLUME_INFORMATION pFSVolInfo = NULL;
    PSMB_FS_INFO_VOLUME_HEADER pFSInfoVolHeader = NULL;
    USHORT   usVolumeLabelByteLen = 0;
    USHORT   usAlignment = 0;

    pFSVolInfo = (PFILE_FS_VOLUME_INFORMATION)pVolumeInfo;

    usBytesRequired = sizeof(SMB_FS_INFO_VOLUME_HEADER);
    usDataOffset += sizeof(SMB_FS_INFO_VOLUME_HEADER);

    if (pFSVolInfo->VolumeLabel && *pFSVolInfo->VolumeLabel)
    {
        usAlignment = usDataOffset % 2;

        usBytesRequired += usAlignment;
        // usDataOffset += usAlignment;

        usVolumeLabelByteLen = SMB_MIN(UINT8_MAX, pFSVolInfo->VolumeLabelLength);

        usBytesRequired += usVolumeLabelByteLen;

        if (usBytesRequired > usMaxDataCount)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    ntStatus = SrvAllocateMemory(usBytesRequired, (PVOID*)&pData);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pData;
    pFSInfoVolHeader = (PSMB_FS_INFO_VOLUME_HEADER)pDataCursor;
    pFSInfoVolHeader->ulVolumeSerialNumber = pFSVolInfo->VolumeSerialNumber;
    pFSInfoVolHeader->ucNumLabelChars = usVolumeLabelByteLen;

    pDataCursor += sizeof(SMB_FS_INFO_VOLUME_HEADER);

    if (usVolumeLabelByteLen)
    {
        pDataCursor += usAlignment;

        memcpy(pDataCursor, (PBYTE)pFSVolInfo->VolumeLabel, usVolumeLabelByteLen);
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
SrvQueryFSVolumeInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    return SrvQueryFSInfoVolume(pExecContext);
}

static
NTSTATUS
SrvBuildFSVolumeInfoResponse(
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
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    PBYTE    pOutBuffer            = pSmbResponse->pBuffer;
    ULONG    ulBytesAvailable      = pSmbResponse->ulBytesAvailable;
    ULONG    ulOffset              = 0;
    USHORT   usBytesUsed           = 0;
    ULONG    ulTotalBytesUsed      = 0;
    PBYTE    pData = NULL;
    USHORT   usDataLen = 0;
    PUSHORT  pSetup = NULL;
    BYTE     setupCount = 0;
    USHORT   usDataOffset = 0;
    USHORT   usParameterOffset = 0;

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

    ntStatus = SrvMarshallFSVolumeInfo(
                    pTrans2State->pData2,
                    pTrans2State->usBytesAllocated,
                    pTrans2State->pRequestHeader->maxDataCount,
                    &pData,
                    &usDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
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
SrvMarshallFSVolumeInfo(
    PBYTE   pVolumeInfo,
    USHORT  usBytesAllocated,
    USHORT  usMaxDataCount,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    PBYTE    pDataCursor = NULL;
    USHORT   usBytesRequired = 0;
    PFILE_FS_VOLUME_INFORMATION pFSVolInfo = NULL;
    PSMB_FS_VOLUME_INFO_HEADER pFSVolInfoHeader = NULL;

    pFSVolInfo = (PFILE_FS_VOLUME_INFORMATION)pVolumeInfo;

    usBytesRequired = sizeof(SMB_FS_VOLUME_INFO_HEADER);

    usBytesRequired += pFSVolInfo->VolumeLabelLength;

    if (usBytesRequired > usMaxDataCount)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(usBytesRequired, (PVOID*)&pData);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pData;
    pFSVolInfoHeader = (PSMB_FS_VOLUME_INFO_HEADER)pDataCursor;
    pFSVolInfoHeader->bSupportsObjects = pFSVolInfo->SupportsObjects;
    pFSVolInfoHeader->llVolumeCreationTime = pFSVolInfo->VolumeCreationTime;
    pFSVolInfoHeader->ulVolumeSerialNumber = pFSVolInfo->VolumeSerialNumber;
    pFSVolInfoHeader->ulVolumeLabelLength = pFSVolInfo->VolumeLabelLength;

    pDataCursor += sizeof(SMB_FS_VOLUME_INFO_HEADER);

    if (pFSVolInfo->VolumeLabelLength)
    {
        memcpy( pDataCursor,
                (PBYTE)pFSVolInfo->VolumeLabel,
                pFSVolInfo->VolumeLabelLength);
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
SrvQueryFSSizeInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    ntStatus = pTrans2State->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pTrans2State->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_FS_SIZE_INFORMATION),
                        (PVOID*)&pTrans2State->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pTrans2State->usBytesAllocated = sizeof(FILE_FS_SIZE_INFORMATION);

        ntStatus = IoQueryVolumeInformationFile(
                            pTrans2State->hFile,
                            pTrans2State->pAcb,
                            &pTrans2State->ioStatusBlock,
                            pTrans2State->pData2,
                            pTrans2State->usBytesAllocated,
                            FileFsSizeInformation);
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFSSizeInfoResponse(
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
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    PBYTE   pOutBuffer        = pSmbResponse->pBuffer;
    ULONG   ulBytesAvailable  = pSmbResponse->ulBytesAvailable;
    ULONG   ulOffset          = 0;
    USHORT  usBytesUsed       = 0;
    ULONG   ulTotalBytesUsed  = 0;
    PUSHORT pSetup            = NULL;
    BYTE    setupCount        = 0;
    USHORT  usDataOffset      = 0;
    USHORT  usParameterOffset = 0;

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

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
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
SrvQueryFSAttributeInfo(
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
                                        sizeof(FILE_FS_ATTRIBUTE_INFORMATION) +
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

                    ntStatus = IoQueryVolumeInformationFile(
                                    pTrans2State->hFile,
                                    pTrans2State->pAcb,
                                    &pTrans2State->ioStatusBlock,
                                    pTrans2State->pData2,
                                    pTrans2State->usBytesAllocated,
                                    FileFsAttributeInformation);
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

static
NTSTATUS
SrvBuildFSAttributeInfoResponse(
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
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    PBYTE   pOutBuffer        = pSmbResponse->pBuffer;
    ULONG   ulBytesAvailable  = pSmbResponse->ulBytesAvailable;
    ULONG   ulOffset          = 0;
    USHORT  usBytesUsed       = 0;
    ULONG   ulTotalBytesUsed  = 0;
    PBYTE   pData             = NULL;
    USHORT  usDataLen         = 0;
    PUSHORT pSetup            = NULL;
    BYTE    setupCount        = 0;
    USHORT  usDataOffset      = 0;
    USHORT  usParameterOffset = 0;

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

    ntStatus = SrvMarshallFSAttributeInfo(
                    pTrans2State->pData2,
                    pTrans2State->usBytesAllocated,
                    pTrans2State->pRequestHeader->maxDataCount,
                    &pData,
                    &usDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
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
SrvMarshallFSAttributeInfo(
    PBYTE   pVolumeInfo,
    USHORT  usBytesAllocated,
    USHORT  usMaxDataCount,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    PBYTE    pDataCursor = NULL;
    USHORT   usBytesRequired = 0;
    PFILE_FS_ATTRIBUTE_INFORMATION pFSAttrInfo = NULL;
    PSMB_FS_ATTRIBUTE_INFO_HEADER pFSAttrInfoHeader = NULL;

    pFSAttrInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)pVolumeInfo;

    usBytesRequired = sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);

    if (!pFSAttrInfo->FileSystemNameLength)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usBytesRequired += pFSAttrInfo->FileSystemNameLength;

    if (usBytesRequired > usMaxDataCount)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(usBytesRequired, (PVOID*)&pData);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pData;
    pFSAttrInfoHeader = (PSMB_FS_ATTRIBUTE_INFO_HEADER)pDataCursor;
    pFSAttrInfoHeader->ulFSAttributes = pFSAttrInfo->FileSystemAttributes;
    pFSAttrInfoHeader->lMaxFilenameLen = pFSAttrInfo->MaximumComponentNameLength;
    pFSAttrInfoHeader->ulFileSystemNameLen = pFSAttrInfo->FileSystemNameLength;

    pDataCursor += sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);

    memcpy(pDataCursor,
           (PBYTE)pFSAttrInfo->FileSystemName,
           pFSAttrInfo->FileSystemNameLength);

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


