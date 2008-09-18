/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include <ctlock.h>
#include <ctmemory.h>
#include <ctgoto.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#define VOID_WRAP(call, args) \
    do { \
        int error = call args; \
        if (error) \
        { \
            fprintf(stderr, #call " error %d: %s", error, strerror(error)); \
        } \
    } while (0)

void
CtLockAcquireMutex(
    IN pthread_mutex_t* Mutex
    )
{
    VOID_WRAP(pthread_mutex_lock, (Mutex));
}

void
CtLockReleaseMutex(
    IN pthread_mutex_t* Mutex
    )
{
    VOID_WRAP(pthread_mutex_unlock, (Mutex));
}

void
CtLockWaitCond(
    IN pthread_cond_t* Cond,
    IN pthread_mutex_t* Mutex
    )
{
    VOID_WRAP(pthread_cond_wait, (Cond, Mutex));
}

unsigned long
CtLockWaitCondTimeout(
    IN pthread_cond_t* Cond,
    IN pthread_mutex_t* Mutex,
    IN unsigned long Timeout /* MS */
    )
{
    struct timeval before, after;
    struct timespec absolute;

    gettimeofday(&before, NULL);

    absolute.tv_sec = before.tv_sec + Timeout / 1000;
    absolute.tv_nsec = before.tv_usec * 1000 + (Timeout % 1000) * 1000000;

    pthread_cond_timedwait(Cond, Mutex, &absolute);

    gettimeofday(&after, NULL);

    if (after.tv_sec * 1000 + after.tv_usec * 1000000 > Timeout)
        return 0;
    else
        return Timeout - after.tv_sec * 1000 - after.tv_usec * 1000000;
}

void
CtLockSignalCond(
    IN pthread_cond_t* Cond
    )
{
    VOID_WRAP(pthread_cond_signal, (Cond));
}

CT_STATUS
CtLockCreateMutex(
    OUT pthread_mutex_t** Mutex
    )
{
    CT_STATUS status;
    pthread_mutex_t* mutex = NULL;

    status = CtAllocateMemory((void**)&mutex, sizeof(*mutex));
    GOTO_CLEANUP_ON_STATUS(status);

    status = CtLockInitMutex(mutex);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (status)
    {
        CT_SAFE_FREE(mutex);
    }
    *Mutex = mutex;
    return status;
}

void
CtLockFreeMutex(
    IN OUT pthread_mutex_t* Mutex
    )
{
    if (Mutex)
    {
        CtLockCleanupMutex(Mutex);
        CtFreeMemory(Mutex);
    }
}

CT_STATUS
CtLockInitMutex(
    OUT pthread_mutex_t* Mutex
    )
{
    int error;
    pthread_mutexattr_t attr;

    error = pthread_mutexattr_init(&attr);
    if (error) return CtErrnoToStatus(error);

    error = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (error) return CtErrnoToStatus(error);

    error = pthread_mutex_init(Mutex, &attr);
    return CtErrnoToStatus(error);
}

void
CtLockCleanupMutex(
    IN OUT pthread_mutex_t* Mutex
    )
{
    VOID_WRAP(pthread_mutex_destroy, (Mutex));
}

CT_STATUS
CtLockCreateCond(
    OUT pthread_cond_t** Cond
    )
{
    CT_STATUS status;
    pthread_cond_t* cond = NULL;

    status = CtAllocateMemory((void**)&cond, sizeof(*cond));
    GOTO_CLEANUP_ON_STATUS(status);

    status = CtLockInitCond(cond);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (status)
    {
        CT_SAFE_FREE(cond);
    }
    *Cond = cond;
    return status;
}

void
CtLockFreeCond(
    IN OUT pthread_cond_t* Cond
    )
{
    if (Cond)
    {
        CtLockCleanupCond(Cond);
        CtFreeMemory(Cond);
    }
}

CT_STATUS
CtLockInitCond(
    OUT pthread_cond_t* Cond
    )
{
    int error;
    error = pthread_cond_init(Cond, NULL);
    return CtErrnoToStatus(error);
}

void
CtLockCleanupCond(
    IN OUT pthread_cond_t* Cond
    )
{
    VOID_WRAP(pthread_cond_destroy, (Cond));
}

