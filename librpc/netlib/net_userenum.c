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
NetUserEnum(
    const wchar16_t *hostname,
    uint32 level, uint32 filter,
    void **buffer,
    uint32 maxlen,
    uint32 *out_entries,
    uint32 *out_total,
    uint32 *out_resume
    )
{
    const uint32 dom_flags = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                             DOMAIN_ACCESS_OPEN_ACCOUNT;
    const uint16 dominfo_level = 2;
    const uint16 infolevel = 21;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    NetConn *conn = NULL;
    handle_t samr_b = NULL;
    PolicyHandle dom_h, user_h;
    DomainInfo *dominfo = NULL;
    uint32 num_entries = 0;
    uint32 max_size = 0;
    uint32 i = 0;
    wchar16_t **usernames = NULL;
    uint32 *userrids = NULL;
    uint32 acct_flags = 0;
    uint32 user_flags = 0;
    void *ninfo = NULL;         /* "Net" user info */
    UserInfo21 *sinfo = NULL;   /* "Samr" user info */
    UserInfo *ui = NULL;
    uint32 total = 0;
    uint32 resume = 0;
    PIO_ACCESS_TOKEN access_token = NULL;

    BAIL_ON_INVALID_PTR(hostname);
    BAIL_ON_INVALID_PTR(buffer);
    BAIL_ON_INVALID_PTR(out_entries);
    BAIL_ON_INVALID_PTR(out_total);
    BAIL_ON_INVALID_PTR(out_resume);

    switch (filter) {
    case FILTER_NORMAL_ACCOUNT:
        acct_flags = ACB_NORMAL;
        break;

    case FILTER_WORKSTATION_TRUST_ACCOUNT:
        acct_flags = ACB_WSTRUST;
        break;

    case FILTER_SERVER_TRUST_ACCOUNT:
        acct_flags = ACB_SVRTRUST;
        break;

    case FILTER_INTERDOMAIN_TRUST_ACCOUNT:
        acct_flags = ACB_DOMTRUST;
        break;

    default:
        err = NtStatusToWin32Error(STATUS_INVALID_PARAMETER);
        goto error;
    }

    if (!(level == 0 ||
          level == 1 ||
          level == 2 ||
          level == 20)) {
        err = ERROR_INVALID_LEVEL;
        goto error;
    }

    status = LwIoGetThreadAccessToken(&access_token);
    BAIL_ON_NTSTATUS_ERROR(status);

    samr_b = conn->samr.bind;
    dom_h  = conn->samr.dom_handle;

    status = NetConnectSamr(&conn, hostname, dom_flags, 0, access_token);
    BAIL_ON_NT_STATUS(status);


    samr_b = conn->samr.bind;
    dom_h  = conn->samr.dom_handle;

    status = SamrQueryDomainInfo(samr_b, &dom_h, dominfo_level, &dominfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    total    = dominfo->info2.num_users;
    resume   = *out_resume;
    max_size = maxlen;

    status = SamrEnumDomainUsers(samr_b, &dom_h, &resume, acct_flags,
                                 max_size, &usernames, &userrids,
                                 &num_entries);
    if (status != 0 &&
        status != STATUS_MORE_ENTRIES) {
        err = NtStatusToWin32Error(status);
        goto error;
    }

    status = NetAllocateMemory((void**)&sinfo,
                               sizeof(UserInfo) * num_entries,
                               NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < num_entries; i++) {
        if (level != 0) {
            /* full query user info of user accounts (one by one)
               is necessary */
            user_flags = USER_ACCESS_GET_NAME_ETC |
                         USER_ACCESS_GET_ATTRIBUTES |
                         USER_ACCESS_GET_LOCALE |
                         USER_ACCESS_GET_LOGONINFO |
                         USER_ACCESS_GET_GROUPS;

            status = SamrOpenUser(samr_b, &dom_h, user_flags, userrids[i],
                                  &user_h);
            BAIL_ON_NTSTATUS_ERROR(status);

            status = SamrQueryUserInfo(samr_b, &user_h, infolevel, &ui);
            BAIL_ON_NTSTATUS_ERROR(status);

            if (ui) {
                memcpy(&(sinfo[i]), &ui->info21, sizeof(UserInfo21));
                NetFreeMemory((void*)ui);
            }

            status = SamrClose(samr_b, &user_h);
            BAIL_ON_NTSTATUS_ERROR(status);
        }
    }

    switch (level) {
    case 0: status = PullUserInfo0(&ninfo, usernames, num_entries);
        break;

    case 1: status = PullUserInfo1(&ninfo, sinfo, num_entries);
        break;

    case 2: status = PullUserInfo2(&ninfo, sinfo, num_entries);
        break;

    case 20: status = PullUserInfo20(&ninfo, sinfo, num_entries);
        break;
    }

    BAIL_ON_NTSTATUS_ERROR(status);

    *buffer      = ninfo;
    *out_resume  = resume;
    *out_entries = num_entries;
    *out_total   = total;

cleanup:
    if (sinfo) {
        NetFreeMemory((void*)sinfo);
    }

    if (dominfo) {
        SamrFreeMemory((void*)dominfo);
    }

    if (usernames) {
        SamrFreeMemory((void*)usernames);
    }

    if (userrids) {
        SamrFreeMemory((void*)userrids);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
    }

    return err;

error:
    if (ninfo) {
        NetFreeMemory((void*)ninfo);
    }

    if (access_token)
    {
        LwIoDeleteAccessToken(access_token);
    }

    *buffer  = NULL;
    *out_resume  = 0;
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
