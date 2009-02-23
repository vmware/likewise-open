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
#include "GroupInfo.h"


NET_API_STATUS NetLocalGroupGetInfo(const wchar16_t *hostname,
				    const wchar16_t *aliasname,
				    uint32 level, void **buffer)
{
    const uint32 access_rights = ALIAS_ACCESS_LOOKUP_INFO;

    NTSTATUS status = STATUS_SUCCESS;
    NetConn *conn;
    handle_t samr_bind;
    PolicyHandle alias_handle;
    uint32 alias_rid;
    AliasInfo *info = NULL;
    LOCALGROUP_INFO_1 *ninfo1;
    PIO_ACCESS_TOKEN access_token = NULL;

    if (hostname == NULL || aliasname == NULL ||
	buffer == NULL) {
	return NtStatusToWin32Error(STATUS_INVALID_PARAMETER);
    }

    status = LwIoGetThreadAccessToken(&access_token);
    BAIL_ON_NT_STATUS(status);

    status = NetConnectSamr(&conn, hostname, 0, 0, access_token);
    BAIL_ON_NT_STATUS(status);

    samr_bind = conn->samr.bind;

    status = NetOpenAlias(conn, aliasname, access_rights, &alias_handle,
			  &alias_rid);
    BAIL_ON_NT_STATUS(status);

    status = SamrQueryAliasInfo(samr_bind, &alias_handle,
				ALIAS_INFO_ALL, &info);
    BAIL_ON_NT_STATUS(status);

    ninfo1 = (LOCALGROUP_INFO_1*) malloc(sizeof(LOCALGROUP_INFO_1*));
    if (ninfo1 == NULL) return NtStatusToWin32Error(STATUS_NO_MEMORY);

    *buffer = PullLocalGroupInfo1((void*)ninfo1, info, 0);

    status = SamrClose(samr_bind, &alias_handle);
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
