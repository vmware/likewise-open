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


NTSTATUS SamrLookupRids(handle_t b, PolicyHandle *domain_handle,
                        uint32 num_rids, uint32 *rids,
                        wchar16_t ***names, uint32 **types)
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32 i = 0;
    UnicodeStringArray n = {0};
    Ids t = {0};
    wchar16_t **out_names = NULL;
    uint32 *out_types = NULL;

    goto_if_no_memory_ntstatus(b, cleanup);
    goto_if_no_memory_ntstatus(domain_handle, cleanup);
    goto_if_no_memory_ntstatus(rids, cleanup);
    goto_if_no_memory_ntstatus(names, cleanup);
    goto_if_no_memory_ntstatus(types, cleanup);
    
    TRY
    {
        status = _SamrLookupRids(b, domain_handle, num_rids, rids, &n, &t);
	
    }
    CATCH_ALL
    {
        status = STATUS_UNHANDLED_EXCEPTION;
    }
    ENDTRY;

    goto_if_ntstatus_not_success(status, error);

    if (n.count > 0) {
        status = SamrAllocateMemory((void**)&out_names,
                                    sizeof(wchar16_t*) * n.count,
                                    NULL);
        goto_if_ntstatus_not_success(status, error);

        status = SamrAllocateMemory((void**)&out_types,
                                    sizeof(uint32) * n.count,
                                    NULL);
        goto_if_ntstatus_not_success(status, error);

        /* TODO: verify termination correctness (similar code has been fixed) */

        for (i = 0; i < n.count; i++) {
            UnicodeString *name = &(n.names[i]);

            out_names[i] = GetFromUnicodeString(name);
            goto_if_no_memory_ntstatus(out_names[i], error);

            status = SamrAddDepMemory(out_names[i], (void*)out_names);
            goto_if_ntstatus_not_success(status, error);

            out_types[i] = t.ids[i];
        }
    }

    *names = out_names;
    *types = out_types;

cleanup:
    SamrCleanStubIds(&t);
    SamrCleanStubUnicodeStringArray(&n);
    
    return status;

error:
    if (out_names) {
        SamrFreeMemory((void*)out_names);
    }

    if (out_types) {
        SamrFreeMemory((void*)out_types);
    }

    *names = NULL;
    *types = NULL;

    goto error;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
