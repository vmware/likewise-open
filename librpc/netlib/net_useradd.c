/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
*/

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/*
 * Abstract: NetUserAdd function (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */


#include "includes.h"


NET_API_STATUS
NetUserAdd(
    const wchar16_t *hostname,
    uint32 level,
    void *buffer,
    uint32 *parm_err
    )
{
    const uint32 user_access = USER_ACCESS_GET_NAME_ETC |
                               USER_ACCESS_SET_LOC_COM |
                               USER_ACCESS_GET_LOCALE |
                               USER_ACCESS_GET_LOGONINFO |
                               USER_ACCESS_GET_ATTRIBUTES |
                               USER_ACCESS_GET_GROUPS |
                               USER_ACCESS_GET_GROUP_MEMBERSHIP |
                               USER_ACCESS_CHANGE_GROUP_MEMBERSHIP |
                               USER_ACCESS_SET_ATTRIBUTES |
                               USER_ACCESS_SET_PASSWORD;
    const uint32 dom_access = DOMAIN_ACCESS_CREATE_USER |
                              DOMAIN_ACCESS_LOOKUP_INFO_1;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    NetConn *conn = NULL;
    handle_t samr_b = NULL;
    PolicyHandle domain_h = {0};
    PolicyHandle user_h = {0};
    wchar16_t *user_name = NULL;
    uint32 rid = 0;
    uint32 samr_infolevel = 0;
    uint32 out_parm_err = 0;
    USER_INFO_X *ninfo = NULL;
    UserInfo *sinfo = NULL;
    PIO_ACCESS_TOKEN access_token = NULL;

    BAIL_ON_INVALID_PTR(buffer);
    BAIL_ON_INVALID_PTR(parm_err);

    status = PushUserInfoAdd(&sinfo, &samr_infolevel, buffer, level,
                             &out_parm_err);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = LwIoGetThreadAccessToken(&access_token);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetConnectSamr(&conn, hostname, dom_access, 0, access_token);
    BAIL_ON_NTSTATUS_ERROR(status);

    samr_b    = conn->samr.bind;
    domain_h  = conn->samr.dom_handle;

    ninfo     = (USER_INFO_X*)buffer;
    user_name = ninfo->name;

    status = SamrCreateUser(samr_b, &domain_h, user_name, user_access,
                            &user_h, &rid);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* If there was password specified do an extra samr call to set it */
    if (ninfo->password) {
        UserInfo pwinfo;
        UserInfo26 *info26 = NULL;

	memset((void*)&pwinfo, 0, sizeof(pwinfo));
	info26 = &pwinfo.info26;

        memset((void*)&pwinfo, 0, sizeof(pwinfo));
        info26 = &pwinfo.info26;

        info26->password_len = wc16slen(ninfo->password);
        status = EncPasswordEx(info26->password.data, ninfo->password,
                               info26->password_len, conn);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = SamrSetUserInfo(samr_b, &user_h, 26, &pwinfo);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    status = SamrSetUserInfo(samr_b, &user_h, samr_infolevel, sinfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrClose(samr_b, &user_h);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    if (parm_err) {
        *parm_err = out_parm_err;
    }

    if (sinfo) {
        NetFreeMemory((void*)sinfo);
    }

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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
