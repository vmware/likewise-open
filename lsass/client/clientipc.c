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
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "client.h"

DWORD
LsaOpenServer(
    PHANDLE phConnection
    )
{
    DWORD dwError = 0;
    int   fd = -1;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext = NULL;
    struct sockaddr_un unixaddr;

    BAIL_ON_INVALID_POINTER(phConnection);

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }

    memset(&unixaddr, 0, sizeof(unixaddr));
    unixaddr.sun_family = AF_UNIX;
    sprintf(unixaddr.sun_path, "%s/%s", CACHEDIR, LSA_SERVER_FILENAME);

    if (connect(fd, (struct sockaddr*)&unixaddr, sizeof(unixaddr)) < 0) {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaAllocateMemory(sizeof(LSA_CLIENT_CONNECTION_CONTEXT),
                                (PVOID*)&pContext);
    BAIL_ON_LSA_ERROR(dwError);

    pContext->fd = fd;
    fd = -1;

    dwError = LsaSendCreds(pContext->fd);
    BAIL_ON_LSA_ERROR(dwError);

    *phConnection = (HANDLE)pContext;

cleanup:

    return dwError;

error:

    if (pContext) {
       if (pContext->fd >= 0) {
          close(pContext->fd);
       }
       LsaFreeMemory(pContext);
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
LsaSendMessage(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
    )
{
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
            (PLSA_CLIENT_CONNECTION_CONTEXT)hConnection;

    return LsaWriteMessage(pContext->fd, pMessage);
}

DWORD
LsaGetNextMessage(
    HANDLE hConnection,
    PLSAMESSAGE* ppMessage
    )
{
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
            (PLSA_CLIENT_CONNECTION_CONTEXT)hConnection;

    return LsaReadNextMessage(pContext->fd, ppMessage);
}

DWORD
LsaCloseServer(
    HANDLE hConnection
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hConnection;

    if (!pContext)
       goto cleanup;

    if (pContext->fd >= 0) {
       close(pContext->fd);
    }
    
    LsaFreeMemory(pContext);

cleanup:

    return dwError;
}

