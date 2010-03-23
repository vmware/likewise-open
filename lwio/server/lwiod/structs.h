/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        lsassd.h
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem
 *
 *        Service Structures
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct {
    /* MT safety */
    pthread_mutex_t lock;
    /* Should start as daemon */
    DWORD dwStartAsDaemon;
    /* where are we logging */
    LWIO_LOG_TARGET logTarget;
    /* How much logging do you want? */
    LWIO_LOG_LEVEL maxAllowedLogLevel;
    /* Enable debug logs */
    BOOLEAN bEnableDebugLogs;
    /* log file path */
    char szLogFilePath[PATH_MAX + 1];
    /* Cache path */
    char szCachePath[PATH_MAX+1];
    /* Prefix path */
    char szPrefixPath[PATH_MAX+1];
    /* Process termination flag */
    BOOLEAN  bProcessShouldExit;
    /* Process Exit Code */
    DWORD dwExitCode;
} SMBSERVERINFO, *PSMBSERVERINFO;

typedef struct __LWIO_CONFIG
{
    pthread_rwlock_t RwLock;
    pthread_rwlock_t* pRwLock;

    DWORD MaxOpenFileDescriptors;

} LWIO_CONFIG, *PLWIO_CONFIG;

#endif /* __STRUCTS_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
