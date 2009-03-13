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
#include "NetConnection.h"


NET_API_STATUS
NetUserSetInfo(
    const wchar16_t *hostname,
    const wchar16_t *username,
    uint32 level,
    void *buffer,
    uint32 *parm_err
    )
{
    /* This is necessary to be able set account password.
       Otherwise we get access denied. Don't ask... */
    const uint32 domain_access = DOMAIN_ACCESS_LOOKUP_INFO_1;

    const uint32 access_rights = USER_ACCESS_GET_NAME_ETC |
                                 USER_ACCESS_GET_LOCALE |
                                 USER_ACCESS_GET_LOGONINFO |
                                 USER_ACCESS_GET_ATTRIBUTES |
                                 USER_ACCESS_GET_GROUPS |
                                 USER_ACCESS_GET_GROUP_MEMBERSHIP |
                                 USER_ACCESS_SET_LOC_COM |
                                 USER_ACCESS_SET_ATTRIBUTES |
                                 USER_ACCESS_CHANGE_PASSWORD |
                                 USER_ACCESS_SET_PASSWORD;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    NetConn *conn = NULL;
    handle_t samr_b = NULL;
    PolicyHandle user_h;
    uint32 user_rid = 0;
    UserInfo *info = NULL;
    USER_INFO_0 *ninfo0 = NULL;
    USER_INFO_1003 *ninfo1003 = NULL;
    USER_INFO_1007 *ninfo1007 = NULL;
    USER_INFO_1008 *ninfo1008 = NULL;
    USER_INFO_1011 *ninfo1011 = NULL;
    uint32 samr_level = 0;
    PIO_ACCESS_TOKEN access_token = NULL;

    memset(&info, 0, sizeof(info));

    goto_if_invalid_param_winerr(hostname, cleanup);
    goto_if_invalid_param_winerr(username, cleanup);
    goto_if_invalid_param_winerr(buffer, cleanup);

    status = LwIoGetThreadAccessToken(&access_token);
    goto_if_ntstatus_not_success(status, error);

    status = NetConnectSamr(&conn, hostname, domain_access, 0, access_token);
    goto_if_ntstatus_not_success(status, error);
    
    samr_b = conn->samr.bind;

    status = NetOpenUser(conn, username, access_rights, &user_h, &user_rid);
    goto_if_ntstatus_not_success(status, error);

    switch (level) {
    case 0:
        ninfo0 = (USER_INFO_0*) buffer;
        status = PushUserInfo0(&info, &samr_level, ninfo0);
        break;

    case 1003:
        ninfo1003 = (USER_INFO_1003*) buffer;
        status = PushUserInfo1003(&info, &samr_level, ninfo1003, conn);
        break;

    case 1007:
        ninfo1007 = (USER_INFO_1007*) buffer;
        status = PushUserInfo1007(&info, &samr_level, ninfo1007);
        break;

    case 1008:
        ninfo1008 = (USER_INFO_1008*) buffer;
        status = PushUserInfo1008(&info, &samr_level, ninfo1008);
        break;

    case 1011:
        ninfo1011 = (USER_INFO_1011*) buffer;
        status = PushUserInfo1011(&info, &samr_level, ninfo1011);
        break;

    case 1:
    case 2:
    case 3:
    case 4:
    case 21:
    case 22:
    case 1005:
    case 1006:
    case 1009:
    case 1010:
    case 1012:
    case 1014:
    case 1017:
    case 1020:
    case 1024:
    case 1051:
    case 1052:
    case 1053:
        status = STATUS_NOT_IMPLEMENTED;
        break;

    default:
        status = STATUS_INVALID_LEVEL;
        break;
    }
    goto_if_ntstatus_not_success(status, error);
	
    status = SamrSetUserInfo(samr_b, &user_h, samr_level, info);
    goto_if_ntstatus_not_success(status, error);
	
    status = SamrClose(samr_b, &user_h);
    goto_if_ntstatus_not_success(status, error);

cleanup:
    if (info) {
        NetFreeMemory((void*)info);
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
