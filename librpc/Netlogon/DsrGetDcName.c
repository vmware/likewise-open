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


WINERR
DsrGetDcName(handle_t b,
             const wchar16_t *server_name,
             const wchar16_t *domain_name,
             const Guid *domain_guid,
             const Guid *site_guid,
             uint32 get_dc_flags,
             DsrDcNameInfo **out_info)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    wchar16_t *server = NULL;
    wchar16_t *domain = NULL;
    Guid *d_guid = NULL;
    Guid *s_guid = NULL;
    DsrDcNameInfo *info = NULL;
    DsrDcNameInfo *dc_info = NULL;

    goto_if_invalid_param_winerr(b, cleanup);
    goto_if_invalid_param_winerr(server_name, cleanup);
    goto_if_invalid_param_winerr(domain_name, cleanup);
    goto_if_invalid_param_winerr(out_info, cleanup);

    server = wc16sdup(server_name);
    goto_if_no_memory_winerr(server, cleanup);

    domain = wc16sdup(domain_name);
    goto_if_no_memory_winerr(server, cleanup);

    if (domain_guid) {
        status = NetrAllocateMemory((void**)&d_guid, sizeof(*d_guid), NULL);
        goto_if_ntstatus_not_success(status, error);

        memcpy(d_guid, domain_guid, sizeof(*d_guid));
    }

    if (site_guid) {
        status = NetrAllocateMemory((void**)&s_guid, sizeof(*s_guid), NULL);
        goto_if_ntstatus_not_success(status, error);

        memcpy(s_guid, site_guid, sizeof(*s_guid));
    }

    DCERPC_CALL(err, _DsrGetDcName(b, server, domain, d_guid, s_guid,
                                   get_dc_flags, &info));

    status = NetrAllocateDcNameInfo(&dc_info, info);
    if (status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
        goto error;
    }

    *out_info = dc_info;

cleanup:
    SAFE_FREE(server);
    SAFE_FREE(domain);

    if (d_guid) {
        NetrFreeMemory(d_guid);
    }

    if (s_guid) {
        NetrFreeMemory(s_guid);
    }

    if (info) {
        NetrFreeStubDcNameInfo(info);
    }

    return err;

error:
    if (err == ERROR_SUCCESS &&
        status != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(status);
    }

    if (dc_info) {
        NetrFreeMemory(dc_info);
    }

    *out_info = NULL;
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
