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


NET_API_STATUS
NetUserGetLocalGroups(
    const wchar16_t *hostname,
    const wchar16_t *username,
    uint32 level,
    uint32 flags,
    void **bufptr,
    uint32 prefmaxlen,
    uint32 *out_entries,
    uint32 *out_total
    )
{
    const uint32 builtin_dom_access = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                      DOMAIN_ACCESS_ENUM_ACCOUNTS;
    const uint32 user_access = USER_ACCESS_GET_GROUP_MEMBERSHIP;
	
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    NetConn *conn = NULL;
    handle_t samr_b = NULL;
    PolicyHandle domain_h;
    PolicyHandle btin_domain_h;
    PolicyHandle user_h;
    PSID domain_sid = NULL;
    PSID user_sid = NULL;
    uint32 user_rid = 0;
    uint32 i = 0;
    SidPtr sid_ptr;
    SidArray sids;
    uint32 *user_rids = NULL;
    uint32 *btin_user_rids = NULL;
    uint32 rids_count = 0;
    uint32 btin_rids_count = 0;
    wchar16_t **alias_names = NULL;
    wchar16_t **btin_alias_names = NULL;
    uint32 *alias_types = NULL;
    uint32 *btin_alias_types = NULL;
    LOCALGROUP_USERS_INFO_0 *info = NULL;
    uint32 entries = 0;
    uint32 total = 0;
    wchar16_t *alias_name = NULL;
    PIO_ACCESS_TOKEN access_token = NULL;

    BAIL_ON_INVALID_PTR(username);
    BAIL_ON_INVALID_PTR(hostname);

    status = LwIoGetThreadAccessToken(&access_token);
    BAIL_ON_NT_STATUS(status);

    status = NetConnectSamr(&conn, hostname, 0, builtin_dom_access, access_token);
    BAIL_ON_NTSTATUS_ERROR(status);
    
    samr_b        = conn->samr.bind;
    domain_h      = conn->samr.dom_handle;
    btin_domain_h = conn->samr.btin_dom_handle;
    domain_sid    = conn->samr.dom_sid;

    status = NetOpenUser(conn, username, user_access, &user_h, &user_rid);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = MsRpcAllocateSidAppendRid(&user_sid, domain_sid, user_rid);
    if (status != 0) return NtStatusToWin32Error(status);

    sids.num_sids = 1;
    sid_ptr.sid = user_sid;
    sids.sids = &sid_ptr;

    status = SamrGetAliasMembership(samr_b, &domain_h, &user_sid, 1,
                                    &user_rids, &rids_count);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrGetAliasMembership(samr_b, &btin_domain_h, &user_sid, 1,
                                    &btin_user_rids, &btin_rids_count);
    BAIL_ON_NTSTATUS_ERROR(status);

    if (rids_count > 0) {
        status = SamrLookupRids(samr_b, &domain_h, rids_count,
                                user_rids, &alias_names, &alias_types);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    if (btin_rids_count > 0) {
        status = SamrLookupRids(samr_b, &btin_domain_h,
                                btin_rids_count, btin_user_rids,
                                &btin_alias_names, &btin_alias_types);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    status = SamrClose(samr_b, &user_h);
    BAIL_ON_NTSTATUS_ERROR(status);

    total = rids_count + btin_rids_count;

    if (total * sizeof(LOCALGROUP_USERS_INFO_0) > prefmaxlen) {
        status = STATUS_MORE_ENTRIES;
    } else {
        status = STATUS_SUCCESS;
    }
    BAIL_ON_NTSTATUS_ERROR(status);

    entries = total;
    while (entries * sizeof(LOCALGROUP_USERS_INFO_0) > prefmaxlen) {
        entries--;
    }

    /* Allocate memory only when entries number is non-zero, otherwise set info
       buffer to NULL. This is because allocating zero bytes of memory still returns
       a valid pointer */
    if (entries) {
        status = NetAllocateMemory((void**)&info,
                                   sizeof(LOCALGROUP_USERS_INFO_0) * entries,
                                   NULL);
        BAIL_ON_NTSTATUS_ERROR(status);

        for (i = 0; i < entries && i < rids_count; i++) {
            alias_name = wc16sdup(alias_names[i]);
            BAIL_ON_NO_MEMORY(alias_name);

            info[i].lgrui0_name = alias_name;

            status = NetAddDepMemory(alias_name, info);
            BAIL_ON_NTSTATUS_ERROR(status);

            alias_name = NULL;
        }

        /* continue copying from where previous loop has finished */
        for (i = rids_count; i < entries; i++) {
            alias_name = wc16sdup(btin_alias_names[i - rids_count]);
            BAIL_ON_NO_MEMORY(alias_name);

            info[i].lgrui0_name = alias_name;

            status = NetAddDepMemory(alias_name, info);
            BAIL_ON_NTSTATUS_ERROR(status);

            alias_name = NULL;
        }

    }

    *out_entries = entries;
    *out_total   = total;
    *bufptr = (void*)info;

cleanup:
    return err;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    if (access_token)
    {
        LwIoDeleteAccessToken(access_token);
    }

    *out_entries = 0;
    *out_total   = 0;
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
