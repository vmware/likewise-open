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
    PSRV_STAT_REQUEST_CONTEXT* ppContext           /*    OUT          */
    );

NTSTATUS
LwioSrvStatSetRequestInfo(
    PSRV_STAT_REQUEST_CONTEXT  pContext,           /* IN              */
    SRV_STAT_SMB_VERSION       protocolVersion,    /* IN              */
    ULONG                      ulRequestLength     /* IN              */
    );

NTSTATUS
LwioSrvStatPushMessage(
    PSRV_STAT_REQUEST_CONTEXT    pContext,         /* IN              */
    ULONG                        ulOpcode,         /* IN              */
    PSRV_STAT_REQUEST_PARAMETERS pParams,
    PBYTE                        pMessage,         /* IN     OPTIONAL */
    ULONG                        ulMessageLen      /* IN              */
    );

NTSTATUS
LwioSrvStatSetSubOpcode(
    PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
    ULONG                     ulSubOpcode          /* IN              */
    );

NTSTATUS
LwioSrvStatSetIOCTL(
    PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
    ULONG                     ulIoCtlCode          /* IN              */
    );

NTSTATUS
LwioSrvStatSessionCreated(
    PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
    PSRV_STAT_SESSION_INFO    pSessionInfo         /* IN              */
    );

NTSTATUS
LwioSrvStatTreeCreated(
    PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
    PSRV_STAT_SESSION_INFO    pSessionInfo,        /* IN              */
    PSRV_STAT_TREE_INFO       pTreeInfo            /* IN              */
    );

NTSTATUS
LwioSrvStatFileCreated(
    PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
    PSRV_STAT_SESSION_INFO    pSessionInfo,        /* IN              */
    PSRV_STAT_TREE_INFO       pTreeInfo,           /* IN              */
    PSRV_STAT_FILE_INFO       pFileInfo            /* IN              */
    );

NTSTATUS
LwioSrvStatFileClosed(
    PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
    PSRV_STAT_FILE_INFO       pFileInfo            /* IN              */
    );

NTSTATUS
LwioSrvStatTreeClosed(
    PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
    PSRV_STAT_TREE_INFO       pTreeInfo            /* IN              */
    );

NTSTATUS
LwioSrvStatSessionClosed(
    PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
    PSRV_STAT_SESSION_INFO    pSessionInfo         /* IN              */
    );

NTSTATUS
LwioSrvStatPopMessage(
    PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
    ULONG                     ulOpCode,            /* IN              */
    NTSTATUS                  msgStatus            /* IN              */
    );

NTSTATUS
LwioSrvStatSetResponseInfo(
    PSRV_STAT_REQUEST_CONTEXT pContext,            /* IN              */
    NTSTATUS                  responseStatus,      /* IN              */
    PBYTE                     pResponseBuffer,     /* IN     OPTIONAL */
    ULONG                     ulResponseLength     /* IN              */
    );

NTSTATUS
LwioSrvStatCloseRequestContext(
    PSRV_STAT_REQUEST_CONTEXT pContext             /* IN              */
    );
