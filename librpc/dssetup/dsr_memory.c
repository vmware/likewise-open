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

/*
 * Abstract: Dsr memory (de)allocation routines (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


NTSTATUS
DsrInitMemory()
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (!bDsrInitialised) {
        status = MemPtrListInit((PtrList**)&dsr_ptr_list);
        BAIL_ON_NTSTATUS_ERROR(status);

        bDsrInitialised = 1;
    }

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    goto cleanup;
}


NTSTATUS
DsrDestroyMemory()
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (bDsrInitialised && dsr_ptr_list) {
        status = MemPtrListDestroy((PtrList**)&dsr_ptr_list);
        BAIL_ON_NTSTATUS_ERROR(status);

        bDsrInitialised = 0;
    }

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    goto cleanup;
}


NTSTATUS
DsrAllocateMemory(
    void **out,
    size_t size,
    void *dep
    )
{
    return MemPtrAllocate((PtrList*)dsr_ptr_list, out, size, dep);
}


void
DsrFreeMemory(
    void *ptr
    )
{
    MemPtrFree((PtrList*)dsr_ptr_list, ptr);
}


NTSTATUS
DsrAddDepMemory(
    void *ptr,
    void *dep
    )
{
    return MemPtrAddDependant((PtrList*)dsr_ptr_list, ptr, dep);
}


static
NTSTATUS
DsrAllocateDsRoleInfoBasic(
    PDS_ROLE_PRIMARY_DOMAIN_INFO_BASIC pOut,
    PDS_ROLE_PRIMARY_DOMAIN_INFO_BASIC pIn,
    void *dep
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwDomainLen = 0;
    DWORD dwDnsDomainLen = 0;
    DWORD dwForestLen = 0;

    pOut->uiRole  = pIn->uiRole;
    pOut->uiFlags = pIn->uiFlags;

    if (pIn->pwszDomain) {
        dwDomainLen = wc16slen(pIn->pwszDomain);

        status = DsrAllocateMemory((void**)&pOut->pwszDomain,
                                   (dwDomainLen + 1) * sizeof(WCHAR),
                                   dep);
        BAIL_ON_NTSTATUS_ERROR(status);

        wc16sncpy(pOut->pwszDomain, pIn->pwszDomain, dwDomainLen);
    }

    if (pIn->pwszDnsDomain) {
        dwDnsDomainLen = wc16slen(pIn->pwszDnsDomain);

        status = DsrAllocateMemory((void**)&pOut->pwszDnsDomain,
                                   (dwDnsDomainLen + 1) * sizeof(WCHAR),
                                   dep);
        BAIL_ON_NTSTATUS_ERROR(status);

        wc16sncpy(pOut->pwszDnsDomain, pIn->pwszDnsDomain, dwDnsDomainLen);
    }

    if (pIn->pwszForest) {
        dwForestLen = wc16slen(pIn->pwszForest);

        status = DsrAllocateMemory((void**)&pOut->pwszForest,
                                   (dwForestLen + 1) * sizeof(WCHAR),
                                   dep);
        BAIL_ON_NTSTATUS_ERROR(status);

        wc16sncpy(pOut->pwszForest, pIn->pwszForest, dwForestLen);
    }

    memcpy(&pOut->DomainGuid, &pIn->DomainGuid,
           sizeof(pOut->DomainGuid));

cleanup:
    return status;

error:
    goto cleanup;
}


NTSTATUS
DsrAllocateDsRoleInfo(
    PDS_ROLE_INFO *ppOut,
    PDS_ROLE_INFO pIn,
    UINT16 uiLevel
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PDS_ROLE_INFO pInfo = NULL;

    BAIL_ON_NULL_PARAM(ppOut);

    status = DsrAllocateMemory((void**)&pInfo,
                               sizeof(*pInfo),
                               NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    if (!pIn) goto cleanup;

    switch(uiLevel) {
    case DS_ROLE_BASIC_INFORMATION:
        status = DsrAllocateDsRoleInfoBasic(&pInfo->basic,
                                            &pIn->basic,
                                            pInfo);
        break;

    case DS_ROLE_UPGRADE_STATUS:
        pInfo->upgrade.uiUpgradeStatus = pIn->upgrade.uiUpgradeStatus;
        pInfo->upgrade.uiPrevious      = pIn->upgrade.uiPrevious;
        break;

    case DS_ROLE_OP_STATUS:
        pInfo->opstatus.uiStatus = pIn->opstatus.uiStatus;
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    BAIL_ON_NTSTATUS_ERROR(status);

    *ppOut = pInfo;

cleanup:
    return status;

error:
    if (pInfo) {
        DsrFreeMemory(pInfo);
    }

    *ppOut = NULL;
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
