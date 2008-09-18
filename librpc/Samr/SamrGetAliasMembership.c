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


NTSTATUS SamrGetAliasMembership(handle_t b, PolicyHandle *domain_handle,
                                DomSid *sids, uint32 num_sids, uint32 **rids,
                                uint32 *count)
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32 i = 0;
    SidArray s = {0};
    Ids r = {0};
    uint32 *out_rids = NULL;

    goto_if_invalid_param_ntstatus(b, cleanup);
    goto_if_invalid_param_ntstatus(domain_handle, cleanup);
    goto_if_invalid_param_ntstatus(sids, cleanup);
    goto_if_invalid_param_ntstatus(rids, cleanup);
    goto_if_invalid_param_ntstatus(count, cleanup);

    s.num_sids = num_sids;
    status = SamrAllocateMemory((void**)&s.sids,
                                sizeof(SidPtr) * num_sids,
                                NULL);
    goto_if_ntstatus_not_success(status, error);

    for (i = 0; i < num_sids; i++) {
        s.sids[i].sid = &(sids[i]);
    }

    TRY
    {
        status = _SamrGetAliasMembership(b, domain_handle, &s, &r);
    }
    CATCH_ALL
    {
        status = STATUS_UNHANDLED_EXCEPTION;
    }
    ENDTRY;

    goto_if_ntstatus_not_success(status, error);

    status = SamrAllocateIds(&out_rids, &r);
    goto_if_ntstatus_not_success(status, error);

    *rids  = out_rids;
    *count = r.count;

cleanup:
    SamrCleanStubIds(&r);

    return status;

error:
    *rids  = NULL;
    *count = 0;

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
