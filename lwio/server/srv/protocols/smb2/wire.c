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
    USHORT      usCredits,
    ULONG       ulPid,
    ULONG       ulTid,
    ULONG64     ullSessionId,
    NTSTATUS    status,
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
    pSmbPacket->pSMB2Header->usCredits      = usCredits;
    pSmbPacket->pSMB2Header->ulPid          = ulPid;
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
SMB2PacketMarshallFooter(
    PSMB_PACKET pPacket
    )
{
    pPacket->pNetBIOSHeader->len = htonl(pPacket->bufferUsed - sizeof(NETBIOS_HEADER));

    return 0;
}
