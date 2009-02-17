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
#include "NetConnection.h"


NET_API_STATUS NetLocalGroupChangeMembers(const wchar16_t *hostname,
					  const wchar16_t *aliasname,
					  uint32 level, void *buffer,
					  uint32 entries,
					  uint32 access)
{
    const uint32 lsa_access = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    const uint32 btin_domain_access = DOMAIN_ACCESS_OPEN_ACCOUNT;

    uint32 access_rights;
    NetConn *conn;
    handle_t samr_bind, lsa_bind;
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t *member;
    PolicyHandle alias_handle;
    DomSid *usr_sid;
    uint32 alias_rid, i;
    LOCALGROUP_MEMBERS_INFO_0 *info0 = NULL;
    LOCALGROUP_MEMBERS_INFO_3 *info3 = NULL;
    PolicyHandle lsa_policy;
    PIO_ACCESS_TOKEN access_token = NULL;

    if (hostname == NULL || aliasname == NULL || buffer == NULL) {
	return NtStatusToWin32Error(STATUS_INVALID_PARAMETER);
    }

    access_rights = access;

    switch (level) {
    case 0: info0 = (LOCALGROUP_MEMBERS_INFO_0*)buffer;
	break;
    case 3: info3 = (LOCALGROUP_MEMBERS_INFO_3*)buffer;
	break;
    default:
	return NtStatusToWin32Error(ERROR_INVALID_LEVEL);
    }

    status = LwIoGetThreadAccessToken(&access_token);
    BAIL_ON_NT_STATUS(status);

    status = NetConnectSamr(&conn, hostname, access, btin_domain_access, access_token);
    BAIL_ON_NT_STATUS(status);

    samr_bind          = conn->samr.bind;

    status = NetOpenAlias(conn, aliasname, access_rights, &alias_handle,
			  &alias_rid);
    if (status == STATUS_NONE_MAPPED) {
	/* No such alias in host's domain.
	   Try to look in builtin domain. */
	status = NetOpenAlias(conn, aliasname, access_rights,
			      &alias_handle, &alias_rid);
	if (status != 0) return status;
	
    } else if (status != 0) {
	return status;
    }

    status = NetConnectLsa(&conn, hostname, lsa_access, access_token);
    BAIL_ON_NT_STATUS(status);

    lsa_bind   = conn->lsa.bind;
    lsa_policy = conn->lsa.policy_handle;

    for (i = 0; i < entries; i++) {
	usr_sid = NULL;

	if (level == 3) {
	    const uint16 level = LSA_LOOKUP_NAMES_ALL;
	    const uint32 num_names = 1;

	    NTSTATUS lookup_status;
	    size_t member_len;
	    wchar16_t *names[1];
	    DomSid *dom_sid;
	    uint32 count, sid_index;
	    TranslatedSid *sids = NULL;
	    RefDomainList *domains = NULL;

	    member = info3[i].lgrmi3_domainandname;
	    member_len = wc16slen(member);

	    if (*member == 0 && wc16slen(member) == 0) {
		return NtStatusToWin32Error(STATUS_INVALID_PARAMETER);
	    }

	    names[0] = member;
	    count    = 0;

	    lookup_status = LsaLookupNames(lsa_bind, &lsa_policy, num_names,
					   names, &domains, &sids, level,
					   &count);
	    if (lookup_status != 0 || count == 0) continue;

	    usr_sid = NULL;
	    dom_sid = NULL;
	    sid_index = sids[0].index;

	    if (sid_index < domains->count) {
		dom_sid = domains->domains[sid_index].sid;
		lookup_status = RtlSidAllocateResizedCopy(&usr_sid,
                                                  dom_sid->subauth_count+1,
                                                  dom_sid);
		if (lookup_status != 0) continue;

		usr_sid->subauth[usr_sid->subauth_count-1] = sids[0].rid;

	    } else {
		continue;
	    }

	} else if (level == 0) {
	    usr_sid = info0[i].lgrmi0_sid;
	}

	if (access_rights == ALIAS_ACCESS_ADD_MEMBER) {
	    status = SamrAddAliasMember(samr_bind, &alias_handle, usr_sid);
	    if (status != 0) return status;

	} else if (access_rights == ALIAS_ACCESS_REMOVE_MEMBER) {
	    status = SamrDeleteAliasMember(samr_bind, &alias_handle, usr_sid);
	    if (status != 0) return status;
	}

	if (usr_sid) SidFree(usr_sid);
    }

    status = SamrClose(samr_bind, &alias_handle);
    if (status != 0) return status;

error:

    if (access_token)
    {
        LwIoDeleteAccessToken(access_token);
    }

    return NtStatusToWin32Error(status);
}


NET_API_STATUS NetLocalGroupAddMembers(const wchar16_t *hostname,
				       const wchar16_t *aliasname,
				       uint32 level, void *buffer,
				       uint32 entries)
{
    return NetLocalGroupChangeMembers(hostname, aliasname, level,
				      buffer, entries,
				      ALIAS_ACCESS_ADD_MEMBER);
}



NET_API_STATUS NetLocalGroupDelMembers(const wchar16_t *hostname,
				       const wchar16_t *aliasname,
				       uint32 level, void *buffer,
				       uint32 entries)
{
    return NetLocalGroupChangeMembers(hostname, aliasname, level,
				      buffer, entries,
				      ALIAS_ACCESS_REMOVE_MEMBER);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
