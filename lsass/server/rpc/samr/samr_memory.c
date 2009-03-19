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
 *        samr_memeory.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Samr memory allocation manager
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrSrvInitMemory(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (!bSamrSrvInitialised && !pSamrSrvMemRoot) {
        pSamrSrvMemRoot = talloc(NULL, 0, NULL);
        BAIL_ON_NO_MEMORY(pSamrSrvMemRoot);
    }

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    goto cleanup;
}


NTSTATUS
SamrSrvDestroyMemory(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (bSamrSrvInitialised && pSamrSrvMemRoot) {
        tfree(pSamrSrvMemRoot);
    }

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    goto cleanup;
}


NTSTATUS
SamrSrvAllocateMemory(
    void **ppOut,
    DWORD dwSize,
    void *pDep
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    void *pOut = NULL;
    void *pParent = NULL;
    int locked = 0;

    pParent = (pDep) ? pDep : pSamrSrvMemRoot;

    GLOBAL_DATA_LOCK(locked);

    pOut = talloc(pParent, dwSize, NULL);
    BAIL_ON_NO_MEMORY(pOut);

    memset(pOut, 0, dwSize);

    *ppOut = pOut;

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    *ppOut = NULL;
    goto cleanup;
}


void
SamrSrvFreeMemory(
    void *pPtr
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    tfree(pPtr);

error:
    GLOBAL_DATA_UNLOCK(locked);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
