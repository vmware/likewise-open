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

#include "includes.h"

#include <lwrpc/memptr.h>

extern void *netr_ptr_list;


NTSTATUS NetrInitMemory()
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (!bInitialised) {
        status = MemPtrListInit((PtrList**)&netr_ptr_list);
        goto_if_ntstatus_not_success(status, done);

        bInitialised = 1;
    }

done:
    GLOBAL_DATA_UNLOCK(locked);

    return status;
}


NTSTATUS NetrDestroyMemory()
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (bInitialised && netr_ptr_list) {
        status = MemPtrListDestroy((PtrList**)&netr_ptr_list);
        goto_if_ntstatus_not_success(status, done);

        bInitialised = 0;
    }

done:
    GLOBAL_DATA_UNLOCK(locked);

    return status;
}


NTSTATUS NetrAllocateMemory(void **out, size_t size, void *dep)
{
    return MemPtrAllocate((PtrList*)netr_ptr_list, out, size, dep);
}


NTSTATUS NetrFreeMemory(void *ptr)
{
    return MemPtrFree((PtrList*)netr_ptr_list, ptr);
}


NTSTATUS NetrAddDepMemory(void *ptr, void *dep)
{
    return MemPtrAddDependant((PtrList*)netr_ptr_list, ptr, dep);
}


NTSTATUS NetrAllocateDomainTrusts(NetrDomainTrust **out,
                                  NetrDomainTrustList *in)
{
    NTSTATUS status = STATUS_SUCCESS;
    NetrDomainTrust *ptr = NULL;
    int i = 0;

    goto_if_invalid_param_ntstatus(out, cleanup);
    goto_if_invalid_param_ntstatus(in, cleanup);

    status = NetrAllocateMemory((void**)&ptr,
                                sizeof(NetrDomainTrust) * in->count,
                                NULL);
    goto_if_ntstatus_not_success(status, cleanup);

    for (i = 0; i < in->count; i++) {
        NetrDomainTrust *tout = &ptr[i];
        NetrDomainTrust *tin  = &in->array[i];

        tout->trust_flags  = tin->trust_flags;
        tout->parent_index = tin->parent_index;
        tout->trust_type   = tin->trust_type;
        tout->trust_attrs  = tin->trust_attrs;

        if (tin->netbios_name) {
            tout->netbios_name = wc16sdup(tin->netbios_name);
            goto_if_no_memory_ntstatus(tout->netbios_name, error);

            status = NetrAddDepMemory((void*)tout->netbios_name, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
        }

        if (tin->dns_name) {
            tout->dns_name = wc16sdup(tin->dns_name);
            goto_if_no_memory_ntstatus(tout->dns_name, error);

            status = NetrAddDepMemory((void*)tout->dns_name, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
        }

        if (tin->sid)
        {
            SidCopyAlloc(&tout->sid, tin->sid);
            goto_if_no_memory_ntstatus(tout->sid, error);

            status = NetrAddDepMemory((void*)tout->sid, (void*)ptr);
            goto_if_ntstatus_not_success(status, error);
        }

        tout->guid = tin->guid;
    }

cleanup:
    *out = ptr;

    return status;

error:
    NetrFreeMemory((void*)ptr);
    ptr = NULL;
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
