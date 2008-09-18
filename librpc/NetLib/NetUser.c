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


NTSTATUS NetOpenUser(NetConn *conn, const wchar16_t *username, uint32 access_mask,
                     PolicyHandle *user_handle, uint32 *rid)
{
    const uint32 num_users = 1;

    WINERR err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    handle_t samr_bind = NULL;
    PolicyHandle domain_handle = {0};
    wchar16_t *usernames[1] = {0};
    uint32 *rids = NULL;
    uint32 *types = NULL;

    if (conn == NULL || username == NULL ||
        user_handle == NULL || rid == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    samr_bind          = conn->samr.bind;
    domain_handle      = conn->samr.dom_handle;

    usernames[0] = wc16sdup(username);
    goto_if_no_memory_ntstatus(usernames[0], error);
	
    status = SamrLookupNames(samr_bind, &domain_handle, num_users, usernames,
                             &rids, &types, NULL);
    goto_if_ntstatus_not_success(status, error);

    status = SamrOpenUser(samr_bind, &domain_handle, access_mask, rids[0],
                          user_handle);
    goto_if_ntstatus_not_success(status, error);


    *rid = rids[0];

cleanup:
    if (rids) SamrFreeMemory((void*)rids);
    if (types) SamrFreeMemory((void*)types);
    SAFE_FREE(usernames[0]);

    return status;

error:
    *rid = 0;
    goto cleanup;
}


NTSTATUS NetOpenAlias(NetConn *conn, const wchar16_t *aliasname, uint32 access_mask,
		      PolicyHandle *alias_handle, uint32 *rid)
{
    const uint32 num_aliases = 1;

    NTSTATUS status;
    handle_t samr_bind;
    PolicyHandle domain_handle, btin_domain_handle, handle;
    wchar16_t *aliasnames[1];
    uint32 *rids, *types;

    if (conn == NULL || aliasname == NULL ||
        alias_handle == NULL || rid == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    samr_bind          = conn->samr.bind;
    domain_handle      = conn->samr.dom_handle;
    btin_domain_handle = conn->samr.btin_dom_handle;

    aliasnames[0] = wc16sdup(aliasname);
    if (aliasnames[0] == NULL) return STATUS_NO_MEMORY;

    status = SamrLookupNames(samr_bind, &domain_handle, num_aliases, aliasnames,
                             &rids, &types, NULL);
    if (status == STATUS_NONE_MAPPED) {
        status = SamrLookupNames(samr_bind, &btin_domain_handle, num_aliases,
                                 aliasnames, &rids, &types, NULL);
	if (status != 0) return status;

	/* user has been found in builtin domain so pass
	   its handle further down */
	handle = btin_domain_handle;
	
    } else if (status != 0) {
        return status;

    } else {
        /* user has been found in host's domain so pass
           its handle further down */
        handle = domain_handle;
    }


    status = SamrOpenAlias(samr_bind, &handle, access_mask, rids[0],
                           alias_handle);

    SAFE_FREE(aliasnames[0]);

    *rid = rids[0];
    SAFE_FREE(rids);
    SAFE_FREE(types);

    return status;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
