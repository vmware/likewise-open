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
 *        srv/protocols/structs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Transport
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __SRV_TRANSPORT_STRUCTS_H__
#define __SRV_TRANSPORT_STRUCTS_H__

typedef struct _LWIO_SRV_SOCKET
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    int fd;

    struct sockaddr_in cliaddr;

} LWIO_SRV_SOCKET, *PLWIO_SRV_SOCKET;

typedef struct _LWIO_SRV_CONNECTION
{
    LONG                refCount;

    pthread_rwlock_t     mutex;
    pthread_rwlock_t*    pMutex;

    LWIO_SRV_CONN_STATE  state;

    PLWIO_SRV_SOCKET     pSocket;

    SRV_PROPERTIES        serverProperties;
    SRV_CLIENT_PROPERTIES clientProperties;

    ULONG               ulSequence;

    // Invariant
    // Not owned
    HANDLE              hPacketAllocator;

    struct
    {
        BOOLEAN         bReadHeader;
        size_t          sNumBytesToRead;
        size_t          sOffset;
        PSMB_PACKET     pRequestPacket;

    } readerState;

    PBYTE               pSessionKey;
    ULONG               ulSessionKeyLength;

    PSRV_HOST_INFO       pHostinfo;
    PLWIO_SRV_SHARE_LIST pShareList;

    HANDLE              hGssContext;
    HANDLE              hGssNegotiate;

    PLWRTL_RB_TREE      pSessionCollection;

    USHORT              nextAvailableUid;

} LWIO_SRV_CONNECTION, *PLWIO_SRV_CONNECTION;

typedef struct _LWIO_SRV_CONTEXT
{
    PLWIO_SRV_CONNECTION pConnection;

    PSMB_PACKET         pRequest;

    ULONG               ulRequestSequence;

} LWIO_SRV_CONTEXT, *PLWIO_SRV_CONTEXT;

#endif /* __SRV_TRANSPORT_STRUCTS_H__ */
