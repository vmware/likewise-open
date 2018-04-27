/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        ioctl.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        IOCTL
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildIOCTLState_SMB_V2(
    PLWIO_SRV_CONNECTION       pConnection,
    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader,
    PBYTE                      pData,
    PLWIO_SRV_FILE_2           pFile,
    PSRV_IOCTL_STATE_SMB_V2*   ppIOCTLState
    );

static
NTSTATUS
SrvExecuteIOCTL_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildIOCTLResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareIOCTLStateAsync_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState,
    PSRV_EXEC_CONTEXT       pExecContext
    );

static
VOID
SrvExecuteIOCTLAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
SrvReleaseIOCTLStateAsync_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState
    );

static
VOID
SrvReleaseIOCTLStateHandle_SMB_V2(
    HANDLE hIOCTLState
    );

static
VOID
SrvReleaseIOCTLState_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState
    );

static
VOID
SrvFreeIOCTLState_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState
    );

static
NTSTATUS
_SrvMarshalIOCTLReferralPayload(
    PSRV_FSCTL_REFERRAL_RESPONSE_DATA pReferrals,
    PBYTE *ppData,
    ULONG *pulDataLen)
{
    NTSTATUS ntStatus = 0;
    ULONG ulDataLen = 0;
    ULONG i = 0;
    ULONG stringLen = 0;
    PBYTE pData = NULL;
    PBYTE p = NULL;
    PBYTE *ppDomainOffset = NULL;
    UINT16 ui16Zero = 0;
    UINT16 ui16Val = 0;
    UINT32 ui32TTL = 0;
    UINT16 ui16Flags = 0;
    UINT16 ui16Type = 0;
    UINT16 offsetValue = 0;
    BYTE pad16buf[16] = {0};
    PWSTR pwszReferralName = NULL;

    /* Count length of all referral strings; +1 for null terminator */
    for (i=0; i<pReferrals->numReferrals; i++)
    {
        stringLen += (ULONG) LwRtlCStringNumChars(
                                 pReferrals->pEntry[i].pszDomainName) + 1;
    }

    ulDataLen = SMB2_IOCTL_REFERRAL_PREFIX_SIZE +
                    (SMB2_IOCTL_REFERRAL_HEADER_SIZE * i) +
                    (stringLen * sizeof(WCHAR));

    ntStatus = SrvAllocateMemory(
                   ulDataLen * sizeof(BYTE),
                   (PVOID*) &pData);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateMemory(
                   i * sizeof(PBYTE),
                   (PVOID*) &ppDomainOffset);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Marshal the prefix data, all data is little-endian */
    p = pData;

    /* Path Consumed */
    memcpy(p, &ui16Zero, sizeof(ui16Zero)), p += sizeof ui16Zero;

    /* Num Referrals */
    ui16Val = i;
    memcpy(p, &ui16Val, sizeof(ui16Val)), p += sizeof ui16Val;

    /* Flags */
    ui16Val = (UINT16) pReferrals->flags;
    memcpy(p, &ui16Val, sizeof(ui16Val)), p += sizeof ui16Val;

    /* Padding */
    memcpy(p, &ui16Zero, sizeof(ui16Zero)), p += sizeof ui16Zero;

    /* Marshall each referral header entry; 34 bytes long */
    for (i=0; i<pReferrals->numReferrals; i++)
    {
        ui32TTL = pReferrals->pEntry[i].TTL;
        ui16Flags = (UINT16) pReferrals->pEntry[i].flags;
        ui16Type = (UINT16) pReferrals->pEntry[i].serverType;

        /* Version = 3 */
        ui16Val = 3;
        memcpy(p, &ui16Val, sizeof(ui16Val)), p += sizeof(ui16Val);

        /* Entry length = 34 */
        ui16Val = SMB2_IOCTL_REFERRAL_HEADER_SIZE;
        memcpy(p, &ui16Val, sizeof(ui16Val)), p += sizeof(ui16Val);

        /* Server type */
        memcpy(p, &ui16Type, sizeof(ui16Type)), p += sizeof(ui16Type);

        /* Referrer Flags */
        memcpy(p, &ui16Flags, sizeof(ui16Flags)), p += sizeof(ui16Flags);

        /* Referrer Time To Live */
        memcpy(p, &ui32TTL, sizeof(ui32TTL)), p += sizeof(ui32TTL);

        /* Domain Offset; back fill later */
        ppDomainOffset[i] = p, p += sizeof(UINT16);

        /* Number of expanded names */
        ui16Val = 0;
        memcpy(p, &ui16Val, sizeof(ui16Val)), p += sizeof(ui16Val);

        /* Expanded names */
        memcpy(p, &ui16Val, sizeof(ui16Val)), p += sizeof(ui16Val);

        /* Pad buffer "Unknown Data" */
        memcpy(p, pad16buf, sizeof(pad16buf)), p += sizeof(pad16buf);
    }

    for (i=0; i<pReferrals->numReferrals; i++)
    {
        /* First, fill in Domain Offset value */
        offsetValue = (UINT16) ((p - ppDomainOffset[i]) + 12);
        memcpy(ppDomainOffset[i], &offsetValue, sizeof(offsetValue));

        /* Populate this referral string value, WC16 including \0 */
        SRV_SAFE_FREE_MEMORY(pwszReferralName);
        ntStatus = LwRtlWC16StringAllocateFromCString(
                       &pwszReferralName,
                       pReferrals->pEntry[i].pszDomainName);
        BAIL_ON_NT_STATUS(ntStatus);

        /* Length of string, in bytes, including \0 terminator */
        stringLen = (LwRtlWC16StringNumChars(pwszReferralName) + 1) *
                        sizeof (pwszReferralName[0]);
        memcpy(p, (PVOID) pwszReferralName, stringLen);
        p += stringLen;
    }

    *ppData = pData;
    *pulDataLen = p - pData;

cleanup:
    SRV_SAFE_FREE_MEMORY(ppDomainOffset);
    SRV_SAFE_FREE_MEMORY(pwszReferralName);
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
SrvBuildIOCTLReferralResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext,
    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader,
    PBYTE pRequestData,
    PSRV_FSCTL_REFERRAL_RESPONSE_DATA pInReferrals)
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection   = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_IOCTL_STATE_SMB_V2    pIOCTLState   = NULL;
    PLWIO_SRV_FILE_2           pFile         = NULL;
    ULONG                      ulFunctionCode = 0;
    PBYTE sMBRespBuf = NULL;
    ULONG  ulSMBRespBufLen = 0;

    ntStatus = _SrvMarshalIOCTLReferralPayload(
                   pInReferrals,
                   &sMBRespBuf,
                   &ulSMBRespBufLen);
    BAIL_ON_NT_STATUS(ntStatus);

    pIOCTLState = (PSRV_IOCTL_STATE_SMB_V2)pCtxSmb2->hState;
    ulFunctionCode = pRequestHeader->ulFunctionCode;

    if (pIOCTLState || ulFunctionCode != IO_FSCTL_GET_DFS_REFERRALS)
    {
        /* Cannot process this as a referral request */
        ntStatus = STATUS_DS_ATTRIBUTE_TYPE_UNDEFINED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvBuildIOCTLState_SMB_V2(
                    pConnection,
                    pRequestHeader,
                    pRequestData,
                    pFile,
                    &pIOCTLState);
    BAIL_ON_NT_STATUS(ntStatus);

    pCtxSmb2->hState = pIOCTLState;
    InterlockedIncrement(&pIOCTLState->refCount);
    pCtxSmb2->pfnStateRelease = &SrvReleaseIOCTLStateHandle_SMB_V2;

    pIOCTLState->ulResponseBufferLen = ulSMBRespBufLen;
    pIOCTLState->pResponseBuffer = sMBRespBuf;

    /* Referral reply is appended to the pExecContext response buffer */
    ntStatus = SrvBuildIOCTLResponse_SMB_V2(pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;

}

static
VOID
SrvFreeIOCTLReferralData_SMB_V2(
    PSRV_FSCTL_REFERRAL_RESPONSE_DATA pReferrals)
{
    ULONG i = 0;

    if (!pReferrals)
    {
        goto cleanup;
    }
    if (!pReferrals->pEntry)
    {
        goto cleanup;
    }

    for (i=0; i<pReferrals->numReferrals; i++)
    {
        SRV_SAFE_FREE_MEMORY(pReferrals->pEntry[i].pszDomainName);
    }
    SRV_SAFE_FREE_MEMORY(pReferrals->pEntry);

cleanup:
    SRV_SAFE_FREE_MEMORY(pReferrals);

    return;
}


static
NTSTATUS
SrvGetIOCTLReferralData_SMB_V2(
    ULONG TTL,
    PSTR *pszReferrals,
    PSRV_FSCTL_REFERRAL_RESPONSE_DATA *ppReferrals)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_FSCTL_REFERRAL_RESPONSE_DATA_ENTRY pReferralEntries = NULL;
    PSRV_FSCTL_REFERRAL_RESPONSE_DATA pReferrals = NULL;
    ULONG i = 0;
    ULONG cnt = 0;

    for (i=0; pszReferrals[i]; i++)
        ;
    cnt = i;

    ntStatus = SrvAllocateMemory(
                     i * sizeof(SRV_FSCTL_REFERRAL_RESPONSE_DATA_ENTRY),
                    (PVOID*) &pReferralEntries);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_FSCTL_REFERRAL_RESPONSE_DATA),
                    (PVOID*) &pReferrals);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i=0; pszReferrals[i]; i++)
    {
        pReferralEntries[i].TTL = TTL;
        pReferralEntries[i].flags = SMB2_IOCTL_FLAG_NAME_LIST_REFERRAL;
        pReferralEntries[i].pszDomainName = pszReferrals[i];
    }

    pReferrals->flags = 0;
    pReferrals->numReferrals = i;
    pReferrals->pEntry = pReferralEntries;

    *ppReferrals = pReferrals;

cleanup:
    return ntStatus;

error:
    if (pReferralEntries)
    {
        for (i=0; i<cnt; i++)
        {
            SRV_SAFE_FREE_MEMORY(pReferralEntries[i].pszDomainName);
        }
    }
    SRV_SAFE_FREE_MEMORY(pReferralEntries);
    SRV_SAFE_FREE_MEMORY(pReferrals);
    goto cleanup;
}

/* TBD:Adam-Stub function to get referral data from directory */
static
NTSTATUS
SrvGetLdap_IOCTLReferralData_SMB_V2(
    PSRV_FSCTL_REFERRAL_RESPONSE_DATA *ppReferrals)
{
    NTSTATUS ntStatus = 0;
    PSRV_FSCTL_REFERRAL_RESPONSE_DATA pReferrals = 0;

    /* TBD:Adam-Get this data from vmdir */
    PSTR pszReferralsArray[] = { "\\LIGHTWAVE",
                                 "\\lightwave.local",
                                 NULL,
                               };

    ntStatus = SrvGetIOCTLReferralData_SMB_V2(
                   600,
                   pszReferralsArray,
                   &pReferrals);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppReferrals = pReferrals;

cleanup:
    return ntStatus;

error:
    SrvFreeIOCTLReferralData_SMB_V2(pReferrals);
    goto cleanup;
}

NTSTATUS
SrvProcessIOCTL_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection   = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_IOCTL_STATE_SMB_V2    pIOCTLState   = NULL;
    PLWIO_SRV_SESSION_2        pSession      = NULL;
    PLWIO_SRV_TREE_2           pTree         = NULL;
    PLWIO_SRV_FILE_2           pFile         = NULL;
    BOOLEAN                    bInLock       = FALSE;
    ULONG                      ulFunctionCode = 0;
    PSRV_FSCTL_REFERRAL_RESPONSE_DATA pReferrals = NULL;

    pIOCTLState = (PSRV_IOCTL_STATE_SMB_V2)pCtxSmb2->hState;

    if (pIOCTLState)
    {
        InterlockedIncrement(&pIOCTLState->refCount);
    }
    else
    {
        ULONG                      iMsg           = pCtxSmb2->iMsg;
        PSRV_MESSAGE_SMB_V2        pSmbRequest    = &pCtxSmb2->pRequests[iMsg];
        PSMB2_IOCTL_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PBYTE                      pData          = NULL; // Do not free

        ntStatus = SrvConnection2FindSession_SMB_V2(
                        pCtxSmb2,
                        pConnection,
                        pSmbRequest->pHeader->ullSessionId,
                        &pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSetStatSession2Info(pExecContext, pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSession2FindTree_SMB_V2(
                        pCtxSmb2,
                        pSession,
                        pSmbRequest->pHeader->ulTid,
                        &pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMB2UnmarshalIOCTLRequest(
                        pSmbRequest,
                        &pRequestHeader,
                        &pData);
        BAIL_ON_NT_STATUS(ntStatus);

        SRV_LOG_DEBUG(
            pExecContext->pLogContext,
            SMB_PROTOCOL_VERSION_2,
            pSmbRequest->pHeader->command,
            "IOCTL request params: "
            "command(%u),uid(%llu),cmd-seq(%llu),pid(%u),tid(%u),"
            "credits(%u),flags(0x%x),chain-offset(%u),"
            "file-id(persistent:0x%x,volatile:0x%x),"
            "flags(0x%x),function-code(%u),"
            "input-length(%u),input-offset(%u),max-input-length(%u),"
            "output-length(%u),output-offset(%u),max-output-length(%u)",
            pSmbRequest->pHeader->command,
            (long long)pSmbRequest->pHeader->ullSessionId,
            (long long)pSmbRequest->pHeader->ullCommandSequence,
            pSmbRequest->pHeader->ulPid,
            pSmbRequest->pHeader->ulTid,
            pSmbRequest->pHeader->usCredits,
            pSmbRequest->pHeader->ulFlags,
            pSmbRequest->pHeader->ulChainOffset,
            (long long)pRequestHeader->fid.ullPersistentId,
            (long long)pRequestHeader->fid.ullVolatileId,
            pRequestHeader->ulFlags,
            pRequestHeader->ulFunctionCode,
            pRequestHeader->ulInLength,
            pRequestHeader->ulInOffset,
            pRequestHeader->ulMaxInLength,
            pRequestHeader->ulOutLength,
            pRequestHeader->ulOutOffset,
            pRequestHeader->ulMaxOutLength);

        ulFunctionCode = pRequestHeader->ulFunctionCode;
        switch (pRequestHeader->ulFunctionCode)
        {
            case IO_FSCTL_GET_DFS_REFERRALS:
                if (!pIOCTLState &&
                    ulFunctionCode == IO_FSCTL_GET_DFS_REFERRALS)
                {
                    ntStatus = SrvGetLdap_IOCTLReferralData_SMB_V2(
                                  &pReferrals);
                    BAIL_ON_NT_STATUS(ntStatus);

                    ntStatus = SrvBuildIOCTLReferralResponse_SMB_V2(
                                   pExecContext,
                                   pRequestHeader,
                                   pData,
                                   pReferrals);
                    BAIL_ON_NT_STATUS(ntStatus);
                    goto cleanup;
                }
                break;

            case IO_FSCTL_PIPE_WAIT:

                ntStatus = STATUS_NOT_SUPPORTED;

                break;

            default:

                ntStatus = SrvTree2FindFile_SMB_V2(
                                pCtxSmb2,
                                pTree,
                                &pRequestHeader->fid,
                                &pFile);

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildIOCTLState_SMB_V2(
                        pConnection,
                        pRequestHeader,
                        pData,
                        pFile,
                        &pIOCTLState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pIOCTLState;
        InterlockedIncrement(&pIOCTLState->refCount);
        pCtxSmb2->pfnStateRelease = &SrvReleaseIOCTLStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pIOCTLState->mutex);

    switch (pIOCTLState->stage)
    {
        case SRV_IOCTL_STAGE_SMB_V2_INITIAL:

            pIOCTLState->stage = SRV_IOCTL_STAGE_SMB_V2_ATTEMPT_IO;

            // intentional fall through

        case SRV_IOCTL_STAGE_SMB_V2_ATTEMPT_IO:

            ntStatus = SrvExecuteIOCTL_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pIOCTLState->stage = SRV_IOCTL_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case SRV_IOCTL_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = SrvBuildIOCTLResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pIOCTLState->stage = SRV_IOCTL_STAGE_SMB_V2_DONE;

            // intentional fall through

        case SRV_IOCTL_STAGE_SMB_V2_DONE:

            break;
    }

cleanup:

    if (pFile)
    {
        SrvFile2Release(pFile);
    }

    if (pTree)
    {
        SrvTree2Release(pTree);
    }

    if (pSession)
    {
        SrvSession2Release(pSession);
    }

    if (pIOCTLState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pIOCTLState->mutex);

        SrvReleaseIOCTLState_SMB_V2(pIOCTLState);
    }

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_PENDING:

            // TODO: Add an indicator to the file object to trigger a
            //       cleanup if the connection gets closed and all the
            //       files involved have to be closed

            break;

        default:

            if (pIOCTLState)
            {
                SrvReleaseIOCTLStateAsync_SMB_V2(pIOCTLState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildIOCTLState_SMB_V2(
    PLWIO_SRV_CONNECTION       pConnection,
    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader,
    PBYTE                      pData,
    PLWIO_SRV_FILE_2           pFile,
    PSRV_IOCTL_STATE_SMB_V2*   ppIOCTLState
    )
{
    NTSTATUS                ntStatus    = STATUS_SUCCESS;
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState = NULL;


    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_IOCTL_STATE_SMB_V2),
                    (PVOID*)&pIOCTLState);
    BAIL_ON_NT_STATUS(ntStatus);

    pIOCTLState->refCount = 1;

    pthread_mutex_init(&pIOCTLState->mutex, NULL);
    pIOCTLState->pMutex = &pIOCTLState->mutex;

    pIOCTLState->pConnection = SrvConnectionAcquire(pConnection);

    if (pFile)
    {
        pIOCTLState->pFile = SrvFile2Acquire(pFile);
    }

    pIOCTLState->pRequestHeader = pRequestHeader;
    pIOCTLState->pData          = pData;

    *ppIOCTLState = pIOCTLState;

cleanup:

    return ntStatus;

error:

    *ppIOCTLState = NULL;

    if (pIOCTLState)
    {
        SrvFreeIOCTLState_SMB_V2(pIOCTLState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteIOCTL_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection   = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_IOCTL_STATE_SMB_V2    pIOCTLState   = NULL;

    pIOCTLState = (PSRV_IOCTL_STATE_SMB_V2)pCtxSmb2->hState;

    if (pExecContext->pStatInfo)
    {
        ntStatus = SrvStatisticsSetIOCTL(
                        pExecContext->pStatInfo,
                        pIOCTLState->pRequestHeader->ulFunctionCode);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = pIOCTLState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pIOCTLState->pResponseBuffer)
    {
        // TODO: Should we just allocate 64 * 1024 bytes in this buffer?

        ntStatus = SMBPacketBufferAllocate(
                        pConnection->hPacketAllocator,
                        pIOCTLState->pRequestHeader->ulMaxOutLength,
                        &pIOCTLState->pResponseBuffer,
                        &pIOCTLState->sResponseBufferLen);
        BAIL_ON_NT_STATUS(ntStatus);

        pIOCTLState->ulResponseBufferLen =
            pIOCTLState->pRequestHeader->ulMaxOutLength > 0 ?
            pIOCTLState->pRequestHeader->ulMaxOutLength:
            pIOCTLState->sResponseBufferLen;

        SrvPrepareIOCTLStateAsync_SMB_V2(pIOCTLState, pExecContext);

        if (pIOCTLState->pRequestHeader->ulFlags & 0x1)
        {
            ntStatus = IoFsControlFile(
                                    pIOCTLState->pFile->hFile,
                                    pIOCTLState->pAcb,
                                    &pIOCTLState->ioStatusBlock,
                                    pIOCTLState->pRequestHeader->ulFunctionCode,
                                    pIOCTLState->pData,
                                    pIOCTLState->pRequestHeader->ulInLength,
                                    pIOCTLState->pResponseBuffer,
                                    pIOCTLState->ulResponseBufferLen);
        }
        else
        {
            ntStatus = IoDeviceIoControlFile(
                                    pIOCTLState->pFile->hFile,
                                    pIOCTLState->pAcb,
                                    &pIOCTLState->ioStatusBlock,
                                    pIOCTLState->pRequestHeader->ulFunctionCode,
                                    pIOCTLState->pData,
                                    pIOCTLState->pRequestHeader->ulInLength,
                                    pIOCTLState->pResponseBuffer,
                                    pIOCTLState->ulResponseBufferLen);
        }
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseIOCTLStateAsync_SMB_V2(pIOCTLState); // completed sync
    }

    pIOCTLState->ulResponseBufferLen =
                                    pIOCTLState->ioStatusBlock.BytesTransferred;

cleanup:

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_NOT_SUPPORTED:

            ntStatus = STATUS_INVALID_DEVICE_REQUEST;

            break;

        default:

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildIOCTLResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_IOCTL_STATE_SMB_V2    pIOCTLState   = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE                     pOutBuffer       = pSmbResponse->pBuffer;
    ULONG                     ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG                     ulOffset         = 0;
    ULONG                     ulBytesUsed      = 0;
    ULONG                     ulTotalBytesUsed = 0;

    pIOCTLState = (PSRV_IOCTL_STATE_SMB_V2)pCtxSmb2->hState;

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
                    COM2_IOCTL,
                    pSmbRequest->pHeader->usEpoch,
                    pExecContext->usCreditsGranted,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL, /* Async Id */
                    (pIOCTLState->ulResponseBufferLen < pIOCTLState->pRequestHeader->ulMaxOutLength ?
                            STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW),
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

    ntStatus = SMB2MarshalIOCTLResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pIOCTLState->pRequestHeader,
                    pIOCTLState->pResponseBuffer,
                    SMB_MIN(pIOCTLState->ulResponseBufferLen,
                            pIOCTLState->pRequestHeader->ulMaxOutLength),
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += ulBytesUsed;
    // ulOffset         += ulBytesUsed;
    // ulBytesAvailable -= ulBytesUsed;
    ulTotalBytesUsed += ulBytesUsed;

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

static
VOID
SrvPrepareIOCTLStateAsync_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState,
    PSRV_EXEC_CONTEXT       pExecContext
    )
{
    pIOCTLState->acb.Callback        = &SrvExecuteIOCTLAsyncCB_SMB_V2;

    pIOCTLState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pIOCTLState->acb.AsyncCancelContext = NULL;

    pIOCTLState->pAcb = &pIOCTLState->acb;
}

static
VOID
SrvExecuteIOCTLAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    // NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_IOCTL_STATE_SMB_V2    pIOCTLState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pIOCTLState = (PSRV_IOCTL_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pIOCTLState->mutex);

    if (pIOCTLState->pAcb && pIOCTLState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pIOCTLState->pAcb->AsyncCancelContext);
    }

    pIOCTLState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pIOCTLState->mutex);

    SrvProtocolExecute(pExecContext);
    // ntStatus = SrvProtocolExecute(pExecContext);
    // (!NT_SUCCESS(ntStatus)) - Error has already been logged

    SrvReleaseExecContext(pExecContext);

    return;
}

static
VOID
SrvReleaseIOCTLStateAsync_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState
    )
{
    if (pIOCTLState->pAcb)
    {
        pIOCTLState->acb.Callback        = NULL;

        if (pIOCTLState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pIOCTLState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pIOCTLState->pAcb->CallbackContext = NULL;
        }

        if (pIOCTLState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pIOCTLState->pAcb->AsyncCancelContext);
        }

        pIOCTLState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseIOCTLStateHandle_SMB_V2(
    HANDLE hIOCTLState
    )
{
    return SrvReleaseIOCTLState_SMB_V2((PSRV_IOCTL_STATE_SMB_V2)hIOCTLState);
}

static
VOID
SrvReleaseIOCTLState_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState
    )
{
    if (InterlockedDecrement(&pIOCTLState->refCount) == 0)
    {
        SrvFreeIOCTLState_SMB_V2(pIOCTLState);
    }
}

static
VOID
SrvFreeIOCTLState_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState
    )
{
    if (pIOCTLState->pAcb && pIOCTLState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pIOCTLState->pAcb->AsyncCancelContext);
    }

    if (pIOCTLState->pFile)
    {
        SrvFile2Release(pIOCTLState->pFile);
    }

    if (pIOCTLState->pResponseBuffer)
    {
        SMBPacketBufferFree(
            pIOCTLState->pConnection->hPacketAllocator,
            pIOCTLState->pResponseBuffer,
            pIOCTLState->sResponseBufferLen);
    }

    if (pIOCTLState->pConnection)
    {
        SrvConnectionRelease(pIOCTLState->pConnection);
    }

    if (pIOCTLState->pMutex)
    {
        pthread_mutex_destroy(&pIOCTLState->mutex);
    }

    SrvFreeMemory(pIOCTLState);
}
