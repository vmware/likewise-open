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

    // TODO: Query for share

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

    return (ntStatus);

error:

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

