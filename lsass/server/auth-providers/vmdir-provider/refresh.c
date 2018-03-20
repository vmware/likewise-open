/*
 * Copyright (C) 2015 VMware, Inc. All rights reserved.
 */

#include "includes.h"

static
DWORD
VmDirCreateRefreshContext(
    PVMDIR_REFRESH_CONTEXT *ppRefreshContext
    );

static
VOID
VmDirDestroyRefreshContext(
    PVMDIR_REFRESH_CONTEXT pRefreshContext
    );

static
PVOID
VmDirMachineAccountRefreshRoutine(
    PVOID pData
    );

static
VOID
VmDirSetRefreshState(
    PVMDIR_REFRESH_CONTEXT pRefreshContext,
    VMDIR_REFRESH_STATE state
    );

static
VMDIR_REFRESH_STATE
VmDirGetRefreshState(
    PVMDIR_REFRESH_CONTEXT pRefreshContext
    );

DWORD
VmDirStartMachineAccountRefresh(
    PVMDIR_REFRESH_CONTEXT *ppRefreshContext)
{
    int errCode = 0;
    DWORD dwError = 0;
    PVMDIR_REFRESH_CONTEXT pRefreshContext = NULL;

    dwError = VmDirCreateRefreshContext(&pRefreshContext);
    BAIL_ON_VMDIR_ERROR(dwError);

    errCode = pthread_create(
                 &pRefreshContext->thread,
                 NULL,
                 VmDirMachineAccountRefreshRoutine,
                 (void *)pRefreshContext);
    BAIL_ON_ERRNO_ERROR(errCode);

    pRefreshContext->pThread = &pRefreshContext->thread;

    *ppRefreshContext = pRefreshContext;

cleanup:

    return dwError;

error:

    if (pRefreshContext)
    {
        VmDirDestroyRefreshContext(pRefreshContext);
    }

    goto cleanup;
}

VOID
VmDirSignalMachineAccountRefresh(
    PVMDIR_REFRESH_CONTEXT pRefreshContext)
{
    if (pRefreshContext)
    {
        VmDirSetRefreshState(pRefreshContext, VMDIR_REFRESH_STATE_CONFIGURING);
    }
}

VOID
VmDirStopMachineAccountRefresh(
    PVMDIR_REFRESH_CONTEXT pRefreshContext)
{
    if (pRefreshContext)
    {
        VmDirSetRefreshState(pRefreshContext, VMDIR_REFRESH_STATE_STOPPING);

        if (pRefreshContext->pThread)
        {
            pthread_join(*pRefreshContext->pThread, NULL);
            pRefreshContext->pThread = NULL;
        }

        VmDirDestroyRefreshContext(pRefreshContext);
    }
}

static
DWORD
VmDirCreateRefreshContext(
    PVMDIR_REFRESH_CONTEXT *ppRefreshContext)
{
    DWORD dwError = 0;
    PVMDIR_REFRESH_CONTEXT pRefreshContext = NULL;
    int errCode = 0;

    dwError = LwAllocateMemory(
                  sizeof(*pRefreshContext),
                  (PVOID*)&pRefreshContext);
    BAIL_ON_VMDIR_ERROR(dwError);

    errCode = pthread_mutex_init(&pRefreshContext->mutex, NULL);
    BAIL_ON_ERRNO_ERROR(errCode);
    pRefreshContext->pMutex = &pRefreshContext->mutex;

    errCode = pthread_cond_init(&pRefreshContext->cond, NULL);
    BAIL_ON_ERRNO_ERROR(errCode);
    pRefreshContext->pCond = &pRefreshContext->cond;

    errCode = pthread_rwlock_init(&pRefreshContext->rwlock, NULL);
    BAIL_ON_ERRNO_ERROR(errCode);
    pRefreshContext->pRwlock = &pRefreshContext->rwlock;

    pRefreshContext->state = VMDIR_REFRESH_STATE_UNSET;
    pRefreshContext->pThread = NULL;

    *ppRefreshContext = pRefreshContext;

cleanup:

    return dwError;

error:

    VmDirDestroyRefreshContext(pRefreshContext);

    goto cleanup;
}

static
VOID
VmDirDestroyRefreshContext(
    PVMDIR_REFRESH_CONTEXT pRefreshContext)
{
    if (pRefreshContext)
    {
        if (pRefreshContext->pMutex)
        {
            pthread_mutex_destroy(pRefreshContext->pMutex);
        }
        if (pRefreshContext->pCond)
        {
            pthread_cond_destroy(pRefreshContext->pCond);
        }
        if (pRefreshContext->pRwlock)
        {
            pthread_rwlock_destroy(pRefreshContext->pRwlock);
        }
        LwFreeMemory(pRefreshContext);
    }
}

static
VMDIR_REFRESH_STATE
VmDirGetRefreshState(
    PVMDIR_REFRESH_CONTEXT pRefreshContext)
{
    VMDIR_REFRESH_STATE state = VMDIR_REFRESH_STATE_UNSET;

    pthread_mutex_lock(&pRefreshContext->mutex);
    state = pRefreshContext->state;
    pthread_mutex_unlock(&pRefreshContext->mutex);

    return state;
}

static
VOID
VmDirSetRefreshState(
    PVMDIR_REFRESH_CONTEXT pRefreshContext,
    VMDIR_REFRESH_STATE state)
{
    pthread_mutex_lock(&pRefreshContext->mutex);
    if (pRefreshContext->state != VMDIR_REFRESH_STATE_STOPPING)
    {
        pRefreshContext->state = state;
        pthread_cond_signal(&pRefreshContext->cond);
    }
    pthread_mutex_unlock(&pRefreshContext->mutex);
}

static
PVOID
VmDirMachineAccountRefreshRoutine(
    PVOID pData)
{
    DWORD dwError = 0;
    PVMDIR_REFRESH_CONTEXT pRefreshContext = (PVMDIR_REFRESH_CONTEXT)pData;
    struct timespec ts = {0};
    DWORD dwExpiryTime = 0;
    PVMDIR_BIND_INFO pBindInfo = NULL;
    PSTR pszPassword = NULL;
    VMDIR_REFRESH_STATE state = VMDIR_REFRESH_STATE_UNSET;

    VmDirSetRefreshState(pRefreshContext, VMDIR_REFRESH_STATE_CONFIGURING);

    while (1)
    {
        int errCode = 0;
        DWORD dwEndTime = 0;

        switch ( (state = VmDirGetRefreshState(pRefreshContext)) )
        {
            case VMDIR_REFRESH_STATE_SLEEPING:

                pthread_mutex_lock(&pRefreshContext->mutex);
                errCode = 0;
                while (pRefreshContext->state == VMDIR_REFRESH_STATE_SLEEPING && errCode == 0)
                {
                    errCode = pthread_cond_wait(
                                  &pRefreshContext->cond,
                                  &pRefreshContext->mutex);
                }
                pthread_mutex_unlock(&pRefreshContext->mutex);
                VmDirSetRefreshState(pRefreshContext, VMDIR_REFRESH_STATE_CONFIGURING);
                break;

            case VMDIR_REFRESH_STATE_POLLING:

                pthread_mutex_lock(&pRefreshContext->mutex);
                ts.tv_sec = time(NULL) + 30;
                ts.tv_nsec = 0;
                errCode = 0;
                while (pRefreshContext->state == VMDIR_REFRESH_STATE_POLLING && errCode == 0)
                {
                    errCode = pthread_cond_timedwait(
                                  &pRefreshContext->cond,
                                  &pRefreshContext->mutex,
                                  &ts);
                }
                pthread_mutex_unlock(&pRefreshContext->mutex);
                VmDirSetRefreshState(pRefreshContext, VMDIR_REFRESH_STATE_CONFIGURING);
                break;

            case VMDIR_REFRESH_STATE_CONFIGURING:

                if (pBindInfo)
                {
                    VmDirReleaseBindInfo(pBindInfo);
                    pBindInfo = NULL;
                }

                dwError = VmDirGetBindInfo(&pBindInfo);
                if (dwError)
                {
                    VmDirSetRefreshState(pRefreshContext, VMDIR_REFRESH_STATE_SLEEPING);
                    break;
                }

                VmDirSetRefreshState(pRefreshContext, VMDIR_REFRESH_STATE_REFRESHING);
                break;

            case VMDIR_REFRESH_STATE_REFRESHING:

                dwError = VmDirCreateBindInfoPassword(&pszPassword);
                if (dwError)
                {
                    VmDirSetRefreshState(pRefreshContext, VMDIR_REFRESH_STATE_SLEEPING);
                    break;
                }

                pthread_rwlock_wrlock(&pRefreshContext->rwlock);
                dwError = LwKrb5InitializeCredentials(
                              pBindInfo->pszUPN,
                              pszPassword,
                              VMDIR_KRB5_CC_NAME,
                              &dwEndTime);
                pthread_rwlock_unlock(&pRefreshContext->rwlock);

                LW_SECURE_FREE_STRING(pszPassword);

                if (dwError == 0)
                {
                    /* refresh TGT again 30 minutes before expiration */
                    dwExpiryTime = dwEndTime - (30 * 60);
                    VmDirSetRefreshState(pRefreshContext, VMDIR_REFRESH_STATE_WAITING);
                }
                else
                {
                    VmDirSetRefreshState(pRefreshContext, VMDIR_REFRESH_STATE_POLLING);
                }
                break;

            case VMDIR_REFRESH_STATE_WAITING:

                pthread_mutex_lock(&pRefreshContext->mutex);
                ts.tv_sec = dwExpiryTime;
                ts.tv_nsec = 0;
                errCode = 0;
                while (pRefreshContext->state == VMDIR_REFRESH_STATE_WAITING && errCode == 0)
                {
                    errCode = pthread_cond_timedwait(
                                  &pRefreshContext->cond,
                                  &pRefreshContext->mutex,
                                  &ts);
                }
                pthread_mutex_unlock(&pRefreshContext->mutex);
                if (errCode == ETIMEDOUT)
                {
                    VmDirSetRefreshState(pRefreshContext, VMDIR_REFRESH_STATE_REFRESHING);
                }
                break;

            case VMDIR_REFRESH_STATE_STOPPING:

                goto cleanup;

            default:

                LSA_LOG_ERROR("Error: unhandled machine account refresh state (%d)",
                               state);

                dwError = LW_ERROR_INTERNAL;
                BAIL_ON_VMDIR_ERROR(dwError);
                break;
        }
    }

cleanup:

    if (pBindInfo)
    {
        VmDirReleaseBindInfo(pBindInfo);
    }

    return NULL;

error:

    goto cleanup;
}
