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

// config.c

NTSTATUS
LwioSrvStatConfigInit(
    VOID
    );

VOID
LwioSrvStatConfigClearContents(
    PSRV_STAT_HANDLER_CONFIG pConfig
    );

VOID
LwioSrvStatConfigShutdown(
    VOID
    );

// filelog.c

NTSTATUS
LwioSrvStatFilelogInit(
    PCSTR                       pszFilePath,
    PSRV_STAT_HANDLER_FILE_LOG* ppFileLog
    );

VOID
LwioSrvStatFilelogMessage(
    PSRV_STAT_HANDLER_FILE_LOG pFileLog,
    PCSTR                      pszFormat,
    va_list                    msgList
    );

VOID
LwioSrvStatFilelogShutdown(
    PSRV_STAT_HANDLER_FILE_LOG pFileLog
    );

// logging.c

NTSTATUS
LwioSrvStatLoggingInit(
    VOID
    );

VOID
LwioSrvStatLogMessage(
    PSRV_STAT_HANDLER_LOGGER pLogger,
    PCSTR                    pszFormat,
    ...
    );

VOID
LwioSrvStatLoggingShutdown(
    VOID
    );

// statistics.c

NTSTATUS
LwioSrvStatCreateRequestContext(
    PSRV_STAT_CONNECTION_INFO  pConnection,        /* IN              */
    SRV_STAT_SMB_VERSION       protocolVersion,    /* IN              */
    ULONG                      ulRequestLength,    /* IN              */
    PHANDLE                    phContext           /*    OUT          */
    );

NTSTATUS
LwioSrvStatPushMessage(
    HANDLE                       hContext,         /* IN              */
    ULONG                        ulOpcode,         /* IN              */
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
LwioSrvStatSetSessionInfo(
    HANDLE                    hContext,            /* IN              */
    PSRV_STAT_SESSION_INFO    pSessionInfo         /* IN              */
    );

NTSTATUS
LwioSrvStatPopMessage(
    HANDLE                    hContext,            /* IN              */
    ULONG                     ulOpCode,            /* IN              */
    ULONG                     ulResponseLength,    /* IN              */
    NTSTATUS                  msgStatus            /* IN              */
    );

NTSTATUS
LwioSrvStatSetResponseInfo(
    HANDLE                    hContext,            /* IN              */
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
