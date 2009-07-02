/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:
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

/*
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


NTSTATUS
NetrSamLogonEx(
    handle_t b,
    const wchar16_t *server,
    const wchar16_t *domain,
    const wchar16_t *computer,
    const wchar16_t *username,
    const wchar16_t *password,
    uint16 logon_level,
    uint16 validation_level,
    NetrValidationInfo **out_info,
    uint8 *out_authoritative)
{
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t *srv = NULL;
    wchar16_t *comp = NULL;
    NetrLogonInfo *logon_info = NULL;
    NetrValidationInfo validation = {0};
    NetrValidationInfo *validation_info = NULL;
    uint8 authoritative = 0;
    uint32 flags = 0;

    BAIL_ON_INVALID_PTR(b);
    BAIL_ON_INVALID_PTR(server);
    BAIL_ON_INVALID_PTR(domain);
    BAIL_ON_INVALID_PTR(computer);
    BAIL_ON_INVALID_PTR(username);
    BAIL_ON_INVALID_PTR(password);
    BAIL_ON_INVALID_PTR(out_info);
    BAIL_ON_INVALID_PTR(out_authoritative);

    status = NetrAllocateUniString(&srv, server, NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetrAllocateUniString(&comp, computer, NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetrAllocateLogonInfo(&logon_info, logon_level, domain, computer,
                                   username, password);
    BAIL_ON_NTSTATUS_ERROR(status);

    DCERPC_CALL(status, _NetrLogonSamLogonEx(b, srv, comp,
                                             logon_level, logon_info,
                                             validation_level, &validation,
                                             &authoritative, &flags));
    BAIL_ON_NTSTATUS_ERROR(status);

    status = NetrAllocateValidationInfo(&validation_info, &validation,
                                        validation_level);
    BAIL_ON_NTSTATUS_ERROR(status);

    *out_info          = validation_info;
    *out_authoritative = authoritative;

cleanup:
    NetrCleanStubValidationInfo(&validation, validation_level);

    if (srv) {
        NetrFreeMemory((void*)srv);
    }

    if (comp) {
        NetrFreeMemory((void*)comp);
    }

    if (logon_info) {
        NetrFreeMemory((void*)logon_info);
    }

    return status;

error:
    if (validation_info) {
        NetrFreeMemory((void*)validation_info);
    }

    *out_info          = NULL;
    *out_authoritative = 0;

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
