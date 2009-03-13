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
NetLocalGroupChangeMembers(
    const wchar16_t *hostname,
    const wchar16_t *aliasname,
    uint32 level,
    void *buffer,
    uint32 entries,
    uint32 access)
{
    const uint32 lsa_access = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    const uint32 btin_domain_access = DOMAIN_ACCESS_OPEN_ACCOUNT;
    const uint16 lookup_level = LSA_LOOKUP_NAMES_ALL;
    const uint32 num_names = 1;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    uint32 access_rights = 0;
    NetConn *conn = NULL;
    handle_t samr_b = NULL;
    handle_t lsa_b = NULL;
    wchar16_t *member = NULL;
    size_t member_len = 0;
    PolicyHandle alias_h;
    PSID user_sid = NULL;
    PSID dom_sid = NULL;
    uint32 alias_rid = 0;
    uint32 i = 0;
    LOCALGROUP_MEMBERS_INFO_0 *info0 = NULL;
    LOCALGROUP_MEMBERS_INFO_3 *info3 = NULL;
    PolicyHandle lsa_h;
    wchar16_t *names[1] = {NULL};
    uint32 count = 0;
    uint32 sid_index = 0;
    TranslatedSid *sids = NULL;
    RefDomainList *domains = NULL;
    PIO_ACCESS_TOKEN access_token = NULL;

    goto_if_invalid_param_winerr(hostname, cleanup);
    goto_if_invalid_param_winerr(aliasname, cleanup);
    goto_if_invalid_param_winerr(buffer, cleanup);

    if (!(level == 0 || level == 3)) {
        err = ERROR_INVALID_LEVEL;
        goto cleanup;
    }

    access_rights = access;

    status = LwIoGetThreadAccessToken(&access_token);
    BAIL_ON_NT_STATUS(status);

    status = NetConnectSamr(&conn, hostname, access, btin_domain_access, access_token);
    goto_if_ntstatus_not_success(status, error);

    samr_b = conn->samr.bind;

    status = NetOpenAlias(conn, aliasname, access_rights, &alias_h,
                          &alias_rid);
    if (status == STATUS_NONE_MAPPED) {
        /* No such alias in host's domain.
           Try to look in builtin domain. */
        status = NetOpenAlias(conn, aliasname, access_rights,
                              &alias_h, &alias_rid);
        goto_if_ntstatus_not_success(status, error);
	
    } else if (status != STATUS_SUCCESS) {
        goto error;
    }

    status = NetConnectLsa(&conn, hostname, lsa_access, access_token);
    goto_if_ntstatus_not_success(status, error);

    lsa_b = conn->lsa.bind;
    lsa_h = conn->lsa.policy_handle;

    for (i = 0; i < entries; i++) {
        if (level == 3) {
            info3 = (LOCALGROUP_MEMBERS_INFO_3*)buffer;

            member = info3[i].lgrmi3_domainandname;
            if (member == NULL) {
                err = ERROR_INVALID_PARAMETER;
                goto error;
            }

            member_len = wc16slen(member);
            if (member_len == 0) {
                err = ERROR_INVALID_PARAMETER;
                goto error;
            }

            names[0] = member;
            count    = 0;

            status = LsaLookupNames(lsa_b, &lsa_h, num_names, names,
                                    &domains, &sids, lookup_level, &count);
            goto_if_ntstatus_not_success(status, error);

            if (count == 0) continue;

            user_sid  = NULL;
            dom_sid   = NULL;
            sid_index = sids[0].index;

            if (sid_index < domains->count) {
                dom_sid = domains->domains[sid_index].sid;
                status = MsRpcAllocateSidAppendRid(&user_sid,
                                                    dom_sid,
                                                    sids[0].rid);
                goto_if_ntstatus_not_success(status, error);

            }

            if (domains) {
                SamrFreeMemory((void*)domains);
            }

            if (sids) {
                SamrFreeMemory((void*)sids);
            }

        } else if (level == 0) {
            info0 = (LOCALGROUP_MEMBERS_INFO_0*)buffer;

            MsRpcDuplicateSid(&user_sid, info0[i].lgrmi0_sid);
            goto_if_no_memory_ntstatus(user_sid, error);
        }

        if (access_rights == ALIAS_ACCESS_ADD_MEMBER) {
            status = SamrAddAliasMember(samr_b, &alias_h, user_sid);
            goto_if_ntstatus_not_success(status, error);

        } else if (access_rights == ALIAS_ACCESS_REMOVE_MEMBER) {
            status = SamrDeleteAliasMember(samr_b, &alias_h, user_sid);
            goto_if_ntstatus_not_success(status, error);
        }

        if (user_sid) {
            MsRpcFreeSid(user_sid);
        }
    }

    status = SamrClose(samr_b, &alias_h);
    goto_if_ntstatus_not_success(status, error);

cleanup:
    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
    }

    return err;

error:
    if (access_token)
    {
        LwIoDeleteAccessToken(access_token);
    }

    goto cleanup;
}


NET_API_STATUS NetLocalGroupAddMembers(const wchar16_t *hostname,
				       const wchar16_t *aliasname,
				       uint32 level, void *buffer,
				       uint32 entries)
{
    return  NetLocalGroupChangeMembers(hostname, aliasname, level,
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
