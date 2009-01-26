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
SrvGetShareName(
    PCSTR  pszHostname,
    PCSTR  pszDomain,
    PWSTR  pwszPath,
    PWSTR* ppwszSharename
    );

static
NTSTATUS
SrvBuildTreeConnectResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_SRV_TREE       pTree,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessTreeConnectAndX(
    PLWIO_SRV_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET pSmbRequest = pContext->pRequest;
    PSMB_PACKET pSmbResponse = NULL;
    PSMB_SRV_SESSION pSession = NULL;
    PSMB_SRV_TREE pTree = NULL;
    PSHARE_DB_INFO pShareInfo = NULL;
    TREE_CONNECT_REQUEST_HEADER* pRequestHeader = NULL; // Do not free
    uint8_t* pszPassword = NULL; // Do not free
    uint8_t* pszService = NULL; // Do not free
    PWSTR    pwszPath = NULL; // Do not free
    PWSTR    pwszSharename = NULL;
    BOOLEAN  bInLock = FALSE;

    if (pConnection->serverProperties.bRequireSecuritySignatures &&
        pConnection->pSessionKey)
    {
        ntStatus = SMBPacketVerifySignature(
                        pSmbRequest,
                        pContext->ulRequestSequence,
                        pConnection->pSessionKey,
                        pConnection->ulSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvConnectionFindSession(
                    pConnection,
                    pSmbRequest->pSMBHeader->uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = UnmarshallTreeConnectRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->bufferLen - pSmbRequest->bufferUsed,
                    0, // TODO: figure out alignment
                    &pRequestHeader,
                    &pszPassword,
                    &pwszPath,
                    &pszService);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pRequestHeader->flags & 0x1)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvSessionRemoveTree(
                        pSession,
                        pSmbRequest->pSMBHeader->tid);
        if (ntStatus2)
        {
            SMB_LOG_ERROR("Failed to remove tid [%u] from session [uid=%u]. [code:%d]",
                            pSmbRequest->pSMBHeader->tid,
                            pSmbRequest->pSMBHeader->uid,
                            ntStatus2);
        }
    }

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->pHostinfo->mutex);

    ntStatus = SrvGetShareName(
                    pConnection->pHostinfo->pszHostname,
                    pConnection->pHostinfo->pszDomain,
                    pwszPath,
                    &pwszSharename);
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->pHostinfo->mutex);

    ntStatus = SrvShareFindShareByName(
                    pConnection->pShareDbContext,
                    pwszSharename,
                    &pShareInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionCreateTree(
                    pSession,
                    pShareInfo,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildTreeConnectResponse(
                    pConnection,
                    pSmbRequest,
                    pTree,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pConnection->serverProperties.bRequireSecuritySignatures &&
        pConnection->pSessionKey)
    {
        ntStatus = SMBPacketSign(
                        pSmbResponse,
                        pContext->ulResponseSequence,
                        pConnection->pSessionKey,
                        pConnection->ulSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvConnectionWriteMessage(
                    pConnection,
                    pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->pHostinfo->mutex);

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pShareInfo)
    {
        SrvShareDbReleaseInfo(pShareInfo);
    }

    SMB_SAFE_FREE_MEMORY(pwszSharename);

    return (ntStatus);

error:

    goto cleanup;
}

static
NTSTATUS
SrvGetShareName(
    PCSTR  pszHostname,
    PCSTR  pszDomain,
    PWSTR  pwszPath,
    PWSTR* ppwszSharename
    )
{
    NTSTATUS  ntStatus = 0;
    PSTR      pszHostPrefix = NULL;
    PWSTR     pwszHostPrefix = NULL;
    PWSTR     pwszPath_copy = NULL;
    PWSTR     pwszSharename = NULL;
    size_t    len = 0, len_prefix = 0, len_sharename = 0;

    len = wc16slen(pwszPath);
    if (!len)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBAllocateMemory(
                    (len + 1) * sizeof(wchar16_t),
                    (PVOID*)&pwszPath_copy);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(pwszPath_copy, pwszPath, len * sizeof(wchar16_t));

    wc16supper(pwszPath_copy);

    ntStatus = SMBAllocateStringPrintf(
                    &pszHostPrefix,
                    "\\\\%s\\",
                    pszHostname,
                    pszDomain);
    BAIL_ON_NT_STATUS(ntStatus);

    SMBStrToUpper(pszHostPrefix);

    ntStatus = SMBMbsToWc16s(
                    pszHostPrefix,
                    &pwszHostPrefix);
    BAIL_ON_NT_STATUS(ntStatus);

    len_prefix = wc16slen(pwszHostPrefix);
    if (len <= len_prefix)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *(pwszPath_copy + len_prefix * sizeof(wchar16_t)) = WNUL;
    if (wc16scmp(pwszPath_copy, pwszHostPrefix) != 0)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    len_sharename = wc16slen(pwszPath_copy + (len_prefix  + 1) * sizeof(wchar16_t));
    if (!len_sharename)
    {
        ntStatus = STATUS_OBJECT_PATH_INVALID;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBAllocateMemory(
                    (len_sharename + 1) * sizeof(wchar16_t),
                    (PVOID*)&pwszSharename);
    BAIL_ON_NT_STATUS(ntStatus);

    // copy from original path
    memcpy(pwszSharename, pwszPath + (len_prefix + 1) * sizeof(wchar16_t), len_sharename);

    *ppwszSharename = pwszSharename;

cleanup:

    SMB_SAFE_FREE_STRING(pszHostPrefix);
    SMB_SAFE_FREE_MEMORY(pwszHostPrefix);
    SMB_SAFE_FREE_MEMORY(pwszPath_copy);

    return ntStatus;

error:

    *ppwszSharename = NULL;

    SMB_SAFE_FREE_MEMORY(pwszSharename);

    goto cleanup;
}

static
NTSTATUS
SrvBuildTreeConnectResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_SRV_TREE       pTree,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PTREE_CONNECT_RESPONSE_HEADER pResponseHeader = NULL;
    USHORT packetByteCount = 0;

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
                COM_TREE_CONNECT,
                0,
                TRUE,
                pTree->tid,
                getpid(),
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 3;

    pResponseHeader = (PTREE_CONNECT_RESPONSE_HEADER)pSmbResponse->pParams;
    pSmbResponse->pData = pSmbResponse->pParams + sizeof(TREE_CONNECT_RESPONSE_HEADER);
    pSmbResponse->bufferUsed += sizeof(TREE_CONNECT_RESPONSE_HEADER);

    // TODO: Marshall tree connect response

    pSmbResponse->pByteCount = &pResponseHeader->byteCount;
    *pSmbResponse->pByteCount = packetByteCount;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

