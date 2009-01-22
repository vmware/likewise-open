/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#include "includes.h"

DWORD
SMBOpenServer(
    PHANDLE phConnection
    )
{
    DWORD dwError = 0;
    PSMB_SERVER_CONNECTION pConnection = NULL;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_INVALID_POINTER(gpSMBProtocol);

    dwError = SMBAllocateMemory(
                    sizeof(SMB_SERVER_CONNECTION),
                    (PVOID*)&pConnection);
    BAIL_ON_SMB_ERROR(dwError);

    status = lwmsg_connection_new(
                    gpSMBProtocol,
                    &pConnection->pAssoc);
    BAIL_ON_SMB_ERROR(status);

    status = lwmsg_connection_set_endpoint(
                    pConnection->pAssoc,
                    LWMSG_CONNECTION_MODE_LOCAL,
                    SMB_SERVER_FILENAME);
    BAIL_ON_SMB_ERROR(status);

    status = lwmsg_connection_establish(pConnection->pAssoc);
    BAIL_ON_SMB_ERROR(status);

    *phConnection = (HANDLE)pConnection;

cleanup:

    return dwError;

error:
    if (status && !dwError)
    {
        dwError = -1;
    }

    if (pConnection)
    {
        SMBCloseServer((HANDLE)pConnection);
    }

    *phConnection = (HANDLE)NULL;

    goto cleanup;
}

DWORD
SMBCloseServer(
    HANDLE hConnection
    )
{
    DWORD dwError = 0;
    PSMB_SERVER_CONNECTION pConnection = NULL;

    pConnection = (PSMB_SERVER_CONNECTION)hConnection;

    if (pConnection)
    {
        if (pConnection->pAssoc)
        {
            LWMsgStatus status = lwmsg_assoc_close(pConnection->pAssoc);
            if (status)
            {
                SMB_LOG_ERROR("Failed to close association [Error code:%d]", status);
            }

            lwmsg_assoc_delete(pConnection->pAssoc);
        }

        SMBFreeMemory(pConnection);
    }

    return dwError;
}
