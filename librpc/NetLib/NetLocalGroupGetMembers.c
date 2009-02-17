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


NET_API_STATUS NetLocalGroupGetMembers(const wchar16_t *hostname,
				       const wchar16_t *aliasname,
				       uint32 level, void **bufptr,
				       uint32 prefmaxlen, uint32 *entries,
				       uint32 *total, uint32 *resume)
{
    const uint32 lsa_access = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    const uint32 alias_access = ALIAS_ACCESS_GET_MEMBERS;
    const uint16 lookup_level = 1;

    NTSTATUS status;
    NetConn *conn;
    handle_t samr_bind, lsa_bind;
    PolicyHandle domain_handle, btin_domain_handle;
    PolicyHandle alias_handle;
    DomSid **sids;
    uint32 alias_rid;
    uint32 i, num_sids, count;
    LOCALGROUP_MEMBERS_INFO_3 *info;
    PolicyHandle lsa_policy;
    SidArray sid_array;
    RefDomainList *domains = NULL;
    TranslatedName *names = NULL;
    PIO_ACCESS_TOKEN access_token = NULL;

    if (hostname == NULL || aliasname == NULL || bufptr == NULL) {
	return NtStatusToWin32Error(STATUS_INVALID_PARAMETER);
    }

    *total   = 0;
    *resume  = 0;
    num_sids = 0;

    status = LwIoGetThreadAccessToken(&access_token);
    BAIL_ON_NT_STATUS(status);

    status = NetConnectSamr(&conn, hostname, 0, 0, access_token);
    BAIL_ON_NT_STATUS(status);

    samr_bind          = conn->samr.bind;
    domain_handle      = conn->samr.dom_handle;
    btin_domain_handle = conn->samr.btin_dom_handle;

    status = NetOpenAlias(conn, aliasname, alias_access, &alias_handle,
			  &alias_rid);
    if (status == STATUS_NONE_MAPPED) {
	/* No such alias in host's domain.
	   Try to look in builtin domain. */
	status = NetOpenAlias(conn, aliasname, alias_access,
			      &alias_handle, &alias_rid);
	BAIL_ON_NT_STATUS(status);

    } else if (status != 0) {
	return NtStatusToWin32Error(status);
    }

    status = SamrGetMembersInAlias(samr_bind, &alias_handle, &sids, &num_sids);
    BAIL_ON_NT_STATUS(status);

    *total += num_sids;
    *entries = *total;
    while ((*entries) * sizeof(LOCALGROUP_MEMBERS_INFO_3) > prefmaxlen) (*entries)--;

    info = (LOCALGROUP_MEMBERS_INFO_3*) malloc(sizeof(LOCALGROUP_MEMBERS_INFO_3) *
					       (*entries));

    status = NetConnectLsa(&conn, hostname, lsa_access, access_token);
    BAIL_ON_NT_STATUS(status);

    lsa_bind   = conn->lsa.bind;
    lsa_policy = conn->lsa.policy_handle;

    sid_array.num_sids = num_sids;
    sid_array.sids = (SidPtr*) malloc(sid_array.num_sids
				      * sizeof(SidPtr));
    for (i = 0; i < num_sids; i++) {
	sid_array.sids[i].sid = sids[i];
    }

    status = LsaLookupSids(lsa_bind, &lsa_policy, &sid_array, &domains,
			   &names, lookup_level, &count);
    if (status != STATUS_SUCCESS &&
	status != STATUS_SOME_UNMAPPED) {
	return NtStatusToWin32Error(status);
    }

    free(sid_array.sids);

    for (i = 0; i < count; i++) {
	size_t domainname_size, username_size, name_size;
	uint32 sid_index;
	wchar16_t *domainname, *username, *can_username;

	sid_index = names[i].sid_index;

	if (domains->domains[sid_index].name.size > 0) {
	    domainname = domains->domains[sid_index].name.string;
	    domainname[domains->domains[sid_index].name.len / 2] = 0;

	} else {
	    domainname = NULL;
	}

	if (names[i].name.size > 0) {
	    username = names[i].name.string;
	    username[names[i].name.len / 2] = 0;

	} else {
	    username = NULL;
	}		
		
	domainname_size = (domainname) ? wc16slen(domainname) : 0;
	username_size = (username) ? wc16slen(username) : 0;
	/* include name termination and '\' separator */
	name_size = domainname_size + username_size + 4;

	can_username = (wchar16_t*) malloc(name_size * sizeof(wchar16_t));
	if (can_username == NULL) {
	    return NtStatusToWin32Error(STATUS_NO_MEMORY);
	}

	/* format name depending on what's available */
	if (domainname && username) {
	    /* fully resolvable account name */
	    sw16printf(can_username, "%S\\%S", domainname, username);

	} else if (domainname && !username) {
	    /* common case when account has been deleted but its SID
	       membership remains with the alias */
	    sw16printf(can_username, "%S\\", domainname);

	} else if (!domainname && username) {
	    /* this one shouldn't be possible */
	    sw16printf(can_username, "\\%S", username);

	} else {
	    /* total unknown */
	    sw16printf(can_username, "\\");
	}

	info[i].lgrmi3_domainandname = can_username;
    }

    *bufptr = (void*)info;

    status = SamrClose(samr_bind, &alias_handle);
    BAIL_ON_NT_STATUS(status);

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
