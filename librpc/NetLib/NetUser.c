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
NetOpenUser(
    NetConn *conn,
    const wchar16_t *username,
    uint32 access_mask,
    PolicyHandle *user_h,
    uint32 *rid
    )
{
    const uint32 num_users = 1;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    handle_t samr_b = NULL;
    PolicyHandle domain_h = {0};
    wchar16_t *usernames[1] = {0};
    uint32 *rids = NULL;
    uint32 *types = NULL;

    BAIL_ON_INVALID_PTR(conn);
    BAIL_ON_INVALID_PTR(username);
    BAIL_ON_INVALID_PTR(user_h);
    BAIL_ON_INVALID_PTR(rid);

    samr_b   = conn->samr.bind;
    domain_h = conn->samr.dom_handle;

    usernames[0] = wc16sdup(username);
    BAIL_ON_NO_MEMORY(usernames[0]);
	
    status = SamrLookupNames(samr_b, &domain_h, num_users, usernames,
                             &rids, &types, NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrOpenUser(samr_b, &domain_h, access_mask, rids[0],
                          user_h);
    BAIL_ON_NTSTATUS_ERROR(status);

    *rid = rids[0];

cleanup:
    if (rids) {
        SamrFreeMemory((void*)rids);
    }

    if (types) {
        SamrFreeMemory((void*)types);
    }

    SAFE_FREE(usernames[0]);

    return status;

error:
    *rid = 0;
    goto cleanup;
}


NTSTATUS
NetOpenAlias(
    NetConn *conn,
    const wchar16_t *aliasname,
    uint32 access_mask,
    PolicyHandle *out_alias_h,
    uint32 *out_rid
    )
{
    const uint32 num_aliases = 1;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    handle_t samr_b = NULL;
    PolicyHandle domains_h[2], domain_h, alias_h;
    wchar16_t *aliasnames[1] = {0};
    uint32 *rids = NULL;
    uint32 *types = NULL;
    uint32 alias_rid = 0;
    int i = 0;

    BAIL_ON_INVALID_PTR(conn);
    BAIL_ON_INVALID_PTR(aliasname);
    BAIL_ON_INVALID_PTR(out_alias_h);
    BAIL_ON_INVALID_PTR(out_rid);

    samr_b        = conn->samr.bind;
    domains_h[0]  = conn->samr.dom_handle;
    domains_h[1]  = conn->samr.btin_dom_handle;

    aliasnames[0] = wc16sdup(aliasname);
    BAIL_ON_NO_MEMORY(aliasnames[0]);

    /*
     * Try to look for alias in host domain first, then in builtin
     */
    for (i = 0; i < sizeof(domains_h)/sizeof(domains_h[0]); i++) {
        status = SamrLookupNames(samr_b, &domains_h[i], num_aliases, aliasnames,
                                 &rids, &types, NULL);

        if (status == STATUS_SUCCESS) {
            /* alias has been found in one of domain so pass the domain's
               handle further down */
            domain_h  = domains_h[i];
            alias_rid = rids[0];
            break;

        } else if (status == STATUS_NONE_MAPPED) {
            if (rids) {
                SamrFreeMemory((void*)rids);
                rids = NULL;
            }

            if (types) {
                SamrFreeMemory((void*)types);
                types = NULL;
            }

            continue;
        }

        /* Catch other possible errors */
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    /* Allow to open alias only if a valid one has been found */
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrOpenAlias(samr_b, &domain_h, access_mask, alias_rid,
                           &alias_h);
    BAIL_ON_NTSTATUS_ERROR(status);

    *out_rid     = alias_rid;
    *out_alias_h = alias_h;

cleanup:
    SAFE_FREE(aliasnames[0]);

    if (rids) {
        SamrFreeMemory((void*)rids);
    }

    if (types) {
        SamrFreeMemory((void*)types);
    }

    return status;

error:
    *out_rid = 0;
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
