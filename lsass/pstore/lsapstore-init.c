/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *     lsapstore-init.c
 *
 * Abstract:
 *
 *     LSASS Password Store API Implementation
 *
 *     Initialization code
 *
 *  Authors: Danilo Almeida (dalmeida@likewise.com)
 */

#include "lsapstore-includes.h"
#include <lw/errno.h>
#include <pthread.h>
#include "lsautils.h"

typedef struct _LSA_PSTORE_STATE {
    struct {
        pthread_once_t OnceControl;
        DWORD Error;
        BOOLEAN Done;
    } Init;
    pthread_mutex_t Mutex;
    pthread_mutex_t* pMutex;
    LONG RefCount;

    struct {
        PSTR Path;
        PVOID Handle;
        PLSA_PSTORE_PLUGIN_DISPATCH Dispatch;
        PLSA_PSTORE_PLUGIN_CONTEXT Context;
    } Plugin;

    BOOLEAN NeedClose;

} LSA_PSTORE_STATE, *PLSA_PSTORE_STATE;

static LSA_PSTORE_STATE LsaPstoreState = { { ONCE_INIT } };

//
// Prototypes
//

static
VOID
LsaPstorepCleanupLibraryInternal(
    VOID
    );

//
// Functions
//

__attribute__((constructor))
VOID
LsaPstoreInitializeLibrary(
    VOID
    )
{
    LwInterlockedIncrement(&LsaPstoreState.RefCount);
}

__attribute__((destructor))
VOID
LsaPstoreCleanupLibrary(
    VOID
    )
{
    if (0 == LwInterlockedDecrement(&LsaPstoreState.RefCount))
    {
        LsaPstorepCleanupLibraryInternal();
    }
}

static
DWORD
LsaPstorepInitializeLibraryInternal(
    VOID
    )
{
    DWORD dwError = 0;
    int EE = 0;
    PLSA_PSTORE_STATE pState = &LsaPstoreState;
    int error = 0;
    pthread_mutexattr_t mutexAttributes;
    pthread_mutexattr_t* pMutexAttributes = NULL;

    if (pState->Init.Done)
    {
        LwInterlockedIncrement(&pState->RefCount);

        dwError = pState->Init.Error;
        GOTO_CLEANUP_EE(EE);
    }

    error = pthread_mutexattr_init(&mutexAttributes);
    dwError = LwErrnoToWin32Error(error);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    pMutexAttributes = &mutexAttributes;

    error = pthread_mutexattr_settype(pMutexAttributes, PTHREAD_MUTEX_RECURSIVE);
    dwError = LwErrnoToWin32Error(error);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    error = pthread_mutex_init(&pState->Mutex, pMutexAttributes);
    dwError = LwErrnoToWin32Error(error);
    GOTO_CLEANUP_ON_WINERROR_EE(dwError, EE);

    pState->pMutex = &pState->Mutex;

cleanup:
    if (dwError)
    {
        LsaPstorepCleanupLibraryInternal();
    }

    if (pMutexAttributes)
    {
        pthread_mutexattr_destroy(pMutexAttributes);
    }

    pState->Init.Error = dwError;
    pState->Init.Done = TRUE;

    LSA_PSTORE_LOG_LEAVE_ERROR_EE(dwError, EE);
    return dwError;
}

static
VOID
LsaPstorepCleanupLibraryInternal(
    VOID
    )
{
    PLSA_PSTORE_STATE pState = &LsaPstoreState;
    pthread_once_t onceControl = ONCE_INIT;

    if (pState->NeedClose)
    {
        pState->NeedClose = FALSE;
    }

    if (pState->pMutex)
    {
        pthread_mutex_destroy(pState->pMutex);
        pState->pMutex = NULL;
    }

    pState->Init.OnceControl = onceControl;
    pState->Init.Error = 0;
    pState->Init.Done = FALSE;
}

static
VOID
LsaPstoreInitializeLibraryOnce(
    VOID
    )
{
    LsaPstorepInitializeLibraryInternal();
}

DWORD
LsaPstorepEnsureInitialized(
    VOID
    )
{
    DWORD dwError = 0;

    if (LsaPstoreState.RefCount)
    {
        pthread_once(&LsaPstoreState.Init.OnceControl, LsaPstoreInitializeLibraryOnce);
        dwError = LsaPstoreState.Init.Error;
    }
    else
    {
        dwError = ERROR_DLL_INIT_FAILED;
    }

    LSA_PSTORE_LOG_LEAVE_ERROR(dwError);
    return dwError;
}

VOID
LsaPstorepLock(
    VOID
    )
{
    int error = 0;
    PLSA_PSTORE_STATE pState = &LsaPstoreState;

    error = pthread_mutex_lock(pState->pMutex);
    if (error)
    {
        LSA_LOG_ERROR("Failed to lock mutex with errno = %d (%s)",
                         error, LwErrnoToName(error));
        assert(!error);
    }
}

VOID
LsaPstorepUnlock(
    VOID
    )
{
    int error = 0;
    PLSA_PSTORE_STATE pState = &LsaPstoreState;

    error = pthread_mutex_unlock(pState->pMutex);
    if (error)
    {
        LSA_LOG_ERROR("Failed to unlock mutex with errno = %d (%s)",
                         error, LwErrnoToName(error));
        assert(!error);
    }
}
