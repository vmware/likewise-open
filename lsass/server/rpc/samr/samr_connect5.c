/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        samr_connect5.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrConnect5 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


NTSTATUS
SamrSrvConnect5(
    /* [in] */ handle_t hBinding,
    /* [in] */ uint32 size,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ uint32 access_mask,
    /* [in] */ uint32 level_in,
    /* [in] */ SamrConnectInfo *info_in,
    /* [out] */ uint32 *level_out,
    /* [out] */ SamrConnectInfo *info_out,
    /* [out] */ CONNECT_HANDLE *hConn
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PCONNECT_CONTEXT pConn = NULL;
    SAMR_CONNECT_INFO Info;
    DWORD dwLevel = 0;

    status = RTL_ALLOCATE(&pConn, CONNECT_CONTEXT, sizeof(*pConn));
    BAIL_ON_NTSTATUS_ERROR(status);

    dwError = DirectoryOpen(&pConn->hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    dwLevel = 1;
    Info.info1.client_version = info_in->info1.client_version;
    Info.info1.unknown1       = 0;

    pConn->Type     = SamrContextConnect;
    pConn->refcount = 1;

    /* Increase ref count because DCE/RPC runtime is about to use this
       pointer as well */
    InterlockedIncrement(&pConn->refcount);

    *level_out = dwLevel;
    *info_out  = Info;
    *hConn     = (CONNECT_HANDLE)pConn;

cleanup:
    return status;

error:
    if (pConn) {
        InterlockedDecrement(&pConn->refcount);
        CONNECT_HANDLE_rundown((CONNECT_HANDLE)pConn);
    }

    *hConn = NULL;
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
