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
 *        fcb.h
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        File Control Block routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#ifndef _PVFS_FCB_H
#define _PVFS_FCB_H

#include "pvfs.h"


// From fcb.c

NTSTATUS
PvfsAllocateFCB(
    PPVFS_FCB *ppFcb
    );

PPVFS_FCB
PvfsReferenceFCB(
    IN PPVFS_FCB pFcb
    );

VOID
PvfsReleaseFCB(
    PPVFS_FCB *ppFcb
    );

NTSTATUS
PvfsCreateFCB(
    OUT PPVFS_FCB *ppFcb,
    IN PSTR pszFilename,
    IN BOOLEAN bCheckShareAccess,
    IN FILE_SHARE_FLAGS SharedAccess,
    IN ACCESS_MASK DesiredAccess
    );

NTSTATUS
PvfsGetDirectoryFCB(
    IN  PSTR pszFilename,
    OUT PPVFS_FCB *ppFcb
    );

NTSTATUS
PvfsAddCCBToFCB(
    PPVFS_FCB pFcb,
    PPVFS_CCB pCcb
    );


NTSTATUS
PvfsRemoveCCBFromFCB(
    PPVFS_FCB pFcb,
    PPVFS_CCB pCcb
    );

BOOLEAN
PvfsFileHasOtherOpens(
    IN PPVFS_FCB pFcb,
    IN PPVFS_CCB pCcb
    );

BOOLEAN
PvfsFileIsOplocked(
    IN PPVFS_FCB pFcb
    );

BOOLEAN
PvfsFileIsOplockedExclusive(
    IN PPVFS_FCB pFcb
    );

BOOLEAN
PvfsFileIsOplockedShared(
    IN PPVFS_FCB pFcb
    );

NTSTATUS
PvfsPendOplockBreakTest(
    IN PPVFS_FCB pFcb,
    IN PPVFS_IRP_CONTEXT pIrpContext,
    IN PPVFS_CCB pCcb,
    IN PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK pfnCompletion,
    IN PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX pfnFreeContext,
    IN PVOID pCompletionContext
    );

NTSTATUS
PvfsAddItemPendingOplockBreakAck(
    IN OUT PPVFS_FCB pFcb,
    IN     PPVFS_IRP_CONTEXT pIrpContext,
    IN     PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK pfnCompletion,
    IN     PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX pfnFreeContext,
    IN     PVOID pCompletionContext
    );

NTSTATUS
PvfsAddOplockRecord(
    IN OUT PPVFS_FCB pFcb,
    IN     PPVFS_IRP_CONTEXT pIrpContext,
    IN     PPVFS_CCB pCcb,
    IN     ULONG OplockType
    );

VOID
PvfsFreeOplockRecord(
    PPVFS_OPLOCK_RECORD *ppOplockRec
    );

NTSTATUS
PvfsScheduleCancelPendingOp(
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsRenameFCB(
    PPVFS_FCB pFcb,
    PPVFS_CCB pCcb,
    PSTR pszNewFilename
    );

BOOLEAN
PvfsFcbIsPendingDelete(
    PPVFS_FCB pFcb
    );

VOID
PvfsFcbSetPendingDelete(
    PPVFS_FCB pFcb,
    BOOLEAN bPendingDelete
    );

PPVFS_FCB
PvfsGetParentFCB(
    PPVFS_FCB pFcb
    );

LONG64
PvfsClearLastWriteTimeFCB(
    PPVFS_FCB pFcb
    );

VOID
PvfsSetLastWriteTimeFCB(
    PPVFS_FCB pFcb,
    LONG64 LastWriteTime
    );

// From fcbtable.c

NTSTATUS
PvfsFcbTableInitialize(
    VOID
    );

NTSTATUS
PvfsFcbTableDestroy(
    VOID
    );

NTSTATUS
PvfsFcbTableLookup(
    PPVFS_FCB *ppFcb,
    PPVFS_FCB_TABLE_ENTRY pBucket,
    PSTR pszFilename
    );

NTSTATUS
PvfsFcbTableLookup_inlock(
    PPVFS_FCB *ppFcb,
    PPVFS_FCB_TABLE_ENTRY pBucket,
    PCSTR pszFilename
    );

NTSTATUS
PvfsFcbTableAdd_inlock(
    PPVFS_FCB_TABLE_ENTRY pBucket,
    PPVFS_FCB pFcb
    );

NTSTATUS
PvfsFcbTableRemove_inlock(
    PPVFS_FCB_TABLE_ENTRY pBucket,
    PPVFS_FCB pFcb
    );

NTSTATUS
PvfsFcbTableRemove(
    PPVFS_FCB_TABLE_ENTRY pBucket,
    PPVFS_FCB pFcb
    );

NTSTATUS
PvfsFcbTableGetBucket(
    OUT PPVFS_FCB_TABLE_ENTRY *ppBucket,
    IN PPVFS_FCB_TABLE pFcbTable,
    IN PVOID pKey
    );






#endif   /* _PVFS_FCB_H */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
