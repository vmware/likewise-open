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
 *        treeconnect.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Tree Connect
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */
#include "includes.h"

NTSTATUS
SrvProcessTreeConnect_SMB_V2(
    IN     PLWIO_SRV_CONNECTION pConnection,
    IN     PSMB2_MESSAGE        pSmbRequest,
    IN OUT PSMB_PACKET          pSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_TREE_CONNECT_REQUEST_HEADER pTreeConnectHeader = NULL;// Do not free
    UNICODE_STRING    wszPath = {0}; // Do not free
    PLWIO_SRV_SESSION_2 pSession = NULL;
    PLWIO_SRV_TREE_2 pTree = NULL;
    PSRV_SHARE_INFO pShareInfo = NULL;
    PWSTR pwszPath = NULL;
    PWSTR pwszSharename = NULL;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bRemoveTreeFromSession = FALSE;
    PBYTE pOutBufferRef = pSmbResponse->pRawBuffer + pSmbResponse->bufferUsed;
    PBYTE pOutBuffer = pOutBufferRef;
    ULONG ulBytesAvailable = pSmbResponse->bufferLen - pSmbResponse->bufferUsed;
    ULONG ulOffset    = pSmbResponse->bufferUsed - sizeof(NETBIOS_HEADER);
    ULONG ulBytesUsed = 0;
    ULONG ulTotalBytesUsed = 0;

    ntStatus = SrvConnection2FindSession(
                    pConnection,
                    pSmbRequest->pHeader->ullSessionId,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2UnmarshalTreeConnect(
                    pSmbRequest,
                    &pTreeConnectHeader,
                    &wszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateMemory(
                    wszPath.Length + sizeof(wchar16_t),
                    (PVOID*)&pwszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy((PBYTE)pwszPath, (PBYTE)wszPath.Buffer, wszPath.Length);

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->pHostinfo->mutex);

    ntStatus = SrvGetShareName(
                    pConnection->pHostinfo->pszHostname,
                    pConnection->pHostinfo->pszDomain,
                    pwszPath,
                    &pwszSharename);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->pHostinfo->mutex);

    ntStatus = SrvShareFindByName(
                    pConnection->pShareList,
                    pwszSharename,
                    &pShareInfo);
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_BAD_NETWORK_NAME;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2CreateTree(
                    pSession,
                    pShareInfo,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    bRemoveTreeFromSession = TRUE;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_TREE_CONNECT,
                    0,
                    8,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pTree->ulTid,
                    pSession->ullUid,
                    STATUS_SUCCESS,
                    TRUE,
                    NULL,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;
    pOutBuffer += ulBytesUsed;
    ulOffset += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;

    ntStatus = SMB2MarshalTreeConnectResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pConnection,
                    pTree,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;
    // pOutBuffer += ulBytesUsed;
    // ulOffset += ulBytesUsed;
    // ulBytesAvailable -= ulBytesUsed;

    pSmbResponse->bufferUsed += ulTotalBytesUsed;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->pHostinfo->mutex);

    if (pSession)
    {
        SrvSession2Release(pSession);
    }

    if (pTree)
    {
        SrvTree2Release(pTree);
    }

    if (pShareInfo)
    {
        SrvShareReleaseInfo(pShareInfo);
    }

    SRV_SAFE_FREE_MEMORY(pwszPath);

    return ntStatus;

error:

    if (bRemoveTreeFromSession)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvSession2RemoveTree(
                        pSession,
                        pTree->ulTid);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to remove tid [%u] from session [uid=%u][code:%d]",
                            pTree->ulTid,
                            pSmbRequest->pHeader->ullSessionId,
                            ntStatus2);
        }
    }

    if (ulTotalBytesUsed)
    {
        memset(pOutBufferRef, 0, ulTotalBytesUsed);
    }

    goto cleanup;
}
