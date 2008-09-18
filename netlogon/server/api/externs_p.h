/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        externs_p.h
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Server API Externals (Library)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __EXTERNS_P_H__
#define __EXTERNS_P_H__

extern pthread_rwlock_t ghEventLogItf_rwlock;
extern HANDLE ghEventLogItf;

/* Global Value locks */
extern pthread_rwlock_t gLWNetGlobalDataLock;

#define ENTER_LWNET_GLOBAL_DATA_RW_READER_LOCK(bInLock)       \
        if (!bInLock) {                                    \
           pthread_rwlock_rdlock(&gLWNetGlobalDataLock);      \
           bInLock = TRUE;                                 \
        }

#define LEAVE_LWNET_GLOBAL_DATA_RW_READER_LOCK(bInLock)       \
        if (bInLock) {                                     \
           pthread_rwlock_unlock(&gLWNetGlobalDataLock);      \
           bInLock = FALSE;                                \
        }

#define ENTER_LWNET_GLOBAL_DATA_RW_WRITER_LOCK(bInLock)       \
        if (!bInLock) {                                    \
           pthread_rwlock_wrlock(&gLWNetGlobalDataLock);      \
           bInLock = TRUE;                                 \
        }

#define LEAVE_LWNET_GLOBAL_DATA_RW_WRITER_LOCK(bInLock)       \
        if (bInLock) {                                     \
           pthread_rwlock_unlock(&gLWNetGlobalDataLock);      \
           bInLock = FALSE;                                \
        }


/* Cache */
extern PSTR gpszLWNetCurrentSiteName;

extern const DWORD gdwLWNetCacheReaperTimeoutSecsMinimum;
extern const DWORD gdwLWNetCacheReaperTimeoutSecsDefault;
extern const DWORD gdwLWNetCacheReaperTimeoutSecsMaximum;
extern DWORD gdwLWNetCacheReaperTimeoutSecs;

extern const DWORD gdwLWNetCacheEntryExpirySecsMinimum;
extern const DWORD gdwLWNetCacheEntryExpirySecsDefault;
extern const DWORD gdwLWNetCacheEntryExpirySecsMaximum;
extern DWORD gdwLWNetCacheEntryExpirySecs;

extern const DWORD gdwLWNetCacheWriteRetryIntervalMilliseconds;
extern const DWORD gdwLWNetCacheWriteRetryAttempts;

extern pthread_t       gLWNetCacheReaperThread;
extern pthread_mutex_t gLWNetCacheReaperThreadLock;
extern pthread_cond_t  gLWNetCacheReaperThreadCondition;
extern pthread_t*      gpLWNetCacheReaperThread;

/* eventlog */

#define ENTER_EVENTLOG_READER_LOCK(bInLock)              \
        if (!bInLock) {                                  \
           pthread_rwlock_rdlock(&ghEventLogItf_rwlock); \
           bInLock = TRUE;                               \
        }

#define LEAVE_EVENTLOG_READER_LOCK(bReleaseLock)         \
        if (bReleaseLock) {                              \
           pthread_rwlock_unlock(&ghEventLogItf_rwlock); \
           bReleaseLock = FALSE;                         \
        }

#define ENTER_EVENTLOG_WRITER_LOCK(bInLock)              \
        if (!bInLock) {                                  \
           pthread_rwlock_wrlock(&ghEventLogItf_rwlock); \
           bInLock = TRUE;                               \
        }

#define LEAVE_EVENTLOG_WRITER_LOCK(bReleaseLock)         \
        if (bReleaseLock) {                              \
           pthread_rwlock_unlock(&ghEventLogItf_rwlock); \
           bReleaseLock = FALSE;                         \
        }

#endif /* __EXTERNS_P_H__ */

