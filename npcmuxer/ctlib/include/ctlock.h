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

#ifndef __CT_LOCK_H__
#define __CT_LOCK_H__

#include <ctstatus.h>
#include <pthread.h>

/* Abstract out non-portable constructs and standadize defs */
#define CT_LOCK_INITIALIZER_MUTEX           PTHREAD_MUTEX_INITIALIZER
/*
#ifdef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#define CT_LOCK_INITIALIZER_MUTEX_RECURSIVE PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#else
#define CT_LOCK_INITIALIZER_MUTEX_RECURSIVE CT_LOCK_INITIALIZER_MUTEX
#endif
*/

#define CT_SAFE_FREE_MUTEX(x) \
    do { \
        if (x) \
        { \
            CtLockFreeMutex(x); \
            (x) = NULL; \
        } \
    } while (0)

#define CT_SAFE_FREE_COND(x) \
    do { \
        if (x) \
        { \
            CtLockFreeCond(x); \
            (x) = NULL; \
        } \
    } while (0)

void
CtLockAcquireMutex(
    IN pthread_mutex_t* Mutex
    );

void
CtLockReleaseMutex(
    IN pthread_mutex_t* Mutex
    );

void
CtLockWaitCond(
    IN pthread_cond_t* Cond,
    IN pthread_mutex_t* Mutex
    );

unsigned long
CtLockWaitCondTimeout(
    IN pthread_cond_t* Cond,
    IN pthread_mutex_t* Mutex,
    IN unsigned long Timeout
    );

void
CtLockSignalCond(
    IN pthread_cond_t* Cond
    );

CT_STATUS
CtLockCreateMutex(
    OUT pthread_mutex_t** Mutex
    );

void
CtLockFreeMutex(
    IN OUT pthread_mutex_t* Mutex
    );

CT_STATUS
CtLockInitMutex(
    OUT pthread_mutex_t* Mutex
    );

void
CtLockCleanupMutex(
    IN OUT pthread_mutex_t* Mutex
    );

CT_STATUS
CtLockCreateCond(
    OUT pthread_cond_t** Cond
    );

void
CtLockFreeCond(
    IN OUT pthread_cond_t* Cond
    );

CT_STATUS
CtLockInitCond(
    OUT pthread_cond_t* Cond
    );

void
CtLockCleanupCond(
    IN OUT pthread_cond_t* Cond
    );

#endif /* __CT_LOCK_H__ */
