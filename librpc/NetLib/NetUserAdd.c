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


NET_API_STATUS NetUserAdd(const wchar16_t *hostname, uint32 level, void *buffer,
                          uint32 *parm_err)
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

    NTSTATUS status;
    NetConn *conn;
    handle_t samr_bind;
    PolicyHandle domain_handle, user_handle;
    wchar16_t *user_name;
    uint32 rid;
    uint32 samr_infolevel;
    USER_INFO_X *ninfo;
    UserInfo info, *qinfo;

    memset(&info, 0, sizeof(info));
    qinfo = NULL;

    status = PushUserInfoAdd(&info, &samr_infolevel, buffer, level, parm_err);
    if (status != 0) return NtStatusToWin32Error(ERROR_INVALID_PARAMETER);

    status = NetConnectSamr(&conn, hostname, dom_access, 0);
    if (status != 0) return NtStatusToWin32Error(status);

    samr_bind     = conn->samr.bind;
    domain_handle = conn->samr.dom_handle;

    ninfo = (USER_INFO_X*)buffer;
    user_name = ninfo->name;

    status = SamrCreateUser(samr_bind, &domain_handle, user_name,
                            user_access, &user_handle, &rid);
    if (status != 0) return NtStatusToWin32Error(status);

    if (ninfo->password) {
        UserInfo pwinfo;
        UserInfo26 *info26 = NULL;

	memset((void*)&pwinfo, 0, sizeof(pwinfo));
	info26 = &pwinfo.info26;

        info26->password_len = wc16slen(ninfo->password);
        status = EncPasswordEx(info26->password.data, ninfo->password,
                               info26->password_len, conn);
        if (status != 0) return NtStatusToWin32Error(status);

        status = SamrSetUserInfo(samr_bind, &user_handle, 26, &pwinfo);
        if (status != 0) return NtStatusToWin32Error(status);
    }

    status = SamrSetUserInfo(samr_bind, &user_handle, samr_infolevel,
                             &info);
    if (status != 0) return NtStatusToWin32Error(status);
	
    status = SamrClose(samr_bind, &user_handle);
    if (status != 0) return NtStatusToWin32Error(status);

    return ERROR_SUCCESS;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
