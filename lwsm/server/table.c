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

#define RESTART_PERIOD 30
#define RESTART_LIMIT 2

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

static PLW_THREAD_POOL gpPool = NULL;

static SM_TABLE gServiceTable = 
{
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .pLock = &gServiceTable.lock,
    .entries = {&gServiceTable.entries, &gServiceTable.entries}
};

static PCSTR gLoaderTable[] =
{
    [LW_SERVICE_TYPE_LEGACY_EXECUTABLE] = "executable",
    [LW_SERVICE_TYPE_EXECUTABLE] = "executable",
    [LW_SERVICE_TYPE_DRIVER] = "driver",
    [LW_SERVICE_TYPE_MODULE] = "svcm",
    [LW_SERVICE_TYPE_STUB] = "stub"
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

static
DWORD
LwSmTableReconstructEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    PWSTR pwszLoaderName = NULL;

    if (pEntry->object.pData)
    {
        pEntry->pVtbl->pfnDestruct(&pEntry->object);
        pEntry->object.pData = NULL;
    }

    dwError = LwMbsToWc16s(gLoaderTable[pEntry->pInfo->type], &pwszLoaderName);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmLoaderGetVtbl(pwszLoaderName, &pEntry->pVtbl);
    BAIL_ON_ERROR(dwError);

    dwError = pEntry->pVtbl->pfnConstruct(&pEntry->object, pEntry->pInfo, &pEntry->object.pData);
    BAIL_ON_ERROR(dwError);

    pEntry->bDirty = FALSE;

error:

    LW_SAFE_FREE_MEMORY(pwszLoaderName);

    return dwError;
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

    LwSmLinkInit(&pEntry->link);
    LwSmLinkInit(&pEntry->waiters);

    pEntry->bValid = TRUE;

    dwError = LwSmCopyServiceInfo(pInfo, &pEntry->pInfo);
    
    dwError = LwMapErrnoToLwError(pthread_mutex_init(&pEntry->lock, NULL));
    BAIL_ON_ERROR(dwError);
    pEntry->pLock = &pEntry->lock;

    dwError = LwMapErrnoToLwError(pthread_cond_init(&pEntry->event, NULL));
    BAIL_ON_ERROR(dwError);
    pEntry->pEvent = &pEntry->event;

    dwError = LwSmTableReconstructEntry(pEntry);
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
    PCLW_SERVICE_INFO pInfo,
    LW_SERVICE_INFO_MASK mask
    )
{
    DWORD dwError = 0;
    BOOL bLocked = FALSE;
    BOOL bTableLocked = FALSE;
    PLW_SERVICE_INFO pUpdate = NULL;

    LOCK(bLocked, pEntry->pLock);
    /* We must also hold the service table lock to prevent
       concurrent access to pEntry->pInfo by LwSmTableGetEntry */
    LOCK(bTableLocked, gServiceTable.pLock);

    dwError = LwAllocateMemory(sizeof(*pUpdate), OUT_PPVOID(&pUpdate));
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyString(
        mask & LW_SERVICE_INFO_MASK_NAME ? pInfo->pwszName : pEntry->pInfo->pwszName,
        &pUpdate->pwszName);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyString(
        mask & LW_SERVICE_INFO_MASK_PATH ? pInfo->pwszPath : pEntry->pInfo->pwszPath,
        &pUpdate->pwszPath);

    dwError = LwSmCopyString(
        mask & LW_SERVICE_INFO_MASK_DESCRIPTION ? pInfo->pwszDescription : pEntry->pInfo->pwszDescription,
        &pUpdate->pwszDescription);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyStringList(
        mask & LW_SERVICE_INFO_MASK_ARGS ? pInfo->ppwszArgs : pEntry->pInfo->ppwszArgs,
        &pUpdate->ppwszArgs);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyStringList(
        mask & LW_SERVICE_INFO_MASK_ENVIRONMENT ? pInfo->ppwszEnv : pEntry->pInfo->ppwszEnv,
        &pUpdate->ppwszEnv);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmCopyStringList(
        mask & LW_SERVICE_INFO_MASK_DEPENDENCIES ? pInfo->ppwszDependencies : pEntry->pInfo->ppwszDependencies,
        &pUpdate->ppwszDependencies);
    BAIL_ON_ERROR(dwError);

    pUpdate->type = mask & LW_SERVICE_INFO_MASK_TYPE ? pInfo->type : pEntry->pInfo->type;
    pUpdate->bAutostart = mask & LW_SERVICE_INFO_MASK_AUTOSTART ? pInfo->bAutostart : pEntry->pInfo->bAutostart;
    pUpdate->dwFdLimit = mask & LW_SERVICE_INFO_MASK_AUTOSTART ? pInfo->dwFdLimit : pEntry->pInfo->dwFdLimit;

    /* Atomically replace previous info structure */
    LwSmCommonFreeServiceInfo(pEntry->pInfo);
    pEntry->pInfo = pUpdate;
    pUpdate = NULL;

    pEntry->bDirty = TRUE;

cleanup:

    if (pUpdate)
    {
        LwSmCommonFreeServiceInfo(pUpdate);
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
        pEntry->pVtbl->pfnDestruct(&pEntry->object);
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

    LwFreeMemory(pEntry);
}

DWORD
LwSmTableStartEntry(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    LW_SERVICE_STATUS status = {.state = LW_SERVICE_STATE_DEAD};
    DWORD dwAttempts = 0;
    PSTR pszServiceName = NULL;

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

    while (status.state != LW_SERVICE_STATE_RUNNING)
    {
        dwError = LwSmTablePollEntry(pEntry, &status);
        BAIL_ON_ERROR(dwError);

        switch (status.state)
        {
        case LW_SERVICE_STATE_RUNNING:
            break;
        case LW_SERVICE_STATE_STOPPED:
        case LW_SERVICE_STATE_DEAD:
            if (dwAttempts == 0)
            {
                LW_SAFE_FREE_MEMORY(pszServiceName);

                dwError = LwWc16sToMbs(pEntry->pInfo->pwszName, &pszServiceName);
                BAIL_ON_ERROR(dwError);

                SM_LOG_INFO("Starting service: %s", pszServiceName);

                if (pEntry->bDirty)
                {
                    dwError = LwSmTableReconstructEntry(pEntry);
                    BAIL_ON_ERROR(dwError);
                }

                UNLOCK(bLocked, pEntry->pLock);
                dwError = pEntry->pVtbl->pfnStart(&pEntry->object);
                LOCK(bLocked, pEntry->pLock);
                BAIL_ON_ERROR(dwError);
                dwAttempts++;
            }
            else
            {
                dwError = LW_ERROR_SERVICE_UNRESPONSIVE;
                BAIL_ON_ERROR(dwError);
            }
            break;
        case LW_SERVICE_STATE_STARTING:
        case LW_SERVICE_STATE_STOPPING:
            dwError = LwSmTableWaitEntryChanged(pEntry);
            BAIL_ON_ERROR(dwError);
            break;
        case LW_SERVICE_STATE_PAUSED:
            dwError = LW_ERROR_INVALID_SERVICE_TRANSITION;
            BAIL_ON_ERROR(dwError);
            break;
        }
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pszServiceName);

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
    LW_SERVICE_STATUS status = {.state = LW_SERVICE_STATE_DEAD};
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

        if (status.state != LW_SERVICE_STATE_RUNNING &&
            status.state != LW_SERVICE_STATE_PAUSED)
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
    LW_SERVICE_STATUS status = {.state = LW_SERVICE_STATE_RUNNING};
    DWORD dwAttempts = 0;
    PSTR pszServiceName = NULL;

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

    while (status.state != LW_SERVICE_STATE_STOPPED)
    {
        dwError = LwSmTablePollEntry(pEntry, &status);
        BAIL_ON_ERROR(dwError);

        switch (status.state)
        {
        case LW_SERVICE_STATE_RUNNING:
        case LW_SERVICE_STATE_DEAD:
            /* A service that is dead should go directly
               to the stop state when requested */
            if (dwAttempts == 0)
            {
                LW_SAFE_FREE_MEMORY(pszServiceName);

                dwError = LwWc16sToMbs(pEntry->pInfo->pwszName, &pszServiceName);
                BAIL_ON_ERROR(dwError);

                SM_LOG_INFO("Stopping service: %s", pszServiceName);

                UNLOCK(bLocked, pEntry->pLock);
                dwError = pEntry->pVtbl->pfnStop(&pEntry->object);
                LOCK(bLocked, pEntry->pLock);
                BAIL_ON_ERROR(dwError);
                dwAttempts++;
            }
            else
            {
                dwError = LW_ERROR_SERVICE_UNRESPONSIVE;
                BAIL_ON_ERROR(dwError);
            }
            break;
        case LW_SERVICE_STATE_STOPPED:
            break;
        case LW_SERVICE_STATE_STARTING:
        case LW_SERVICE_STATE_STOPPING:
            dwError = LwSmTableWaitEntryChanged(pEntry);
            BAIL_ON_ERROR(dwError);
            break;
        case LW_SERVICE_STATE_PAUSED:
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

    LW_SAFE_FREE_MEMORY(pszServiceName);

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
    LW_SERVICE_STATUS status = {.state = LW_SERVICE_STATE_DEAD};

    LOCK(bLocked, pEntry->pLock);

    if (!pEntry->bValid)
    {
        dwError = LW_ERROR_INVALID_HANDLE;
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwSmTablePollEntry(pEntry, &status);
    BAIL_ON_ERROR(dwError);

    switch (status.state)
    {
    case LW_SERVICE_STATE_RUNNING:
        UNLOCK(bLocked, pEntry->pLock);
        dwError = pEntry->pVtbl->pfnRefresh(&pEntry->object);
        LOCK(bLocked, pEntry->pLock);
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

static
DWORD
LwSmTablePollEntry(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_STATUS pStatus
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = TRUE;

    UNLOCK(bLocked, pEntry->pLock);
    dwError = pEntry->pVtbl->pfnGetStatus(&pEntry->object, pStatus);
    LOCK(bLocked, pEntry->pLock);
    BAIL_ON_ERROR(dwError);

    /* If an unannounced change in the service status occured,
       we may need to unmark or mark dependencies */
    if ((pStatus->state == LW_SERVICE_STATE_STOPPED ||
         pStatus->state == LW_SERVICE_STATE_DEAD) &&
        pEntry->bDepsMarked)
    {
        dwError = LwSmTableUnmarkDependencies(pEntry);
        BAIL_ON_ERROR(dwError);
        pEntry->bDepsMarked = FALSE;
    }
    else if ((pStatus->state != LW_SERVICE_STATE_STOPPED &&
              pStatus->state != LW_SERVICE_STATE_DEAD) &&
             !pEntry->bDepsMarked)
    {
        dwError = LwSmTableVerifyAndMarkDependencies(pEntry);
        switch (dwError)
        {
        case LW_ERROR_SUCCESS:
            pEntry->bDepsMarked = TRUE;
            break;
        case LW_ERROR_SERVICE_DEPENDENCY_UNMET:
            /* This means we're in an inconsistent state where
               an active service has an inactive dependency,
               but we should not raise an error about it
               when merely polling status */
            dwError = LW_ERROR_SUCCESS;
            break;
        default:
            BAIL_ON_ERROR(dwError);
            break;
        }
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

static
DWORD
LwSmTableStartRecursive(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD error = 0;
    PSM_TABLE_ENTRY pOtherEntry = NULL;
    PWSTR* ppServices = NULL;
    DWORD index = 0;

    /* Start all dependencies */
    error = LwSmTableGetEntryDependencyClosure(pEntry, &ppServices);
    BAIL_ON_ERROR(error);

    for (index = 0; ppServices[index]; index++)
    {
        error = LwSmTableGetEntry(ppServices[index], &pOtherEntry);
        BAIL_ON_ERROR(error);

        error = LwSmTableStartEntry(pOtherEntry);
        BAIL_ON_ERROR(error);

        LwSmTableReleaseEntry(pOtherEntry);
        pOtherEntry = NULL;
    }

    error = LwSmTableStartEntry(pEntry);
    BAIL_ON_ERROR(error);

error:

    if (ppServices)
    {
        LwSmFreeStringList(ppServices);
    }

    if (pOtherEntry)
    {
        LwSmTableReleaseEntry(pOtherEntry);
    }

    return error;
}

static
VOID
LwSmTableWatchdog(
    PVOID pContext
    )
{
    DWORD error = 0;
    PSM_TABLE_ENTRY pEntry = pContext;
    PSM_TABLE_ENTRY pOtherEntry = NULL;
    LW_SERVICE_STATUS status = {0};
    PWSTR* ppServices = NULL;
    DWORD index = 0;

    error = LwSmTableStartRecursive(pEntry);
    BAIL_ON_ERROR(error);

    /* See if any reverse dependencies changed state without announcing it */
    error = LwSmTableGetEntryReverseDependencyClosure(pEntry, &ppServices);
    BAIL_ON_ERROR(error);

    for (index = 0; ppServices[index]; index++)
    {
        error = LwSmTableGetEntry(ppServices[index], &pOtherEntry);
        BAIL_ON_ERROR(error);

        error = LwSmTableGetEntryStatus(pOtherEntry, &status);
        BAIL_ON_ERROR(error);

        if (status.state == LW_SERVICE_STATE_DEAD)
        {
            (void) LwSmTableStartRecursive(pOtherEntry);
        }

        LwSmTableReleaseEntry(pOtherEntry);
        pOtherEntry = NULL;
    }

error:

    if (ppServices)
    {
        LwSmFreeStringList(ppServices);
    }

    if (pOtherEntry)
    {
        LwSmTableReleaseEntry(pOtherEntry);
    }

    LwSmTableReleaseEntry(pEntry);

    return;
}

VOID
LwSmTableNotifyEntryStateChanged(
    PSM_TABLE_ENTRY pEntry,
    LW_SERVICE_STATE state
    )
{
    DWORD error = ERROR_SUCCESS;
    BOOLEAN bLocked = FALSE;
    PSM_LINK pLink = NULL;
    PSM_LINK pNext = NULL;
    PSM_ENTRY_NOTIFY pNotify = NULL;
    PSTR pServiceName = NULL;
    time_t now = 0;

    LOCK(bLocked, pEntry->pLock);

    for (pLink = LwSmLinkBegin(&pEntry->waiters);
         LwSmLinkValid(&pEntry->waiters, pLink);
         pLink = pNext)
    {
        pNext = LwSmLinkNext(pLink);
        pNotify = STRUCT_FROM_MEMBER(pLink, SM_ENTRY_NOTIFY, link);

        LwSmLinkRemove(pLink);

        pNotify->pfnNotifyEntryStateChange(state, pNotify->pData);

        LwFreeMemory(pNotify);
    }

    pthread_cond_broadcast(pEntry->pEvent);

    if (state == LW_SERVICE_STATE_DEAD)
    {
        now = time(NULL);
        if (now == (time_t) -1)
        {
            error = LwErrnoToWin32Error(errno);
            BAIL_ON_ERROR(error);
        }

        error = LwWc16sToMbs(pEntry->pInfo->pwszName, &pServiceName);
        BAIL_ON_ERROR(error);

        if ((now - pEntry->LastRestartPeriod) > RESTART_PERIOD)
        {
            pEntry->RestartAttempts = 0;
            pEntry->LastRestartPeriod = now;
        }

        if (pEntry->RestartAttempts < RESTART_LIMIT)
        {
            pEntry->RestartAttempts++;
            pEntry->dwRefCount++;

            SM_LOG_WARNING(
                "Restarting dead service: %s (attempt %u/%u)",
                pServiceName,
                (unsigned int) pEntry->RestartAttempts,
                (unsigned int) RESTART_LIMIT);


            error = LwNtStatusToWin32Error(LwRtlQueueWorkItem(gpPool, LwSmTableWatchdog, pEntry, 0));
            if (error)
            {
                pEntry->dwRefCount--;
            }
            BAIL_ON_ERROR(error);
        }
        else
        {
            SM_LOG_ERROR(
                "Service died: %s (restarted %u times in %lu seconds)",
                pServiceName,
                (unsigned int) pEntry->RestartAttempts,
                (unsigned long) (now - pEntry->LastRestartPeriod));
        }
    }

error:

    UNLOCK(bLocked, pEntry->pLock);

    LW_SAFE_FREE_MEMORY(pServiceName);
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

DWORD
LwSmTableRegisterEntryNotify(
    PSM_TABLE_ENTRY pEntry,
    LW_SERVICE_STATE currentState,
    VOID (*pfnNotifyEntryStateChange)(LW_SERVICE_STATE state, PVOID pData),
    PVOID pData
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    PSM_ENTRY_NOTIFY pNotify = NULL;
    LW_SERVICE_STATUS status = {0};

    LOCK(bLocked, pEntry->pLock);

    dwError = LwSmTablePollEntry(pEntry, &status);
    BAIL_ON_ERROR(dwError);

    if (status.state != currentState)
    {
        pfnNotifyEntryStateChange(status.state, pData);
    }
    else
    {
        dwError = LwAllocateMemory(sizeof(*pNotify), OUT_PPVOID(&pNotify));
        BAIL_ON_ERROR(dwError);

        LwSmLinkInit(&pNotify->link);
        pNotify->pfnNotifyEntryStateChange = pfnNotifyEntryStateChange;
        pNotify->pData = pData;

        LwSmLinkInsertBefore(&pEntry->waiters, &pNotify->link);
    }

cleanup:

    UNLOCK(bLocked, pEntry->pLock);

    return dwError;

error:

    LW_SAFE_FREE_MEMORY(pNotify);

    goto cleanup;
}

DWORD
LwSmTableUnregisterEntryNotify(
    PSM_TABLE_ENTRY pEntry,
    VOID (*pfnNotifyEntryStateChange)(LW_SERVICE_STATE state, PVOID pData),
    PVOID pData
    )
{
    DWORD dwError = 0;
    BOOLEAN bLocked = FALSE;
    PSM_LINK pLink = NULL;
    PSM_LINK pNext = NULL;
    PSM_ENTRY_NOTIFY pNotify = NULL;

    LOCK(bLocked, pEntry->pLock);

    for (pLink = LwSmLinkBegin(&pEntry->waiters);
         LwSmLinkValid(&pEntry->waiters, pLink);
         pLink = pNext)
    {
        pNext = LwSmLinkNext(pLink);
        pNotify = STRUCT_FROM_MEMBER(pLink, SM_ENTRY_NOTIFY, link);

        if (pNotify->pfnNotifyEntryStateChange == pfnNotifyEntryStateChange &&
            pNotify->pData == pData)
        {
            LwSmLinkRemove(pLink);
            LwFreeMemory(pNotify);
            goto cleanup;
        }
    }

    dwError = LW_ERROR_INVALID_PARAMETER;
    BAIL_ON_ERROR(dwError);

cleanup:

    UNLOCK(bLocked, pEntry->pLock);

    return dwError;

error:

    goto cleanup;
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
        LwSmCommonFreeServiceInfo(pDepInfo);
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

static
PVOID
LwSmTableGetServiceObjectData(
    PLW_SERVICE_OBJECT pObject
    )
{
    return pObject->pData;
}

static
VOID
LwSmTableRetainServiceObject(
    PLW_SERVICE_OBJECT pObject
    )
{
    PSM_TABLE_ENTRY pEntry = STRUCT_FROM_MEMBER(pObject, SM_TABLE_ENTRY, object);

    LwSmTableRetainEntry(pEntry);
}

static
VOID
LwSmTableReleaseServiceObject(
    PLW_SERVICE_OBJECT pObject
    )
{
    PSM_TABLE_ENTRY pEntry = STRUCT_FROM_MEMBER(pObject, SM_TABLE_ENTRY, object);

    LwSmTableReleaseEntry(pEntry);
}

static
VOID
LwSmTableNotifyServiceObjectStateChange(
    PLW_SERVICE_OBJECT pObject,
    LW_SERVICE_STATE newState
    )
{
    PSM_TABLE_ENTRY pEntry = STRUCT_FROM_MEMBER(pObject, SM_TABLE_ENTRY, object);

    return LwSmTableNotifyEntryStateChanged(pEntry, newState);
}

DWORD
LwSmTableInit(
    VOID
    )
{
    DWORD error = ERROR_SUCCESS;

    error = LwNtStatusToWin32Error(LwRtlCreateThreadPool(&gpPool, NULL));
    BAIL_ON_ERROR(error);

error:

    return error;
}

VOID
LwSmTableShutdown(
    VOID
    )
{
    BOOL bLocked = FALSE;
    PSM_LINK pLink = NULL;
    PSM_LINK pNext = NULL;
    PSM_TABLE_ENTRY pEntry = NULL;

    LOCK(bLocked, gServiceTable.pLock);

    for (pLink = LwSmLinkBegin(&gServiceTable.entries);
         LwSmLinkValid(&gServiceTable.entries, pLink);
         pLink = pNext)
    {
        pNext = LwSmLinkNext(pLink);
        pEntry = STRUCT_FROM_MEMBER(pLink, SM_TABLE_ENTRY, link);

        LwSmTableFreeEntry(pEntry);
    }

    UNLOCK(bLocked, gServiceTable.pLock);
    pthread_mutex_destroy(gServiceTable.pLock);
    LwRtlFreeThreadPool(&gpPool);
}

SM_LOADER_CALLS gTableCalls =
{
    .pfnGetServiceObjectData = LwSmTableGetServiceObjectData,
    .pfnRetainServiceObject = LwSmTableRetainServiceObject,
    .pfnReleaseServiceObject = LwSmTableReleaseServiceObject,
    .pfnNotifyServiceObjectStateChange = LwSmTableNotifyServiceObjectStateChange
};
