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
 *        lsa_openpolicy2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaOpenPolicy2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvOpenPolicy2(
    /* [in] */ handle_t hBinding,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ ObjectAttribute *attrib,
    /* [in] */ uint32 access_mask,
    /* [out] */ POLICY_HANDLE *hPolicy
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PPOLICY_CONTEXT pPolCtx = NULL;
    HANDLE hDirectory = NULL;

    status = LsaSrvAllocateMemory((void**)&pPolCtx,
                                  sizeof(*pPolCtx),
                                  NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    dwError = DirectoryOpen(&hDirectory);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    pPolCtx->Type        = LsaContextPolicy;
    pPolCtx->refcount    = 1;
    pPolCtx->hDirectory  = hDirectory;

    /* Increase ref count because DCE/RPC runtime is about to use this
       pointer as well */
    InterlockedIncrement(&pPolCtx->refcount);

    *hPolicy = (POLICY_HANDLE)pPolCtx;

cleanup:
    return status;

error:
    if (pPolCtx) {
        InterlockedDecrement(&pPolCtx->refcount);
        POLICY_HANDLE_rundown((POLICY_HANDLE)pPolCtx);
    }

    *hPolicy = NULL;
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
