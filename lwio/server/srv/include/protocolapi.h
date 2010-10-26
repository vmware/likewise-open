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
 *        protocolapi.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __PROTOCOL_API_H__
#define __PROTOCOL_API_H__

// Switch this to PFN_SRV_CONNECTION_IO_COMPLETE
typedef VOID (*PFN_SRV_PROTOCOL_SEND_COMPLETE)(
    PVOID    pContext, /* IN */
    NTSTATUS Status    /* IN */
    );

NTSTATUS
SrvProtocolInit(
    PSMB_PROD_CONS_QUEUE       pWorkQueue,        /* IN */
    PLWIO_PACKET_ALLOCATOR     hPacketAllocator,  /* IN */
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList         /* IN */
    );

NTSTATUS
SrvProtocolExecute(
    PSRV_EXEC_CONTEXT pContext /* IN */
    );

NTSTATUS
SrvProtocolAddContext(
    PSRV_EXEC_CONTEXT pExecContext, /* IN OUT */
    BOOLEAN bInConnectionLock       /* IN */
    );

NTSTATUS
SrvProtocolTransportSendResponse(
    PLWIO_SRV_CONNECTION pConnection, /* IN */
    PSMB_PACKET          pPacket,     /* IN */
    PSRV_STAT_INFO       pStatInfo
    );

NTSTATUS
SrvProtocolTransportSendZctResponse(
    IN PLWIO_SRV_CONNECTION pConnection,
    IN PLW_ZCT_VECTOR pZct,
    IN PSRV_STAT_INFO pStatInfo,
    IN OPTIONAL PFN_SRV_PROTOCOL_SEND_COMPLETE pfnCallback,
    IN OPTIONAL PVOID pCallbackContext
    );

NTSTATUS
SrvProtocolTransportContinueAsNonZct(
    IN PLWIO_SRV_CONNECTION pConnection,
    IN PSRV_EXEC_CONTEXT pZctExecContext
    );

NTSTATUS
SrvProtocolTransportReceiveZct(
    IN PLWIO_SRV_CONNECTION pConnection,
    IN PLW_ZCT_VECTOR pZct,
    IN PFN_SRV_CONNECTION_IO_COMPLETE pfnCallback,
    IN PVOID pCallbackContext
    );

NTSTATUS
SrvProtocolTransportResumeFromZct(
    IN PLWIO_SRV_CONNECTION pConnection
    );

NTSTATUS
SrvProtocolEnumerateSessions(
    PWSTR  pwszUncClientname, /* IN     OPTIONAL */
    PWSTR  pwszUsername,      /* IN     OPTIONAL */
    ULONG  ulInfoLevel,       /* IN              */
    PBYTE  pBuffer,           /* IN              */
    ULONG  ulBufferSize,      /* IN              */
    PULONG pulBytesUsed,      /* IN OUT          */
    PULONG pulEntriesRead,    /* IN OUT          */
    PULONG pulTotalEntries,   /* IN OUT          */
    PULONG pulResumeHandle    /* IN OUT OPTIONAL */
    );

NTSTATUS
SrvProtocolDeleteSession(
    PWSTR pwszUncClientname,  /* IN              */
    PWSTR pwszUncUsername     /* IN              */
    );

NTSTATUS
SrvProtocolEnumerateFiles(
    PWSTR  pwszBasepath,      /* IN     OPTIONAL */
    PWSTR  pwszUsername,      /* IN     OPTIONAL */
    ULONG  ulInfoLevel,       /* IN              */
    PBYTE  pBuffer,           /* IN              */
    ULONG  ulBufferSize,      /* IN              */
    PULONG pulBytesUsed,      /* IN OUT          */
    PULONG pulEntriesRead,    /* IN OUT          */
    PULONG pulTotalEntries,   /* IN OUT          */
    PULONG pulResumeHandle    /* IN OUT OPTIONAL */
    );

NTSTATUS
SrvProtocolEnumerateConnections(
    PWSTR  pwszComputerName,     /* IN     OPTIONAL */
    PWSTR  pwszShareName,        /* IN     OPTIONAL */
    ULONG  ulInfoLevel,          /* IN              */
    ULONG  ulPreferredMaxLength, /* IN              */
    PBYTE  pBuffer,              /* IN              */
    ULONG  ulBufferSize,         /* IN              */
    PULONG pulBytesUsed,         /* IN OUT          */
    PULONG pulEntriesRead,       /* IN OUT          */
    PULONG pulTotalEntries,      /* IN OUT          */
    PULONG pulResumeHandle       /* IN OUT OPTIONAL */
    );

NTSTATUS
SrvProtocolGetFileInfo(
    ULONG  ulInfoLevel,       /* IN              */
    ULONG  ulFileId,          /* IN              */
    PBYTE  pBuffer,           /* IN              */
    ULONG  ulBufferSize,      /* IN              */
    PULONG pulBytesUsed       /* IN OUT          */
    );

NTSTATUS
SrvProtocolCloseFile(
    ULONG  ulFileId           /* IN              */
    );

VOID
SrvProtocolShutdown(
    VOID
    );

#endif /* __PROTOCOL_API_H__ */
