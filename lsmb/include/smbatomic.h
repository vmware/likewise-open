/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        smbatomic.h
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 *        Atomic Operations
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 */

#ifndef __SMB_ATOMIC_H__
#define __SMB_ATOMIC_H__

#include <lsmb/lsmb.h>

LONG
InterlockedCompareExchange(
    LONG volatile *plDestination,
    LONG lNewValue,
    LONG lCompareValue
    );

LONG
InterlockedRead(
    LONG volatile *plSource
    );

LONG
InterlockedIncrement(
    LONG volatile* plDestination
    );

LONG
InterlockedDecrement(
    LONG volatile* plDestination
    );

#endif /* __SMB_ATOMIC_H__ */
