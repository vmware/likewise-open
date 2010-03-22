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
// From fsctl.c
//

NTSTATUS
DfsFsIoControl(
    PDFS_IRP_CONTEXT  pIrpContext
    );


//
// From fcb.c
//

NTSTATUS
DfsInitializeFCBTable(
    VOID
    );

NTSTATUS
DfsInitializeFCBTable(
    VOID
    );

NTSTATUS
DfsAllocateFCB(
    PDFS_FCB *ppFcb
    );

NTSTATUS
DfsAddReferralFCB(
    PDFS_FCB pFcb,
    PSTR pszReferral
    );

PDFS_FCB
DfsReferenceFCB(
    IN PDFS_FCB pFcb
    );

VOID
DfsReleaseFCB(
    PDFS_FCB *ppFcb
    );

NTSTATUS
DfsFindFCB(
    PDFS_FCB *ppFcb,
    PSTR pszFilename
    );

NTSTATUS
DfsCreateFCB(
    OUT PDFS_FCB *ppFcb,
    IN  PSTR pszPathname
    );

NTSTATUS
DfsAddCCBToFCB(
    PDFS_FCB pFcb,
    PDFS_CCB pCcb
    );

NTSTATUS
DfsRemoveCCBFromFCB(
    PDFS_FCB pFcb,
    PDFS_CCB pCcb
    );


//
// From ccb.c
//

NTSTATUS
DfsAllocateCCB(
    PDFS_CCB *ppCcb
    );

NTSTATUS
DfsFreeCCB(
    PDFS_CCB pCcb
    );

VOID
DfsReleaseCCB(
    PDFS_CCB pCcb
    );

PDFS_CCB
DfsReferenceCCB(
    PDFS_CCB pCcb
    );

NTSTATUS
DfsStoreCCB(
    IO_FILE_HANDLE FileHandle,
    PDFS_CCB pCcb
    );

NTSTATUS
DfsAcquireCCB(
    IO_FILE_HANDLE FileHandle,
    PDFS_CCB * ppCcb
    );


//
// From config.c
//

NTSTATUS
DfsConfigRegistryInit(
    VOID
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

