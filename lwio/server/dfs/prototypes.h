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
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise Distributed File System (DFS)
 *
 *        Driver funtion prototypes
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */


#ifndef __DFS_PROTOTYPES_H__
#define __DFS_PROTOTYPES_H__

#include "dfs.h"


//
// From memory.c
//

NTSTATUS
DfsAllocateMemory(
    OUT PVOID *ppBuffer,
    IN DWORD dwSize
    );

NTSTATUS
DfsReallocateMemory(
    IN OUT PVOID *ppBuffer,
    IN DWORD dwNewSize
    );

VOID
DfsFreeMemory(
    IN OUT PVOID *ppBuffer
    );


//
// From irpctx.c
//

NTSTATUS
DfsAllocateIrpContext(
    PDFS_IRP_CONTEXT *ppIrpContext,
    PIRP pIrp
    );

PDFS_IRP_CONTEXT
DfsReferenceIrpContext(
    PDFS_IRP_CONTEXT pIrpContext
    );

VOID
DfsReleaseIrpContext(
    PDFS_IRP_CONTEXT *ppIrpContext
    );

BOOLEAN
DfsIrpContextCheckFlag(
    PDFS_IRP_CONTEXT pIrpContext,
    USHORT BitToCheck
    );

VOID
DfsIrpContextSetFlag(
    PDFS_IRP_CONTEXT pIrpContext,
    USHORT BitToSet
    );

VOID
DfsIrpContextClearFlag(
    PDFS_IRP_CONTEXT pIrpContext,
    USHORT BitToClear
    );

USHORT
DfsIrpContextConditionalSetFlag(
    PDFS_IRP_CONTEXT pIrpContext,
    USHORT BitToCheck,
    USHORT BitToSetOnTrue,
    USHORT BitToSetOnFalse
    );

VOID
DfsIrpMarkPending(
    IN PDFS_IRP_CONTEXT pIrpContext,
    IN PIO_IRP_CALLBACK CancelCallback,
    IN OPTIONAL PVOID CancelCallbackContext
    );

VOID
DfsAsyncIrpComplete(
    PDFS_IRP_CONTEXT pIrpContext
    );


//
// From close.c
//

NTSTATUS
DfsClose(
    PDFS_IRP_CONTEXT  pIrpContext
    );


//
// From create.c
//

NTSTATUS
DfsCreate(
    PDFS_IRP_CONTEXT  pIrpContext
    );


//
// From deviceio.c
//

NTSTATUS
DfsDeviceIoControl(
    PDFS_IRP_CONTEXT  pIrpContext
    );


//
// From ccb.c
//

NTSTATUS
DfsAllocateCCB(
    PDFS_CREATE_CONTROL_BLOCK *ppCcb
    );

VOID
DfsFreeCCB(
    PDFS_CREATE_CONTROL_BLOCK pCcb
    );

VOID
DfsReleaseCCB(
    PDFS_CREATE_CONTROL_BLOCK pCcb
    );

PDFS_CREATE_CONTROL_BLOCK
DfsReferenceCCB(
    PDFS_CREATE_CONTROL_BLOCK pCcb
    );

NTSTATUS
DfsStoreCCB(
    IO_FILE_HANDLE FileHandle,
    PDFS_CREATE_CONTROL_BLOCK pCcb
    );

NTSTATUS
DfsAcquireCCB(
    IO_FILE_HANDLE FileHandle,
    PDFS_CREATE_CONTROL_BLOCK * ppCcb
    );


//
// From rootcb.c
//

NTSTATUS
DfsAllocateRootCB(
    PDFS_ROOT_CONTROL_BLOCK *ppRootCB,
    PWSTR pwszDfsRootName
    );

VOID
DfsReleaseRootCB(
    PDFS_ROOT_CONTROL_BLOCK *ppRootCB
    );

PDFS_ROOT_CONTROL_BLOCK
DfsReferenceRootCB(
    PDFS_ROOT_CONTROL_BLOCK pRootCB
    );


//
// From refcb.c
//

NTSTATUS
DfsAllocateReferralCB(
    PDFS_REFERRAL_CONTROL_BLOCK *ppReferralCB,
    PWSTR pwszReferralName
    );

VOID
DfsReleaseReferralCB(
    PDFS_REFERRAL_CONTROL_BLOCK *ppReferralCB
    );

PDFS_REFERRAL_CONTROL_BLOCK
DfsReferenceReferralCB(
    PDFS_REFERRAL_CONTROL_BLOCK pReferralCB
    );

NTSTATUS
DfsReferralParseTarget(
    PDFS_REFERRAL_CONTROL_BLOCK pReferralCB,
    PWSTR pwszTarget
    );


//
// From config.c
//

NTSTATUS
DfsConfigReadStandaloneRoots(
    VOID
    );

//
// From roottable.c
//

NTSTATUS
DfsRootCtrlBlockTableInitialize(
    PDFS_ROOT_CONTROL_BLOCK_TABLE pRootCtrlBlockTable
    );

NTSTATUS
DfsRootCtrlBlockTableAdd(
    PDFS_ROOT_CONTROL_BLOCK_TABLE pRootTable,
    PDFS_ROOT_CONTROL_BLOCK pRootCB
    );


//
// From reftable.c
//

NTSTATUS
DfsReferralTableInitialize(
    PDFS_REFERRAL_TABLE pReferralTable
    );

NTSTATUS
DfsReferralTableAdd_inlock(
    PDFS_REFERRAL_TABLE pReferralTable,
    PDFS_REFERRAL_CONTROL_BLOCK pReferralCB
    );


//
// From target.c
//

NTSTATUS
DfsAllocateReferralTarget(
    OUT PDFS_REFERRAL_TARGET *ppReferralTarget,
    IN PWSTR pwszTarget,
    IN ULONG TTL
    );

VOID
DfsFreeReferralTarget(
    PDFS_REFERRAL_TARGET pTarget
    );

//
// From string.c
//

WCHAR*
DfsWC16StringFindCharacter(
    PWSTR pwszString,
    WCHAR Needle
    );


#endif /* __DFS_PROTOTYPES_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

