/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        getquotainfo.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Query Quota Information
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 *
 *
 */

#include "includes.h"

NTSTATUS
SrvGetQuotaInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    PSMB2_QUERY_QUOTA_INFO_HEADER pQueryQuotaHeader = NULL;
    PBYTE                      pData         = NULL;
    ULONG                      ulDataLen     = 0;
    PBYTE                      pStartSid     = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pQueryQuotaHeader =
                (PSMB2_QUERY_QUOTA_INFO_HEADER)pGetInfoState->pInputBuffer;

    if (!pQueryQuotaHeader)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pGetInfoState->ulInputBufferLength < sizeof(*pQueryQuotaHeader))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pData = pGetInfoState->pInputBuffer + sizeof(*pQueryQuotaHeader);
    ulDataLen = pGetInfoState->ulInputBufferLength - sizeof(*pQueryQuotaHeader);

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        // Allocate output buffer
        ULONG ulBufLen = pGetInfoState->pRequestHeader->ulOutputBufferLen;

        if (!ulBufLen || ulBufLen > UINT16_MAX)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = SrvAllocateMemory(ulBufLen, (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength = ulBufLen;

        // StartSid
        if (pQueryQuotaHeader->ulStartSidLength != 0)
        {
            if ((pQueryQuotaHeader->ulStartSidOffset +
                 pQueryQuotaHeader->ulStartSidLength) > ulDataLen)
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pStartSid = pData + pQueryQuotaHeader->ulStartSidOffset;
        }

        // SidList
        if (pQueryQuotaHeader->ulSidListLength > ulDataLen)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryQuotaInformationFile(
                        pGetInfoState->pFile->hFile,
                        pGetInfoState->pAcb,
                        &pGetInfoState->ioStatusBlock,
                        (PFILE_QUOTA_INFORMATION)pGetInfoState->pData2,
                        pGetInfoState->ulDataLength,
                        pQueryQuotaHeader->bReturnSingle,
                        (PFILE_GET_QUOTA_INFORMATION)pData,
                        pQueryQuotaHeader->ulSidListLength,
                        (PFILE_GET_QUOTA_INFORMATION)pStartSid,
                        pQueryQuotaHeader->bRestartScan);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState); // sync completion
    }

    pGetInfoState->ulActualDataLength =
                            pGetInfoState->ioStatusBlock.BytesTransferred;

error:

    return ntStatus;
}

NTSTATUS
SrvBuildQuotaInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SrvCreditorAdjustCredits(
                    pExecContext->pConnection->pCreditor,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pExecContext->ullAsyncId,
                    pSmbRequest->pHeader->usCredits,
                    &pExecContext->usCreditsGranted);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    if (ulBytesAvailable < pGetInfoState->ulActualDataLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pGetInfoState->ulActualDataLength)
    {
        memcpy(pOutBuffer,
               pGetInfoState->pData2,
               pGetInfoState->ulActualDataLength);
    }

    pGetInfoResponseHeader->ulOutBufferLength =
                                        pGetInfoState->ulActualDataLength;

    // pOutBuffer       += pGetInfoState->ulDataLength;
    // ulBytesAvailable -= pGetInfoState->ulDataLength;
    // ulOffset         += pGetInfoState->ulDataLength;

    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}
