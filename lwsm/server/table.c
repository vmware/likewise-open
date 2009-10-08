/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        table.c
 *
 * Abstract:
 *
 *        Object table logic
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

static
DWORD
LwSmTablePollEntry(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_STATUS pStatus
    );

static
DWORD
LwSmTableVerifyAndMarkDependencies(
    PSM_TABLE_ENTRY pEntry
    );

static
DWORD
LwSmTableUnmarkDependencies(
    PSM_TABLE_ENTRY pEntry
    );

static
VOID
LwSmTableFreeEntry(
    PSM_TABLE_ENTRY pEntry
    );

static SM_TABLE gServiceTable = 
{
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .pLock = &gServiceTable.lock,
    .entries = {&gServiceTable.entries, &gServiceTable.entries}
};

static PSM_OBJECT_VTBL gVtblTable[] =
{
    [LW_SERVICE_EXECUTABLE] = &gExecutableVtbl,
    [LW_SERVICE_SM_EXECUTABLE] = &gExecutableVtbl,
    [LW_SERVICE_DRIVER] = &gDriverVtbl
};

DWORD
LwSmTableGetEntry(
    PCWSTR pwszName,
    PSM_TABLE_ENTRY* ppEntry
    )
{
    DWORD dwError = 0;
    BOOL bLocked = FALSE;
    PSM_LINK pLink = NULL;
    PSM_TABLE_ENTRY pEntry = NULL;

    LOCK(bLocked, gServiceTable.pLock);

    for (pLink = LwSmLinkBegin(&gServiceTable.entries);
         LwSmLinkValid(&gServiceTable.entries, pLink);
         pLink = LwSmLinkNext(pLink))
    {
        pEntry = STRUCT_FROM_MEMBER(pLink, SM_TABLE_ENTRY, link);
        
        if (LwRtlWC16StringIsEqual(pEntry->pInfo->pwszName, pwszName, TRUE))
        {
            pEntry->dwRefCount++;
            *ppEntry = pEntry;
            goto cleanup;
        }
    }

    dwError = LW_ERROR_NO_SUCH_SERVICE;
    BAIL_ON_ERROR(dwError);

cleanup:

    UNLOCK(bLocked, gServiceTable.pLock);

    return dwError;

error:

    *ppEntry = NULL;

    goto cleanup;
}

DWORD
LwSmTableEnumerateEntries(
    PWSTR** pppwszServiceNames
    )
{
    DWORD dwError = 0;
    BOOL bLocked = FALSE;
    PSM_LINK pLink = NULL;
    PSM_TABLE_ENTRY pEntry = NULL;
    size_t count = 0;
    size_t i = 0;
    PWSTR* ppwszServiceNames = NULL;

    LOCK(bLocked, gServiceTable.pLock);

    for (pLink = NULL; (pLink = SM_LINK_ITERATE(&gServiceTable.entries, pLink));)
    {
        count++;
    }

    dwError = LwAllocateMemory(
        sizeof(*ppwszServiceNames) * (count + 1),
        OUT_PPVOID(&ppwszServiceNames));
    BAIL_ON_ERROR(dwError);

    for (pLink = NULL, i = 0; (pLink = SM_LINK_ITERATE(&gServiceTable.entries, pLink)); i++)
    {
        pEntry = STRUCT_FROM_MEMBER(pLink, SM_TABLE_ENTRY, link);

        dwError = LwAllocateWc16String(&ppwszServiceNames[i], pEntry->pInfo->pwszName);
        BAIL_ON_ERROR(dwError);
    }

    *pppwszServiceNames = ppwszServiceNames;

cleanup:

    UNLOCK(bLocked, gServiceTable.pLock);

    return dwError;

error:

    *pppwszServiceNames = NULL;

    if (ppwszServiceNames)
    {
        LwSmFreeStringList(ppwszServiceNames);
    }

    goto cleanup;
}

DWORD
LwSmTableAddEntry(
    PLW_SERVICE_INFO pInfo,
    PSM_TABLE_ENTRY* ppEntry
    )
{
    DWORD dwError = 0;
    BOOL bLocked = TRUE;
    PSM_TABLE_ENTRY pEntry = NULL;

    dwError = LwAllocateMemory(sizeof(*pEntry), OUT_PPVOID(&pEntry));
    BAIL_ON_ERROR(dwError);

    pEntry->bValid = TRUE;

    dwError = LwSmCopyServiceInfo(pInfo, &pEntry->pInfo);
    
    dwError = LwMapErrnoToLwError(pthread_mutex_init(&pEntry->lock, NULL));
    BAIL_ON_ERROR(dwError);
    pEntry->pLock = &pEntry->lock;

    dwError = LwMapErrnoToLwError(pthread_cond_init(&pEntry->event, NULL));
    BAIL_ON_ERROR(dwError);
    pEntry->pEvent = &pEntry->event;

    pEntry->pVtbl = gVtblTable[pEntry->pInfo->type];

    dwError = pEntry->pVtbl->pfnConstruct(pEntry);
    BAIL_ON_ERROR(dwError);

    LOCK(bLocked, gServiceTable.pLock);

    LwSmLinkInsertBefore(&gServiceTable.entries, &pEntry->link);

    pEntry->dwRefCount++;

    UNLOCK(bLocked, gServiceTable.pLock);

    *ppEntry = pEntry;

cleanup:

    return dwError;

error:

    if (pEntry)
    {
        LwSmTableFreeEntry(pEntry);
    }

    goto cleanup;
}

DWORD
LwSmTableUpdateEntry(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_INFO pInfo
    )
{
    DWORD dwError = 0;
    BOOL bLocked = FALSE;
    BOOL bTableLocked = FALSE;
    PLW_SERVICE_INFO pInfoCopy = NULL;

    dwError = LwSmCopyServiceInfo(pInfo, &pInfoCopy);
    BAIL_ON_ERROR(dwError);

    LOCK(bLocked, pEntry->pLock);
    LOCK(bTableLocked, gServiceTable.pLock);

    LwSmCommonFreeServiceInfo(pEntry->pInfo);
    pEntry->pInfo = pInfoCopy;
    pInfoCopy = NULL;

cleanup:

    if (pInfoCopy)
    {
        LwSmCommonFreeServiceInfo(pInfoCopy);
    }

    UNLOCK(bTableLocked, gServiceTable.pLock);
    UNLOCK(bLocked, pEntry->pLock);

    return dwError;

error:

    goto cleanup;
}

VOID
LwSmTableRetainEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    BOOL bTableLocked = FALSE;

    LOCK(bTableLocked, gServiceTable.pLock);

    ++pEntry->dwRefCount;
    
    UNLOCK(bTableLocked, gServiceTable.pLock);
}

VOID
LwSmTableReleaseEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    BOOL bEntryLocked = FALSE;
    BOOL bTableLocked = FALSE;

    LOCK(bEntryLocked, pEntry->pLock);
    LOCK(bTableLocked, gServiceTable.pLock);

    if (--pEntry->dwRefCount == 0 && !pEntry->bValid)
    {
        UNLOCK(bEntryLocked, pEntry->pLock);
        LwSmTableFreeEntry(pEntry);
    }

    UNLOCK(bTableLocked, gServiceTable.pLock);
    UNLOCK(bEntryLocked, pEntry->pLock);
}

static
VOID
LwSmTableFreeEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    if (pEntry->pVtbl)
    {
        pEntry->pVtbl->pfnDestruct(pEntry);
    }

    if (pEntry->pInfo)
    {
        LwSmCommonFreeServiceInfo(pEntry->pInfo);
    }

    if (pEntry->pLock)
    {
        pthread_mutex_destroy(pEntry->pLock);
    }

    if (pEntry->pEvent)
    {
        pthread_cond_destroy(pEntry->pEvent);
    }
}

DWORD
LwSmTableStartEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    LW_SERVICE_STATUS status = LW_SERVICE_DEAD;
    DWORD dwAttempts = 0;

    LOCK(bLocked, pEntry->pLock);

    if (!pEntry->bValid)
    {
        dwError = LW_ERROR_INVALID_HANDLE;
        BAIL_ON_ERROR(dwError);
    }

    if (!pEntry->bDepsMarked)
    {
        dwError = LwSmTableVerifyAndMarkDependencies(pEntry);
        BAIL_ON_ERROR(dwError);
        pEntry->bDepsMarked = TRUE;
    }

    while (status != LW_SERVICE_RUNNING)
    {
        dwError = LwSmTablePollEntry(pEntry, &status);
        BAIL_ON_ERROR(dwError);

        switch (status)
        {
        case LW_SERVICE_RUNNING:
            break;
        case LW_SERVICE_STOPPED:
        case LW_SERVICE_DEAD:
            if (dwAttempts == 0)
            {
                dwError = pEntry->pVtbl->pfnStart(pEntry);
                BAIL_ON_ERROR(dwError);
                dwAttempts++;
            }
            else
            {
                dwError = LW_ERROR_SERVICE_UNRESPONSIVE;
                BAIL_ON_ERROR(dwError);
            }
            break;
        case LW_SERVICE_STARTING:
        case LW_SERVICE_STOPPING:
            dwError = LwSmTableWaitEntryChanged(pEntry);
            BAIL_ON_ERROR(dwError);
            break;
        case LW_SERVICE_PAUSED:
            dwError = LW_ERROR_INVALID_SERVICE_TRANSITION;
            BAIL_ON_ERROR(dwError);
            break;
        }
    }

cleanup:

    UNLOCK(bLocked, pEntry->pLock);

    return dwError;

error:

    if (pEntry->bDepsMarked)
    {
        LwSmTableUnmarkDependencies(pEntry);
        pEntry->bDepsMarked = FALSE;
    }

    goto cleanup;
}

static
DWORD
LwSmTableVerifyAndMarkDependencies(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    DWORD dwMaxIndex = 0;
    PSM_TABLE_ENTRY pDependency = NULL;
    LW_SERVICE_STATUS status = LW_SERVICE_DEAD;
    BOOLEAN bLocked = FALSE;

    for (dwIndex = 0; pEntry->pInfo->ppwszDependencies[dwIndex]; dwIndex++)
    {
        dwError = LwSmTableGetEntry(
            pEntry->pInfo->ppwszDependencies[dwIndex],
            &pDependency);
        if (dwError == LW_ERROR_NO_SUCH_SERVICE)
        {
            dwError = LW_ERROR_SERVICE_DEPENDENCY_UNMET;
        }
        BAIL_ON_ERROR(dwError);

        LOCK(bLocked, pDependency->pLock);

        if (!pDependency->bValid)
        {
            dwError = LW_ERROR_SERVICE_DEPENDENCY_UNMET;
            BAIL_ON_ERROR(dwError);
        }

        dwError = LwSmTablePollEntry(pDependency, &status);
        BAIL_ON_ERROR(dwError);

        if (status != LW_SERVICE_RUNNING &&
            status != LW_SERVICE_PAUSED)
        {
            dwError = LW_ERROR_SERVICE_DEPENDENCY_UNMET;
            BAIL_ON_ERROR(dwError);
        }

        /* Mark running dependent service */
        pDependency->dwDepCount++;

        UNLOCK(bLocked, pDependency->pLock);
        
        LwSmTableReleaseEntry(pDependency);
        pDependency = NULL;
    }

cleanup:

    return dwError;
    
error:

    UNLOCK(bLocked, pDependency->pLock);

    if (pDependency)
    {
        LwSmTableReleaseEntry(pDependency);
    }

    dwMaxIndex = dwIndex;

    /* On error, unmark any services we have already marked */
    for (dwIndex = 0; dwIndex < dwMaxIndex; dwIndex++)
    {
        if (LwSmTableGetEntry(
                pEntry->pInfo->ppwszDependencies[dwIndex],
                &pDependency) == 0)
        {
            LOCK(bLocked, pDependency->pLock);
            pDependency->dwDepCount--;
            UNLOCK(bLocked, pDependency->pLock);
            LwSmTableReleaseEntry(pDependency);
            pDependency = NULL;
        }
    }

    goto cleanup;
}

DWORD
LwSmTableStopEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    LW_SERVICE_STATUS status = LW_SERVICE_RUNNING;
    DWORD dwAttempts = 0;

    LOCK(bLocked, pEntry->pLock);

    if (!pEntry->bValid)
    {
        dwError = LW_ERROR_INVALID_HANDLE;
        BAIL_ON_ERROR(dwError);
    }

    if (pEntry->dwDepCount > 0)
    {
        dwError = LW_ERROR_DEPENDENT_SERVICE_STILL_RUNNING;
        BAIL_ON_ERROR(dwError);
    }

    while (status != LW_SERVICE_STOPPED)
    {
        dwError = LwSmTablePollEntry(pEntry, &status);
        BAIL_ON_ERROR(dwError);

        switch (status)
        {
        case LW_SERVICE_RUNNING:
        case LW_SERVICE_DEAD:
            /* A service that is dead should go directly
               to the stop state when requested */
            if (dwAttempts == 0)
            {
                dwError = pEntry->pVtbl->pfnStop(pEntry);
                BAIL_ON_ERROR(dwError);
                dwAttempts++;
            }
            else
            {
                dwError = LW_ERROR_SERVICE_UNRESPONSIVE;
                BAIL_ON_ERROR(dwError);
            }
            break;
        case LW_SERVICE_STOPPED:
            break;
        case LW_SERVICE_STARTING:
        case LW_SERVICE_STOPPING:
            dwError = LwSmTableWaitEntryChanged(pEntry);
            BAIL_ON_ERROR(dwError);
            break;
        case LW_SERVICE_PAUSED:
            dwError = LW_ERROR_INVALID_SERVICE_TRANSITION;
            BAIL_ON_ERROR(dwError);
            break;
        }
    }

    if (pEntry->bDepsMarked)
    {
        dwError = LwSmTableUnmarkDependencies(pEntry);
        BAIL_ON_ERROR(dwError);
        pEntry->bDepsMarked = FALSE;
    }

cleanup:

    UNLOCK(bLocked, pEntry->pLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmTableRefreshEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    LW_SERVICE_STATUS status = LW_SERVICE_RUNNING;

    LOCK(bLocked, pEntry->pLock);

    if (!pEntry->bValid)
    {
        dwError = LW_ERROR_INVALID_HANDLE;
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwSmTablePollEntry(pEntry, &status);
    BAIL_ON_ERROR(dwError);

    switch (status)
    {
    case LW_SERVICE_RUNNING:
        dwError = pEntry->pVtbl->pfnRefresh(pEntry);
        BAIL_ON_ERROR(dwError);
        break;
    default:
        break;
    }

cleanup:

    UNLOCK(bLocked, pEntry->pLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmTableGetEntryStatus(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_STATUS pStatus
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, pEntry->pLock);
    
    if (!pEntry->bValid)
    {
        dwError = LW_ERROR_INVALID_HANDLE;
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwSmTablePollEntry(pEntry, pStatus);
    BAIL_ON_ERROR(dwError);
    
error:
    
    UNLOCK(bLocked, pEntry->pLock);

    return dwError;
}

DWORD
LwSmTableGetEntryProcess(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_PROCESS pProcess,
    pid_t* pPid
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, pEntry->pLock);

    if (!pEntry->bValid)
    {
        dwError = LW_ERROR_INVALID_HANDLE;
        BAIL_ON_ERROR(dwError);
    }

    dwError = pEntry->pVtbl->pfnGetProcess(pEntry, pProcess, pPid);
    BAIL_ON_ERROR(dwError);

error:

    UNLOCK(bLocked, pEntry->pLock);

    return dwError;
}

static
DWORD
LwSmTablePollEntry(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_STATUS pStatus
    )
{
    DWORD dwError = 0;

    dwError = pEntry->pVtbl->pfnGetStatus(pEntry, pStatus);
    BAIL_ON_ERROR(dwError);

    if ((*pStatus == LW_SERVICE_STOPPED ||
         *pStatus == LW_SERVICE_DEAD) &&
        pEntry->bDepsMarked)
    {
        dwError = LwSmTableUnmarkDependencies(pEntry);
        BAIL_ON_ERROR(dwError);
        pEntry->bDepsMarked = FALSE;
    }
    else if ((*pStatus != LW_SERVICE_STOPPED &&
              *pStatus != LW_SERVICE_DEAD) &&
             !pEntry->bDepsMarked)
    {
        dwError = LwSmTableVerifyAndMarkDependencies(pEntry);
        BAIL_ON_ERROR(dwError);
        pEntry->bDepsMarked = TRUE;
    }

error:

    return dwError;
}

static
DWORD
LwSmTableUnmarkDependencies(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;
    PSM_TABLE_ENTRY pDependency = NULL;
    BOOLEAN bLocked = FALSE;

    for (dwIndex = 0; pEntry->pInfo->ppwszDependencies[dwIndex]; dwIndex++)
    {
        dwError = LwSmTableGetEntry(
            pEntry->pInfo->ppwszDependencies[dwIndex],
            &pDependency);
        if (dwError == LW_ERROR_NOT_MAPPED)
        {
            dwError = 0;
        }
        BAIL_ON_ERROR(dwError);

        LOCK(bLocked, pDependency->pLock);

        pDependency->dwDepCount--;

        UNLOCK(bLocked, pDependency->pLock);
        
        LwSmTableReleaseEntry(pDependency);
        pDependency = NULL;
    }

cleanup:

    return dwError;
    
error:

    UNLOCK(bLocked, pDependency->pLock);

    if (pDependency)
    {
        LwSmTableReleaseEntry(pDependency);
    }

    goto cleanup;
}

VOID
LwSmTableNotifyEntryChanged(
    PSM_TABLE_ENTRY pEntry
    )
{
    pthread_cond_broadcast(pEntry->pEvent);
}

DWORD
LwSmTableWaitEntryChanged(
    PSM_TABLE_ENTRY pEntry
    )
{
    if (!pEntry->bValid)
    {
        return LW_ERROR_INVALID_HANDLE;
    }

    pthread_cond_wait(pEntry->pEvent, pEntry->pLock);

    if (!pEntry->bValid)
    {
        return LW_ERROR_INVALID_HANDLE;
    }

    return 0;
}

static
DWORD
LwSmTableGetEntryDependencyClosureHelper(
    PSM_TABLE_ENTRY pEntry,
    PWSTR** pppwszServiceList
    )
{
    DWORD dwError = 0;
    PLW_SERVICE_INFO pInfo = NULL;
    PSM_TABLE_ENTRY pDepEntry = NULL;
    PWSTR pwszDepName = NULL;
    size_t i = 0;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, pEntry->pLock);
    dwError = LwSmCopyServiceInfo(pEntry->pInfo, &pInfo);
    UNLOCK(bLocked, pEntry->pLock);
    BAIL_ON_ERROR(dwError);

    for (i = 0; pInfo->ppwszDependencies[i]; i++)
    {
        dwError = LwSmTableGetEntry(pInfo->ppwszDependencies[i], &pDepEntry);
        BAIL_ON_ERROR(dwError);

        dwError = LwSmTableGetEntryDependencyClosureHelper(pDepEntry, pppwszServiceList);
        BAIL_ON_ERROR(dwError);

        if (!LwSmStringListContains(*pppwszServiceList, pInfo->ppwszDependencies[i]))
        {
            dwError = LwAllocateWc16String(&pwszDepName,  pInfo->ppwszDependencies[i]);
            BAIL_ON_ERROR(dwError);

            dwError = LwSmStringListAppend(pppwszServiceList, pwszDepName);
            BAIL_ON_ERROR(dwError);

            pwszDepName = NULL;
        }
        
        LwSmTableReleaseEntry(pDepEntry);
        pDepEntry = NULL;
    }

cleanup:
    
    LW_SAFE_FREE_MEMORY(pwszDepName);
    
    if (pInfo)
    {
        LwSmCommonFreeServiceInfo(pInfo);
    }

    if (pDepEntry)
    {
        LwSmTableReleaseEntry(pDepEntry);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmTableGetEntryDependencyClosure(
    PSM_TABLE_ENTRY pEntry,
    PWSTR** pppwszServiceList
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszServiceList = NULL;

    dwError = LwAllocateMemory(sizeof(*ppwszServiceList) * 1, OUT_PPVOID(&ppwszServiceList));
    BAIL_ON_ERROR(dwError);

    dwError = LwSmTableGetEntryDependencyClosureHelper(pEntry, &ppwszServiceList);
    BAIL_ON_ERROR(dwError);

    *pppwszServiceList = ppwszServiceList;

cleanup:

    return dwError;

error:

    *pppwszServiceList = NULL;

    if (ppwszServiceList)
    {
        LwSmFreeStringList(ppwszServiceList);
    }

    goto cleanup;
}

static
DWORD
LwSmTableGetEntryReverseDependencyClosureHelper(
    PSM_TABLE_ENTRY pEntry,
    PWSTR* ppwszAllServices,
    PWSTR** pppwszServiceList
    )
{
    DWORD dwError = 0;
    PLW_SERVICE_INFO pInfo = NULL;
    PLW_SERVICE_INFO pDepInfo = NULL;
    size_t i = 0;
    PSM_TABLE_ENTRY pDepEntry = NULL;
    PWSTR pwszDepName = NULL;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, pEntry->pLock);
    dwError = LwSmCopyServiceInfo(pEntry->pInfo, &pInfo);
    UNLOCK(bLocked, pEntry->pLock);
    BAIL_ON_ERROR(dwError);

    for (i = 0; ppwszAllServices[i]; i++)
    {
        dwError = LwSmTableGetEntry(ppwszAllServices[i], &pDepEntry);
        BAIL_ON_ERROR(dwError);

        LOCK(bLocked, pEntry->pLock);
        dwError = LwSmCopyServiceInfo(pDepEntry->pInfo, &pDepInfo);
        UNLOCK(bLocked, pEntry->pLock);
        BAIL_ON_ERROR(dwError);

        if (LwSmStringListContains(pDepInfo->ppwszDependencies, pInfo->pwszName))
        {
            dwError = LwSmTableGetEntryReverseDependencyClosureHelper(
                pDepEntry,
                ppwszAllServices,
                pppwszServiceList);
            BAIL_ON_ERROR(dwError);
            
            dwError = LwAllocateWc16String(&pwszDepName, pDepInfo->pwszName);
            BAIL_ON_ERROR(dwError);

            dwError = LwSmStringListAppend(pppwszServiceList, pwszDepName);
            BAIL_ON_ERROR(dwError);

            pwszDepName = NULL;
        }

        LwSmCommonFreeServiceInfo(pDepInfo);
        pDepInfo = NULL;

        LwSmTableReleaseEntry(pDepEntry);
        pDepEntry = NULL;
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pwszDepName);

    if (pInfo)
    {
        LwSmCommonFreeServiceInfo(pInfo);
    }

    if (pDepInfo)
    {
        LwSmCommonFreeServiceInfo(pInfo);
    }

    if (pDepEntry)
    {
        LwSmTableReleaseEntry(pDepEntry);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LwSmTableGetEntryReverseDependencyClosure(
    PSM_TABLE_ENTRY pEntry,
    PWSTR** pppwszServiceList
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszServiceList = NULL;
    PWSTR* ppwszAllServices = NULL;

    dwError = LwAllocateMemory(sizeof(*ppwszServiceList) * 1, OUT_PPVOID(&ppwszServiceList));
    BAIL_ON_ERROR(dwError);

    dwError = LwSmTableEnumerateEntries(&ppwszAllServices);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmTableGetEntryReverseDependencyClosureHelper(
        pEntry,
        ppwszAllServices,
        &ppwszServiceList);
    BAIL_ON_ERROR(dwError);

    *pppwszServiceList = ppwszServiceList;

cleanup:

    if (ppwszAllServices)
    {
        LwSmFreeStringList(ppwszAllServices);
    }

    return dwError;

error:

    *pppwszServiceList = NULL;

    if (ppwszServiceList)
    {
        LwSmFreeStringList(ppwszServiceList);
    }

    goto cleanup;
}
