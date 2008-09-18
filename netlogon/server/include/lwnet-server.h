/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        lwnet-server.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LWNETSS) Server
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __LWNETSERVER_H__
#define __LWNETSERVER_H__

#include "lwnet-utils.h"
#include "lwnet-ipc.h"

typedef struct __LWNETSERVERCONNECTIONCONTEXT {
    int    fd;
    uid_t  peerUID;
    gid_t  peerGID;
    short  bConnected;
    HANDLE hServer;
} LWNETSERVERCONNECTIONCONTEXT, *PLWNETSERVERCONNECTIONCONTEXT;

#define LWNET_SAFE_FREE_CONNECTION_CONTEXT(pContext) \
    if (pContext)  {                                  \
       LWNetSrvFreeContext(pContext);                \
       pContext = NULL;                               \
    }

DWORD
LWNetSrvOpenConnection(
    int     fd,
    uid_t   uid,
    gid_t   gid,
    PHANDLE phConnection
    );

void
LWNetSrvCloseConnection(
    HANDLE hConnection
    );

void
LWNetSrvHandleConnection(
    HANDLE hConnection
    );

#endif /* __LWNETSERVER_H__ */

