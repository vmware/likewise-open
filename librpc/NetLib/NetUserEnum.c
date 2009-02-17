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
#include "NetLibUserInfo.h"


NET_API_STATUS NetUserEnum(const wchar16_t *hostname, uint32 level, uint32 filter,
                           void **buffer, uint32 maxlen, uint32 *entries,
                           uint32 *total, uint32 *resume)
{
    const uint32 dom_flags = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                             DOMAIN_ACCESS_OPEN_ACCOUNT;
    const uint16 dominfo_level = 2;

    NTSTATUS status = STATUS_SUCCESS;
    NetConn *conn;
    handle_t samr_bind;
    PolicyHandle dom_handle, user_handle;
    DomainInfo *dominfo;
    uint32 res, num_entries, max_size, i;
    wchar16_t **usernames;
    uint32 *userrids;
    uint32 acct_flags = 0;
    uint32 user_flags = 0;
    void *infobuffer = NULL;
    size_t sizebuffer = 0;
    int error = 0;
    PIO_ACCESS_TOKEN access_token = NULL;

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
        return NtStatusToWin32Error(STATUS_INVALID_PARAMETER);
    }

    status = LwIoGetThreadAccessToken(&access_token);
    BAIL_ON_NT_STATUS(status);

    status = NetConnectSamr(&conn, hostname, dom_flags, 0, access_token);
    BAIL_ON_NT_STATUS(status);

    samr_bind  = conn->samr.bind;
    dom_handle = conn->samr.dom_handle;

    status = SamrQueryDomainInfo(samr_bind, &dom_handle, dominfo_level,
                                 &dominfo);
    BAIL_ON_NT_STATUS(status);

    *total = dominfo->info2.num_users;

    res      = *resume;
    max_size = maxlen;

    status = SamrEnumDomainUsers(samr_bind, &dom_handle, &res, acct_flags,
                                 max_size, &usernames, &userrids,
                                 &num_entries);
    if (status != 0 &&
        status != STATUS_MORE_ENTRIES) {
        error = NtStatusToWin32Error(status);
        goto done;
    }

    switch (level) {
    case 0: sizebuffer = sizeof(USER_INFO_0) * num_entries;
        break;
    case 1: sizebuffer = sizeof(USER_INFO_1) * num_entries;
        break;
    case 2: sizebuffer = sizeof(USER_INFO_2) * num_entries;
        break;
    case 3: sizebuffer = sizeof(USER_INFO_3) * num_entries;
        break;
    case 10: sizebuffer = sizeof(USER_INFO_10) * num_entries;
        break;
    case 11: sizebuffer = sizeof(USER_INFO_11) * num_entries;
        break;
    case 20: sizebuffer = sizeof(USER_INFO_20) * num_entries;
        break;
    case 23: sizebuffer = sizeof(USER_INFO_23) * num_entries;
        break;
    default:
        error = ERROR_INVALID_LEVEL;
        goto done;
    }

    infobuffer = (void*) malloc(sizebuffer);
    if (infobuffer == NULL) return NtStatusToWin32Error(STATUS_NO_MEMORY);

    for (i = 0; i < num_entries; i++) {
        if (level == 0) {
            /* very simple infolevel - only a username */
            USER_INFO_0 *info = (USER_INFO_0*)infobuffer;
            if (usernames[i] != NULL) {
                info[i].usri0_name = wc16sdup(usernames[i]);
            }
		
        } else {
            /* more complicated situation - full query user info of user accounts
               (one by one) is necessary */
            const uint16 infolevel = 21;
            UserInfo *ui = NULL;
            NTSTATUS user_status;
            user_flags = USER_ACCESS_GET_NAME_ETC | USER_ACCESS_GET_ATTRIBUTES |
                         USER_ACCESS_GET_LOCALE | USER_ACCESS_GET_LOGONINFO |
                         USER_ACCESS_GET_GROUPS;

            user_status = SamrOpenUser(samr_bind, &dom_handle, user_flags,
                                       userrids[i], &user_handle);
            if (user_status) {
                error = NtStatusToWin32Error(user_status);
                goto done;
            }

            user_status = SamrQueryUserInfo(samr_bind, &user_handle,
                                            infolevel, &ui);
            if (user_status) {
                error = NtStatusToWin32Error(user_status);
                SamrClose(samr_bind, &user_handle);
                goto done;
            }

            switch (level) {
            case 1: infobuffer = PullUserInfo1(infobuffer, &ui->info21, i);
                break;
            case 2: infobuffer = PullUserInfo2(infobuffer, &ui->info21, i);
                break;
            case 20: infobuffer = PullUserInfo20(infobuffer, &ui->info21, i);
                break;
            default:
                error = ERROR_INVALID_LEVEL;
                SamrClose(samr_bind, &user_handle);
                goto done;
            }

            user_status = SamrClose(samr_bind, &user_handle);

            if (user_status) {
                error = NtStatusToWin32Error(user_status);
                goto done;
            }
        }
    }

    *resume = res;
    *buffer = infobuffer;
    *entries = num_entries;


error:

    if (access_token)
    {
        LwIoDeleteAccessToken(access_token);
    }

    error = NtStatusToWin32Error(status);

done:
    return error;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
