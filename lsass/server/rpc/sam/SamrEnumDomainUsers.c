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
SamrEnumDomainUsers(
    handle_t b,
    PolicyHandle *domain_h,
    uint32 *resume,
    uint32 account_flags,
    uint32 max_size,
    wchar16_t ***names,
    uint32 **rids,
    uint32 *count
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS ret_status = STATUS_SUCCESS;
    uint32 num = 0;
    uint32 r = 0;
    RidNameArray *entries = NULL;
    wchar16_t **out_names = NULL;
    uint32 *out_rids = NULL;

    goto_if_invalid_param_ntstatus(b, cleanup);
    goto_if_invalid_param_ntstatus(domain_h, cleanup);
    goto_if_invalid_param_ntstatus(resume, cleanup);
    goto_if_invalid_param_ntstatus(names, cleanup);
    goto_if_invalid_param_ntstatus(rids, cleanup);
    goto_if_invalid_param_ntstatus(count, cleanup);

    r = *resume;

    DCERPC_CALL(_SamrEnumDomainUsers(b, domain_h, &r, account_flags,
                                     max_size, &entries, &num));

    /* Preserve returned status code */
    ret_status = status;

    /* Status other than success doesn't have to mean failure here */
    if (ret_status != STATUS_SUCCESS &&
        ret_status != STATUS_MORE_ENTRIES) goto error;

    if (entries) {
        status = SamrAllocateNamesAndRids(&out_names, &out_rids, entries);
        goto_if_ntstatus_not_success(status, error);
    }

    *resume = r;
    *count  = num;
    *names  = out_names;
    *rids   = out_rids;

cleanup:
    if (entries) {
        SamrFreeStubRidNameArray(entries);
    }

    if (status == STATUS_SUCCESS &&
        (ret_status == STATUS_SUCCESS ||
         ret_status == STATUS_MORE_ENTRIES)) {
        status = ret_status;
    }

    return status;

error:
    if (out_names) {
        SamrFreeMemory((void*)out_names);
    }

    if (out_rids) {
        SamrFreeMemory((void*)out_rids);
    }

    *resume = 0;
    *count  = 0;
    *names  = NULL;
    *rids   = NULL;
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
