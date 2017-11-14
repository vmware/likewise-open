/*
 * Copyright (C) VMware. All rights reserved.
 */

#include "includes.h"

DWORD
VmDirRWLockAcquire(
    pthread_rwlock_t* pMutex,
    BOOLEAN           bExclusive,
    PBOOLEAN          pbLocked
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    if (!*pbLocked)
    {
        int errCode = bExclusive ?     pthread_rwlock_wrlock(pMutex) :
                                    pthread_rwlock_rdlock(pMutex);
        if (errCode)
        {
            dwError = ERROR_LOCK_FAILED;
        }
        else
        {
            *pbLocked = TRUE;
        }
    }
    else
    {
        dwError = ERROR_LOCKED;
    }

    return dwError;
}

DWORD
VmDirRWLockRelease(
    pthread_rwlock_t* pMutex,
    PBOOLEAN          pbLocked
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    if (*pbLocked)
    {
        int errCode = pthread_rwlock_unlock(pMutex);

        if (errCode)
        {
            dwError = ERROR_LOCK_FAILED;
        }
        else
        {
            *pbLocked = FALSE;
        }
    }

    return dwError;
}
