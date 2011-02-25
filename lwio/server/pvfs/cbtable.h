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
 *        cbtable.h
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        File Control Block routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#ifndef _PVFS_CBTABLE_H
#define _PVFS_CBTABLE_H

#include "pvfs.h"

NTSTATUS
PvfsCbTableInitialize(
    PPVFS_CB_TABLE pCbTable
    );

NTSTATUS
PvfsCbTableDestroy(
    PPVFS_CB_TABLE pCbTable
    );


NTSTATUS
PvfsHashTableCreate(
    size_t sTableSize,
    PFN_LWRTL_RB_TREE_COMPARE fnCompare,
    PFN_LWRTL_RB_TREE_FREE_KEY fnFreeHashKey,
    PVFS_HASH_KEY fnHash,
    PVFS_HASH_FREE_ENTRY fnFree,
    PPVFS_HASH_TABLE* ppTable
    );

VOID
PvfsHashTableDestroy(
    PVFS_HASH_TABLE** ppTable
    );

size_t
PvfsCbTableHashKey(
    PCVOID KeyString
    );

VOID
PvfsCbTableFreeHashEntry(
    PPVFS_CB_TABLE_ENTRY *ppEntry
    );

int
PvfsCbTableFilenameCompare(
    PCVOID a,
    PCVOID b
    );

VOID
PvfsCbTableFreeHashKey(
    PVOID pKey
    );

NTSTATUS
PvfsCbTableLookup(
    PPVFS_CONTROL_BLOCK *ppCb,
    PPVFS_CB_TABLE_ENTRY pBucket,
    PCSTR KeyString
    );

NTSTATUS
PvfsCbTableLookup_inlock(
    PPVFS_CONTROL_BLOCK *ppCb,
    PPVFS_CB_TABLE_ENTRY pBucket,
    PCSTR KeyString
    );

NTSTATUS
PvfsCbTableAdd_inlock(
    PPVFS_CB_TABLE_ENTRY pBucket,
    PSTR KeyString,
    PPVFS_CONTROL_BLOCK pCb
    );

NTSTATUS
PvfsCbTableRemove_inlock(
    PPVFS_CB_TABLE_ENTRY pBucket,
    PSTR KeyString
    );

NTSTATUS
PvfsCbTableRemove(
    PPVFS_CB_TABLE_ENTRY pBucket,
    PSTR KeyString
    );

NTSTATUS
PvfsCbTableGetBucket(
    OUT PPVFS_CB_TABLE_ENTRY *ppBucket,
    IN PPVFS_CB_TABLE pScbTable,
    IN PVOID pKey
    );


#endif   /* _PVFS_CBTABLE_H */

