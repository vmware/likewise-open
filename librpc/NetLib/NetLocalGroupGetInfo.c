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
NetLocalGroupGetInfo(
    const wchar16_t *hostname,
    const wchar16_t *aliasname,
    uint32 level,
    void **buffer
    )
{
    const uint32 access_rights = ALIAS_ACCESS_LOOKUP_INFO;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    NetConn *conn = NULL;
    handle_t samr_b = NULL;
    PolicyHandle alias_h;
    uint32 alias_rid = 0;
    AliasInfo *info = NULL;
    void *ninfo = NULL;
    PIO_ACCESS_TOKEN access_token = NULL;

    goto_if_invalid_param_winerr(hostname, cleanup);
    goto_if_invalid_param_winerr(aliasname, cleanup);
    goto_if_invalid_param_winerr(buffer, cleanup);

    status = LwIoGetThreadAccessToken(&access_token);
    goto_if_ntstatus_not_success(status, error);

    status = NetConnectSamr(&conn, hostname, 0, 0, access_token);
    goto_if_ntstatus_not_success(status, error);

    samr_b = conn->samr.bind;

    status = NetOpenAlias(conn, aliasname, access_rights, &alias_h, &alias_rid);
    goto_if_ntstatus_not_success(status, error);

    status = SamrQueryAliasInfo(samr_b, &alias_h, ALIAS_INFO_ALL, &info);
    goto_if_ntstatus_not_success(status, error);

    switch (level) {
    case 0:
        status = PullLocalGroupInfo0((void**)&ninfo, info, 1);
        break;

    case 1:
        status = PullLocalGroupInfo1((void**)&ninfo, info, 1);
        break;

    default:
        status = STATUS_INVALID_LEVEL;
    }
    goto_if_ntstatus_not_success(status, error);

    status = SamrClose(samr_b, &alias_h);
    goto_if_ntstatus_not_success(status, error);

    *buffer = ninfo;

cleanup:
    if (info) {
        SamrFreeMemory((void*)info);
    }

    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
    }

    return status;

error:
    if (ninfo) {
        NetFreeMemory((void*)ninfo);
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
