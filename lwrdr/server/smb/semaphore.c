/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        semaphore.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSMB)
 *
 *        Semaphore Code
 *
 * Author: Danilo Almeida (dalmeida@likewise.com)
 */

#include "includes.h"

// Need to check the OS after including config.h, which is
// included by includes.h.

#if defined(__LWI_DARWIN__)
DWORD
SMBSemaphoreInit(
    OUT PLSMB_SEMAPHORE pSemaphore,
    IN DWORD Count
    )
{
    DWORD dwError = 0;
    BOOLEAN IsMutexInitialized = FALSE;
    BOOLEAN IsConditionInitialized = FALSE;

    dwError = pthread_mutex_init(&pSemaphore->Mutex, NULL);
    BAIL_ON_SMB_ERROR(dwError);
    IsMutexInitialized = TRUE;

    dwError = pthread_cond_init(&pSemaphore->Condition, NULL);
    BAIL_ON_SMB_ERROR(dwError);
    IsConditionInitialized = TRUE;

    pSemaphore->Count = Count;

error:
    if (dwError)
    {
        if (IsConditionInitialized)
        {
            pthread_cond_destroy(&pSemaphore->Condition);
        }
        if (IsMutexInitialized)
        {
            pthread_mutex_destroy(&pSemaphore->Mutex);
        }
    }

    return dwError;
}

DWORD
SMBSemaphoreWait(
    IN PLSMB_SEMAPHORE pSemaphore
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pSemaphore->Mutex);

    while (pSemaphore->Count <= 0)
    {
        dwError = pthread_cond_wait(&pSemaphore->Condition, &pSemaphore->Mutex);
        BAIL_ON_SMB_ERROR(dwError);
    }
    pSemaphore->Count--;

error:
    SMB_UNLOCK_MUTEX(bInLock, &pSemaphore->Mutex);

    return dwError;
}

DWORD
SMBSemaphorePost(
    IN PLSMB_SEMAPHORE pSemaphore
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pSemaphore->Mutex);

    pSemaphore->Count++;
    dwError = pthread_cond_signal(&pSemaphore->Condition);
    assert(!dwError);
    dwError = 0;

    SMB_UNLOCK_MUTEX(bInLock, &pSemaphore->Mutex);

    return dwError;
}

VOID
SMBSemaphoreDestroy(
    IN OUT PLSMB_SEMAPHORE pSemaphore
    )
{
    int error = 0;
    error = pthread_cond_destroy(&pSemaphore->Condition);
    if (error)
    {
        SMB_LOG_ERROR("Failed to destroy semaphore condition [code: %d]", error);
    }
    error = pthread_mutex_destroy(&pSemaphore->Mutex);
    if (error)
    {
        SMB_LOG_ERROR("Failed to destroy semaphore mutex [code: %d]", error);
    }
    pSemaphore->Count = 0;
}
#endif /* __LWI_DARWIN__ */
