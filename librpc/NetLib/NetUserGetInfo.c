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


NET_API_STATUS NetUserGetInfo(const wchar16_t *hostname, const wchar16_t *username,
			      uint32 level, void **buffer)
{
    const uint32 access_rights = USER_ACCESS_GET_NAME_ETC |
                                 USER_ACCESS_GET_LOCALE |
                                 USER_ACCESS_GET_LOGONINFO |
                                 USER_ACCESS_GET_ATTRIBUTES |
                                 USER_ACCESS_GET_GROUPS |
                                 USER_ACCESS_GET_GROUP_MEMBERSHIP;
    const uint32 samr_lvl = 21;
    NTSTATUS status = STATUS_SUCCESS;
    NetConn *conn = NULL;
    handle_t samr_bind = NULL;
    PolicyHandle user_handle;
    uint32 user_rid;
    UserInfo *info = NULL;
    USER_INFO_20 *ninfo20 = NULL;
    PIO_ACCESS_TOKEN access_token = NULL;


    if (hostname == NULL || username == NULL ||
        buffer == NULL)
    {
        return NtStatusToWin32Error(STATUS_INVALID_PARAMETER);
    }

    status = LwIoGetThreadAccessToken(&access_token);
    BAIL_ON_NT_STATUS(status);

    status = NetConnectSamr(&conn, hostname, 0, 0, access_token);
    BAIL_ON_NT_STATUS(status);

    samr_bind = conn->samr.bind;

    status = NetOpenUser(conn, username, access_rights, &user_handle,
			 &user_rid);
    BAIL_ON_NT_STATUS(status);

    status = SamrQueryUserInfo(samr_bind, &user_handle, samr_lvl, &info);
    BAIL_ON_NT_STATUS(status);

    ninfo20 = (USER_INFO_20*) malloc(sizeof(USER_INFO_20));
    if (ninfo20 == NULL) BAIL_ON_NT_STATUS(status = STATUS_INSUFFICIENT_RESOURCES);

    *buffer = PullUserInfo20((void*)ninfo20, &info->info21, 0);
	
    status = SamrClose(samr_bind, &user_handle);
    BAIL_ON_NT_STATUS(status);

error:

    if (access_token)
    {
        LwIoDeleteAccessToken(access_token);
    }

    return NtStatusToWin32Error(status);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
