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

/* File globals */

static PVOID gpDsrMemoryList = NULL;
static pthread_mutex_t gDsrDataMutex = PTHREAD_MUTEX_INITIALIZER;
static BOOLEAN bDsrInitialised = FALSE;

/* Code */

NTSTATUS
DsrInitMemory(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;

    LIBRPC_LOCK_MUTEX(bLocked, &gDsrDataMutex);

    if (!bDsrInitialised)
    {
        ntStatus = MemPtrListInit((PtrList**)&gpDsrMemoryList);
        BAIL_ON_NT_STATUS(ntStatus);

        bDsrInitialised = TRUE;
    }

cleanup:
    LIBRPC_UNLOCK_MUTEX(bLocked, &gDsrDataMutex);

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
DsrDestroyMemory(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;

    LIBRPC_LOCK_MUTEX(bLocked, &gDsrDataMutex);

    if (bDsrInitialised && gpDsrMemoryList)
    {
        ntStatus = MemPtrListDestroy((PtrList**)&gpDsrMemoryList);
        BAIL_ON_NT_STATUS(ntStatus);

        bDsrInitialised = 0;
    }

cleanup:
    LIBRPC_UNLOCK_MUTEX(bLocked, &gDsrDataMutex);

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
DsrAllocateMemory(
    OUT PVOID *ppOutBuffer,
    IN  size_t Size,
    IN  PVOID pDependent
    )
{
    return MemPtrAllocate(
               (PtrList*)gpDsrMemoryList,
               ppOutBuffer,
               Size,
               pDependent);
}


VOID
DsrFreeMemory(
    IN OUT PVOID pBuffer
    )
{
    if (pBuffer == NULL)
    {
        return;
    }

    MemPtrFree((PtrList*)gpDsrMemoryList, pBuffer);
}


NTSTATUS
DsrAddDepMemory(
    IN PVOID pBuffer,
    IN PVOID pDependent
    )
{
    return MemPtrAddDependant(
               (PtrList*)gpDsrMemoryList,
               pBuffer,
               pDependent);
}


static
NTSTATUS
DsrAllocateDsRoleInfoBasic(
    OUT PDS_ROLE_PRIMARY_DOMAIN_INFO_BASIC pOut,
    IN  PDS_ROLE_PRIMARY_DOMAIN_INFO_BASIC pIn,
    IN  PVOID pDependent
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwDomainLen = 0;
    DWORD dwDnsDomainLen = 0;
    DWORD dwForestLen = 0;

    pOut->uiRole  = pIn->uiRole;
    pOut->uiFlags = pIn->uiFlags;

    if (pIn->pwszDomain)
    {
        dwDomainLen = RtlWC16StringNumChars(pIn->pwszDomain);

        ntStatus = DsrAllocateMemory(
                       (PVOID*)&pOut->pwszDomain,
                       (dwDomainLen + 1) * sizeof(WCHAR),
                       pDependent);
        BAIL_ON_NT_STATUS(ntStatus);

        wc16sncpy(pOut->pwszDomain, pIn->pwszDomain, dwDomainLen);
    }

    if (pIn->pwszDnsDomain)
    {
        dwDnsDomainLen = RtlWC16StringNumChars(pIn->pwszDnsDomain);

        ntStatus = DsrAllocateMemory(
                       (PVOID*)&pOut->pwszDnsDomain,
                       (dwDnsDomainLen + 1) * sizeof(WCHAR),
                       pDependent);
        BAIL_ON_NT_STATUS(ntStatus);

        wc16sncpy(pOut->pwszDnsDomain, pIn->pwszDnsDomain, dwDnsDomainLen);
    }

    if (pIn->pwszForest)
    {
        dwForestLen = RtlWC16StringNumChars(pIn->pwszForest);

        ntStatus = DsrAllocateMemory(
                       (PVOID*)&pOut->pwszForest,
                       (dwForestLen + 1) * sizeof(WCHAR),
                       pDependent);
        BAIL_ON_NT_STATUS(ntStatus);

        wc16sncpy(pOut->pwszForest, pIn->pwszForest, dwForestLen);
    }

    memcpy(&pOut->DomainGuid,
           &pIn->DomainGuid,
           sizeof(pOut->DomainGuid));

cleanup:
    return ntStatus;

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
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDS_ROLE_INFO pInfo = NULL;

    BAIL_ON_NULL_PTR(ppOut, ntStatus);

    ntStatus = DsrAllocateMemory(
                   (PVOID*)&pInfo,
                   sizeof(*pInfo),
                   NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn == NULL)
    {
        goto cleanup;
    }

    switch(uiLevel)
    {
    case DS_ROLE_BASIC_INFORMATION:
        ntStatus = DsrAllocateDsRoleInfoBasic(
                       &pInfo->basic,
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
        ntStatus = STATUS_INVALID_PARAMETER;
        break;
    }

    BAIL_ON_NT_STATUS(ntStatus);

    *ppOut = pInfo;
    pInfo = NULL;

cleanup:
    return ntStatus;

error:
    DsrFreeMemory(pInfo);

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
