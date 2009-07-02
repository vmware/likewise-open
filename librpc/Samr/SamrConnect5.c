/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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


NTSTATUS
SamrConnect5(
    handle_t b,
    const wchar16_t *sysname,
    uint32 access_mask,
    uint32 level_in,
    SamrConnectInfo *info_in,
    uint32 *level_out,
    SamrConnectInfo *info_out,
    PolicyHandle *conn_handle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t *system_name = NULL;
    uint32 system_name_len = 0;
    PolicyHandle handle;
    uint32 level = 0;
    SamrConnectInfo info;

    memset(&handle, 0, sizeof(handle));
    memset(&info, 0, sizeof(info));

    BAIL_ON_INVALID_PTR(b);
    BAIL_ON_INVALID_PTR(sysname);
    BAIL_ON_INVALID_PTR(info_in);
    BAIL_ON_INVALID_PTR(level_out);
    BAIL_ON_INVALID_PTR(info_out);
    BAIL_ON_INVALID_PTR(conn_handle);

    system_name = wc16sdup(sysname);
    BAIL_ON_NO_MEMORY(system_name);

    system_name_len = wc16slen(system_name) + 1;

    DCERPC_CALL(_SamrConnect5(b, system_name_len, system_name, access_mask,
			      level_in, info_in, &level, &info, &handle));
    BAIL_ON_NTSTATUS_ERROR(status);

    *level_out   = level;
    *info_out    = info;
    *conn_handle = handle;

cleanup:
    SAFE_FREE(system_name);

    return status;

error:
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
