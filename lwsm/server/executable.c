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
 *        executable.c
 *
 * Abstract:
 *
 *        Logic for managing external executable service objects
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

typedef struct _SM_PROCESS_TABLE
{
    pthread_mutex_t lock;
    pthread_mutex_t* pLock;
    SM_LINK execs;
    pthread_t thread;
    BOOLEAN bThreadStarted;
    BOOLEAN bThreadStop;
} SM_PROCESS_TABLE;

typedef struct _SM_EXECUTABLE
{
    LW_SERVICE_STATUS status;
    pid_t pid;
    PSM_TABLE_ENTRY pEntry;
    SM_LINK link;
    SM_LINK threadLink;
} SM_EXECUTABLE, *PSM_EXECUTABLE;

static SM_PROCESS_TABLE gProcTable =
{
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .pLock = &gProcTable.lock,
    .execs = {&gProcTable.execs, &gProcTable.execs},
    .thread = (pthread_t) -1,
    .bThreadStarted = FALSE,
    .bThreadStop = FALSE
};

static
DWORD
LwSmExecProgram(
    PSM_EXECUTABLE pExec
    );

static
PVOID
LwSmExecutableThread(
    PVOID pData
    );

static
DWORD
LwSmExecutableStart(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    PSM_EXECUTABLE pExec = pEntry->pData;
    BOOLEAN bLocked = FALSE;
    pid_t pid = -1;
    struct timespec ts = {1, 0};

    LOCK(bLocked, &gProcTable.lock);

    if (pExec->status != LW_SERVICE_STOPPED &&
        pExec->status != LW_SERVICE_DEAD)
    {
        dwError = LW_ERROR_INVALID_SERVICE_TRANSITION;
        BAIL_ON_ERROR(dwError);
    }

    pid = fork();

    if (pid == -1)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }
    else if (pid == 0)
    {
        dwError = LwSmExecProgram(pExec);
        BAIL_ON_ERROR(dwError);
    }
    else
    {
        pExec->pid = pid;
        pExec->status = LW_SERVICE_STARTING;
        
        /* Take an additional reference to the table entry
           because our child monitoring thread will need it */
        LwSmTableRetainEntry(pEntry);
        
        /* Signal state change */
        LwSmTableNotifyEntryChanged(pEntry);
        
        /* Wait for process to start up */
        if (nanosleep(&ts, NULL) != 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }
        
        pExec->status = LW_SERVICE_RUNNING;
        
        /* Signal state change */
        LwSmTableNotifyEntryChanged(pEntry);
    }

cleanup:

    UNLOCK(bLocked, &gProcTable.lock);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmExecProgram(
    PSM_EXECUTABLE pExec
    )
{
    DWORD dwError = 0;
    PSM_TABLE_ENTRY pEntry = pExec->pEntry;
    PSTR pszPath = NULL;
    PSTR* ppszArgs = NULL;
    size_t argsLen = 0;
    size_t i = 0;
    sigset_t set;

    sigemptyset(&set);

    dwError = LwMapErrnoToLwError(pthread_sigmask(SIG_SETMASK, &set, NULL));
    BAIL_ON_ERROR(dwError);

    dwError = LwWc16sToMbs(pEntry->pInfo->pwszPath, &pszPath);
    BAIL_ON_ERROR(dwError);

    argsLen = LwSmStringListLength(pEntry->pInfo->ppwszArgs);

    dwError = LwAllocateMemory((argsLen + 1) * sizeof(*ppszArgs), OUT_PPVOID(&ppszArgs));
    BAIL_ON_ERROR(dwError);

    for (i = 0; i < argsLen; i++)
    {
        dwError = LwWc16sToMbs(pEntry->pInfo->ppwszArgs[i], &ppszArgs[i]);
        BAIL_ON_ERROR(dwError);
    }

    if (execv(pszPath, ppszArgs) < 0)
    {
        dwError = LwMapErrnoToLwError(errno);
        BAIL_ON_ERROR(dwError);
    }

    return dwError;

error:

    /* We are in trouble */
    abort();
}

static
DWORD
LwSmExecutableStop(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    PSM_EXECUTABLE pExec = pEntry->pData;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, &gProcTable.lock);

    switch (pExec->status)
    {
    default:
        dwError = LW_ERROR_INVALID_SERVICE_TRANSITION;
        BAIL_ON_ERROR(dwError);
        break;
    case LW_SERVICE_RUNNING:
        if (kill(pExec->pid, SIGTERM) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }
        pExec->status = LW_SERVICE_STOPPING;
        LwSmTableNotifyEntryChanged(pEntry);
        
        /* The background thread will notice when the
           child process finally exits and update the
           status to LW_SERVICE_STOPPED */
        break;
    case LW_SERVICE_DEAD:
        /* Go directly to stopped state */
        pExec->status = LW_SERVICE_STOPPED;
        LwSmTableNotifyEntryChanged(pEntry);
        break;
    }

cleanup:

    UNLOCK(bLocked, &gProcTable.lock);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmExecutableGetStatus(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_STATUS pStatus
    )
{
    DWORD dwError = 0;
    PSM_EXECUTABLE pExec = pEntry->pData;

    *pStatus = pExec->status;

    return dwError;
}

static
DWORD
LwSmExecutableRefresh(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    PSM_EXECUTABLE pExec = pEntry->pData;
    BOOLEAN bLocked = FALSE;

    LOCK(bLocked, &gProcTable.lock);

    switch (pExec->status)
    {
    case LW_SERVICE_RUNNING:
        if (kill(pExec->pid, SIGHUP) < 0)
        {
            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_ERROR(dwError);
        }
        break;
    default:
        break;
    }

cleanup:

    UNLOCK(bLocked, &gProcTable.lock);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmExecutableConstruct(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;
    PSM_EXECUTABLE pExec = NULL;
    BOOLEAN bLocked = FALSE;

    dwError = LwAllocateMemory(sizeof(*pExec), OUT_PPVOID(&pExec));
    BAIL_ON_ERROR(dwError);

    pExec->pid = -1;
    pExec->status = LW_SERVICE_STOPPED;
    pExec->pEntry = pEntry;
    LwSmLinkInit(&pExec->link);
    LwSmLinkInit(&pExec->threadLink);
    pEntry->pData = pExec;

    LOCK(bLocked, &gProcTable.lock);

    LwSmLinkInsertBefore(&gProcTable.execs, &pExec->link);

    if (!gProcTable.bThreadStarted)
    {
        dwError = LwMapErrnoToLwError(pthread_create(
                                          &gProcTable.thread,
                                          NULL,
                                          LwSmExecutableThread,
                                          NULL));
        BAIL_ON_ERROR(dwError);

        gProcTable.bThreadStarted = TRUE;
    }

cleanup:

    UNLOCK(bLocked, &gProcTable.lock);

    return dwError;

error:

    goto cleanup;
}

static
VOID
LwSmExecutableDestruct(
    PSM_TABLE_ENTRY pEntry
    )
{
    return;
}

SM_OBJECT_VTBL gExecutableVtbl =
{
    .pfnStart = LwSmExecutableStart,
    .pfnStop = LwSmExecutableStop,
    .pfnGetStatus = LwSmExecutableGetStatus,
    .pfnRefresh = LwSmExecutableRefresh,
    .pfnConstruct = LwSmExecutableConstruct,
    .pfnDestruct = LwSmExecutableDestruct
};

static
PVOID
LwSmExecutableThread(
    PVOID pData
    )
{
    BOOLEAN bLocked = FALSE;
    PSM_LINK pLink = NULL;
    PSM_LINK pNext = NULL;
    PSM_EXECUTABLE pExec = NULL;
    PSM_TABLE_ENTRY pEntry = NULL;
    pid_t pid = -1;
    int status = 0;
    SM_LINK changed;
    sigset_t set;
    int sig = 0;

    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    
    LwSmLinkInit(&changed);
    
    for (;;)
    {
        LwSmLinkRemove(&changed);
        
        LOCK(bLocked, &gProcTable.lock);
        
        if (gProcTable.bThreadStop)
        {
            break;
        }
        
        pLink = NULL;

        while ((pLink = SM_LINK_ITERATE(&gProcTable.execs, pLink)))
        {
            pExec = STRUCT_FROM_MEMBER(pLink, SM_EXECUTABLE, link);
            
            if (pExec->pid != -1)
            {
                pid = waitpid(pExec->pid, &status, WNOHANG);
                
                if (pid == pExec->pid)
                {
                    switch (pExec->status)
                    {
                    case LW_SERVICE_STOPPING:
                        pExec->status = LW_SERVICE_STOPPED;
                        break;
                    default:
                        pExec->status = LW_SERVICE_DEAD;
                        break;
                    }
                    
                    pExec->pid = -1;
                    
                    LwSmLinkRemove(&pExec->threadLink);
                    LwSmLinkInsertBefore(&changed, &pExec->threadLink);
                }
            }
        }

        UNLOCK(bLocked, &gProcTable.lock);

        for (pLink = changed.pNext; pLink != &changed; pLink = pNext)
        {
            pNext = pLink->pNext;
            pExec = STRUCT_FROM_MEMBER(pLink, SM_EXECUTABLE, threadLink);
            pEntry = pExec->pEntry;
            
            LwSmLinkRemove(pLink);
            
            LwSmTableNotifyEntryChanged(pEntry);
            LwSmTableReleaseEntry(pEntry);
        }

        do
        {
            sigwait(&set, &sig);
        } while (sig != SIGCHLD);
    }

    return NULL;
}
