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
DisableWksAccount(
    NetConn *conn,
    wchar16_t *machine,
    PolicyHandle *account_h
    )
{
	const uint32 user_access = USER_ACCESS_GET_ATTRIBUTES | 
                                   USER_ACCESS_SET_ATTRIBUTES;
	const uint32 acct_flags_level = 16;

	NTSTATUS status;
	handle_t samr_b;
	PolicyHandle *domain_h;
	wchar16_t *account_name, *names[1];
	UserInfo16 *info16;
	uint32 *rids, *types;
	UserInfo sinfo;
    UserInfo *qinfo = NULL;
    size_t account_name_cch = 0;

    memset((void*)&sinfo, 0, sizeof(sinfo));

	samr_b   = conn->samr.bind;
	domain_h = &conn->samr.dom_handle;
	info16   = &sinfo.info16;

	/* prepare account$ name */
    account_name_cch = wc16slen(machine) + 2;
	account_name = (wchar16_t*) malloc(sizeof(wchar16_t) *
                                       (account_name_cch));
	if (account_name == NULL) return STATUS_NO_MEMORY;
    if (sw16printfw(
                account_name,
                account_name_cch,
                L"%ws$",
                machine) < 0)
    {
        status = ErrnoToNtStatus(errno);
        goto done;
    }

	names[0] = account_name;
	status = SamrLookupNames(samr_b, domain_h, 1, names, &rids, &types, NULL);
	if (status != STATUS_SUCCESS) goto done;

	/* TODO: what should we actually do if the number of rids found
	   is greater than 1 ? */

	status = SamrOpenUser(samr_b, domain_h, user_access, rids[0], account_h);
	if (status != STATUS_SUCCESS) goto done;

	status = SamrQueryUserInfo(samr_b, account_h, acct_flags_level, &qinfo);
	if (status != STATUS_SUCCESS) goto done;

	/* set "account disabled" flag */
	info16->account_flags = qinfo->info16.account_flags;
	info16->account_flags |= ACB_DISABLED;

	status = SamrSetUserInfo(samr_b, account_h, acct_flags_level, &sinfo);

done:
    if (rids) SamrFreeMemory((void*)rids);
    if (types) SamrFreeMemory((void*)types);
    if (qinfo) SamrFreeMemory((void*)qinfo);
	SAFE_FREE(account_name);

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
