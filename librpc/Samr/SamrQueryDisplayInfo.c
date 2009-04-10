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


NTSTATUS
SamrQueryDisplayInfo(
    handle_t b,
    PolicyHandle *domain_h,
    uint16 level,
    uint32 start_idx,
    uint32 max_entries,
    uint32 buf_size,
    uint32 *out_total_size,
    uint32 *out_returned_size,
    SamrDisplayInfo **out_info
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS ret_status = STATUS_SUCCESS;
    uint32 total_size = 0;
    uint32 returned_size = 0;
    SamrDisplayInfo info;
    SamrDisplayInfo *disp_info = NULL;

    memset(&info, 0, sizeof(info));

    goto_if_invalid_param_ntstatus(b, cleanup);
    goto_if_invalid_param_ntstatus(domain_h, cleanup);
    goto_if_invalid_param_ntstatus(out_total_size, cleanup);
    goto_if_invalid_param_ntstatus(out_returned_size, cleanup);
    goto_if_invalid_param_ntstatus(out_info, cleanup);

    DCERPC_CALL(_SamrQueryDisplayInfo(b, domain_h, level, start_idx,
                                      max_entries, buf_size,
                                      &total_size, &returned_size,
                                      &info));
    /* Preserve returned status code */
    ret_status = status;

    /* Status other than success doesn't have to mean failure here */
    if (status != STATUS_SUCCESS &&
        status != STATUS_MORE_ENTRIES) {
        goto_if_ntstatus_not_success(status, error);
    }

    status = SamrAllocateDisplayInfo(&disp_info, &info, level);
    goto_if_ntstatus_not_success(status, error);

    *out_total_size    = total_size;
    *out_returned_size = returned_size;
    *out_info          = disp_info;

cleanup:
    SamrCleanStubDisplayInfo(&info, level);

    if (status == STATUS_SUCCESS &&
        (ret_status == STATUS_SUCCESS ||
         ret_status == STATUS_MORE_ENTRIES)) {
        status = ret_status;
    }

    return status;

error:
    if (disp_info) {
        SamrFreeMemory((void*)disp_info);
    }

    *out_total_size    = 0;
    *out_returned_size = 0;
    *out_info          = NULL;
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
