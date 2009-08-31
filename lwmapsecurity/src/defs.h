/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software. All rights reserved.
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        defs.h
 *
 * Abstract:
 *
 *        Likewise Map Security - Defines
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#define SAFE_LOG_STRING(String) \
    ( (String) ? (String) : "<null>" )

#if ENABLE_LOGGING
#define LOG_ERROR(Format, ...) \
    fprintf(stderr, Format, ## __VA_ARGS__)
#else
#define LOG_ERROR(Format, ...)
#endif

#define LW_MAP_SECURITY_PLUGIN_PATH LIBDIR "/liblwmapsecurity_lsass" MOD_EXT

#define LW_MAP_SEC_LOCK_MUTEX(bInLock, mutex) \
    if (!bInLock) { \
       int thr_err = pthread_mutex_lock(mutex); \
       if (thr_err) { \
           LOG_ERROR("Failed to lock mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define LW_MAP_SEC_UNLOCK_MUTEX(bInLock, mutex) \
    if (bInLock) { \
       int thr_err = pthread_mutex_unlock(mutex); \
       if (thr_err) { \
           LOG_ERROR("Failed to unlock mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = FALSE; \
    }

//
// Unmapped Unix User and Group SIDs
//
// S-1-22-1-UID for users
// S-1-22-2-GID for groups
//

#define SECURITY_UNMAPPED_UNIX_AUTHORITY    { 0, 0, 0, 0, 0, 22 }
#define SECURITY_UNMAPPED_UNIX_UID_RID      1
#define SECURITY_UNMAPPED_UNIX_GID_RID      2
#define SECURITY_UNMAPPED_UNIX_RID_COUNT    2

#define SECURITY_UNMAPPED_UNIX_UID_PREFIX "S-1-22-1-"
#define SECURITY_UNMAPPED_UNIX_GID_PREFIX "S-1-22-2-"

