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

#ifndef __IPC_H__
#define __IPC_H__

LWMsgStatus
SMBSrvIpcRefreshConfiguration(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcSetLogInfo(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcGetLogInfo(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcCallNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcCreateNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcGetNamedPipeInfo(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcConnectNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcTransactNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcWaitNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcGetClientComputerName(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcGetClientProcessId(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcGetServerProcessId(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcGetClientSessionId(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcPeekNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcDisconnectNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcCreateFile(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcSetNamedPipeHandleState(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcReadFile(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcWriteFile(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcCloseFile(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

LWMsgStatus
SMBSrvIpcGetSessionKey(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    );

#endif /* __IPC_H__ */
