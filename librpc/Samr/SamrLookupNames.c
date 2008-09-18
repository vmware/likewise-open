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


// rids_count can be NULL, in which case the number of returned rids must
// match num_names.
NTSTATUS SamrLookupNames(handle_t b, PolicyHandle *handle,
                         uint32 num_names, wchar16_t *names[],
                         uint32 **rids, uint32 **types,
                         uint32 *rids_count)
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32 i = 0;
    UnicodeString *samr_names = NULL;
    Ids r = {0};
    Ids t = {0};
    uint32 *out_rids = NULL;
    uint32 *out_types = NULL;

    goto_if_invalid_param_ntstatus(b, cleanup);
    goto_if_invalid_param_ntstatus(handle, cleanup);
    goto_if_invalid_param_ntstatus(names, cleanup);
    goto_if_invalid_param_ntstatus(rids, cleanup);
    goto_if_invalid_param_ntstatus(types, cleanup);

    samr_names = InitUnicodeStringArray(names, num_names);
    goto_if_no_memory_ntstatus(samr_names, error);

    TRY
    {
        status = _SamrLookupNames(b, handle, num_names, samr_names, &r, &t);
    }
    CATCH_ALL
    {
        status = STATUS_UNHANDLED_EXCEPTION;
    }
    ENDTRY;

    goto_if_ntstatus_not_success(status, error);

    if (r.count != t.count)
    {
        status = STATUS_REPLY_MESSAGE_MISMATCH;
        goto error;
    }

    if (rids_count != NULL)
    {
        *rids_count = r.count;
    }
    else if(r.count != num_names)
    {
        status = STATUS_REPLY_MESSAGE_MISMATCH;
        goto error;
    }

    status = SamrAllocateIds(&out_rids, &r);
    goto_if_ntstatus_not_success(status, error);

    status = SamrAllocateIds(&out_types, &t);
    goto_if_ntstatus_not_success(status, error);

    *rids  = out_rids;
    *types = out_types;

cleanup:
    SamrCleanStubIds(&r);
    SamrCleanStubIds(&t);

    FreeUnicodeStringArray(samr_names, num_names);

    return status;

error:
    if (out_rids) {
        SamrFreeMemory((void*)out_rids);
    }

    if (out_types) {
        SamrFreeMemory((void*)out_types);
    }

    *rids  = NULL;
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
