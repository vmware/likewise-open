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
 *        scb.h
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        File Control Block routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#ifndef _PVFS_SCB_H
#define _PVFS_SCB_H

#include "pvfs.h"


// From scb.c

NTSTATUS
PvfsAllocateSCB(
    PPVFS_SCB *ppScb
    );

PPVFS_SCB
PvfsReferenceSCB(
    IN PPVFS_SCB pScb
    );

VOID
PvfsReleaseSCB(
    PPVFS_SCB *ppScb
    );

NTSTATUS
PvfsCreateSCB(
    OUT PPVFS_SCB *ppScb,
    IN PSTR pszFilename,
    IN BOOLEAN bCheckShareAccess,
    IN FILE_SHARE_FLAGS SharedAccess,
    IN ACCESS_MASK DesiredAccess
    );

NTSTATUS
PvfsGetFullStreamname(
    PSTR *ppszFullStreamname,
    PPVFS_SCB pScb
    );

NTSTATUS
PvfsGetDirectorySCB(
    IN  PSTR pszFilename,
    OUT PPVFS_SCB *ppScb
    );

NTSTATUS
PvfsAddCCBToSCB(
    PPVFS_SCB pScb,
    PPVFS_CCB pCcb
    );

NTSTATUS
PvfsRemoveCCBFromSCB(
    PPVFS_SCB pScb,
    PPVFS_CCB pCcb
    );

BOOLEAN
PvfsStreamHasOtherOpens(
    IN PPVFS_SCB pScb,
    IN PPVFS_CCB pCcb
    );

BOOLEAN
PvfsStreamIsOplocked(
    IN PPVFS_SCB pScb
    );

BOOLEAN
PvfsStreamIsOplockedExclusive(
    IN PPVFS_SCB pScb
    );

BOOLEAN
PvfsStreamIsOplockedShared(
    IN PPVFS_SCB pScb
    );

NTSTATUS
PvfsPendOplockBreakTest(
    IN PPVFS_SCB pScb,
    IN PPVFS_IRP_CONTEXT pIrpContext,
    IN PPVFS_CCB pCcb,
    IN PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK pfnCompletion,
    IN PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX pfnFreeContext,
    IN PVOID pCompletionContext
    );

NTSTATUS
PvfsAddItemPendingOplockBreakAck(
    IN OUT PPVFS_SCB pScb,
    IN     PPVFS_IRP_CONTEXT pIrpContext,
    IN     PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK pfnCompletion,
    IN     PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX pfnFreeContext,
    IN     PVOID pCompletionContext
    );

NTSTATUS
PvfsAddOplockRecord(
    IN OUT PPVFS_SCB pScb,
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
PvfsRenameSCB(
    PPVFS_SCB pScb,
    PPVFS_CCB pCcb,
    PSTR pszNewFilename
    );

BOOLEAN
PvfsScbIsPendingDelete(
    PPVFS_SCB pScb
    );

VOID
PvfsScbSetPendingDelete(
    PPVFS_SCB pScb,
    BOOLEAN bPendingDelete
    );

PPVFS_SCB
PvfsGetParentSCB(
    PPVFS_SCB pScb
    );

LONG64
PvfsClearLastWriteTimeSCB(
    PPVFS_SCB pScb
    );

VOID
PvfsSetLastWriteTimeSCB(
    PPVFS_SCB pScb,
    LONG64 LastWriteTime
    );

#endif   /* _PVFS_SCB_H */

