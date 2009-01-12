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



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        globals.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem
 *
 *        Global Variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

SMBSERVERINFO gServerInfo =
{
    PTHREAD_MUTEX_INITIALIZER,  /* Lock              */
    0,                          /* Start as daemon   */
    SMB_LOG_TARGET_DISABLED,    /* log disabled      */
    SMB_LOG_LEVEL_ERROR,        /* Max Log Level     */
    FALSE,                      /* Enable debug logs */
    "",                         /* Log file path     */
    "",                         /* Cache path        */
    "",                         /* Prefix path       */
    0,                          /* Process exit flag */
    0                           /* Process exit code */
};

LWMsgDispatchSpec gLSMBdispatch[] =
{
    LWMSG_DISPATCH(SMB_REFRESH_CONFIG,           SMBSrvIpcRefreshConfiguration),
    LWMSG_DISPATCH(SMB_SET_LOG_LEVEL,            SMBSrvIpcSetLogInfo),
    LWMSG_DISPATCH(SMB_GET_LOG_INFO,             SMBSrvIpcGetLogInfo),
    LWMSG_DISPATCH(SMB_CALL_NAMED_PIPE,          SMBSrvIpcCallNamedPipe),
    LWMSG_DISPATCH(SMB_CREATE_NAMED_PIPE,        SMBSrvIpcCreateNamedPipe),
    LWMSG_DISPATCH(SMB_GET_NAMED_PIPE_INFO,      SMBSrvIpcGetNamedPipeInfo),
    LWMSG_DISPATCH(SMB_CONNECT_NAMED_PIPE,       SMBSrvIpcConnectNamedPipe),
    LWMSG_DISPATCH(SMB_TRANSACT_NAMED_PIPE,      SMBSrvIpcTransactNamedPipe),
    LWMSG_DISPATCH(SMB_WAIT_NAMED_PIPE,          SMBSrvIpcWaitNamedPipe),
    LWMSG_DISPATCH(SMB_GET_CLIENT_COMPUTER_NAME, SMBSrvIpcGetClientComputerName),
    LWMSG_DISPATCH(SMB_GET_CLIENT_PROCESS_ID,    SMBSrvIpcGetClientProcessId),
    LWMSG_DISPATCH(SMB_GET_SERVER_PROCESS_ID,    SMBSrvIpcGetServerProcessId),
    LWMSG_DISPATCH(SMB_GET_CLIENT_SESSION_ID,    SMBSrvIpcGetClientSessionId),
    LWMSG_DISPATCH(SMB_PEEK_NAMED_PIPE,          SMBSrvIpcPeekNamedPipe),
    LWMSG_DISPATCH(SMB_DISCONNECT_NAMED_PIPE,    SMBSrvIpcDisconnectNamedPipe),
    LWMSG_DISPATCH(SMB_CREATE_FILE,              SMBSrvIpcCreateFile),
    LWMSG_DISPATCH(SMB_SET_NAMED_PIPE_HANDLE_STATE, SMBSrvIpcSetNamedPipeHandleState),
    LWMSG_DISPATCH(SMB_READ_FILE,                SMBSrvIpcReadFile),
    LWMSG_DISPATCH(SMB_WRITE_FILE,               SMBSrvIpcWriteFile),
    LWMSG_DISPATCH(SMB_CLOSE_FILE,               SMBSrvIpcCloseFile),
    LWMSG_DISPATCH(SMB_GET_SESSION_KEY,          SMBSrvIpcGetSessionKey),
    LWMSG_DISPATCH_END
};

PSMBSERVERINFO gpServerInfo = &gServerInfo;

pthread_t  gSignalHandlerThread;
pthread_t* gpSignalHandlerThread = NULL;

pthread_mutex_t gServerConfigLock = PTHREAD_MUTEX_INITIALIZER;
SMB_CONFIG gServerConfig;
PSMB_CONFIG gpServerConfig = &gServerConfig;


