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


NET_API_STATUS NetLocalGroupEnum(const wchar16_t *hostname, uint32 level,
				 void **bufptr, uint32 prefmaxlen,
				 uint32 *entries, uint32 *total, uint32 *resume)
{
    const uint32 account_flags = 0;
    const uint32 alias_access = ALIAS_ACCESS_LOOKUP_INFO;
    const uint16 dominfo_level = 2;
	
    NTSTATUS status, ret;
    NetConn *conn;
    handle_t samr_bind;
    PolicyHandle domain_handle, btin_domain_handle;
    PolicyHandle alias_handle;
    DomainInfo *dominfo;
    AliasInfo *aliasinfo;
    wchar16_t **names;
    uint32 res, *rids, num_entries, num_btin_aliases, num_dom_aliases;
    uint32 i, info_idx, res_idx, count;
    LOCALGROUP_INFO_1 *info;

    if (hostname == NULL || bufptr == NULL || entries == NULL ||
	total == NULL || resume == NULL) {
	return NtStatusToWin32Error(STATUS_INVALID_PARAMETER);
    }

    *entries = 0;
    *total   = 0;

    status = NetConnectSamr(&conn, hostname, 0, 0);
    if (status != 0) return NtStatusToWin32Error(status);

    samr_bind          = conn->samr.bind;
    domain_handle      = conn->samr.dom_handle;
    btin_domain_handle = conn->samr.btin_dom_handle;

    status = SamrQueryDomainInfo(samr_bind, &domain_handle, dominfo_level,
				 &dominfo);
    if (status != 0) return NtStatusToWin32Error(status);

    num_dom_aliases = dominfo->info2.num_aliases;

    status = SamrQueryDomainInfo(samr_bind, &btin_domain_handle,
				 dominfo_level, &dominfo);
    if (status != 0) return NtStatusToWin32Error(status);

    num_btin_aliases = dominfo->info2.num_aliases;

    *total   = num_dom_aliases + num_btin_aliases;
    *entries = *total;
    while ((*entries) * sizeof(LOCALGROUP_INFO_1) > prefmaxlen) (*entries)--;

    if (*entries == 0) {
	return NtStatusToWin32Error(STATUS_INVALID_PARAMETER); /* this should be insufficient space */
    }
	
    info = (LOCALGROUP_INFO_1*) malloc(sizeof(LOCALGROUP_INFO_1) *
				       (*entries));
    info_idx = 0;

    if (*resume < num_dom_aliases) {
	res = *resume;
	goto enum_domain;

    } else /* *resume >= num_dom_aliases */ {
	res = *resume - num_dom_aliases;
	goto enum_btin_domain;
    }

enum_domain:
    do {
	status = SamrEnumDomainAliases(samr_bind, &domain_handle, &res,
				       account_flags, &names,
				       &rids, &num_entries);
	for (i = 0; i < num_entries; i++) {
	    NTSTATUS group_status;
	    info[info_idx].lgrpi1_name = wc16sdup(names[i]);

	    group_status = SamrOpenAlias(samr_bind, &domain_handle,
					 alias_access, rids[i],
					 &alias_handle);
	    if (group_status == 0) {
		group_status = SamrQueryAliasInfo(samr_bind, &alias_handle,
						  ALIAS_INFO_DESCRIPTION,
						  &aliasinfo);
		if (group_status == 0 &&
		    aliasinfo->description.len > 0) {
		    UnicodeString *desc = &aliasinfo->description;
		    info[info_idx].lgrpi1_comment = wc16sndup(desc->string,
							      desc->len/2);
		} else {
		    info[info_idx].lgrpi1_comment = 0;
		}

		SamrClose(samr_bind, &alias_handle);
	    }

	    info_idx++;
	    (*resume)++;
	}

    } while (status == STATUS_MORE_ENTRIES && info_idx < (*entries));

    res = 0;

enum_btin_domain:
    do {
	status = SamrEnumDomainAliases(samr_bind, &btin_domain_handle,
				       &res, account_flags, &names,
				       &rids, &num_entries);
	for (i = 0; i < num_entries; i++) {
	    NTSTATUS group_status;
	    info[info_idx].lgrpi1_name = wc16sdup(names[i]);

	    group_status = SamrOpenAlias(samr_bind, &btin_domain_handle,
					 alias_access, rids[i], &alias_handle);
	    if (group_status == 0) {
		group_status = SamrQueryAliasInfo(samr_bind, &alias_handle,
						  ALIAS_INFO_DESCRIPTION,
						  &aliasinfo);
		if (group_status == 0 &&
		    aliasinfo->description.len > 0) {
		    UnicodeString *desc = &aliasinfo->description;
		    info[info_idx].lgrpi1_comment = wc16sndup(desc->string,
							      desc->len/2);
		} else {
		    info[info_idx].lgrpi1_comment = 0;
		}

		SamrClose(samr_bind, &alias_handle);
	    }

	    info_idx++;
	    (*resume)++;
	}

    } while (status == STATUS_MORE_ENTRIES && info_idx < (*entries));

    if ((*total) > (*resume)) {
	ret = NtStatusToWin32Error(STATUS_MORE_ENTRIES);
    } else {
	ret = NtStatusToWin32Error(STATUS_SUCCESS);
    }

    *bufptr = (void*)info;
    *entries = info_idx;

    return STATUS_SUCCESS;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
