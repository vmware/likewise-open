/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __CTRWLOCK_H__
#define __CTRWLOCK_H__

#include <pthread.h>

typedef struct __CTRWLOCK {
    pthread_mutex_t mutex;
    pthread_cond_t  read_condition;
    pthread_cond_t  write_condition;
    int             num_active_readers;
    int             num_active_writers;
    int             num_waiting_readers;
    int             num_waiting_writers;
} CTRWLOCK, *PCTRWLOCK;

#define CTRWLOCK_INITIALIZER         \
        {                            \
          PTHREAD_MUTEX_INITIALIZER, \
          PTHREAD_COND_INITIALIZER,  \
          PTHREAD_COND_INITIALIZER,  \
          0,                         \
          0,                         \
          0,                         \
          0                          \
        }

void
CTInitRWLock(
    PCTRWLOCK pLock
    );

CENTERROR
CTFreeRWLock(
    PCTRWLOCK pLock
    );

void
CTAcquireReadLock(
    PCTRWLOCK pLock
    );

void
CTReleaseReadLock(
    PCTRWLOCK pLock
    );

void
CTAcquireWriteLock(
    PCTRWLOCK pLock
    );

void
CTReleaseWriteLock(
    PCTRWLOCK pLock
    );

#endif /* __CTRWLOCK_H__ */

