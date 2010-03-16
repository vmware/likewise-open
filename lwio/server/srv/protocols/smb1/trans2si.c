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
SrvSetBasicInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetDispositionInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetAllocationInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetEndOfFileInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvRenameFile(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvUnmarshalRenameInformation(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetEaList(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvUnmarshalSetEaListInformation(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvSetFileInfo(
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

    switch (*pTrans2State->pSmbInfoLevel)
    {
        case SMB_SET_FILE_BASIC_INFO :
        case SMB_SET_FILE_BASIC_INFO_ALIAS :

            ntStatus = SrvSetBasicInfo(pExecContext);

            break;

        case SMB_SET_FILE_DISPOSITION_INFO :
        case SMB_SET_FILE_DISPOSITION_INFO_ALIAS :

            ntStatus = SrvSetDispositionInfo(pExecContext);

            break;

        case SMB_SET_FILE_ALLOCATION_INFO :
        case SMB_SET_FILE_ALLOCATION_INFO_ALIAS:

	    ntStatus = SrvSetAllocationInfo(pExecContext);

            break;

        case SMB_SET_FILE_END_OF_FILE_INFO :
        case SMB_SET_FILE_END_OF_FILE_INFO_ALIAS:

            ntStatus = SrvSetEndOfFileInfo(pExecContext);

            break;

        case SMB_SET_FILE_RENAME_INFO :

            ntStatus = SrvRenameFile(pExecContext);

            break;

        case SMB_INFO_QUERY_EA_SIZE :

            ntStatus = SrvSetEaList(pExecContext);

            break;

        case SMB_INFO_STANDARD :
        case SMB_SET_FILE_UNIX_BASIC :
        case SMB_SET_FILE_UNIX_LINK :
        case SMB_SET_FILE_UNIX_HLINK :
        case SMB_SET_FILE_POSITION_INFO :
        case SMB_SET_FILE_MODE_INFO :

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        default:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

            break;
    }

error:

    return ntStatus;
}

NTSTATUS
SrvSetPathInfo(
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

    switch (*pTrans2State->pSmbInfoLevel)
    {
        case SMB_SET_FILE_BASIC_INFO :
        case SMB_SET_FILE_BASIC_INFO_ALIAS :

            ntStatus = SrvSetBasicInfo(pExecContext);

            break;

        case SMB_SET_FILE_DISPOSITION_INFO :
        case SMB_SET_FILE_DISPOSITION_INFO_ALIAS :

            ntStatus = SrvSetDispositionInfo(pExecContext);

            break;

        case SMB_SET_FILE_ALLOCATION_INFO :
        case SMB_SET_FILE_ALLOCATION_INFO_ALIAS:

            ntStatus = SrvSetAllocationInfo(pExecContext);

            break;

        case SMB_SET_FILE_END_OF_FILE_INFO :

            ntStatus = STATUS_INVALID_LEVEL;

            break;

        case SMB_SET_FILE_END_OF_FILE_INFO_ALIAS:

            ntStatus = SrvSetEndOfFileInfo(pExecContext);

            break;

        case SMB_SET_FILE_RENAME_INFO :

            ntStatus = SrvRenameFile(pExecContext);

            break;

        case SMB_INFO_STANDARD :
        case SMB_INFO_QUERY_EA_SIZE :
        case SMB_SET_FILE_UNIX_BASIC :
        case SMB_SET_FILE_UNIX_LINK :
        case SMB_SET_FILE_UNIX_HLINK :
        case SMB_SET_FILE_POSITION_INFO :
        case SMB_SET_FILE_MODE_INFO :

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        default:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

            break;
    }

error:

    return ntStatus;
}

ACCESS_MASK
SrvGetPathAccessMask(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    switch (*pTrans2State->pSmbInfoLevel)
    {
        case SMB_SET_FILE_END_OF_FILE_INFO:
        case SMB_SET_FILE_END_OF_FILE_INFO_ALIAS:
        case SMB_SET_FILE_ALLOCATION_INFO:
        case SMB_SET_FILE_ALLOCATION_INFO_ALIAS:

            return FILE_WRITE_DATA;

        case SMB_SET_FILE_DISPOSITION_INFO:
        case SMB_SET_FILE_DISPOSITION_INFO_ALIAS:
        case SMB_SET_FILE_RENAME_INFO:

            return DELETE;

        default:

            return FILE_WRITE_ATTRIBUTES;
    }
}

NTSTATUS
SrvBuildSetInfoResponse(
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

static
NTSTATUS
SrvSetBasicInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus       = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol   = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1       = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State   = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    if (pTrans2State->pRequestHeader->dataCount < sizeof(FILE_BASIC_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }


    if (!pTrans2State->bSetInfoAttempted)
    {
        PFILE_BASIC_INFORMATION pFileBasicInfo =
                            (PFILE_BASIC_INFORMATION)pTrans2State->pData;

        pTrans2State->bSetInfoAttempted = TRUE;

        SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

        ntStatus = IoSetInformationFile(
                        (pTrans2State->pFile ? pTrans2State->pFile->hFile :
                                               pTrans2State->hFile),
                        pTrans2State->pAcb,
                        &pTrans2State->ioStatusBlock,
                        pFileBasicInfo,
                        sizeof(FILE_BASIC_INFORMATION),
                        FileBasicInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvSetDispositionInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                      ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT    pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1      pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1      pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    if (pTrans2State->pRequestHeader->dataCount  < sizeof(FILE_DISPOSITION_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (!pTrans2State->bSetInfoAttempted)
    {
        PFILE_DISPOSITION_INFORMATION pFileDispositionInfo =
                        (PFILE_DISPOSITION_INFORMATION)pTrans2State->pData;

		pTrans2State->bSetInfoAttempted = TRUE;

        SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

        ntStatus = IoSetInformationFile(
                        (pTrans2State->pFile ? pTrans2State->pFile->hFile :
                                               pTrans2State->hFile),
                        pTrans2State->pAcb,
                        &pTrans2State->ioStatusBlock,
                        pFileDispositionInfo,
                        sizeof(FILE_DISPOSITION_INFORMATION),
                        FileDispositionInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvSetAllocationInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1     pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1     pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    if (pTrans2State->pRequestHeader->dataCount < sizeof(FILE_ALLOCATION_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (!pTrans2State->bSetInfoAttempted)
    {
        PFILE_ALLOCATION_INFORMATION pFileAllocationInfo =
                            (PFILE_ALLOCATION_INFORMATION)pTrans2State->pData;

		pTrans2State->bSetInfoAttempted = TRUE;

        SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

        ntStatus = IoSetInformationFile(
                        (pTrans2State->pFile ? pTrans2State->pFile->hFile :
                                               pTrans2State->hFile),
                        pTrans2State->pAcb,
                        &pTrans2State->ioStatusBlock,
                        pFileAllocationInfo,
                        sizeof(FILE_ALLOCATION_INFORMATION),
                        FileAllocationInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously
    }

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

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    if (pTrans2State->pRequestHeader->dataCount < sizeof(FILE_END_OF_FILE_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (!pTrans2State->bSetInfoAttempted)
    {
        PFILE_END_OF_FILE_INFORMATION pFileEofInfo =
                            (PFILE_END_OF_FILE_INFORMATION)pTrans2State->pData;

		pTrans2State->bSetInfoAttempted = TRUE;

        SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

        ntStatus = IoSetInformationFile(
                        (pTrans2State->pFile ? pTrans2State->pFile->hFile :
                                               pTrans2State->hFile),
                        pTrans2State->pAcb,
                        &pTrans2State->ioStatusBlock,
                        pFileEofInfo,
                        sizeof(FILE_END_OF_FILE_INFORMATION),
                        FileEndOfFileInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvRenameFile(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    BOOLEAN                    bTreeInLock  = FALSE;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pTrans2State->pData2)
    {
        ntStatus = SrvUnmarshalRenameInformation(pExecContext);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pTrans2State->pRootDir)
    {
        ((PFILE_RENAME_INFORMATION)pTrans2State->pData2)->RootDirectory =
                                                  pTrans2State->pRootDir->hFile;
    }
    else if (!pTrans2State->hDir)
    {
        wchar16_t wszBackSlash[] = { '\\', 0 };

        PFILE_RENAME_INFORMATION pRenameInfo    =
                        (PFILE_RENAME_INFORMATION)pTrans2State->pData2;

        if (*pRenameInfo->FileName == wszBackSlash[0])
        {
            ntStatus = STATUS_NOT_SUPPORTED;
        }
        else if (pTrans2State->pFile)
        {
            ntStatus = SrvGetParentPath(
                            pTrans2State->pFile->pFilename->FileName,
                            &pTrans2State->dirPath.FileName);
        }
        else if (pTrans2State->hFile)
        {
            ntStatus = SrvGetParentPath(
                            pTrans2State->fileName.FileName,
                            &pTrans2State->dirPath.FileName);
        }
        else
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        // Catch failed CreateFile calls when they come back around

        ntStatus = pTrans2State->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);

        SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

        ntStatus = SrvIoCreateFile(
                                pTrans2State->pTree->pShareInfo,
                                &pTrans2State->hDir,
                                pTrans2State->pAcb,
                                &pTrans2State->ioStatusBlock,
                                pCtxSmb1->pSession->pIoSecurityContext,
                                &pTrans2State->dirPath,
                                pTrans2State->pSecurityDescriptor,
                                pTrans2State->pSecurityQOS,
                                GENERIC_READ,
                                0,
                                FILE_ATTRIBUTE_NORMAL,
                                0,
                                FILE_OPEN,
                                FILE_DIRECTORY_FILE,
                                NULL, /* EA Buffer */
                                0,    /* EA Length */
                                &pTrans2State->pEcpList
                                );
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously
    }

    if (!pTrans2State->pRootDir)
    {
        ((PFILE_RENAME_INFORMATION)pTrans2State->pData2)->RootDirectory =
                                                            pTrans2State->hDir;
    }

    SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

    ntStatus = IoSetInformationFile(
                    (pTrans2State->pFile ? pTrans2State->pFile->hFile :
                                           pTrans2State->hFile),
                    pTrans2State->pAcb,
                    &pTrans2State->ioStatusBlock,
                    (PFILE_RENAME_INFORMATION)pTrans2State->pData2,
                    pTrans2State->usBytesAllocated,
                    FileRenameInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously

error:

    LWIO_UNLOCK_RWMUTEX(bTreeInLock,
                        &pCtxSmb1->pTree->pShareInfo->mutex);

    return ntStatus;
}

static
NTSTATUS
SrvUnmarshalRenameInformation(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    ULONG                      ulBytesAvailable  = 0;
    ULONG                      ulOffset          = 0;
    PWSTR                      pwszFilename      = NULL;
    PBYTE                      pDataCursor       = NULL;
    BOOLEAN                    bTreeInLock       = FALSE;
    PSMB_FILE_RENAME_INFO_HEADER pFileRenameInfo = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    pDataCursor = pTrans2State->pData;
    ulOffset    = pTrans2State->pRequestHeader->dataOffset;
    ulBytesAvailable = pTrans2State->pRequestHeader->dataCount;

    if (ulBytesAvailable < sizeof(SMB_FILE_RENAME_INFO_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileRenameInfo   = (PSMB_FILE_RENAME_INFO_HEADER)pDataCursor;
    pDataCursor      += sizeof(SMB_FILE_RENAME_INFO_HEADER);
    ulBytesAvailable -= sizeof(SMB_FILE_RENAME_INFO_HEADER);
    ulOffset         += sizeof(SMB_FILE_RENAME_INFO_HEADER);

    if (ulOffset % 2)
    {
        if (ulBytesAvailable < 1)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pDataCursor++;
        ulOffset++;
        ulBytesAvailable--;
    }

    if (!pFileRenameInfo->ulFileNameLength ||
        (ulBytesAvailable < pFileRenameInfo->ulFileNameLength))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pFileRenameInfo->ulRootDir)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pwszFilename = (PWSTR)pDataCursor;

    pTrans2State->usBytesAllocated =
            sizeof(FILE_RENAME_INFORMATION) + pFileRenameInfo->ulFileNameLength;

    ntStatus = SrvAllocateMemory(
                    pTrans2State->usBytesAllocated,
                    (PVOID*)&pTrans2State->pData2);
    BAIL_ON_NT_STATUS(ntStatus);

    ((PFILE_RENAME_INFORMATION)pTrans2State->pData2)->ReplaceIfExists =
                            pFileRenameInfo->ucReplaceIfExists ? TRUE : FALSE;

    ((PFILE_RENAME_INFORMATION)pTrans2State->pData2)->FileNameLength =
                            pFileRenameInfo->ulFileNameLength;

    memcpy((PBYTE)((PFILE_RENAME_INFORMATION)pTrans2State->pData2)->FileName,
           (PBYTE)pwszFilename,
           pFileRenameInfo->ulFileNameLength);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pCtxSmb1->pTree->mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvSetEaList(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pTrans2State->pData2)
    {
        ntStatus = SrvUnmarshalSetEaListInformation(pExecContext);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (!pTrans2State->bSetInfoAttempted)
    {
        SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

		pTrans2State->bSetInfoAttempted = TRUE;

        ntStatus = IoSetInformationFile(
                        (pTrans2State->pFile ? pTrans2State->pFile->hFile :
                                               pTrans2State->hFile),
                        pTrans2State->pAcb,
                        &pTrans2State->ioStatusBlock,
                        (PFILE_FULL_EA_INFORMATION)pTrans2State->pData2,
                        pTrans2State->usBytesAllocated,
                        FileFullEaInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvUnmarshalSetEaListInformation(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    ULONG                      ulBytesAvailable  = 0;
    ULONG                      ulOffset          = 0;
    PBYTE                      pDataCursor       = NULL;
    PTRANS2_FILE_EA_LIST_INFORMATION pFileEaListInfo = NULL;
    PFILE_FULL_EA_INFORMATION  pFileFullEaInformation = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    pDataCursor = pTrans2State->pData;
    ulOffset    = pTrans2State->pRequestHeader->dataOffset;
    ulBytesAvailable = pTrans2State->pRequestHeader->dataCount;

    if (ulBytesAvailable < sizeof(TRANS2_FILE_EA_LIST_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileEaListInfo   = (PTRANS2_FILE_EA_LIST_INFORMATION)pDataCursor;
    pDataCursor      += sizeof(TRANS2_FILE_EA_LIST_INFORMATION);
    ulBytesAvailable -= sizeof(TRANS2_FILE_EA_LIST_INFORMATION);
    ulOffset         += sizeof(TRANS2_FILE_EA_LIST_INFORMATION);

    if (pFileEaListInfo->ulEaListSize <= 0)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Assume for now that we are only allocating one
       FILE_FULL_EA_INFORMATION record which is all that has
       been observed from WinXP --jerry */

    pTrans2State->usBytesAllocated = pFileEaListInfo->ulEaListSize + sizeof(ULONG);

    ntStatus = SrvAllocateMemory(
                    pTrans2State->usBytesAllocated,
                    (PVOID*)&pTrans2State->pData2);
    BAIL_ON_NT_STATUS(ntStatus);

    pFileFullEaInformation = (PFILE_FULL_EA_INFORMATION)pTrans2State->pData2;

    pFileFullEaInformation->NextEntryOffset = 0;
    memcpy((PBYTE)&pFileFullEaInformation->Flags,
           (PBYTE)&pFileEaListInfo->pEaList[0],
           pFileEaListInfo->ulEaListSize);

cleanup:

    return ntStatus;

error:

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
