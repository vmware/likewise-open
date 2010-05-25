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
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Reference Statistics Logging Module (SRV)
 *
 *        Private Prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

// statistics.c

NTSTATUS
LwioSrvStatCreateRequestContext(
    PSRV_STAT_CONNECTION_INFO  pConnection,        /* IN              */
    PHANDLE                    phContext           /*    OUT          */
    );

NTSTATUS
LwioSrvStatSetRequestInfo(
    HANDLE                    hContext,            /* IN              */
    SRV_STAT_SMB_VERSION       protocolVersion,    /* IN              */
    ULONG                      ulRequestLength     /* IN              */
    );

NTSTATUS
LwioSrvStatPushMessage(
    HANDLE                       hContext,         /* IN              */
    ULONG                        ulOpcode,         /* IN              */
    PSRV_STAT_REQUEST_PARAMETERS pParams,          /* IN     OPTIONAL */
    PBYTE                        pMessage,         /* IN     OPTIONAL */
    ULONG                        ulMessageLen      /* IN              */
    );

NTSTATUS
LwioSrvStatSetSubOpcode(
    HANDLE                    hContext,            /* IN              */
    ULONG                     ulSubOpcode          /* IN              */
    );

NTSTATUS
LwioSrvStatSetIOCTL(
    HANDLE                    hContext,            /* IN              */
    ULONG                     ulIoCtlCode          /* IN              */
    );

NTSTATUS
LwioSrvStatSessionCreated(
    HANDLE                    hContext,            /* IN              */
    PSRV_STAT_SESSION_INFO    pSessionInfo         /* IN              */
    );

NTSTATUS
LwioSrvStatTreeCreated(
    HANDLE                    hContext,            /* IN              */
    PSRV_STAT_SESSION_INFO    pSessionInfo,        /* IN              */
    PSRV_STAT_TREE_INFO       pTreeInfo            /* IN              */
    );

NTSTATUS
LwioSrvStatFileCreated(
    HANDLE                    hContext,            /* IN              */
    PSRV_STAT_SESSION_INFO    pSessionInfo,        /* IN              */
    PSRV_STAT_TREE_INFO       pTreeInfo,           /* IN              */
    PSRV_STAT_FILE_INFO       pFileInfo            /* IN              */
    );

NTSTATUS
LwioSrvStatFileClosed(
    HANDLE                    hContext,            /* IN              */
    PSRV_STAT_FILE_INFO       pFileInfo            /* IN              */
    );

NTSTATUS
LwioSrvStatTreeClosed(
    HANDLE                    hContext,            /* IN              */
    PSRV_STAT_TREE_INFO       pTreeInfo            /* IN              */
    );

NTSTATUS
LwioSrvStatSessionClosed(
    HANDLE                    hContext,            /* IN              */
    PSRV_STAT_SESSION_INFO    pSessionInfo         /* IN              */
    );

NTSTATUS
LwioSrvStatPopMessage(
    HANDLE                    hContext,            /* IN              */
    ULONG                     ulOpCode,            /* IN              */
    NTSTATUS                  msgStatus            /* IN              */
    );

NTSTATUS
LwioSrvStatSetResponseInfo(
    HANDLE                    hContext,            /* IN              */
    NTSTATUS                  responseStatus,      /* IN              */
    PBYTE                     pResponseBuffer,     /* IN     OPTIONAL */
    ULONG                     ulResponseLength     /* IN              */
    );

NTSTATUS
LwioSrvStatCloseRequestContext(
    HANDLE                    hContext             /* IN              */
    );

// times.c

NTSTATUS
LwioSrvStatGetCurrentNTTime(
    PLONG64 pllCurTime
    );
