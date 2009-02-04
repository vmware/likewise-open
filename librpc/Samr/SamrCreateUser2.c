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
SamrCreateUser2(
    handle_t b,
    PolicyHandle *domain_h,
    wchar16_t *account_name,
    uint32 account_flags,
    uint32 account_mask,
    PolicyHandle *account_h,
    uint32 *out_access_granted,
    uint32 *out_rid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UnicodeStringEx acct_name = {0};
    uint32 access = 0;
    uint32 rid = 0;

    goto_if_invalid_param_ntstatus(b, cleanup);
    goto_if_invalid_param_ntstatus(domain_h, cleanup);
    goto_if_invalid_param_ntstatus(account_name, cleanup);
    goto_if_invalid_param_ntstatus(account_h, cleanup);
    goto_if_invalid_param_ntstatus(out_access_granted, cleanup);
    goto_if_invalid_param_ntstatus(out_rid, cleanup);

    status = InitUnicodeStringEx(&acct_name, account_name);
    goto_if_ntstatus_not_success(status, error);

    DCERPC_CALL(_SamrCreateUser2(b, domain_h, &acct_name,
                                 account_flags, account_mask,
                                 account_h, &access, &rid));

    goto_if_ntstatus_not_success(status, error);

    *out_access_granted = access;
    *out_rid            = rid;

cleanup:
    FreeUnicodeStringEx(&acct_name);

    return status;

error:
    *out_access_granted = 0;
    *out_rid            = 0;
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
