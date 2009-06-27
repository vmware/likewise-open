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
 *        wire.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Wire protocol
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SMB2MarshalHeader(
    PSMB_PACKET pSmbPacket,
    USHORT      usCommand,
    USHORT      usEpoch,
    USHORT      usCredits,
    ULONG       ulPid,
    ULONG64     ullMid,
    ULONG       ulTid,
    ULONG64     ullSessionId,
    NTSTATUS    status,
    BOOLEAN     bCommandAllowsSignature,
    BOOLEAN     bIsResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    static uchar8_t smb2Magic[4] = { 0xFE, 'S', 'M', 'B' };
    ULONG    ulBufferUsed = 0;
    ULONG    ulBufferAvailable = pSmbPacket->bufferLen;
    PBYTE pBuffer = pSmbPacket->pRawBuffer;

    if (ulBufferAvailable < sizeof(NETBIOS_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSmbPacket->pNetBIOSHeader = (NETBIOS_HEADER *) (pBuffer);
    ulBufferUsed += sizeof(NETBIOS_HEADER);
    ulBufferAvailable -= sizeof(NETBIOS_HEADER);
    pBuffer += sizeof(NETBIOS_HEADER);

    if (ulBufferAvailable < sizeof(SMB2_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSmbPacket->protocolVer = SMB_PROTOCOL_VERSION_2;
    pSmbPacket->pSMB2Header = (PSMB2_HEADER)(pBuffer);

    ulBufferUsed += sizeof(SMB2_HEADER);
    ulBufferAvailable -= sizeof(SMB2_HEADER);
    pBuffer += sizeof(SMB2_HEADER);

    memcpy(&pSmbPacket->pSMB2Header->smb[0], &smb2Magic[0], sizeof(smb2Magic));
    pSmbPacket->pSMB2Header->command        = usCommand;
    pSmbPacket->pSMB2Header->usEpoch        = usEpoch;
    pSmbPacket->pSMB2Header->usCredits      = usCredits;
    pSmbPacket->pSMB2Header->ulPid          = ulPid;
    pSmbPacket->pSMB2Header->ullCommandSequence = ullMid;
    pSmbPacket->pSMB2Header->ulTid          = ulTid;
    pSmbPacket->pSMB2Header->ullSessionId   = ullSessionId;
    pSmbPacket->pSMB2Header->error         = status;
    pSmbPacket->pSMB2Header->usHeaderLen = sizeof(SMB2_HEADER);

    if (bIsResponse)
    {
        pSmbPacket->pSMB2Header->ulFlags |= SMB2_FLAGS_SERVER_TO_REDIR;
    }

    pSmbPacket->pParams = pSmbPacket->pRawBuffer + ulBufferUsed;

    pSmbPacket->bufferUsed = ulBufferUsed;

    pSmbPacket->allowSignature = bCommandAllowsSignature;

error:

    return ntStatus;
}

NTSTATUS
SMB2UnmarshallSessionSetup(
    PSMB_PACKET                         pPacket,
    PSMB2_SESSION_SETUP_REQUEST_HEADER* ppHeader,
    PBYTE*                              ppSecurityBlob,
    PULONG                              pulSecurityBlobLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulOffset = sizeof(SMB2_HEADER);
    ULONG ulPacketSize = pPacket->bufferLen - sizeof(NETBIOS_HEADER);
    PBYTE pBuffer = (PBYTE)pPacket->pSMB2Header + ulOffset;
    ULONG ulBytesAvailable = 0;
    PSMB2_SESSION_SETUP_REQUEST_HEADER pHeader = NULL;
    PBYTE pSecurityBlob = NULL;
    ULONG ulSecurityBlobLen = 0;

    ulBytesAvailable = pPacket->bufferLen -
                       sizeof(NETBIOS_HEADER) -
                       sizeof(SMB2_HEADER);

    if (ulBytesAvailable < sizeof(SMB2_SESSION_SETUP_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_SESSION_SETUP_REQUEST_HEADER)pBuffer;
    ulOffset += sizeof(SMB2_SESSION_SETUP_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SMB2_SESSION_SETUP_REQUEST_HEADER);

    // Is dynamic part present?
    if (pHeader->usLength & 0x1)
    {
        USHORT usAlign = ulOffset %8;
        if (usAlign)
        {
            if (ulBytesAvailable < usAlign)
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ulOffset += usAlign;
            ulBytesAvailable -= usAlign;
        }

        if (!pHeader->usBlobLength ||
            (pHeader->usBlobOffset < ulOffset) ||
            (pHeader->usBlobOffset % 8) ||
            (pHeader->usBlobOffset > ulPacketSize) ||
            (pHeader->usBlobOffset + pHeader->usBlobLength) > ulPacketSize)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pSecurityBlob = (PBYTE)pPacket->pSMB2Header + pHeader->usBlobOffset;
        ulSecurityBlobLen = pHeader->usBlobLength;
    }

    *ppHeader = pHeader;
    *ppSecurityBlob = pSecurityBlob;
    *pulSecurityBlobLen = ulSecurityBlobLen;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;
    *ppSecurityBlob = NULL;
    *pulSecurityBlobLen = 0;

    goto cleanup;
}

NTSTATUS
SMB2MarshalSessionSetup(
    PSMB_PACKET        pPacket,
    SMB2_SESSION_FLAGS usFlags,
    PBYTE              pSecurityBlob,
    ULONG              ulSecurityBlobLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_SESSION_SETUP_RESPONSE_HEADER pHeader = NULL;
    ULONG ulOffset = (PBYTE)pPacket->pParams - (PBYTE)pPacket->pSMB2Header;
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    ULONG ulBytesUsed = 0;
    PBYTE pBuffer = pPacket->pParams;
    USHORT usAlign = 0;

    if (ulBytesAvailable < sizeof(SMB2_SESSION_SETUP_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_SESSION_SETUP_RESPONSE_HEADER)pBuffer;
    ulOffset += sizeof(SMB2_SESSION_SETUP_RESPONSE_HEADER);
    ulBytesUsed += sizeof(SMB2_SESSION_SETUP_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_SESSION_SETUP_RESPONSE_HEADER);
    pBuffer += sizeof(SMB2_SESSION_SETUP_RESPONSE_HEADER);

    if ((usAlign = ulOffset % 8))
    {
        if (ulBytesAvailable < usAlign)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulOffset += usAlign;
        ulBytesUsed += usAlign;
        ulBytesAvailable -= usAlign;
        pBuffer += usAlign;
    }

    pHeader->usLength = ulBytesUsed;

    if (ulSecurityBlobLen)
    {
        if (ulBytesAvailable < ulSecurityBlobLen)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        memcpy(pBuffer, pSecurityBlob, ulSecurityBlobLen);

        pHeader->usLength++;

        ulBytesUsed += ulSecurityBlobLen;
    }

    pHeader->usSessionFlags = usFlags;
    pHeader->usBlobOffset = ulOffset;
    pHeader->usBlobLength = ulSecurityBlobLen;

    pPacket->bufferUsed += ulBytesUsed;

error:

    return ntStatus;
}

NTSTATUS
SMB2UnmarshalLogoffRequest(
    PSMB_PACKET                  pPacket,
    PSMB2_LOGOFF_REQUEST_HEADER* ppHeader
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_LOGOFF_REQUEST_HEADER pHeader = NULL; // Do not free
    ULONG ulOffset = sizeof(SMB2_HEADER);
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    PBYTE pBuffer = (PBYTE)pPacket->pSMB2Header + ulOffset;

    if (ulBytesAvailable < sizeof(SMB2_LOGOFF_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_LOGOFF_REQUEST_HEADER)pBuffer;

    *ppHeader = pHeader;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalLogoffResponse(
    PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_LOGOFF_RESPONSE_HEADER pHeader = NULL;
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    ULONG ulBytesUsed = 0;
    PBYTE pBuffer = pPacket->pParams;

    if (ulBytesAvailable < sizeof(SMB2_LOGOFF_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_LOGOFF_RESPONSE_HEADER)pBuffer;
    ulBytesUsed += sizeof(SMB2_LOGOFF_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_LOGOFF_RESPONSE_HEADER);
    pBuffer += sizeof(SMB2_LOGOFF_RESPONSE_HEADER);

    pHeader->usLength = ulBytesUsed;

    pPacket->bufferUsed += ulBytesUsed;

error:

    return ntStatus;
}

NTSTATUS
SMB2UnmarshalTreeConnect(
    PSMB_PACKET                        pPacket,
    PSMB2_TREE_CONNECT_REQUEST_HEADER* ppTreeConnectRequestHeader,
    PUNICODE_STRING                    pwszPath
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulOffset = sizeof(SMB2_HEADER);
    PBYTE pDataBuffer = (PBYTE)pPacket->pSMB2Header + ulOffset;
    ULONG ulPacketSize = pPacket->bufferLen - sizeof(NETBIOS_HEADER);
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    PSMB2_TREE_CONNECT_REQUEST_HEADER pHeader = NULL;
    UNICODE_STRING wszPath = {0};

    if (ulBytesAvailable < sizeof(SMB2_TREE_CONNECT_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_TREE_CONNECT_REQUEST_HEADER)pDataBuffer;
    ulBytesAvailable -= sizeof(SMB2_TREE_CONNECT_REQUEST_HEADER);
    ulOffset += sizeof(SMB2_TREE_CONNECT_REQUEST_HEADER);

    if (pHeader->usLength & 0x1)
    {
        if ((pHeader->usPathOffset < ulOffset) ||
            (pHeader->usPathOffset % 2) ||
            (pHeader->usPathLength % 2) ||
            (pHeader->usPathOffset > ulPacketSize) ||
            (pHeader->usPathLength + pHeader->usPathOffset) > ulPacketSize)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        wszPath.Buffer = (PWSTR)((PBYTE)pPacket->pSMB2Header + pHeader->usPathOffset);
        wszPath.Length = wszPath.MaximumLength = pHeader->usPathLength;
    }

    if (!wszPath.Length)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppTreeConnectRequestHeader = pHeader;
    *pwszPath = wszPath;

cleanup:

    return ntStatus;

error:

    *ppTreeConnectRequestHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalTreeConnectResponse(
    PSMB_PACKET          pPacket,
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_TREE_2     pTree
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_TREE_CONNECT_RESPONSE_HEADER pHeader = NULL;
    ULONG ulOffset = (PBYTE)pPacket->pParams - (PBYTE)pPacket->pSMB2Header;
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    ULONG ulBytesUsed = 0;
    PBYTE pBuffer = pPacket->pParams;

    if (ulBytesAvailable < sizeof(SMB2_TREE_CONNECT_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_TREE_CONNECT_RESPONSE_HEADER)pBuffer;
    ulOffset += sizeof(SMB2_TREE_CONNECT_RESPONSE_HEADER);
    ulBytesUsed += sizeof(SMB2_TREE_CONNECT_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_TREE_CONNECT_RESPONSE_HEADER);
    pBuffer += sizeof(SMB2_TREE_CONNECT_RESPONSE_HEADER);

    pHeader->usLength = ulBytesUsed;

    ntStatus = SrvGetMaximalShareAccessMask(
                    pTree->pShareInfo,
                    &pHeader->ulShareAccessMask);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pTree->pShareInfo->service)
    {
        case SHARE_SERVICE_DISK_SHARE:

            pHeader->usShareType = 1;

            break;

        case SHARE_SERVICE_NAMED_PIPE:

            pHeader->usShareType = 2;

            break;

        default:

            LWIO_LOG_DEBUG("Unrecognized share type %d", pTree->pShareInfo->service);

            break;
    }

    // TODO: Fill in Share capabilities
    // TODO: Fill in Share flags

    pPacket->bufferUsed += ulBytesUsed;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalTreeDisconnectRequest(
    PSMB_PACKET                           pSmbRequest,
    PSMB2_TREE_DISCONNECT_REQUEST_HEADER* ppTreeDisconnectHeader
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulBytesAvailable = pSmbRequest->bufferLen - pSmbRequest->bufferUsed;
    PBYTE pBuffer = (PBYTE)pSmbRequest->pSMB2Header + sizeof(SMB2_HEADER);

    if (ulBytesAvailable < sizeof(SMB2_TREE_DISCONNECT_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppTreeDisconnectHeader = (PSMB2_TREE_DISCONNECT_REQUEST_HEADER)pBuffer;

cleanup:

    return ntStatus;

error:

    *ppTreeDisconnectHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalTreeDisconnectResponse(
    PSMB_PACKET      pPacket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_TREE_DISCONNECT_RESPONSE_HEADER pHeader = NULL;
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    ULONG ulBytesUsed = 0;
    PBYTE pBuffer = pPacket->pParams;

    if (ulBytesAvailable < sizeof(SMB2_TREE_DISCONNECT_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_TREE_DISCONNECT_RESPONSE_HEADER)pBuffer;

    ulBytesUsed += sizeof(SMB2_TREE_DISCONNECT_RESPONSE_HEADER);

    pHeader->usLength = sizeof(SMB2_TREE_DISCONNECT_RESPONSE_HEADER);

    pPacket->bufferUsed += ulBytesUsed;

error:

    return ntStatus;
}

NTSTATUS
SMB2UnmarshalCreateRequest(
    PSMB_PACKET                  pPacket,
    PSMB2_CREATE_REQUEST_HEADER* ppCreateRequestHeader,
    PUNICODE_STRING              pwszFileName
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulOffset = sizeof(SMB2_HEADER);
    PBYTE pDataBuffer = (PBYTE)pPacket->pSMB2Header + ulOffset;
    ULONG ulPacketSize = pPacket->bufferLen - sizeof(NETBIOS_HEADER);
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    PSMB2_CREATE_REQUEST_HEADER pHeader = NULL; // Do not free
    UNICODE_STRING wszFileName = {0}; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_CREATE_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_CREATE_REQUEST_HEADER)pDataBuffer;

    ulBytesAvailable -= sizeof(SMB2_CREATE_REQUEST_HEADER);
    ulOffset += sizeof(SMB2_CREATE_REQUEST_HEADER);

    if (pHeader->usLength & 0x1)
    {
        if ((pHeader->usNameOffset < ulOffset) ||
            (pHeader->usNameOffset % 2) ||
            (pHeader->usNameLength % 2) ||
            (pHeader->usNameOffset > ulPacketSize) ||
            (pHeader->usNameLength + pHeader->usNameOffset) > ulPacketSize)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        wszFileName.Buffer = (PWSTR)((PBYTE)pPacket->pSMB2Header + pHeader->usNameOffset);
        wszFileName.Length = wszFileName.MaximumLength = pHeader->usNameLength;
    }

    if (!wszFileName.Length)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppCreateRequestHeader = pHeader;
    *pwszFileName = wszFileName;

cleanup:

    return ntStatus;

error:

    *ppCreateRequestHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalCloseRequest(
   PSMB_PACKET pPacket,
   PSMB2_FID*  ppFid
   )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulOffset = sizeof(SMB2_HEADER);
    PBYTE pDataBuffer = (PBYTE)pPacket->pSMB2Header + ulOffset;
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    PSMB2_CLOSE_REQUEST_HEADER pHeader = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_CLOSE_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_CLOSE_REQUEST_HEADER)pDataBuffer;

    *ppFid = &pHeader->fid;

cleanup:

    return ntStatus;

error:

    *ppFid = NULL;

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalFlushRequest(
   PSMB_PACKET pPacket,
   PSMB2_FID*  ppFid
   )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulOffset = sizeof(SMB2_HEADER);
    PBYTE pDataBuffer = (PBYTE)pPacket->pSMB2Header + ulOffset;
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    PSMB2_FLUSH_REQUEST_HEADER pHeader = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_FLUSH_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_FLUSH_REQUEST_HEADER)pDataBuffer;

    *ppFid = &pHeader->fid;

cleanup:

    return ntStatus;

error:

    *ppFid = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalFlushResponse(
    PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_FLUSH_RESPONSE_HEADER pHeader = NULL;
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    ULONG ulBytesUsed = 0;
    PBYTE pBuffer = pPacket->pParams;

    if (ulBytesAvailable < sizeof(SMB2_FLUSH_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_FLUSH_RESPONSE_HEADER)pBuffer;
    ulBytesUsed += sizeof(SMB2_FLUSH_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_FLUSH_RESPONSE_HEADER);
    pBuffer += sizeof(SMB2_FLUSH_RESPONSE_HEADER);

    pHeader->usLength = ulBytesUsed;

    pPacket->bufferUsed += ulBytesUsed;

error:

    return ntStatus;
}

NTSTATUS
SMB2UnmarshalEchoRequest(
   PSMB_PACKET                 pPacket,
   PSMB2_ECHO_REQUEST_HEADER*  ppHeader
   )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulOffset = sizeof(SMB2_HEADER);
    PBYTE pDataBuffer = (PBYTE)pPacket->pSMB2Header + ulOffset;
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    PSMB2_ECHO_REQUEST_HEADER pHeader = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_ECHO_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_ECHO_REQUEST_HEADER)pDataBuffer;

    *ppHeader = pHeader;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalEchoResponse(
    PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_ECHO_RESPONSE_HEADER pHeader = NULL;
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    ULONG ulBytesUsed = 0;
    PBYTE pBuffer = pPacket->pParams;

    if (ulBytesAvailable < sizeof(SMB2_ECHO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_ECHO_RESPONSE_HEADER)pBuffer;
    ulBytesUsed += sizeof(SMB2_ECHO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_ECHO_RESPONSE_HEADER);
    pBuffer += sizeof(SMB2_ECHO_RESPONSE_HEADER);

    pHeader->usLength = ulBytesUsed;

    pPacket->bufferUsed += ulBytesUsed;

error:

    return ntStatus;
}

NTSTATUS
SMB2UnmarshalGetInfoRequest(
    PSMB_PACKET                    pPacket,
    PSMB2_GET_INFO_REQUEST_HEADER* ppHeader
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulOffset = sizeof(SMB2_HEADER);
    PBYTE pDataBuffer = (PBYTE)pPacket->pSMB2Header + ulOffset;
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    PSMB2_GET_INFO_REQUEST_HEADER pHeader = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_GET_INFO_REQUEST_HEADER)pDataBuffer;

    *ppHeader = pHeader;

cleanup:

    return ntStatus;

error:

    *ppHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2UnmarshalWriteRequest(
    PSMB_PACKET                 pPacket,
    PSMB2_WRITE_REQUEST_HEADER* ppRequestHeader,
    PBYTE*                      ppData
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulOffset = sizeof(SMB2_HEADER);
    PBYTE pDataBuffer = (PBYTE)pPacket->pSMB2Header + ulOffset;
    ULONG ulPacketSize = pPacket->bufferLen - sizeof(NETBIOS_HEADER);
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    PSMB2_WRITE_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PBYTE                      pData = NULL;

    if (ulBytesAvailable < sizeof(SMB2_WRITE_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PSMB2_WRITE_REQUEST_HEADER)pDataBuffer;
    ulOffset += sizeof(SMB2_WRITE_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SMB2_WRITE_REQUEST_HEADER);

    if (pRequestHeader->usLength & 0x1)
    {
        if ((pRequestHeader->usDataOffset < ulOffset) ||
            (pRequestHeader->usDataOffset > ulPacketSize) ||
            (pRequestHeader->ulDataLength + pRequestHeader->usDataOffset) > ulPacketSize)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pData = (PBYTE)pPacket->pSMB2Header + pRequestHeader->usDataOffset;
    }

    if (pRequestHeader->ulDataLength && !pData)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppRequestHeader = pRequestHeader;
    *ppData = pData;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;
    *ppData = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalWriteResponse(
    PSMB_PACKET pPacket,
    ULONG       ulBytesWritten,
    ULONG       ulBytesRemaining
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_WRITE_RESPONSE_HEADER pHeader = NULL;
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    ULONG ulBytesUsed = 0;
    PBYTE pBuffer = pPacket->pParams;

    if (ulBytesAvailable < sizeof(SMB2_WRITE_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_WRITE_RESPONSE_HEADER)pBuffer;
    ulBytesUsed += sizeof(SMB2_WRITE_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_WRITE_RESPONSE_HEADER);
    pBuffer += sizeof(SMB2_WRITE_RESPONSE_HEADER);

    pHeader->ulBytesWritten = ulBytesWritten;
    pHeader->ulBytesRemaining = ulBytesRemaining;

    pHeader->usLength = ulBytesUsed + 1;

    pPacket->bufferUsed += ulBytesUsed;

error:

    return ntStatus;
}

NTSTATUS
SMB2UnmarshalReadRequest(
    PSMB_PACKET                pPacket,
    PSMB2_READ_REQUEST_HEADER* ppRequestHeader
    )
{
    NTSTATUS ntStatus  = STATUS_SUCCESS;
    ULONG ulOffset     = sizeof(SMB2_HEADER);
    PBYTE pDataBuffer  = (PBYTE)pPacket->pSMB2Header + ulOffset;
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    PSMB2_READ_REQUEST_HEADER pRequestHeader = NULL; // Do not free

    if (ulBytesAvailable < sizeof(SMB2_READ_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PSMB2_READ_REQUEST_HEADER)pDataBuffer;
    ulOffset += sizeof(SMB2_READ_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SMB2_READ_REQUEST_HEADER);

    *ppRequestHeader = pRequestHeader;

cleanup:

    return ntStatus;

error:

    *ppRequestHeader = NULL;

    goto cleanup;
}

NTSTATUS
SMB2MarshalReadResponse(
    PSMB_PACKET pPacket,
    PBYTE       pData,
    ULONG       ulBytesRead,
    ULONG       ulBytesRemaining,
    PULONG      pulDataOffset
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_READ_RESPONSE_HEADER pHeader = NULL;
    ULONG ulDataOffset = sizeof(SMB2_HEADER);
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    ULONG ulBytesUsed = 0;
    PBYTE pBuffer = pPacket->pParams;

    if (ulBytesAvailable < sizeof(SMB2_READ_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_READ_RESPONSE_HEADER)pBuffer;
    ulBytesUsed += sizeof(SMB2_READ_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_READ_RESPONSE_HEADER);
    ulDataOffset += sizeof(SMB2_READ_RESPONSE_HEADER);
    pBuffer += sizeof(SMB2_READ_RESPONSE_HEADER);

    pHeader->ulDataLength = ulBytesRead;
    pHeader->ulRemaining  = ulBytesRemaining;

    if (ulBytesAvailable < ulBytesRead)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader->usDataOffset = ulDataOffset;

    pHeader->usLength = ulBytesUsed + 1;

    if (pData)
    {
        memcpy(pBuffer, pData, ulBytesRead);
        ulBytesUsed += ulBytesRead;
        ulBytesAvailable -= ulBytesRead;
        pBuffer += ulBytesRead;

        pPacket->bufferUsed += ulBytesUsed;
    }

    *pulDataOffset = ulDataOffset;

cleanup:

    return ntStatus;

error:

    *pulDataOffset = 0;

    goto cleanup;
}

NTSTATUS
SMB2MarshalError(
    PSMB_PACKET pPacket,
    NTSTATUS    status
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB2_ERROR_RESPONSE_HEADER pHeader = NULL;
    ULONG ulBytesAvailable = pPacket->bufferLen - pPacket->bufferUsed;
    ULONG ulBytesUsed = 0;
    PBYTE pBuffer = pPacket->pParams;

    if (ulBytesAvailable < sizeof(SMB2_ERROR_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB2_ERROR_RESPONSE_HEADER)pBuffer;
    ulBytesUsed += sizeof(SMB2_ERROR_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_ERROR_RESPONSE_HEADER);
    pBuffer += sizeof(SMB2_ERROR_RESPONSE_HEADER);

    pHeader->usLength = ulBytesUsed;

    pHeader->ulStatus = status;

    pPacket->bufferUsed += ulBytesUsed;

error:

    return ntStatus;
}

NTSTATUS
SMB2MarshalFooter(
    PSMB_PACKET pPacket
    )
{
    pPacket->pNetBIOSHeader->len = htonl(pPacket->bufferUsed - sizeof(NETBIOS_HEADER));

    return 0;
}
