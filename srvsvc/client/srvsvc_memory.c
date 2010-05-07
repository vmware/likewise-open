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

#include "includes.h"

NET_API_STATUS
SrvSvcInitMemory(
    VOID
    )
{
    NET_API_STATUS status = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (!bSrvSvcInitialised) {
        ntStatus = MemPtrListInit((PtrList**)&srvsvc_ptr_list);
        BAIL_ON_NT_STATUS(ntStatus);

        bSrvSvcInitialised = 1;
    }

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    if (status == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        status = LwNtStatusToWin32Error(ntStatus);
    }

    return status;

error:
    goto cleanup;
}

NET_API_STATUS
SrvSvcAllocateMemory(
    void** out,
    size_t size,
    void*  dep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    ntStatus = MemPtrAllocate((PtrList*)srvsvc_ptr_list, out, size, dep);

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return LwNtStatusToWin32Error(ntStatus);

error:
    goto cleanup;
}


NET_API_STATUS
SrvSvcFreeMemory(
    void* ptr
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    ntStatus = MemPtrFree((PtrList*)srvsvc_ptr_list, ptr);

    GLOBAL_DATA_UNLOCK(locked);

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return LwNtStatusToWin32Error(ntStatus);

error:
    goto cleanup;
}

NET_API_STATUS
SrvSvcAddDepStringW(
    PVOID  pDep,
    PCWSTR pwszSource,
    PWSTR* ppwszDest
    )
{
    NET_API_STATUS status = 0;
    PWSTR  pwszCopy = NULL;

    status = LwAllocateWc16String(&pwszCopy, pwszSource);
    BAIL_ON_WIN_ERROR(status);

    status = SrvSvcAddDepMemory(pwszCopy, pDep);
    BAIL_ON_WIN_ERROR(status);

    *ppwszDest = pwszCopy;

cleanup:

    return status;

error:

    *ppwszDest = NULL;

    if (pwszCopy)
    {
        LwFreeMemory(pwszCopy);
    }

    goto cleanup;
}


NET_API_STATUS
SrvSvcAddDepMemory(
    void *ptr,
    void *dep
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    ntStatus = MemPtrAddDependant((PtrList*)srvsvc_ptr_list, ptr, dep);

cleanup:

    GLOBAL_DATA_UNLOCK(locked);

    return LwNtStatusToWin32Error(ntStatus);

error:

    goto cleanup;
}

NET_API_STATUS
SrvSvcDestroyMemory(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (bSrvSvcInitialised && srvsvc_ptr_list)
    {
        ntStatus = MemPtrListDestroy((PtrList**)&srvsvc_ptr_list);
        BAIL_ON_NT_STATUS(ntStatus);

        bSrvSvcInitialised = 0;
    }

cleanup:

    GLOBAL_DATA_UNLOCK(locked);

    return LwNtStatusToWin32Error(ntStatus);;

error:

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
