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
NetLocalGroupGetMembers(
const wchar16_t *hostname,
const wchar16_t *aliasname,
uint32 level,
void **buffer,
uint32 prefmaxlen,
uint32 *out_entries,
uint32 *out_total,
uint32 *out_resume
)
{
const uint32 lsa_access = LSA_ACCESS_LOOKUP_NAMES_SIDS;
const uint32 alias_access = ALIAS_ACCESS_GET_MEMBERS;
const uint16 lookup_level = 1;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    NetConn *conn = NULL;
    handle_t samr_b, lsa_b;
    PolicyHandle domain_h, btin_domain_h;
    PolicyHandle alias_h;
    PSID *sids = NULL;
    uint32 entries = 0;
    uint32 total = 0;
    uint32 resume = 0;
    uint32 alias_rid = 0;
    uint32 i = 0;
    uint32 num_sids = 0;
    uint32 count = 0;
    LOCALGROUP_MEMBERS_INFO_3 *info = NULL;
    PolicyHandle lsa_h;
    SidArray *sid_array = NULL;
    RefDomainList *domains = NULL;
    TranslatedName *names = NULL;
    size_t domainname_size = 0;
    size_t username_size = 0;
    size_t name_size = 0;
    uint32 sid_index = 0;
    wchar16_t *domainname = NULL;
    wchar16_t *username = NULL;
    wchar16_t *can_username = NULL;
    PIO_ACCESS_TOKEN access_token = NULL;

    goto_if_invalid_param_winerr(hostname, cleanup);
    goto_if_invalid_param_winerr(aliasname, cleanup);
    goto_if_invalid_param_winerr(buffer, cleanup);
    goto_if_invalid_param_winerr(out_entries, cleanup);
    goto_if_invalid_param_winerr(out_total, cleanup);
    goto_if_invalid_param_winerr(out_resume, cleanup);

    total    = 0;
    resume   = 0;
    num_sids = 0;

    status = LwIoGetThreadAccessToken(&access_token);
    goto_if_ntstatus_not_success(status, error);

    status = NetConnectSamr(&conn, hostname, 0, 0, access_token);
    goto_if_ntstatus_not_success(status, error);

    samr_b        = conn->samr.bind;
    domain_h      = conn->samr.dom_handle;
    btin_domain_h = conn->samr.btin_dom_handle;

    status = NetOpenAlias(conn, aliasname, alias_access, &alias_h,
                          &alias_rid);
    if (status == STATUS_NONE_MAPPED) {
        /* No such alias in host's domain.
           Try to look in builtin domain. */
        status = NetOpenAlias(conn, aliasname, alias_access,
                              &alias_h, &alias_rid);
        goto_if_ntstatus_not_success(status, error);

    } else if (status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
        goto error;
    }

    status = SamrGetMembersInAlias(samr_b, &alias_h, &sids, &num_sids);
    goto_if_ntstatus_not_success(status, error);

    total += num_sids;
    entries = total;
    while (entries * sizeof(LOCALGROUP_MEMBERS_INFO_3) > prefmaxlen) {
        entries--;
    }

    status = NetAllocateMemory((void**)&info,
                               sizeof(LOCALGROUP_MEMBERS_INFO_3) * entries,
                               NULL);
    goto_if_ntstatus_not_success(status, error);

    status = NetConnectLsa(&conn, hostname, lsa_access, access_token);
    goto_if_ntstatus_not_success(status, error);

    lsa_b = conn->lsa.bind;
    lsa_h = conn->lsa.policy_handle;

    status = NetAllocateMemory((void**)&sid_array, sizeof(SidArray), NULL);
    goto_if_ntstatus_not_success(status, error);

    sid_array->num_sids = num_sids;

    status = NetAllocateMemory((void**)&sid_array->sids,
                               sizeof(SidPtr) * sid_array->num_sids,
                               sid_array);
    goto_if_ntstatus_not_success(status, error);

    for (i = 0; i < sid_array->num_sids; i++) {
        sid_array->sids[i].sid = sids[i];
    }

    status = LsaLookupSids(lsa_b, &lsa_h, sid_array, &domains,
                           &names, lookup_level, &count);
    if (status != STATUS_SUCCESS &&
        status != STATUS_SOME_UNMAPPED) {
        /* bail out only if there's no mapping or any other error occurs */
        err = NtStatusToWin32Error(status);
        goto error;
    }

    for (i = 0; i < count; i++) {
        sid_index = names[i].sid_index;

        domainname = GetFromUnicodeStringEx(&domains->domains[sid_index].name);
        goto_if_no_memory_ntstatus(domainname, error);

        status = NetAddDepMemory(domainname, info);
        goto_if_ntstatus_not_success(status, error);

        username = GetFromUnicodeString(&names[i].name);
        goto_if_no_memory_ntstatus(username, error);

        status = NetAddDepMemory(username, info);
        goto_if_ntstatus_not_success(status, error);

        domainname_size = wc16slen(domainname);
        username_size = wc16slen(username);

        /* include name termination and '\' separator */
        name_size = domainname_size + username_size + 2;

        status = NetAllocateMemory((void**)&can_username,
                                   name_size * sizeof(wchar16_t),
                                   info);
        goto_if_ntstatus_not_success(status, error);

        sw16printf(can_username, "%S\\%S", domainname, username);
        info[i].lgrmi3_domainandname = can_username;

        domainname_size = 0;
        username_size   = 0;
        name_size       = 0;
        sid_index       = 0;
        domainname      = NULL;
        username        = NULL;
        can_username    = NULL;
    }

    status = SamrClose(samr_b, &alias_h);
    goto_if_ntstatus_not_success(status, error);

    *buffer      = (void*)info;
    *out_total   = total;
    *out_entries = entries;

cleanup:
    if (sid_array) {
        NetFreeMemory((void*)sid_array);
    }

    if (sids) {
        SamrFreeMemory((void*)sids);
    }

    if (names) {
        SamrFreeMemory((void*)names);
    }

    if (domains) {
        SamrFreeMemory((void*)domains);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
    }

    return err;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    if (access_token)
    {
        LwIoDeleteAccessToken(access_token);
    }

    *buffer = NULL;
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
