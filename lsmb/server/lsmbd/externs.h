/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem
 *
 *        External Variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __EXTERNS_P_H__
#define __EXTERNS_P_H__

extern SMBSERVERINFO  gServerInfo;
extern PSMBSERVERINFO gpServerInfo;

extern pthread_t gSignalHandlerThread;
extern pthread_t* gpSignalHandlerThread;

extern LWMsgDispatchSpec gLSMBdispatch[];

#define DAEMON_NAME "lsmbd"
#define PID_DIR "/var/run"
#define PID_FILE PID_DIR "/" DAEMON_NAME ".pid"

#define PID_FILE_CONTENTS_SIZE ((9 * 2) + 2)

#define SMB_LOCK_SERVERINFO(bInLock)                  \
        if (!bInLock) {                               \
           pthread_mutex_lock(&gpServerInfo->lock);   \
           bInLock = TRUE;                            \
        }

#define SMB_UNLOCK_SERVERINFO(bInLock)                \
        if (bInLock) {                                \
           pthread_mutex_unlock(&gpServerInfo->lock); \
           bInLock = FALSE;                           \
        }

extern pthread_mutex_t gServerConfigLock;
extern SMB_CONFIG      gServerConfig;
extern PSMB_CONFIG     gpServerConfig;

#define SMB_LOCK_SERVERCONFIG(bInLock)               \
        if (!bInLock) {                              \
           pthread_mutex_lock(&gServerConfigLock);   \
           bInLock = TRUE;                           \
        }

#define SMB_UNLOCK_SERVERCONFIG(bInLock)             \
        if (bInLock) {                               \
           pthread_mutex_unlock(&gServerConfigLock); \
           bInLock = FALSE;                          \
        }

#endif /* __EXTERNS_P_H__ */

