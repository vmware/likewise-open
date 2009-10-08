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
    ACCOUNT_HANDLE *phUser,
    uint32 *rid
    )
{
    const uint32 num_users = 1;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    handle_t samr_b = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    wchar16_t *usernames[1] = {0};
    uint32 *rids = NULL;
    uint32 *types = NULL;

    BAIL_ON_INVALID_PTR(conn);
    BAIL_ON_INVALID_PTR(username);
    BAIL_ON_INVALID_PTR(phUser);
    BAIL_ON_INVALID_PTR(rid);

    samr_b   = conn->samr.bind;
    hDomain  = conn->samr.hDomain;

    usernames[0] = wc16sdup(username);
    BAIL_ON_NO_MEMORY(usernames[0]);

    status = SamrLookupNames(samr_b, hDomain, num_users, usernames,
                             &rids, &types, NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrOpenUser(samr_b, hDomain, access_mask, rids[0],
                          &hUser);
    BAIL_ON_NTSTATUS_ERROR(status);

    *rid    = rids[0];
    *phUser = hUser;

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
    *rid    = 0;
    *phUser = NULL;

    goto cleanup;
}


NTSTATUS
NetOpenAlias(
    NetConn *conn,
    const wchar16_t *aliasname,
    uint32 access_mask,
    ACCOUNT_HANDLE *phAlias,
    uint32 *rid
    )
{
    const uint32 num_aliases = 1;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    handle_t samr_b = NULL;
    DOMAIN_HANDLE hDomains[2] = {0};
    DOMAIN_HANDLE hDomain = NULL;
    ACCOUNT_HANDLE hAlias = NULL;
    wchar16_t *aliasnames[1] = {0};
    uint32 *rids = NULL;
    uint32 *types = NULL;
    uint32 alias_rid = 0;
    int i = 0;

    BAIL_ON_INVALID_PTR(conn);
    BAIL_ON_INVALID_PTR(aliasname);
    BAIL_ON_INVALID_PTR(phAlias);
    BAIL_ON_INVALID_PTR(rid);

    samr_b       = conn->samr.bind;
    hDomains[0]  = conn->samr.hDomain;
    hDomains[1]  = conn->samr.hBtinDomain;

    aliasnames[0] = wc16sdup(aliasname);
    BAIL_ON_NO_MEMORY(aliasnames[0]);

    /*
     * Try to look for alias in host domain first, then in builtin
     */
    for (i = 0; i < sizeof(hDomains)/sizeof(hDomains[0]); i++)
    {
        status = SamrLookupNames(samr_b, hDomains[i], num_aliases, aliasnames,
                                 &rids, &types, NULL);

        if (status == STATUS_SUCCESS) {
            /* alias has been found in one of domain so pass the domain's
               handle further down */
            hDomain   = hDomains[i];
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

    status = SamrOpenAlias(samr_b, hDomain, access_mask, alias_rid,
                           &hAlias);
    BAIL_ON_NTSTATUS_ERROR(status);

    *rid     = alias_rid;
    *phAlias = hAlias;

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
    *rid     = 0;
    *phAlias = NULL;

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
