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
 *        Protocols API - SMBV1
 *
 *        Wire protocol
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SrvInitPacket_SMB_V1(
    IN OUT PSMB_PACKET pSmbPacket,
    IN     BOOLEAN     bAllowSignature
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE    pBuffer = pSmbPacket->pRawBuffer;
    ULONG    ulBufferAvailable = pSmbPacket->bufferLen;

    if (ulBufferAvailable < sizeof(NETBIOS_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSmbPacket->pNetBIOSHeader = (NETBIOS_HEADER *) (pBuffer);
    pSmbPacket->bufferUsed += sizeof(NETBIOS_HEADER);
    pBuffer += sizeof(NETBIOS_HEADER);
    ulBufferAvailable -= sizeof(NETBIOS_HEADER);

    if (ulBufferAvailable < sizeof(SMB_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSmbPacket->pSMBHeader = (PSMB_HEADER)pBuffer;

    pBuffer           += sizeof(SMB_HEADER);
    ulBufferAvailable -= sizeof(SMB_HEADER);

    if (!ulBufferAvailable)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSmbPacket->pParams = pBuffer;

    pSmbPacket->protocolVer = SMB_PROTOCOL_VERSION_1;
    pSmbPacket->allowSignature = bAllowSignature;

error:

    return ntStatus;
}

NTSTATUS
SrvMarshalHeader_SMB_V1(
    PBYTE         pBuffer,
    ULONG         ulOffset,
    ULONG         ulBytesAvailable,
    UCHAR         ucCommand,
    ULONG         ulError,
    BOOLEAN       bIsResponse,
    USHORT        usTid,
    ULONG         ulPid,
    USHORT        usUid,
    USHORT        usMid,
    BOOLEAN       bCommandAllowsSignature,
    PSMB_HEADER*  ppHeader,
    PBYTE*        ppWordCount,
    PANDX_HEADER* ppAndXHeader,
    PUSHORT       pusBytesUsed
    )
{
    NTSTATUS     ntStatus = 0;
    UCHAR        smbMagic[4] = {0xFF, 'S','M','B'};
    USHORT       usBytesUsed = 0;
    PSMB_HEADER  pHeader = NULL;
    PANDX_HEADER pAndXHeader = NULL;
    PBYTE        pBufferRef = pBuffer;

    if (ulBytesAvailable < sizeof(SMB_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB_HEADER)pBuffer;

    ulBytesAvailable -= sizeof(SMB_HEADER);
    usBytesUsed      += sizeof(SMB_HEADER);
    pBuffer          += sizeof(SMB_HEADER);

    memcpy(&pHeader->smb, &smbMagic[0], sizeof(smbMagic));
    pHeader->command = ucCommand;
    pHeader->error = ulError;
    pHeader->flags = bIsResponse ? FLAG_RESPONSE : 0;
    pHeader->flags |= FLAG_CASELESS_PATHS | FLAG_OBSOLETE_2;
    pHeader->flags2 = ((bIsResponse ? 0 : FLAG2_KNOWS_LONG_NAMES) |
                       (bIsResponse ? 0 : FLAG2_IS_LONG_NAME) |
                       FLAG2_KNOWS_EAS | FLAG2_EXT_SEC |
                       FLAG2_ERR_STATUS | FLAG2_UNICODE);
    memset(pHeader->pad, 0, sizeof(pHeader->pad));
    pHeader->extra.pidHigh = ulPid >> 16;
    pHeader->tid = usTid;
    pHeader->pid = ulPid;
    pHeader->uid = usUid;
    pHeader->mid = usMid;

    if (SMBIsAndXCommand(ucCommand))
    {
        if (ulBytesAvailable < sizeof(ANDX_HEADER))
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pAndXHeader = (PANDX_HEADER) pBuffer;

        ulBytesAvailable -= sizeof(ANDX_HEADER);
        usBytesUsed      += sizeof(ANDX_HEADER);
        pBuffer          += sizeof(ANDX_HEADER);

        pAndXHeader->andXCommand  = ulError == 0 ? 0xFF : 0;
        pAndXHeader->andXOffset   = 0;
        pAndXHeader->andXReserved = 0;
    }

    if (ppHeader)
    {
        *ppHeader = pHeader;
    }

    if (ppWordCount)
    {
        *ppWordCount = &pHeader->wordCount;
    }

    if (ppAndXHeader)
    {
        *ppAndXHeader = pAndXHeader;
    }

    *pusBytesUsed = usBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ppHeader)
    {
        *ppHeader = NULL;
    }

    if (ppWordCount)
    {
        *ppWordCount = NULL;
    }

    if (ppAndXHeader)
    {
        *ppAndXHeader = NULL;
    }

    *pusBytesUsed = 0;

    if (usBytesUsed)
    {
        memset(pBufferRef, 0, usBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SrvUnmarshalHeader_SMB_V1(
    PBYTE         pBuffer,
    ULONG         ulOffset,
    ULONG         ulBytesAvailable,
    PSMB_HEADER*  ppHeader,
    PANDX_HEADER* ppAndXHeader,
    PUSHORT       pusBytesUsed
    )
{
    NTSTATUS     ntStatus    = STATUS_SUCCESS;
    UCHAR        smbMagic[4] = {0xFF, 'S','M','B'};
    PSMB_HEADER  pHeader     = NULL; // Do not free
    PANDX_HEADER pAndXHeader = NULL; // Do not free
    USHORT       usBytesUsed = 0;

    if (ulBytesAvailable < sizeof(SMB_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pHeader = (PSMB_HEADER)pBuffer;

    if (memcmp(&smbMagic[0], &pHeader->smb[0], sizeof(smbMagic)))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pBuffer          += sizeof(SMB_HEADER);
    ulOffset         += sizeof(SMB_HEADER);
    ulBytesAvailable -= sizeof(SMB_HEADER);
    usBytesUsed      += sizeof(SMB_HEADER);

    if (SMBIsAndXCommand(pHeader->command))
    {
        if (ulBytesAvailable < sizeof(ANDX_HEADER))
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pAndXHeader = (PANDX_HEADER) pBuffer;

        // pBuffer          += sizeof(ANDX_HEADER);
        // ulOffset         += sizeof(ANDX_HEADER);
        // ulBytesAvailable -= sizeof(ANDX_HEADER);
        usBytesUsed      += sizeof(ANDX_HEADER);
    }

    if (ppHeader)
    {
        *ppHeader     = pHeader;
    }

    if (ppAndXHeader)
    {
        *ppAndXHeader = pAndXHeader;
    }

    *pusBytesUsed = usBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ppHeader)
    {
        *ppHeader     = NULL;
    }

    if (ppAndXHeader)
    {
        *ppAndXHeader = NULL;
    }

    *pusBytesUsed = 0;

    goto cleanup;
}

NTSTATUS
SrvMarshalHeaderAndX_SMB_V1(
    PBYTE         pBuffer,
    ULONG         ulOffset,
    ULONG         ulBytesAvailable,
    UCHAR         ucCommand,
    PBYTE*        ppWordCount,
    PANDX_HEADER* ppAndXHeader,
    PUSHORT       pusBytesUsed
    )
{
    NTSTATUS     ntStatus = 0;
    USHORT       usBytesUsed = 0;
    PBYTE        pWordCount = NULL;
    PANDX_HEADER pAndXHeader = NULL;
    PBYTE        pBufferRef = pBuffer;

    if (ulBytesAvailable < sizeof(BYTE))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pWordCount = pBuffer;

    ulBytesAvailable -= sizeof(BYTE);
    usBytesUsed      += sizeof(BYTE);
    pBuffer          += sizeof(BYTE);

    if (SMBIsAndXCommand(ucCommand))
    {
        if (ulBytesAvailable < sizeof(ANDX_HEADER))
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pAndXHeader = (PANDX_HEADER) pBuffer;

        ulBytesAvailable -= sizeof(ANDX_HEADER);
        usBytesUsed      += sizeof(ANDX_HEADER);
        pBuffer          += sizeof(ANDX_HEADER);

        pAndXHeader->andXCommand  = 0xFF;
        pAndXHeader->andXOffset   = 0;
        pAndXHeader->andXReserved = 0;
    }

    if (ppWordCount)
    {
        *ppWordCount = pWordCount;
    }

    if (ppAndXHeader)
    {
        *ppAndXHeader = pAndXHeader;
    }

    *pusBytesUsed = usBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ppWordCount)
    {
        *ppWordCount = NULL;
    }

    if (ppAndXHeader)
    {
        *ppAndXHeader = NULL;
    }

    *pusBytesUsed = 0;

    if (usBytesUsed)
    {
        memset(pBufferRef, 0, usBytesUsed);
    }

    goto cleanup;
}

NTSTATUS
SrvUnmarshalHeaderAndX_SMB_V1(
    PBYTE         pBuffer,
    ULONG         ulOffset,
    ULONG         ulBytesAvailable,
    UCHAR         ucCommand,
    PBYTE*        ppWordCount,
    PANDX_HEADER* ppAndXHeader,
    PUSHORT       pusBytesUsed
    )
{
    NTSTATUS     ntStatus    = STATUS_SUCCESS;
    PBYTE        pWordCount  = NULL; // Do not free
    PANDX_HEADER pAndXHeader = NULL; // Do not free
    USHORT       usBytesUsed = 0;

    if (ulBytesAvailable < sizeof(BYTE))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pWordCount = pBuffer;

    pBuffer          += sizeof(BYTE);
    ulOffset         += sizeof(BYTE);
    ulBytesAvailable -= sizeof(BYTE);
    usBytesUsed      += sizeof(BYTE);

    if (SMBIsAndXCommand(ucCommand))
    {
        if (ulBytesAvailable < sizeof(ANDX_HEADER))
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pAndXHeader = (PANDX_HEADER) pBuffer;

        // pBuffer          += sizeof(ANDX_HEADER);
        // ulOffset         += sizeof(ANDX_HEADER);
        // ulBytesAvailable -= sizeof(ANDX_HEADER);
        usBytesUsed      += sizeof(ANDX_HEADER);
    }

    if (ppWordCount)
    {
        *ppWordCount  = pWordCount;
    }

    if (ppAndXHeader)
    {
        *ppAndXHeader = pAndXHeader;
    }

    *pusBytesUsed = usBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ppWordCount)
    {
        *ppWordCount = NULL;
    }

    if (ppAndXHeader)
    {
        *ppAndXHeader = NULL;
    }

    *pusBytesUsed = 0;

    goto cleanup;
}

NTSTATUS
SrvVerifyAndXCommandSequence(
    UCHAR ucLeaderCommand,
    UCHAR ucFollowerCommand
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (ucLeaderCommand)
    {
        case COM_SESSION_SETUP_ANDX:

            switch (ucFollowerCommand)
            {
                case COM_TREE_CONNECT_ANDX:
                case COM_OPEN:
                case COM_OPEN_ANDX:
                case COM_CREATE:
                case COM_CREATE_NEW:
                case COM_CREATE_DIRECTORY:
                case COM_DELETE:
                case COM_DELETE_DIRECTORY:
                case COM_FIND:
                case COM_FIND_UNIQUE:
                case COM_COPY:
                case COM_RENAME:
                case COM_NT_RENAME:
                case COM_CHECK_DIRECTORY:
                case COM_QUERY_INFORMATION:
                case COM_SET_INFORMATION:
                case COM_OPEN_PRINT_FILE:
                case COM_GET_PRINT_QUEUE:
                case COM_TRANSACTION:
                case 0xFF:

                    break;

                default:

                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

                    break;
            }

            break;

        case COM_TREE_CONNECT_ANDX:

            switch (ucFollowerCommand)
            {
                case COM_OPEN:
                case COM_CREATE_NEW:
                case COM_DELETE_DIRECTORY:
                case COM_FIND_UNIQUE:
                case COM_CHECK_DIRECTORY:
                case COM_GET_PRINT_QUEUE:
                case COM_TRANSACTION:
                case COM_SET_INFORMATION:
                case COM_OPEN_ANDX:
                case COM_CREATE_DIRECTORY:
                case COM_FIND:
                case COM_RENAME:
                case COM_QUERY_INFORMATION:
                case COM_OPEN_PRINT_FILE:
                case COM_NT_RENAME:
                case COM_CREATE:
                case COM_DELETE:
                case COM_COPY:
                case 0xFF:

                    break;

                default:

                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

                    break;
            }

            break;

        case COM_NT_CREATE_ANDX:
        case COM_OPEN_ANDX:

            switch (ucFollowerCommand)
            {
                case COM_READ:
                case COM_READ_ANDX:
                case COM_IOCTL:
                case 0xFF:

                    break;

                default:

                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

                    break;
            }

            break;

        case COM_LOCKING_ANDX:

            switch (ucFollowerCommand)
            {
                case COM_READ:
                case COM_READ_ANDX:
                case COM_WRITE:
                case COM_WRITE_ANDX:
                case COM_FLUSH:
                case 0xFF:

                    break;

                default:

                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

                    break;
            }

            break;

        case COM_READ_ANDX:

            switch (ucFollowerCommand)
            {
                case COM_CLOSE:
                case 0xFF:

                    break;

                default:

                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

                    break;
            }

            break;

        case COM_WRITE_ANDX:

            switch (ucFollowerCommand)
            {
                case COM_READ:
                case COM_READ_ANDX:
                case COM_LOCK_AND_READ:
                case COM_WRITE_ANDX:
                case COM_CLOSE:
                case 0xFF:

                    break;

                default:

                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

                    break;
            }

            break;

        case COM_LOGOFF_ANDX:

            switch (ucFollowerCommand)
            {
                case COM_SESSION_SETUP_ANDX:
                case 0xFF:

                    break;

                default:

                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

                    break;
            }

            break;

        default:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

            break;
    }

    return ntStatus;
}
