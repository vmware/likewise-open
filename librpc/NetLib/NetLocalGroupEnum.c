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
NetLocalGroupEnum(
    const wchar16_t *hostname,
    uint32 level,
    void **buffer,
    uint32 prefmaxlen,
    uint32 *out_entries,
    uint32 *out_total,
    uint32 *out_resume
    )
{
    const uint32 account_flags = 0;
    const uint32 alias_access = ALIAS_ACCESS_LOOKUP_INFO;
    const uint16 dominfo_level = 2;
	
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    NetConn *conn = NULL;
    handle_t samr_b = NULL;
    PolicyHandle domain_h, btin_domain_h;
    PolicyHandle alias_h;
    DomainInfo *dominfo = NULL;
    DomainInfo *btin_dominfo = NULL;
    AliasInfo *aliasinfo = NULL;
    wchar16_t **names = NULL;
    uint32 entries = 0;
    uint32 res = 0;
    uint32 total = 0;
    uint32 *rids = NULL;
    uint32 num_entries = 0;
    uint32 num_btin_aliases = 0;
    uint32 num_dom_aliases = 0;
    uint32 i = 0;
    uint32 info_idx = 0;
    uint32 res_idx = 0;
    LOCALGROUP_INFO_1 *info = NULL;
    wchar16_t *grp_name = NULL;
    wchar16_t *grp_desc = NULL;
    UnicodeString *desc = NULL;
    PIO_ACCESS_TOKEN access_token = NULL;

    goto_if_invalid_param_winerr(hostname, cleanup);
    goto_if_invalid_param_winerr(buffer, cleanup);
    goto_if_invalid_param_winerr(out_entries, cleanup);
    goto_if_invalid_param_winerr(out_total, cleanup);
    goto_if_invalid_param_winerr(out_resume, cleanup);

    status = LwIoGetThreadAccessToken(&access_token);
    goto_if_ntstatus_not_success(status, error);

    status = NetConnectSamr(&conn, hostname, 0, 0, access_token);
    goto_if_ntstatus_not_success(status, error);

    samr_b         = conn->samr.bind;
    domain_h       = conn->samr.dom_handle;
    btin_domain_h  = conn->samr.btin_dom_handle;

    num_dom_aliases   = 0;
    num_btin_aliases  = 0;

    if (prefmaxlen == MAX_PREFERRED_LENGTH) {
        status = SamrQueryDomainInfo(samr_b, &domain_h, dominfo_level,
                                     &dominfo);
        goto_if_ntstatus_not_success(status, error);

        entries += dominfo->info2.num_aliases;

        status = SamrQueryDomainInfo(samr_b, &btin_domain_h,
                                     dominfo_level, &btin_dominfo);
        goto_if_ntstatus_not_success(status, error);

        entries += dominfo->info2.num_aliases;

    } else {
        while (entries * sizeof(LOCALGROUP_INFO_1) <= prefmaxlen) {
            entries++;
        }
        entries--;
    }

    if (entries == 0) {
        status = NtStatusToWin32Error(STATUS_INVALID_BUFFER_SIZE);
        goto error;
    }
	
    status = NetAllocateMemory((void**)&info,
                               sizeof(LOCALGROUP_INFO_1) * entries,
                               NULL);
    goto_if_ntstatus_not_success(status, error);

    info_idx = 0;
    res_idx  = *out_resume;
    res      = 0;

    do {
        status = SamrEnumDomainAliases(samr_b, &domain_h, &res,
                                       account_flags, &names,
                                       &rids, &num_entries);
        if (status != 0 &&
            status != STATUS_MORE_ENTRIES) {
            err = NtStatusToWin32Error(status);
            goto error;
        }

        for (i = 0; i < num_entries; i++) {
            if (i + num_dom_aliases < res_idx ||
                info_idx >= entries) {
                continue;
            }

            grp_name = wc16sdup(names[i]);
            goto_if_no_memory_ntstatus(grp_name, error);

            info[info_idx].lgrpi1_name = grp_name;
            status = NetAddDepMemory(grp_name, info);
            goto_if_ntstatus_not_success(status, error);

            status = SamrOpenAlias(samr_b, &domain_h,
                                   alias_access, rids[i],
                                   &alias_h);
            goto_if_ntstatus_not_success(status, error);

            status = SamrQueryAliasInfo(samr_b, &alias_h,
                                        ALIAS_INFO_DESCRIPTION,
                                        &aliasinfo);
            goto_if_ntstatus_not_success(status, error);

            if (aliasinfo->description.len > 0) {
                desc = &aliasinfo->description;
                grp_desc = wc16sndup(desc->string, desc->len/2);
                goto_if_no_memory_ntstatus(grp_desc, error);

                info[info_idx].lgrpi1_comment = grp_desc;
                status = NetAddDepMemory(grp_desc, info);
                goto_if_ntstatus_not_success(status, error);

            } else {
                info[info_idx].lgrpi1_comment = 0;
            }

            status = SamrClose(samr_b, &alias_h);
            goto_if_ntstatus_not_success(status, error);

            if (aliasinfo) {
                SamrFreeMemory((void*)aliasinfo);
            }

            info_idx++;
            res_idx++;
        }

        if (rids) {
            SamrFreeMemory((void*)rids);
        }

        if (names) {
            SamrFreeMemory((void*)names);
        }

        num_dom_aliases += num_entries;

    } while (status == STATUS_MORE_ENTRIES);

    res      = 0;
    grp_name = NULL;
    grp_desc = NULL;
    desc     = NULL;

    do {
        status = SamrEnumDomainAliases(samr_b, &btin_domain_h,
                                       &res, account_flags, &names,
                                       &rids, &num_entries);
        goto_if_ntstatus_not_success(status, error);

        for (i = 0; i < num_entries; i++) {
            if (i + num_btin_aliases + num_dom_aliases < res_idx ||
                info_idx >= entries) {
                continue;
            }

            grp_name = wc16sdup(names[i]);
            goto_if_no_memory_ntstatus(grp_name, error);

            info[info_idx].lgrpi1_name = grp_name;
            status = NetAddDepMemory(grp_name, info);
            goto_if_ntstatus_not_success(status, error);

            status = SamrOpenAlias(samr_b, &btin_domain_h,
                                   alias_access, rids[i],
                                   &alias_h);
            goto_if_ntstatus_not_success(status, error);

            status = SamrQueryAliasInfo(samr_b, &alias_h,
                                        ALIAS_INFO_DESCRIPTION,
                                        &aliasinfo);
            goto_if_ntstatus_not_success(status, error);

            if (aliasinfo->description.len > 0) {
                desc = &aliasinfo->description;
                grp_desc = wc16sndup(desc->string, desc->len/2);
                goto_if_no_memory_ntstatus(grp_desc, error);

                info[info_idx].lgrpi1_comment = grp_desc;
                status = NetAddDepMemory(grp_desc, info);
                goto_if_ntstatus_not_success(status, error);

            } else {
                info[info_idx].lgrpi1_comment = 0;
            }

            status = SamrClose(samr_b, &alias_h);
            goto_if_ntstatus_not_success(status, error);

            if (aliasinfo) {
                SamrFreeMemory((void*)aliasinfo);
            }

            info_idx++;
            res_idx++;
        }

        if (rids) {
            SamrFreeMemory((void*)rids);
        }

        if (names) {
            SamrFreeMemory((void*)names);
        }

        num_btin_aliases += num_entries;

    } while (status == STATUS_MORE_ENTRIES);

    total = num_dom_aliases + num_btin_aliases;

    if (total > res_idx) {
        err = NtStatusToWin32Error(STATUS_MORE_ENTRIES);
    } else {
        err = NtStatusToWin32Error(STATUS_SUCCESS);
    }

    *buffer      = (void*)info;
    *out_entries = info_idx;
    *out_resume  = res_idx;
    *out_total   = total;

cleanup:
    if (dominfo) {
        SamrFreeMemory((void*)dominfo);
    }

    if (btin_dominfo) {
        SamrFreeMemory((void*)btin_dominfo);
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

    buffer       = NULL;
    *out_entries = 0;
    *out_resume  = 0;
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
