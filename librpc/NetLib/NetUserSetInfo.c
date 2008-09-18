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
#include "NetConnection.h"


NET_API_STATUS NetUserSetInfo(const wchar16_t *hostname, const wchar16_t *username,
			      uint32 level, void *buffer, uint32 *parm_err)
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

    NTSTATUS status;
    NetConn *conn;
    handle_t samr_bind;
    PolicyHandle user_handle;
    uint32 user_rid;
    UserInfo info;
    USER_INFO_0 *ninfo0;
    USER_INFO_20 *ninfo20;
    USER_INFO_1003 *ninfo1003;
    USER_INFO_1007 *ninfo1007;
    USER_INFO_1008 *ninfo1008;
    USER_INFO_1011 *ninfo1011;
    uint32 samr_level;

    if (username == NULL || buffer == NULL) {
	return NtStatusToWin32Error(STATUS_NO_MEMORY);
    }

    status = NetConnectSamr(&conn, hostname, domain_access, 0);
    if (status != 0) return NtStatusToWin32Error(status);
    
    samr_bind = conn->samr.bind;

    status = NetOpenUser(conn, username, access_rights, &user_handle, &user_rid);
    if (status != 0) return NtStatusToWin32Error(status);

    memset(&info, 0, sizeof(info));

    switch (level) {
    case 0:
	ninfo0 = (USER_INFO_0*) buffer;
	PushUserInfo0(&info, &samr_level, ninfo0);
	break;
    case 1003:
	ninfo1003 = (USER_INFO_1003*) buffer;
	PushUserInfo1003(&info, &samr_level, ninfo1003, conn);
	break;
    case 1007:
	ninfo1007 = (USER_INFO_1007*) buffer;
	PushUserInfo1007(&info, &samr_level, ninfo1007);
	break;
    case 1008:
	ninfo1008 = (USER_INFO_1008*) buffer;
	PushUserInfo1008(&info, &samr_level, ninfo1008);
	break;
    case 1011:
	ninfo1011 = (USER_INFO_1011*) buffer;
	PushUserInfo1011(&info, &samr_level, ninfo1011);
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
	return NtStatusToWin32Error(STATUS_NOT_IMPLEMENTED);
    default:
	return NtStatusToWin32Error(ERROR_INVALID_LEVEL);
    }
	
    status = SamrSetUserInfo(samr_bind, &user_handle, samr_level, &info);
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
