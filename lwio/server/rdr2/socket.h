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

#ifndef __SOCKET_H__
#define __SOCKET_H__

NTSTATUS
SMBSocketCreate(
    IN PCWSTR pwszHostname,
    IN BOOLEAN bUseSignedMessagesIfSupported,
    OUT PSMB_SOCKET* ppSocket
    );

NTSTATUS
SMBSocketConnect(
    PSMB_SOCKET      pSocket
    );

VOID
RdrSocketRevive(
    PSMB_SOCKET pSocket
    );

VOID
SMBSocketInvalidate(
    PSMB_SOCKET    pSocket,
    NTSTATUS ntStatus
    );

NTSTATUS
SMBSocketSend(
    IN PSMB_SOCKET pSocket,
    IN PSMB_PACKET pPacket
    );

VOID
SMBSocketRelease(
    PSMB_SOCKET pSocket
    );

VOID
RdrSocketSetIgnoreServerSignatures(
    PSMB_SOCKET pSocket,
    BOOLEAN bValue
    );

VOID
SMBSocketBeginSequence(
    PSMB_SOCKET pSocket
    );

NTSTATUS
RdrSocketAcquireMid(
    PSMB_SOCKET pSocket,
    USHORT* pusMid
    );

NTSTATUS
RdrSocketAddResponse(
    PSMB_SOCKET pSocket,
    PSMB_RESPONSE pResponse
    );

NTSTATUS
RdrSocketRemoveResponse(
    PSMB_SOCKET pSocket,
    PSMB_RESPONSE pResponse
    );

NTSTATUS
RdrSocketReceiveResponse(
    IN PSMB_SOCKET pSocket,
    IN BOOLEAN bVerifySignature,
    IN DWORD dwExpectedSequence,
    IN PSMB_RESPONSE pResponse,
    OUT PSMB_PACKET* ppResponsePacket
    );

NTSTATUS
RdrSocketTransceive(
    IN OUT PSMB_SOCKET pSocket,
    IN PRDR_OP_CONTEXT pContext
    );

VOID
RdrSocketCancel(
    IN PSMB_SOCKET pSocket,
    IN PRDR_OP_CONTEXT pContext
    );

NTSTATUS
SMBSrvClientSocketAddSessionByUID(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    );

NTSTATUS
SMBSrvClientSocketCreate(
    IN PCWSTR pwszHostname,
    OUT PSMB_SOCKET* ppSocket
    );

NTSTATUS
RdrSocketInit(
    VOID
    );

NTSTATUS
RdrSocketShutdown(
    VOID
    );

NTSTATUS
SMBResponseCreate(
    uint16_t       wMid,
    SMB_RESPONSE **ppResponse
    );

VOID
SMBResponseFree(
    PSMB_RESPONSE pResponse
    );

VOID
SMBResponseInvalidate_InLock(
    PSMB_RESPONSE pResponse,
    NTSTATUS ntStatus
    );

#endif /* __SOCKET_H__ */
