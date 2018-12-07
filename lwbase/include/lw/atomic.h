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
 *        atomic.h
 *
 * Abstract:
 *
 *        Atomic operations
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#ifndef __LWBASE_ATOMIC_H__
#define __LWBASE_ATOMIC_H__

#include <lw/types.h>
#include <lw/attrs.h>

LW_BEGIN_EXTERN_C

LW_LONG
LwInterlockedCompareExchange(
    LW_IN LW_OUT LW_LONG volatile *plDestination,
    LW_IN LW_LONG lNewValue,
    LW_IN LW_LONG lCompareValue
    );

LW_PVOID
LwInterlockedCompareExchangePointer(
    LW_IN LW_OUT LW_PVOID volatile *ppDestination,
    LW_IN LW_PVOID pNewPointer,
    LW_IN LW_PVOID pComparePointer
    );

LW_LONG
LwInterlockedRead(
    LW_IN LW_LONG volatile *plSource
    );

LW_LONG
LwInterlockedIncrement(
    LW_IN LW_OUT LW_LONG volatile* plDestination
    );

LW_LONG
LwInterlockedDecrement(
    LW_IN LW_OUT LW_LONG volatile* plDestination
    );

LW_LONG64
LwInterlockedExchange64(
    LW_IN LW_OUT LW_LONG64 volatile *plDestination,
    LW_IN LW_LONG64 lValue
    );

LW_LONG64
LwInterlockedIncrement64(
    LW_IN LW_OUT LW_LONG64 volatile* plDestination
    );

LW_LONG64
LwInterlockedDecrement64(
    LW_IN LW_OUT LW_LONG64 volatile* plDestination
    );

LW_LONG64
LwInterlockedAdd64(
    LW_IN LW_OUT LW_LONG64 volatile* plDestination,
    LW_IN LW_LONG64 lValue
    );

LW_LONG64
LwInterlockedSubtract64(
    LW_IN LW_OUT LW_LONG64 volatile* plDestination,
    LW_IN LW_LONG64 lValue
    );

#ifndef LW_STRICT_NAMESPACE

#define InterlockedCompareExchange(Desination, NewValue, CompareValue) \
    LwInterlockedCompareExchange(Desination, NewValue, CompareValue)
#define InterlockedCompareExchangePointer(Destination, Exchange, Comparand) \
    LwInterlockedCompareExchangePointer(Destination, Exchange, Comparand)
#define InterlockedRead(Source)             LwInterlockedRead(Source)
#define InterlockedIncrement(Destination)   LwInterlockedIncrement(Destination)
#define InterlockedDecrement(Destination)   LwInterlockedDecrement(Destination)

#define InterlockedExchange64(Desination, Value) \
    LwInterlockedExchange64(Desination, Value)
#define InterlockedIncrement64(Destination)   LwInterlockedIncrement64(Destination)
#define InterlockedDecrement64(Destination)   LwInterlockedDecrement64(Destination)
#define InterlockedAdd64(Destination, Value)   LwInterlockedAdd64(Destination, Value)
#define InterlockedSubtract64(Destination, Value)   LwInterlockedSubtract64(Destination, Value)

#endif /* LW_STRICT_NAMESPACE */

LW_END_EXTERN_C

#endif
