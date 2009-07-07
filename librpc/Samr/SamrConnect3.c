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
SamrConnect3(
    IN  handle_t      hSamrBinding,
    IN  PCWSTR        pwszSysName,
    IN  UINT32        AccessMask,
    OUT PolicyHandle *phConn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszSystemName = NULL;
    UINT32 SystemNameLen = 0;
    UINT32 Unknown = 0;
    PolicyHandle hConn = {0};

    BAIL_ON_INVALID_PTR(hSamrBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszSysName, ntStatus);
    BAIL_ON_INVALID_PTR(phConn, ntStatus);

    pwszSystemName = wc16sdup(pwszSysName);
    BAIL_ON_NULL_PTR(pwszSystemName, ntStatus);

    SystemNameLen = (UINT32) wc16slen(pwszSystemName) + 1;

    DCERPC_CALL(ntStatus, _SamrConnect3(hSamrBinding,
                                        SystemNameLen,
                                        pwszSystemName,
                                        Unknown,
                                        AccessMask,
                                        &hConn));
    BAIL_ON_NT_STATUS(ntStatus);

    *phConn = hConn;

cleanup:
    SAFE_FREE(pwszSystemName);

    return ntStatus;

error:
    memset(phConn, 0, sizeof(*phConn));

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
