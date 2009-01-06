/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        listener.h
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem
 *
 *        SMB Service Listener
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LISTENER_H__
#define __LISTENER_H__

DWORD
SMBSrvListenerStart(
    VOID
    );

DWORD
SMBSrvListenerStop(
    VOID
    );

BOOLEAN
SMBSrvListenerShouldStop(
    VOID
    );

VOID
SMBSrvListenerIndicateMustStop(
    VOID
    );

#endif

