/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        transport.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API
 *
 *        Protocol Transport Driver
 *
 *        [OPTIONAL FILE DESCRIPTION]
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#include "includes.h"

static
int
SrvProtocolTransportConnectionCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SrvProtocolTransportConnectionRelease(
    PVOID pConnection
    );

// Transport Callbacks

static
NTSTATUS
SrvProtocolTransportDriverConnectionNew(
    OUT PSRV_CONNECTION* ppConnection,
    IN PSRV_PROTOCOL_TRANSPORT_CONTEXT pProtocolDispatchContext,
    IN PSRV_SOCKET pSocket
    );

static
NTSTATUS
SrvProtocolTransportDriverConnectionData(
    IN PSRV_CONNECTION pConnection,
    IN ULONG Length
    );

static
VOID
SrvProtocolTransportDriverConnectionDone(
    IN PSRV_CONNECTION pConnection,
    IN NTSTATUS Status
    );

static
NTSTATUS
SrvProtocolTransportDriverSendPrepare(
    IN PSRV_SEND_CONTEXT pSendContext
    );

static
VOID
SrvProtocolTransportDriverSendDone(
    IN PSRV_SEND_CONTEXT pSendContext,
    IN NTSTATUS Status
    );

// Helpers

static
VOID
SrvProtocolTransportDriverSocketFree(
    IN OUT PSRV_SOCKET pSocket
    );

static
VOID
SrvProtocolTransportDriverSocketDisconnect(
    IN PSRV_SOCKET pSocket
    );

static
NTSTATUS
SrvProtocolTransportDriverSocketGetAddressBytes(
    IN PSRV_SOCKET pSocket,
    OUT PVOID* ppAddress,
    OUT PULONG pAddressLength
    );

static
NTSTATUS
SrvProtocolTransportDriverAllocatePacket(
    IN PSRV_CONNECTION pConnection
    );

static
NTSTATUS
SrvProtocolTransportDriverUpdateBuffer(
    IN PSRV_CONNECTION pConnection
    );

static
VOID
SrvProtocolTransportDriverRemoveBuffer(
    IN PSRV_CONNECTION pConnection
    );

static
NTSTATUS
SrvProtocolTransportDriverDetectPacket(
    IN PSRV_CONNECTION pConnection,
    IN OUT PULONG pulBytesAvailable,
    OUT PSMB_PACKET* ppPacket
    );

static
NTSTATUS
SrvProtocolTransportDriverDispatchPacket(
    IN PSRV_CONNECTION pConnection,
    IN PSMB_PACKET pPacket
    );

static
NTSTATUS
SrvProtocolTransportDriverCheckSignature(
    IN PSRV_EXEC_CONTEXT pContext
    );

static
ULONG
SrvProtocolTransportDriverGetNextSequence(
    IN PSRV_CONNECTION pConnection,
    IN PSMB_PACKET pPacket
    );

static
VOID
SrvProtocolTransportDriverFreeResources(
    IN PSRV_SEND_CONTEXT pSendContext
    );

// Implementations

NTSTATUS
SrvProtocolTransportDriverInit(
    PSRV_PROTOCOL_API_GLOBALS pGlobals
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_TRANSPORT_CONTEXT pTransportContext = &pGlobals->transportContext;
    PSRV_TRANSPORT_PROTOCOL_DISPATCH pTransportDispatch = &pTransportContext->dispatch;
    PSRV_CONNECTION_SOCKET_DISPATCH pSocketDispatch = &pTransportContext->socketDispatch;

    RtlZeroMemory(pTransportContext, sizeof(*pTransportContext));

    pTransportContext->pGlobals = pGlobals;

    pTransportDispatch->pfnConnectionNew = SrvProtocolTransportDriverConnectionNew;
    pTransportDispatch->pfnConnectionData = SrvProtocolTransportDriverConnectionData;
    pTransportDispatch->pfnConnectionDone = SrvProtocolTransportDriverConnectionDone;
    pTransportDispatch->pfnSendPrepare = SrvProtocolTransportDriverSendPrepare;
    pTransportDispatch->pfnSendDone = SrvProtocolTransportDriverSendDone;

    pSocketDispatch->pfnFree = SrvProtocolTransportDriverSocketFree;
    pSocketDispatch->pfnDisconnect = SrvProtocolTransportDriverSocketDisconnect;
    pSocketDispatch->pfnGetAddressBytes = SrvProtocolTransportDriverSocketGetAddressBytes;

    uuid_generate(pTransportContext->guid);

    ntStatus = LwRtlRBTreeCreate(
                    &SrvProtocolTransportConnectionCompare,
                    NULL,
                    &SrvProtocolTransportConnectionRelease,
                    &pGlobals->pConnections);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTransportInit(
                    &pTransportContext->hTransport,
                    pTransportDispatch,
                    pTransportContext);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    SrvProtocolTransportDriverShutdown(pGlobals);

    goto cleanup;
}

static
int
SrvProtocolTransportConnectionCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PCSTR pszAddress1 = (PCSTR)pKey1;
    PCSTR pszAddress2 = (PCSTR)pKey2;

    return strcasecmp(pszAddress1, pszAddress2);
}

static
VOID
SrvProtocolTransportConnectionRelease(
    PVOID pConnection
    )
{
    SrvConnectionRelease((PLWIO_SRV_CONNECTION)pConnection);
}

VOID
SrvProtocolTransportDriverShutdown(
    PSRV_PROTOCOL_API_GLOBALS pGlobals
    )
{
    PSRV_PROTOCOL_TRANSPORT_CONTEXT pTransportContext = &pGlobals->transportContext;

    if (pGlobals->pConnections)
    {
        LwRtlRBTreeFree(pGlobals->pConnections);
        pGlobals->pConnections = NULL;
    }

    if (pTransportContext->hTransport)
    {
        // This will cause done notifications to occur,
        // which will get rid of any last remaining connection
        // references.
        SrvTransportShutdown(pTransportContext->hTransport);
        pTransportContext->hTransport = NULL;
    }

    if (pTransportContext->hGssContext)
    {
        SrvGssReleaseContext(pTransportContext->hGssContext);
    }

    RtlZeroMemory(pTransportContext, sizeof(*pTransportContext));
}

static
NTSTATUS
SrvProtocolTransportDriverConnectionNew(
    OUT PSRV_CONNECTION* ppConnection,
    IN PSRV_PROTOCOL_TRANSPORT_CONTEXT pProtocolDispatchContext,
    IN PSRV_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_API_GLOBALS pGlobals = pProtocolDispatchContext->pGlobals;
    SRV_PROPERTIES properties = { 0 };
    PSRV_HOST_INFO pHostinfo = NULL;
    PLWIO_SRV_CONNECTION pConnection = NULL;
    BOOLEAN bInLock = FALSE;

    ntStatus = SrvAcquireHostInfo(NULL, &pHostinfo);
    if (ntStatus)
    {
        LWIO_LOG_ERROR("Failed to acquire current host information (status = 0x%08x)", ntStatus);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // No lock required because only one connect can be
    // happening at a time.

    if (!pProtocolDispatchContext->hGssContext)
    {
        ntStatus = SrvGssAcquireContext(
                        pHostinfo,
                        NULL,
                        &pProtocolDispatchContext->hGssContext);
        if (ntStatus)
        {
            LWIO_LOG_ERROR("Failed to initialize GSS Handle (status = 0x%08x)", ntStatus);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    properties.preferredSecurityMode = SMB_SECURITY_MODE_USER;
    properties.bEncryptPasswords = TRUE;
    properties.bEnableSecuritySignatures = SrvProtocolConfigIsSigningEnabled();
    properties.bRequireSecuritySignatures = SrvProtocolConfigIsSigningRequired();
    properties.ulZctReadThreshold = SrvProtocolConfigGetZctReadThreshold();
    properties.ulZctWriteThreshold = SrvProtocolConfigGetZctWriteThreshold();
    properties.MaxRawSize = 64 * 1024;
    properties.MaxMpxCount = 50;
    properties.MaxNumberVCs = 1;
    properties.MaxBufferSize = 16644;
    properties.Capabilities = 0;
    properties.Capabilities |= CAP_UNICODE;
    properties.Capabilities |= CAP_LARGE_FILES;
    properties.Capabilities |= CAP_NT_SMBS;
    properties.Capabilities |= CAP_RPC_REMOTE_APIS;
    properties.Capabilities |= CAP_STATUS32;
    properties.Capabilities |= CAP_LEVEL_II_OPLOCKS;
    properties.Capabilities |= CAP_LARGE_READX;
    properties.Capabilities |= CAP_LARGE_WRITEX;
    properties.Capabilities |= CAP_EXTENDED_SECURITY;
    uuid_copy(properties.GUID, pProtocolDispatchContext->guid);

    ntStatus = SrvConnectionCreate(
                    SrvTransportSocketGetAddressString(pSocket),
                    pSocket,
                    pGlobals->hPacketAllocator,
                    pProtocolDispatchContext->hGssContext,
                    pGlobals->pShareList,
                    &properties,
                    pHostinfo,
                    &pProtocolDispatchContext->socketDispatch,
                    &pConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvElementsRegisterResource(&pConnection->resource, NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    pConnection->pProtocolTransportDriverContext = pProtocolDispatchContext;

    // Allocate buffer space
    ntStatus = SrvProtocolTransportDriverAllocatePacket(pConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    // Update remaining buffer space
    ntStatus = SrvProtocolTransportDriverUpdateBuffer(pConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gProtocolApiGlobals.mutex);

    ntStatus = LwRtlRBTreeAdd(
                    gProtocolApiGlobals.pConnections,
                    &pConnection->resource.ulResourceId,
                    pConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppConnection = SrvConnectionAcquire(pConnection);;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &gProtocolApiGlobals.mutex);

    if (pHostinfo)
    {
        SrvReleaseHostInfo(pHostinfo);
    }

    return ntStatus;

error:

    if (pConnection)
    {
        SrvConnectionRelease(pConnection);
    }

    goto cleanup;
}

// This implements that state machine for the connection.
// TODO-Add ZCT SMB write support
static
NTSTATUS
SrvProtocolTransportDriverConnectionData(
    IN PSRV_CONNECTION pConnection,
    IN ULONG BytesAvailable
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;
    ULONG bytesRemaining = BytesAvailable;
    PSMB_PACKET pPacket = NULL;

    LWIO_ASSERT(BytesAvailable > 0);

    // TODO-Remove this lock?  This should be doable
    // because the transport guarantees that only one thread can be
    // processing a connection data callback.  The only caveat
    // might be when we introduce buffer manipulation from another thread.
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    while (bytesRemaining > 0)
    {
        ntStatus = SrvProtocolTransportDriverDetectPacket(
                        pConnection,
                        &bytesRemaining,
                        &pPacket);
        BAIL_ON_NT_STATUS(ntStatus);

        if (pPacket)
        {
            // allocate a new packet buffer
            ntStatus = SrvProtocolTransportDriverAllocatePacket(pConnection);
            BAIL_ON_NT_STATUS(ntStatus);

            if (bytesRemaining)
            {
                PVOID pFromBuffer = NULL;

                pFromBuffer = LwRtlOffsetToPointer(
                                    pPacket->pRawBuffer,
                                    pPacket->bufferUsed);

                // need to copy over the bytes into allocated packet
                RtlCopyMemory(
                        pConnection->readerState.pRequestPacket->pRawBuffer,
                        pFromBuffer,
                        bytesRemaining);
            }

            // dispatch packet -- takes a reference
            ntStatus = SrvProtocolTransportDriverDispatchPacket(
                            pConnection,
                            pPacket);
            BAIL_ON_NT_STATUS(ntStatus);

            SMBPacketRelease(
                    pConnection->hPacketAllocator,
                    pPacket);
            pPacket = NULL;
        }
    }

    // Update remaining buffer space
    ntStatus = SrvProtocolTransportDriverUpdateBuffer(pConnection);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    // Do not give any buffer to the socket.
    SrvProtocolTransportDriverRemoveBuffer(pConnection);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    SrvConnectionSetInvalid(pConnection);

    if (pPacket)
    {
        SMBPacketRelease(
                pConnection->hPacketAllocator,
                pPacket);
    }

    goto cleanup;
}

static
VOID
SrvProtocolTransportDriverConnectionDone(
    IN PSRV_CONNECTION pConnection,
    IN NTSTATUS Status
    )
{
    BOOLEAN bInLock = FALSE;

    if (STATUS_CONNECTION_RESET == Status)
    {
        LWIO_LOG_DEBUG("Connection reset by peer '%s' (fd = %d)",
                SrvTransportSocketGetAddressString(pConnection->pSocket),
                SrvTransportSocketGetFileDescriptor(pConnection->pSocket));
    }

    if (pConnection->resource.ulResourceId)
    {
        PSRV_RESOURCE pResource = NULL;

        SrvElementsUnregisterResource(
                pConnection->resource.ulResourceId,
                &pResource);

        LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gProtocolApiGlobals.mutex);

        LwRtlRBTreeRemove(
                gProtocolApiGlobals.pConnections,
                &pConnection->resource.ulResourceId);

        LWIO_UNLOCK_RWMUTEX(bInLock, &gProtocolApiGlobals.mutex);
    }

    SrvConnectionSetInvalid(pConnection);
    SrvConnectionRelease(pConnection);
}

static
NTSTATUS
SrvProtocolTransportDriverSendPrepare(
    IN PSRV_SEND_CONTEXT pSendContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_CONNECTION pConnection = pSendContext->pConnection;
    PSMB_PACKET pPacket = pSendContext->pPacket;

    // Sign the packet.

    if (SrvConnectionIsSigningActive_inlock(pConnection))
    {
        switch (pPacket->protocolVer)
        {
            case SMB_PROTOCOL_VERSION_1:

                pPacket->pSMBHeader->flags2 |= FLAG2_SECURITY_SIG;

                ntStatus = SMBPacketSign(
                                pPacket,
                                pPacket->sequence,
                                pConnection->pSessionKey,
                                pConnection->ulSessionKeyLength);

                break;

            case SMB_PROTOCOL_VERSION_2:

                if (pPacket->pSMB2Header->ullCommandSequence != -1)
                {
                    ntStatus = SMB2PacketSign(
                                   pPacket,
                                   pConnection->pSessionKey,
                                   pConnection->ulSessionKeyLength);
                }

                break;

            default:

                ntStatus = STATUS_INTERNAL_ERROR;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
VOID
SrvProtocolTransportDriverSendDone(
    IN PSRV_SEND_CONTEXT pSendContext,
    IN NTSTATUS Status
    )
{
    PFN_SRV_PROTOCOL_SEND_COMPLETE pfnCallback = NULL;
    PVOID pCallbackContext = NULL;

    // There is no need to close the socket here on an error Status as
    // the transport will take care of doing a ConnectionDone which will
    // trigger the closing of the socket.

    if (pSendContext->bIsZct)
    {
        pfnCallback = pSendContext->pfnCallback;
        pCallbackContext = pSendContext->pCallbackContext;
    }

    SrvProtocolTransportDriverFreeResources(pSendContext);

    if (pfnCallback)
    {
        pfnCallback(pCallbackContext, Status);
    }
}

static
VOID
SrvProtocolTransportDriverSocketFree(
    IN OUT PSRV_SOCKET pSocket
    )
{
    SrvTransportSocketClose(pSocket);
}

static
VOID
SrvProtocolTransportDriverSocketDisconnect(
    IN PSRV_SOCKET pSocket
    )
{
    SrvTransportSocketDisconnect(pSocket);
}

static
NTSTATUS
SrvProtocolTransportDriverSocketGetAddressBytes(
    IN PSRV_SOCKET pSocket,
    OUT PVOID* ppAddress,
    OUT PULONG pAddressLength
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    const struct sockaddr* pSocketAddress = NULL;
    size_t socketAddressLength = 0;
    PVOID pAddressPart = NULL;
    ULONG addressPartLength = 0;

    SrvTransportSocketGetAddress(pSocket, &pSocketAddress, &socketAddressLength);
    switch (pSocketAddress->sa_family)
    {
        case AF_INET:
            pAddressPart = &((struct sockaddr_in*)pSocketAddress)->sin_addr.s_addr;
            addressPartLength = sizeof(((struct sockaddr_in*)pSocketAddress)->sin_addr.s_addr);
            break;

        default:
            LWIO_ASSERT(FALSE);
            ntStatus = STATUS_NOT_SUPPORTED;
            BAIL_ON_NT_STATUS(ntStatus);
            break;
    }

cleanup:

    *ppAddress = pAddressPart;
    *pAddressLength = addressPartLength;

    return ntStatus;

error:

    pAddressPart = NULL;
    addressPartLength = 0;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolTransportDriverAllocatePacket(
    IN PSRV_CONNECTION pConnection
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    LWIO_ASSERT(!pConnection->readerState.pRequestPacket);

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pConnection->readerState.pRequestPacket);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    SMB_PACKET_DEFAULT_SIZE,
                    &pConnection->readerState.pRequestPacket->pRawBuffer,
                    &pConnection->readerState.pRequestPacket->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    pConnection->readerState.bNeedHeader = TRUE;
    pConnection->readerState.sNumBytesToRead = sizeof(NETBIOS_HEADER);
    pConnection->readerState.sOffset = 0;
    pConnection->readerState.pRequestPacket->bufferUsed = 0;

cleanup:

    return ntStatus;

error:

    SMBPacketRelease(
            pConnection->hPacketAllocator,
            pConnection->readerState.pRequestPacket);

    pConnection->readerState.pRequestPacket = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolTransportDriverUpdateBuffer(
    IN PSRV_CONNECTION pConnection
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pBuffer = NULL;
    ULONG Size = 0;
    ULONG Minimum = 0;

    // TODO-Perhaps remove sOffset as it appears to be exactly pRequestPacket->bufferUsed.
    LWIO_ASSERT(pConnection->readerState.sOffset == pConnection->readerState.pRequestPacket->bufferUsed);
    LWIO_ASSERT(pConnection->readerState.pRequestPacket->bufferLen >= pConnection->readerState.pRequestPacket->bufferUsed);

    pBuffer = LwRtlOffsetToPointer(
                    pConnection->readerState.pRequestPacket->pRawBuffer,
                    pConnection->readerState.pRequestPacket->bufferUsed);
    Size = pConnection->readerState.pRequestPacket->bufferLen - pConnection->readerState.pRequestPacket->bufferUsed;
    Minimum = pConnection->readerState.sNumBytesToRead;

    // TODO-Set Minimum = 1 (or something smarter) for ZCT for SMB write.
    // TODO-Test out setting Size = Minimum (perhaps registry config) -- IFF not doing ZCT SMB write support.
    ntStatus = SrvTransportSocketSetBuffer(
                    pConnection->pSocket,
                    pBuffer,
                    Size,
                    Minimum);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
VOID
SrvProtocolTransportDriverRemoveBuffer(
    IN PSRV_CONNECTION pConnection
    )
{
    // Do not give any buffer to the socket.
    SrvTransportSocketSetBuffer(
            pConnection->pSocket,
            NULL,
            0,
            0);
}

static
NTSTATUS
SrvProtocolTransportDriverDetectPacket(
    IN PSRV_CONNECTION pConnection,
    IN OUT PULONG pulBytesAvailable,
    OUT PSMB_PACKET* ppPacket
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulBytesAvailable = *pulBytesAvailable;
    PSMB_PACKET pPacketFound = NULL;

    LWIO_ASSERT(ulBytesAvailable > 0);

    if (pConnection->readerState.bNeedHeader)
    {
        size_t sNumBytesRead = LW_MIN(pConnection->readerState.sNumBytesToRead, ulBytesAvailable);

        pConnection->readerState.sNumBytesToRead -= sNumBytesRead;
        pConnection->readerState.sOffset += sNumBytesRead;
        pConnection->readerState.pRequestPacket->bufferUsed += sNumBytesRead;

        if (!pConnection->readerState.sNumBytesToRead)
        {
            PSMB_PACKET pPacket = pConnection->readerState.pRequestPacket;
            ULONG ulBytesAvailable = pPacket->bufferLen - sizeof(NETBIOS_HEADER);

            pConnection->readerState.bNeedHeader = FALSE;

            pPacket->pNetBIOSHeader = (NETBIOS_HEADER *) pPacket->pRawBuffer;
            pPacket->pNetBIOSHeader->len = ntohl(pPacket->pNetBIOSHeader->len);

            pConnection->readerState.sNumBytesToRead = pPacket->pNetBIOSHeader->len;

            // check if the message fits in our currently allocated buffer
            if (pConnection->readerState.sNumBytesToRead > ulBytesAvailable)
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }

        ulBytesAvailable -= sNumBytesRead;
    }

    if (ulBytesAvailable &&
        !pConnection->readerState.bNeedHeader &&
        pConnection->readerState.sNumBytesToRead)
    {
        size_t sNumBytesRead = LW_MIN(pConnection->readerState.sNumBytesToRead, ulBytesAvailable);

        pConnection->readerState.sNumBytesToRead            -= sNumBytesRead;
        pConnection->readerState.sOffset                    += sNumBytesRead;
        pConnection->readerState.pRequestPacket->bufferUsed += sNumBytesRead;

        ulBytesAvailable -= sNumBytesRead;
    }

    // Packet is complete
    if (!pConnection->readerState.bNeedHeader &&
        !pConnection->readerState.sNumBytesToRead)
    {
        PSMB_PACKET pPacket    = pConnection->readerState.pRequestPacket;
        PBYTE pBuffer          = pPacket->pRawBuffer + sizeof(NETBIOS_HEADER);
        ULONG ulBytesAvailable = pPacket->bufferUsed - sizeof(NETBIOS_HEADER);

        switch (*pBuffer)
        {
            case 0xFF:
                pPacket->protocolVer = SMB_PROTOCOL_VERSION_1;

                if (ulBytesAvailable < sizeof(SMB_HEADER))
                {
                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                pPacket->pSMBHeader = (PSMB_HEADER)(pBuffer);
                pBuffer            += sizeof(SMB_HEADER);
                ulBytesAvailable   -= sizeof(SMB_HEADER);

                if (SMBIsAndXCommand(pPacket->pSMBHeader->command))
                {
                    if (ulBytesAvailable < sizeof(ANDX_HEADER))
                    {
                        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                        BAIL_ON_NT_STATUS(ntStatus);
                    }

                    pPacket->pAndXHeader = (PANDX_HEADER)pBuffer;
                    pBuffer             += sizeof(ANDX_HEADER);
                    ulBytesAvailable    -= sizeof(ANDX_HEADER);
                }

                pPacket->pParams = (ulBytesAvailable > 0) ? pBuffer : NULL;
                pPacket->pData = NULL;

                break;

            case 0xFE:
                pPacket->protocolVer = SMB_PROTOCOL_VERSION_2;

                if (ulBytesAvailable < sizeof(SMB2_HEADER))
                {
                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                pPacket->pSMB2Header = (PSMB2_HEADER)pBuffer;
                pBuffer             += sizeof(SMB2_HEADER);
                ulBytesAvailable    -= sizeof(SMB2_HEADER);

                pPacket->pParams    = NULL;
                pPacket->pData      = NULL;

                break;

            default:
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

        switch (pConnection->protocolVer)
        {
            case SMB_PROTOCOL_VERSION_UNKNOWN:
                ntStatus = SrvConnectionSetProtocolVersion_inlock(
                                pConnection,
                                pPacket->protocolVer);
                break;

            default:
                if (pConnection->protocolVer != pPacket->protocolVer)
                {
                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                }
                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        pPacketFound = pConnection->readerState.pRequestPacket;
        pConnection->readerState.pRequestPacket = NULL;
    }

cleanup:

    *pulBytesAvailable = ulBytesAvailable;
    *ppPacket = pPacketFound;

    return ntStatus;

error:

    pPacketFound = NULL;
    ulBytesAvailable = 0;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolTransportDriverDispatchPacket(
    IN PSRV_CONNECTION pConnection,
    IN PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_TRANSPORT_CONTEXT pDriverContext = (PSRV_PROTOCOL_TRANSPORT_CONTEXT) pConnection->pProtocolTransportDriverContext;
    PSRV_EXEC_CONTEXT pContext = NULL;

    // Already in pConnection lock.

    // Note that building the context takes its own reference.
    ntStatus = SrvBuildExecContext(pConnection, pPacket, FALSE, &pContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProtocolTransportDriverCheckSignature(pContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProdConsEnqueue(
                    pDriverContext->pGlobals->pWorkQueue,
                    pContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pContext = NULL;

cleanup:

    return ntStatus;

error:

    if (pContext)
    {
        SrvReleaseExecContext(pContext);
    }

    goto cleanup;
}

static
NTSTATUS
SrvProtocolTransportDriverCheckSignature(
    IN PSRV_EXEC_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION pConnection = pContext->pConnection;

    // Already in pConnection lock.

    switch (pConnection->protocolVer)
    {
        case SMB_PROTOCOL_VERSION_1:
            // Update the sequence whether we end up signing or not
            pContext->pSmbRequest->sequence = SrvProtocolTransportDriverGetNextSequence(
                            pConnection,
                            pContext->pSmbRequest);
            break;

        default:
            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    if (SrvConnectionIsSigningActive_inlock(pConnection))
    {
        switch (pConnection->protocolVer)
        {
            case SMB_PROTOCOL_VERSION_1:
                ntStatus = SMBPacketVerifySignature(
                                pContext->pSmbRequest,
                                pContext->pSmbRequest->sequence,
                                pConnection->pSessionKey,
                                pConnection->ulSessionKeyLength);

                break;

            case SMB_PROTOCOL_VERSION_2:
                // Allow CANCEL and ECHO requests to be unsigned
                if (SMB2PacketIsSigned(pContext->pSmbRequest))
                {
                    ntStatus = SMB2PacketVerifySignature(
                        pContext->pSmbRequest,
                        pConnection->pSessionKey,
                        pConnection->ulSessionKeyLength);
                }
                else if (pContext->pSmbRequest->pSMB2Header->command != COM2_CANCEL &&
                         pContext->pSmbRequest->pSMB2Header->command != COM2_ECHO)
                {
                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                break;

            default:
                ntStatus = STATUS_INTERNAL_ERROR;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}

static
ULONG
SrvProtocolTransportDriverGetNextSequence(
    IN PSRV_CONNECTION pConnection,
    IN PSMB_PACKET pPacket
    )
{
    ULONG ulRequestSequence = 0;

    // Already in pConnection lock.

    switch (pPacket->pSMBHeader->command)
    {
        case COM_NEGOTIATE:

            break;

        case COM_NT_CANCEL:

            ulRequestSequence = pConnection->ulSequence++;

            break;

        case COM_SESSION_SETUP_ANDX:

            /* Sequence number increments don't start until the last leg
               of the first successful session setup */

            if (pConnection->state == LWIO_SRV_CONN_STATE_NEGOTIATE)
            {
                ulRequestSequence = 0;
                pConnection->ulSequence = 2;
            }
            else
            {
                ulRequestSequence = pConnection->ulSequence;
                pConnection->ulSequence += 2;
            }

            break;

        default:

            ulRequestSequence = pConnection->ulSequence;
            pConnection->ulSequence += 2;

            break;
    }

    return ulRequestSequence;
}

static
VOID
SrvProtocolTransportDriverFreeResources(
    IN PSRV_SEND_CONTEXT pSendContext
    )
{
    if (!pSendContext->bIsZct)
    {
        SMBPacketRelease(
            pSendContext->pConnection->hPacketAllocator,
            pSendContext->pPacket);
    }

    SrvConnectionRelease(pSendContext->pConnection);

    SrvFreeMemory(pSendContext);
}

NTSTATUS
SrvProtocolTransportSendResponse(
    IN PLWIO_SRV_CONNECTION pConnection,
    IN PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_SEND_CONTEXT pSendContext = NULL;

    ntStatus = SrvAllocateMemory(sizeof(*pSendContext), OUT_PPVOID(&pSendContext));
    BAIL_ON_NT_STATUS(ntStatus);

    pSendContext->pConnection = pConnection;
    SrvConnectionAcquire(pConnection);

    // TODO-Should remove refcounting from pPacket altogether?
    pSendContext->pPacket = pPacket;
    InterlockedIncrement(&pPacket->refCount);

    ntStatus = SrvTransportSocketSendReply(
                    pConnection->pSocket,
                    pSendContext,
                    pSendContext->pPacket->pRawBuffer,
                    pSendContext->pPacket->bufferUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvProtocolTransportDriverFreeResources(pSendContext);

cleanup:

    // This function never returns pending because the caller
    // does not have a callback mechanism.
    if (ntStatus == STATUS_PENDING)
    {
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;

error:

    if (ntStatus != STATUS_PENDING)
    {
        if (pSendContext)
        {
            SrvProtocolTransportDriverFreeResources(pSendContext);
        }

        // This will trigger rundown in protocol code.
        SrvConnectionSetInvalid(pConnection);
    }

    goto cleanup;
}

NTSTATUS
SrvProtocolTransportSendZctResponse(
    IN PLWIO_SRV_CONNECTION pConnection,
    IN PLW_ZCT_VECTOR pZct,
    IN OPTIONAL PFN_SRV_PROTOCOL_SEND_COMPLETE pfnCallback,
    IN OPTIONAL PVOID pCallbackContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_SEND_CONTEXT pSendContext = NULL;

    LWIO_ASSERT(!pCallbackContext || pfnCallback);

    ntStatus = SrvAllocateMemory(sizeof(*pSendContext), OUT_PPVOID(&pSendContext));
    BAIL_ON_NT_STATUS(ntStatus);

    pSendContext->pConnection = pConnection;
    SrvConnectionAcquire(pConnection);

    pSendContext->bIsZct = TRUE;

    pSendContext->pZct = pZct;
    pSendContext->pfnCallback = pfnCallback;
    pSendContext->pCallbackContext = pCallbackContext;

    ntStatus = SrvTransportSocketSendZctReply(
                    pConnection->pSocket,
                    pSendContext,
                    pSendContext->pZct);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvProtocolTransportDriverFreeResources(pSendContext);

cleanup:

    return ntStatus;

error:

    if (ntStatus != STATUS_PENDING)
    {
        if (pSendContext)
        {
            SrvProtocolTransportDriverFreeResources(pSendContext);
        }

        // This will trigger rundown in protocol code.
        SrvConnectionSetInvalid(pConnection);
    }

    goto cleanup;
}

