/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ipc_client.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

DWORD
LWNetOpenServer(
    PHANDLE phConnection
    )
{
    DWORD dwError = 0;
    int   fd = -1;
    PLWNET_CLIENT_CONNECTION_CONTEXT pContext = NULL;
    struct sockaddr_un unixaddr;
    
    BAIL_ON_INVALID_POINTER(phConnection);

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        dwError = errno;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    memset(&unixaddr, 0, sizeof(unixaddr));
    unixaddr.sun_family = AF_UNIX;
    sprintf(unixaddr.sun_path, "%s/%s", LWNET_CACHE_DIR, LWNET_SERVER_FILENAME);

    if (connect(fd, (struct sockaddr*)&unixaddr, sizeof(unixaddr)) < 0) {
        dwError = errno;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetAllocateMemory(sizeof(LWNET_CLIENT_CONNECTION_CONTEXT),
                                (PVOID*)&pContext);
    BAIL_ON_LWNET_ERROR(dwError);
    
    pContext->fd = fd;
    fd = -1;

    dwError = LWNetSendCreds(pContext->fd);
    BAIL_ON_LWNET_ERROR(dwError);

    *phConnection = (HANDLE)pContext;

cleanup:

    return dwError;

error:

    if (pContext) {
       if (pContext->fd >= 0) {
          close(pContext->fd);
       }
       LWNetFreeMemory(pContext);
    }

    if (fd >= 0) {
        close(fd);
    }

    if (phConnection) {
        *phConnection = (HANDLE)NULL;
    }

    goto cleanup;
}

DWORD
LWNetSendMessage(
    HANDLE hConnection,
    PLWNETMESSAGE pMessage
    )
{
    PLWNET_CLIENT_CONNECTION_CONTEXT pContext =
            (PLWNET_CLIENT_CONNECTION_CONTEXT)hConnection;

    return LWNetWriteMessage(pContext->fd, pMessage);
}

DWORD
LWNetGetNextMessage(
    HANDLE hConnection,
    PLWNETMESSAGE* ppMessage
    )
{
    PLWNET_CLIENT_CONNECTION_CONTEXT pContext =
            (PLWNET_CLIENT_CONNECTION_CONTEXT)hConnection;

    return LWNetReadNextMessage(pContext->fd, ppMessage);
}

DWORD
LWNetCloseServer(
    HANDLE hConnection
    )
{
    DWORD dwError = 0;
    PLWNET_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLWNET_CLIENT_CONNECTION_CONTEXT)hConnection;

    if (pContext && (pContext->fd >= 0))
    {
       close(pContext->fd);
    }

    LWNetFreeMemory(pContext);

    return dwError;
}

