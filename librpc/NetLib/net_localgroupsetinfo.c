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
NetLocalGroupSetInfo(
    const wchar16_t *hostname,
    const wchar16_t *aliasname,
    uint32 level,
    void *buffer,
    uint32 *parm_err
    )
{
    const uint32 alias_access = ALIAS_ACCESS_SET_INFO;

    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    NetConn *conn = NULL;
    handle_t samr_b = NULL;
    PolicyHandle alias_h;
    uint32 alias_rid = 0;
    AliasInfo *info = NULL;
    uint32 slevel = 0;
    PIO_ACCESS_TOKEN access_token = NULL;

    BAIL_ON_INVALID_PTR(hostname);
    BAIL_ON_INVALID_PTR(aliasname);
    BAIL_ON_INVALID_PTR(buffer);

    status = LwIoGetThreadAccessToken(&access_token);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetConnectSamr(&conn, hostname, 0, 0, access_token);
    BAIL_ON_NTSTATUS_ERROR(status);

    samr_b = conn->samr.bind;

    status = NetOpenAlias(conn, aliasname, alias_access, &alias_h,
                          &alias_rid);
    BAIL_ON_NTSTATUS_ERROR(status);

    switch (level) {
    case 0:
        status = PushLocalGroupInfo0(&info, &slevel, buffer);
        break;

    case 1:
        status = PushLocalGroupInfo1(&info, &slevel, buffer);
        break;

    case 1002:
        status = PushLocalGroupInfo1002(&info, &slevel, buffer);
        break;

    default:
        err = ERROR_INVALID_LEVEL;
        goto error;
    }
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSetAliasInfo(samr_b, &alias_h, slevel, info);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrClose(samr_b, &alias_h);
    BAIL_ON_NTSTATUS_ERROR(status);

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
