/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        externs.h
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 *        Listener Declarations
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __EXTERNS_H__
#define __EXTERNS_H__

extern PSTR           gpszSrvProviderName;
extern NTVFS_DRIVER   gSrvProviderTable;

extern pthread_mutex_t gListenerLock;
extern BOOLEAN         gbStopListener;
extern pthread_t       gListenerThread;
extern PVOID           gpListenerThread;

#define SMB_LOCK_LISTENER   pthread_mutex_lock(&gListenerLock)
#define SMB_UNLOCK_LISTENER pthread_mutex_unlock(&gListenerLock)

#endif /* __EXTERNS_H__ */
