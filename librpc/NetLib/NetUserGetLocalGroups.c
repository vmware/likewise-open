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


NET_API_STATUS NetUserGetLocalGroups(const wchar16_t *hostname,
				     const wchar16_t *username,
				     uint32 level, uint32 flags, void **bufptr,
				     uint32 prefmaxlen, uint32 *entries,
				     uint32 *total)
{
    const uint32 builtin_dom_access = DOMAIN_ACCESS_OPEN_ACCOUNT |
                                      DOMAIN_ACCESS_ENUM_ACCOUNTS;
    const uint32 user_access = USER_ACCESS_GET_GROUP_MEMBERSHIP;
	
    NTSTATUS status = STATUS_SUCCESS;
    NetConn *conn;
    handle_t samr_bind;
    PolicyHandle domain_handle, btin_domain_handle;
    PolicyHandle user_handle;
    PSID domain_sid;
    PSID user_sid;
    uint32 user_rid, i;
    SidPtr sid_ptr;
    SidArray sids;
    uint32 *user_rids, *btin_user_rids, rids_count, btin_rids_count;
    wchar16_t **alias_names, **btin_alias_names;
    uint32 *alias_types, *btin_alias_types;
    LOCALGROUP_USERS_INFO_0 *info;
    PIO_ACCESS_TOKEN access_token = NULL;

    if (username == NULL || bufptr == NULL) {
        return NtStatusToWin32Error(STATUS_NO_MEMORY);
    }

    status = LwIoGetThreadAccessToken(&access_token);
    BAIL_ON_NT_STATUS(status);

    status = NetConnectSamr(&conn, hostname, 0, builtin_dom_access, access_token);
    if (status != 0) return NtStatusToWin32Error(status);
    
    samr_bind          = conn->samr.bind;
    domain_handle      = conn->samr.dom_handle;
    btin_domain_handle = conn->samr.btin_dom_handle;
    domain_sid         = conn->samr.dom_sid;

    status = NetOpenUser(conn, username, user_access, &user_handle, &user_rid);
    if (status != 0) return NtStatusToWin32Error(status);

    status = MsRpcAllocateSidAppendRid(&user_sid, domain_sid, user_rid);
    if (status != 0) return NtStatusToWin32Error(status);

    sids.num_sids = 1;
    sid_ptr.sid = user_sid;
    sids.sids = &sid_ptr;

    status = SamrGetAliasMembership(samr_bind, &domain_handle, user_sid, 1,
				    &user_rids, &rids_count);
    if (status != 0) return NtStatusToWin32Error(status);

    status = SamrGetAliasMembership(samr_bind, &btin_domain_handle, user_sid, 1,
				    &btin_user_rids, &btin_rids_count);
    if (status != 0) return NtStatusToWin32Error(status);

    if (rids_count > 0) {
	status = SamrLookupRids(samr_bind, &domain_handle, rids_count,
				user_rids, &alias_names, &alias_types);
	if (status != 0) return NtStatusToWin32Error(status);
    }

    if (btin_rids_count > 0) {
	status = SamrLookupRids(samr_bind, &btin_domain_handle,
				btin_rids_count, btin_user_rids,
				&btin_alias_names, &btin_alias_types);
	if (status != 0) return NtStatusToWin32Error(status);
    }

    status = SamrClose(samr_bind, &user_handle);
    if (status != 0) return NtStatusToWin32Error(status);

    *total = rids_count + btin_rids_count;

    if ((*total) * sizeof(LOCALGROUP_USERS_INFO_0) > prefmaxlen) {
	status = STATUS_MORE_ENTRIES;
    } else {
	status = STATUS_SUCCESS;
    }

    *entries = *total;
    while ((*entries) * sizeof(LOCALGROUP_USERS_INFO_0) > prefmaxlen) (*entries)--;

    /* Allocate memory only when entries number is non-zero, otherwise set info
       buffer to NULL. This is because allocating zero bytes of memory still returns
       a valid pointer */
    if (*entries) {
	info = (LOCALGROUP_USERS_INFO_0*) malloc(sizeof(LOCALGROUP_USERS_INFO_0) *
						 *entries);

	for (i = 0; i < *entries && i < rids_count; i++) {
	    info[i].lgrui0_name = wc16sdup(alias_names[i]);
	}

	/* continue copying from where previous loop has finished */
	for (i = rids_count; i < *entries; i++) {
	    info[i].lgrui0_name = 
		wc16sdup(btin_alias_names[i - rids_count]);
	}

    } else {
	info = NULL;
    }

    *bufptr = (void*)info;

error:

    if (access_token)
    {
        LwIoDeleteAccessToken(access_token);
    }

    return NtStatusToWin32Error(status);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
