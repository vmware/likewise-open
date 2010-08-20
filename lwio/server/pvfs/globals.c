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
 *        globals.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Driver globals
 *
 * Authors:  Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

PSTR gpszPVFSProviderName = "Posix Virtual File System";
IO_DEVICE_HANDLE gPvfsDeviceHandle = (IO_DEVICE_HANDLE)NULL;

GENERIC_MAPPING gPvfsFileGenericMapping = {
    .GenericRead    = FILE_GENERIC_READ,
    .GenericWrite   = FILE_GENERIC_WRITE,
    .GenericExecute = FILE_GENERIC_EXECUTE,
    .GenericAll     = FILE_ALL_ACCESS
};

PVFS_WORKER_POOL gWorkPool = {0};

PPVFS_WORK_QUEUE gpPvfsIoWorkQueue = NULL;
PPVFS_WORK_QUEUE gpPvfsInternalWorkQueue = NULL;

pthread_rwlock_t* gpPathCacheRwLock = NULL;
pthread_rwlock_t  gPathCacheRwLock;

PSMB_HASH_TABLE gpPathCache = NULL;

pthread_mutex_t gDeviceFcbMutex = PTHREAD_MUTEX_INITIALIZER;
PPVFS_FCB gpPvfsDeviceFcb = NULL;

pthread_mutex_t gPvfsIrpContextMutex = PTHREAD_MUTEX_INITIALIZER;

PVFS_FCB_TABLE gFcbTable;

PLW_MAP_SECURITY_CONTEXT gpPvfsLwMapSecurityCtx = NULL;

pthread_mutex_t gUidMruCacheMutex = PTHREAD_MUTEX_INITIALIZER;
PPVFS_ID_CACHE  gUidMruCache[PVFS_MAX_MRU_SIZE];

pthread_mutex_t gGidMruCacheMutex = PTHREAD_MUTEX_INITIALIZER;
PPVFS_ID_CACHE  gGidMruCache[PVFS_MAX_MRU_SIZE];

PVFS_DRIVER_CONFIG gPvfsDriverConfig;


LONG gPvfsIrpContextCount = 0;
LONG gPvfsFcbCount = 0;
LONG gPvfsCcbCount = 0;
LONG gPvfsWorkContextCount = 0;

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
