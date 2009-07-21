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
 * Abstract: NetApi memory (de)allocation routines (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
NetInitMemory(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    /* Init allocation of dependant rpc libraries first */
    status = LsaRpcInitMemory();
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrInitMemory();
    BAIL_ON_NTSTATUS_ERROR(status);

    if (!bNetApiInitialised) {
        status = MemPtrListInit((PtrList**)&netapi_ptr_list);
        BAIL_ON_NTSTATUS_ERROR(status);

        bNetApiInitialised = 1;
    }
cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    goto cleanup;
}


NTSTATUS
NetDestroyMemory(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (bNetApiInitialised && netapi_ptr_list) {
        status = MemPtrListDestroy((PtrList**)&netapi_ptr_list);
        BAIL_ON_NTSTATUS_ERROR(status);

        bNetApiInitialised = 0;
    }

    /* Destroy allocation of dependant rpc libraries */
    status = LsaRpcDestroyMemory();
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrDestroyMemory();
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    goto cleanup;
}


NTSTATUS
NetAllocateMemory(
    void **out,
    size_t size,
    void *dep
    )
{
    return MemPtrAllocate((PtrList*)netapi_ptr_list, out, size, dep);
}


NTSTATUS
NetFreeMemory(
    void *ptr
    )
{
    return MemPtrFree((PtrList*)netapi_ptr_list, ptr);
}


NTSTATUS
NetAddDepMemory(
    void *ptr,
    void *dep
    )
{
    return MemPtrAddDependant((PtrList*)netapi_ptr_list, ptr, dep);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
