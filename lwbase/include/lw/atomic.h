#ifndef __LWBASE_ATOMIC_H__
#define __LWBASE_ATOMIC_H__

#include <lw/types.h>

LW_LONG
LwInterlockedCompareExchange(
    LW_LONG volatile *plDestination,
    LW_LONG lNewValue,
    LW_LONG lCompareValue
    );

LW_LONG
LwInterlockedRead(
    LW_LONG volatile *plSource
    );

LW_LONG
LwInterlockedIncrement(
    LW_LONG volatile* plDestination
    );

LW_LONG
LwInterlockedDecrement(
    LW_LONG volatile* plDestination
    );

#ifndef LW_STRICT_NAMESPACE
#define InterlockedCompareExchange LwInterlockedCompareExchange
#define InterlockedRead            LwInterlockedRead
#define InterlockedIncrement       LwInterlockedIncrement
#define InterlockedDecrement       LwInterlockedDecrement
#endif /* LW_STRICT_NAMESPACE */

#endif
