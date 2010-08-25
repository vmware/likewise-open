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
 *        reftable.c
 *
 * Abstract:
 *
 *        Likewise Distributed File System Driver (DFS)
 *
 *        DFS Referral Control Block Table routineus
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "dfs.h"

static
int
DfsReferralTableCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
DfsReferralTableFreeKey(
    PVOID pKey
    );

static
VOID
DfsReferralTableFreeData(
    PVOID pData
    );


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsReferralTableInitialize(
    PDFS_REFERRAL_TABLE pReferralTable
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pReferralTable, ntStatus);

    ntStatus = LwRtlRBTreeCreate(
                   DfsReferralTableCompare,
                   DfsReferralTableFreeKey,
                   DfsReferralTableFreeData,
                   &pReferralTable->pTable);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    if (pReferralTable)
    {
        if (pReferralTable->pTable)
        {
            LwRtlRBTreeFree(pReferralTable->pTable);
            pReferralTable->pTable = NULL;
        }
    }

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
int
DfsCtrlBlockTableCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    return wc16scasecmp((const wchar16_t*)pKey1, (const wchar16_t*)pKey2);
}


/***********************************************************************
 **********************************************************************/

static
VOID
DfsReferralTableFreeKey(
    PVOID pKey
    )
{
    return;
}


/***********************************************************************
 **********************************************************************/

static
VOID
DfsReferralTableFreeData(
    PVOID pData
    )
{
    return;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsReferralTableAdd_inlock(
    PDFS_REFERRAL_TABLE pReferralTable,
    PDFS_REFERRAL_CONTROL_BLOCK pReferralCB
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = LwRtlRBTreeAdd(
                   pReferralTable->pTable,
                   pReferralCB->pwszReferralName,
                   pReferralCB);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
