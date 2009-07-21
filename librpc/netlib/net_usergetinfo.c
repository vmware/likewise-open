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
NetUserGetInfo(
    const wchar16_t *hostname,
    const wchar16_t *username,
    uint32 level,
    void **buffer
    )
{
    const uint32 access_rights = USER_ACCESS_GET_NAME_ETC |
                                 USER_ACCESS_GET_LOCALE |
                                 USER_ACCESS_GET_LOGONINFO |
                                 USER_ACCESS_GET_ATTRIBUTES |
                                 USER_ACCESS_GET_GROUPS |
                                 USER_ACCESS_GET_GROUP_MEMBERSHIP;
    const uint32 samr_level = 21;
    const uint32 num = 1;
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    NetConn *conn = NULL;
    handle_t samr_b = NULL;
    PolicyHandle user_h;
    uint32 user_rid = 0;
    UserInfo *info = NULL;
    USER_INFO_20 *ninfo20 = NULL;
    PIO_ACCESS_TOKEN access_token = NULL;

    BAIL_ON_INVALID_PTR(hostname);
    BAIL_ON_INVALID_PTR(username);
    BAIL_ON_INVALID_PTR(buffer);

    if (level != 20) {
        err = ERROR_INVALID_LEVEL;
        goto cleanup;
    }

    status = LwIoGetThreadAccessToken(&access_token);
    BAIL_ON_NTSTATUS_ERROR(status);

    samr_b = conn->samr.bind;

    status = NetConnectSamr(&conn, hostname, 0, 0, access_token);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetOpenUser(conn, username, access_rights, &user_h,
                         &user_rid);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrQueryUserInfo(samr_b, &user_h, samr_level, &info);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = PullUserInfo20((void**)&ninfo20, &info->info21, num);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrClose(samr_b, &user_h);
    BAIL_ON_NTSTATUS_ERROR(status);

    *buffer = ninfo20;

cleanup:
    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
    }

    return err;

error:
    if (ninfo20) {
        NetFreeMemory((void*)ninfo20);
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
