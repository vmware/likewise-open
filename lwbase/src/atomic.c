#include "includes.h"

pthread_mutex_t gAtomicMutex = PTHREAD_MUTEX_INITIALIZER;

LONG
LwInterlockedCompareExchange(
    LONG volatile *plDestination,
    LONG lNewValue,
    LONG lCompareValue
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
    LONG volatile *plSource
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
    LONG volatile* plDestination
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
    LONG volatile* plDestination
    )
{
    LONG lNewValue;

    pthread_mutex_lock(&gAtomicMutex);

    lNewValue = *plDestination -= 1;

    pthread_mutex_unlock(&gAtomicMutex);

    return lNewValue;
}
