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

#include "ctbase.h"

void
CTInitRWLock(
    PCTRWLOCK pLock
    )
{
    pLock->num_active_readers = 0;
    pLock->num_active_writers = 0;
    pLock->num_waiting_readers = 0;
    pLock->num_waiting_writers = 0;

    pthread_mutex_init (&pLock->mutex, NULL);
    pthread_cond_init (&pLock->read_condition, NULL);
    pthread_cond_init (&pLock->write_condition, NULL);
}

CENTERROR
CTFreeRWLock(
    PCTRWLOCK pLock
    )
{
    pthread_mutex_lock (&pLock->mutex);

    if (pLock->num_active_readers > 0 || pLock->num_active_writers > 0) {

        pthread_mutex_unlock (&pLock->mutex);

        return CTMapSystemError(EBUSY);
    }

    if (pLock->num_waiting_readers != 0 || pLock->num_waiting_writers != 0) {

        pthread_mutex_unlock (&pLock->mutex);

        return CTMapSystemError(EBUSY);
    }

    pthread_mutex_unlock (&pLock->mutex);

    pthread_mutex_destroy (&pLock->mutex);

    pthread_cond_destroy (&pLock->read_condition);

    pthread_cond_destroy (&pLock->write_condition);

    return CENTERROR_SUCCESS;
}

void
CTAcquireReadLock(
    PCTRWLOCK pLock
    )
{
    pthread_mutex_lock (&pLock->mutex);

    if (pLock->num_active_writers) {
        pLock->num_waiting_readers++;
        while (pLock->num_active_writers) {
            pthread_cond_wait (&pLock->read_condition, &pLock->mutex);
        }
        pLock->num_waiting_readers--;
    }
    pLock->num_active_readers++;
    pthread_mutex_unlock (&pLock->mutex);
}

void
CTReleaseReadLock(
    PCTRWLOCK pLock
    )
{
    pthread_mutex_lock (&pLock->mutex);
    pLock->num_active_readers--;
    if (pLock->num_active_readers == 0 && pLock->num_waiting_writers > 0)
    {
        pthread_cond_signal (&pLock->write_condition);
    }
    pthread_mutex_unlock (&pLock->mutex);
}

void
CTAcquireWriteLock(
    PCTRWLOCK pLock
    )
{
    pthread_mutex_lock (&pLock->mutex);
    if (pLock->num_active_writers || pLock->num_active_readers > 0) {
        pLock->num_waiting_writers++;
        while (pLock->num_active_writers || pLock->num_active_readers > 0) {
            pthread_cond_wait (&pLock->write_condition, &pLock->mutex);
        }
        pLock->num_waiting_writers--;
    }
    pLock->num_active_writers = 1;
    pthread_mutex_unlock (&pLock->mutex);
}

void
CTReleaseWriteLock(
    PCTRWLOCK pLock
    )
{
    pthread_mutex_lock (&pLock->mutex);
    pLock->num_active_writers = 0;
    if (pLock->num_waiting_readers > 0) {
        pthread_cond_broadcast (&pLock->read_condition);
    } else if (pLock->num_waiting_writers > 0) {
        pthread_cond_signal (&pLock->write_condition);
    }
    pthread_mutex_unlock (&pLock->mutex);
}

