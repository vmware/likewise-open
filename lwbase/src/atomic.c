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
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        atomic.c
 *
 * Abstract:
 *
 *        Atomic operations
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "includes.h"

pthread_mutex_t gAtomicMutex = PTHREAD_MUTEX_INITIALIZER;

LONG
LwInterlockedCompareExchange(
    IN OUT LONG volatile *plDestination,
    IN LONG lNewValue,
    IN LONG lCompareValue
    )
{
    LONG lOldValue;

    pthread_mutex_lock(&gAtomicMutex);

    lOldValue = *plDestination;

    if (lOldValue == lCompareValue)
    {
        *plDestination = lNewValue;
    }

    pthread_mutex_unlock(&gAtomicMutex);

    return lOldValue;
}

LONG
LwInterlockedRead(
    IN LONG volatile *plSource
    )
{
    LONG lValue;

    pthread_mutex_lock(&gAtomicMutex);

    lValue = *plSource;

    pthread_mutex_unlock(&gAtomicMutex);

    return lValue;
}

LONG
LwInterlockedIncrement(
    IN OUT LONG volatile* plDestination
    )
{
    LONG lNewValue;

    pthread_mutex_lock(&gAtomicMutex);

    lNewValue = *plDestination += 1;

    pthread_mutex_unlock(&gAtomicMutex);

    return lNewValue;
}

LONG
LwInterlockedDecrement(
    IN OUT LONG volatile* plDestination
    )
{
    LONG lNewValue;

    pthread_mutex_lock(&gAtomicMutex);

    lNewValue = *plDestination -= 1;

    pthread_mutex_unlock(&gAtomicMutex);

    return lNewValue;
}
