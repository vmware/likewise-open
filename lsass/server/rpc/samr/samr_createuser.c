/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        samr_createuser.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrCreateUser function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvCreateUser(
    /* [in] */ handle_t hBinding,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UnicodeString *account_name,
    /* [in] */ uint32 access_mask,
    /* [out] */ ACCOUNT_HANDLE *hUser,
    /* [out] */ uint32 *rid
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PWSTR pwszUserName = NULL;
    UnicodeStringEx UserName;
    uint32 ulAccessGranted = 0;

    pDomCtx = (PDOMAIN_CONTEXT)hDomain;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain) {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    ntStatus = SamrSrvGetFromUnicodeString(&pwszUserName,
                                         account_name);
    BAIL_ON_NTSTATUS_ERROR(status);

    ntStatus = SamrSrvInitUnicodeStringEx(&UserName,
                                        pwszUserName);
    BAIL_ON_NTSTATUS_ERROR(status);

    ntStatus = SamrSrvCreateAccount(hBinding,
                                  hDomain,
                                  &UserName,
                                  DS_OBJECT_CLASS_USER,
                                  ACB_NORMAL,
                                  access_mask,
                                  hUser,
                                  &ulAccessGranted,
                                  rid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    if (pwszUserName) {
        SamrSrvFreeMemory(pwszUserName);
    }

    SamrSrvFreeUnicodeStringEx(&UserName);

    return ntStatus;

error:
    *hUser          = NULL;
    *rid            = 0;
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
