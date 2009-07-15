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
    IN  handle_t              hNetrBinding,
    IN  PCWSTR                pwszServer,
    IN  PCWSTR                pwszDomain,
    IN  PCWSTR                pwszComputer,
    IN  PCWSTR                pwszUsername,
    IN  PCWSTR                pwszPassword,
    IN  UINT16                LogonLevel,
    IN  UINT16                ValidationLevel,
    OUT NetrValidationInfo  **ppValidationInfo,
    OUT PBYTE                 pAuthoritative
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszServerName = NULL;
    PWSTR pwszComputerName = NULL;
    NetrLogonInfo *pLogonInfo = NULL;
    NetrValidationInfo ValidationInfo = {0};
    NetrValidationInfo *pValidationInfo = NULL;
    BYTE Authoritative = 0;
    UINT32 Flags = 0;

    BAIL_ON_INVALID_PTR(hNetrBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServer, ntStatus);
    BAIL_ON_INVALID_PTR(pwszDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pwszComputer, ntStatus);
    BAIL_ON_INVALID_PTR(pwszUsername, ntStatus);
    BAIL_ON_INVALID_PTR(pwszPassword, ntStatus);
    BAIL_ON_INVALID_PTR(ppValidationInfo, ntStatus);
    BAIL_ON_INVALID_PTR(pAuthoritative, ntStatus);

    ntStatus = NetrAllocateUniString(&pwszServerName,
                                     pwszServer,
                                     NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateUniString(&pwszComputerName,
                                     pwszComputer,
                                     NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateLogonInfo(&pLogonInfo,
                                     LogonLevel,
                                     pwszDomain,
                                     pwszComputer,
                                     pwszUsername,
                                     pwszPassword);
    BAIL_ON_NT_STATUS(ntStatus);

    DCERPC_CALL(ntStatus, _NetrLogonSamLogonEx(hNetrBinding,
                                               pwszServerName,
                                               pwszComputerName,
                                               LogonLevel,
                                               pLogonInfo,
                                               ValidationLevel,
                                               &ValidationInfo,
                                               &Authoritative,
                                               &Flags));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateValidationInfo(&pValidationInfo,
                                          &ValidationInfo,
                                          ValidationLevel);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppValidationInfo = pValidationInfo;
    *pAuthoritative   = Authoritative;

cleanup:
    NetrCleanStubValidationInfo(&ValidationInfo,
                                ValidationLevel);

    if (pwszServerName) {
        NetrFreeMemory((void*)pwszServerName);
    }

    if (pwszComputerName) {
        NetrFreeMemory((void*)pwszComputerName);
    }

    if (pLogonInfo) {
        NetrFreeMemory((void*)pLogonInfo);
    }

    return ntStatus;

error:
    if (pValidationInfo) {
        NetrFreeMemory((void*)pValidationInfo);
    }

    *ppValidationInfo = NULL;
    *pAuthoritative   = 0;

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
