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
SamrConnect4(
    handle_t b,
    const wchar16_t *sysname,
    uint32 client_version,
    uint32 access_mask,
    PolicyHandle *conn_handle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t *system_name = NULL;
    uint32 system_name_len = 0;
    PolicyHandle handle = {0};

    if (!client_version) {
        client_version = SAMR_CONNECT_POST_WIN2K;
    }

    goto_if_invalid_param_ntstatus(b, cleanup);
    goto_if_invalid_param_ntstatus(sysname, cleanup);
    goto_if_invalid_param_ntstatus(conn_handle, cleanup);

    system_name = wc16sdup(sysname);
    goto_if_no_memory_ntstatus(system_name, error);

    system_name_len = wc16slen(system_name) + 1;

    DCERPC_CALL(_SamrConnect4(b, system_name_len, system_name, client_version,
                              access_mask, &handle));
    goto_if_ntstatus_not_success(status, error);

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
