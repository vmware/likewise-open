/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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

PVFS_DRIVER_CONFIG gPvfsDriverConfig;

pthread_mutex_t* gpPathCacheLock = NULL;
pthread_mutex_t  gPathCacheLock;

PLWIO_LRU        gpPathCache = NULL;

FILE_FS_CONTROL_INFORMATION gPvfsFileFsControlInformation = { 0 };

PVFS_DRIVER_STATE gPvfsDriverState = {
    .Flags = 0,

    .Mutex = PTHREAD_MUTEX_INITIALIZER,
    .DriverName = "PVFS",
    .DriverDescription = "Posix Virtual File System",

    .IoDeviceHandle = (HANDLE)NULL,
    .DeviceScb = NULL,

    .ThreadPool = NULL,

    .GidCache =
        {
            .Cache = { 0 },
            .Mutex = PTHREAD_MUTEX_INITIALIZER,
        },
    .UidCache =
        {
            .Cache = { 0 },
            .Mutex = PTHREAD_MUTEX_INITIALIZER,
        },

    .MapSecurityContext = NULL,

    .GenericSecurityMap =
        {
            .GenericRead    = FILE_GENERIC_READ,
            .GenericWrite   = FILE_GENERIC_WRITE,
            .GenericExecute = FILE_GENERIC_EXECUTE,
            .GenericAll     = FILE_ALL_ACCESS
        }
};
