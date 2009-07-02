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


NTSTATUS
SamrLookupRids(
    handle_t b,
    PolicyHandle *domain_h,
    uint32 num_rids,
    uint32 *rids,
    wchar16_t ***names,
    uint32 **types
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32 i = 0;
    UnicodeStringArray n = {0};
    Ids t = {0};
    wchar16_t **out_names = NULL;
    uint32 *out_types = NULL;

    BAIL_ON_INVALID_PTR(b);
    BAIL_ON_INVALID_PTR(domain_h);
    BAIL_ON_INVALID_PTR(rids);
    BAIL_ON_INVALID_PTR(names);
    BAIL_ON_INVALID_PTR(types);
    
    DCERPC_CALL(_SamrLookupRids(b, domain_h, num_rids, rids, &n, &t));
	
    BAIL_ON_NTSTATUS_ERROR(status);

    if (n.count > 0) {
        status = SamrAllocateMemory((void**)&out_names,
                                    sizeof(wchar16_t*) * n.count,
                                    NULL);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = SamrAllocateMemory((void**)&out_types,
                                    sizeof(uint32) * n.count,
                                    NULL);
        BAIL_ON_NTSTATUS_ERROR(status);

        for (i = 0; i < n.count; i++) {
            UnicodeString *name = &(n.names[i]);

            out_names[i] = GetFromUnicodeString(name);
            BAIL_ON_NO_MEMORY(out_names[i]);

            status = SamrAddDepMemory(out_names[i], (void*)out_names);
            BAIL_ON_NTSTATUS_ERROR(status);

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
