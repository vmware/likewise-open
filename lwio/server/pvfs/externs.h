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
 *        externs.h
 *
 * Abstract:
 *
 *        Externs
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#ifndef __EXTERNS_H__
#define __EXTERNS_H__

#include "macros.h"
#include "threads.h"


extern PSTR             gpszPVFSProviderName;
extern GENERIC_MAPPING  gPvfsFileGenericMapping;

extern PVFS_WORKER_POOL gWorkPool;

extern PPVFS_WORK_QUEUE gpPvfsIoWorkQueue;
extern PPVFS_WORK_QUEUE gpPvfsInternalWorkQueue;

extern pthread_rwlock_t  gPathCacheRwLock;
extern pthread_rwlock_t* gpPathCacheRwLock;

extern PSMB_HASH_TABLE   gpPathCache;

extern PVFS_FCB_TABLE gFcbTable;

extern pthread_mutex_t gDeviceFcbMutex;
extern PPVFS_FCB gpPvfsDeviceFcb;

extern PLW_MAP_SECURITY_CONTEXT gpPvfsLwMapSecurityCtx;

extern pthread_mutex_t gUidMruCacheMutex;
extern PPVFS_ID_CACHE gUidMruCache[];

extern pthread_mutex_t gGidMruCacheMutex;
extern PPVFS_ID_CACHE gGidMruCache[];

#ifdef _PVFS_DEVELOPER_DEBUG
extern LONG gPvfsIrpContextCount;
extern LONG gPvfsFcbCount;
extern LONG gPvfsCcbCount;
extern LONG gPvfsWorkContextCount;
#endif


#endif /* __EXTERNS_H__ */
