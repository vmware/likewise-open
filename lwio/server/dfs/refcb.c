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
 *        refcb.c
 *
 * Abstract:
 *
 *        Likewise Distributed File System Driver (DFS)
 *
 *        Referral Control Block routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "dfs.h"

static
VOID
DfsFreeReferralCB(
    PDFS_REFERRAL_CONTROL_BLOCK pReferralCB
    );


/***********************************************************************
 **********************************************************************/


NTSTATUS
DfsAllocateReferralCB(
    PDFS_REFERRAL_CONTROL_BLOCK *ppReferralCB,
    PWSTR pwszReferralName
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDFS_REFERRAL_CONTROL_BLOCK pReferralCB = NULL;

    *ppReferralCB = NULL;

    ntStatus = DfsAllocateMemory(
                   (PVOID*)&pReferralCB,
                   sizeof(DFS_REFERRAL_CONTROL_BLOCK));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlWC16StringDuplicate(
                   &pReferralCB->pwszReferralName,
                   pwszReferralName);
    BAIL_ON_NT_STATUS(ntStatus);

    LwListInit(&pReferralCB->TargetList);

    pthread_rwlock_init(&pReferralCB->RwLock, NULL);
    pReferralCB->pRwLock = &pReferralCB->RwLock;

    pReferralCB->RefCount = 1;

    *ppReferralCB = pReferralCB;

cleanup:
    return ntStatus;

error:
    if (pReferralCB)
    {
        DfsFreeReferralCB(pReferralCB);
    }

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
VOID
DfsFreeReferralCB(
    PDFS_REFERRAL_CONTROL_BLOCK pReferralCB
    )
{
    PLW_LIST_LINKS pTargetLink = NULL;
    PLW_LIST_LINKS pNextTargetLink = NULL;
    PDFS_REFERRAL_TARGET pTarget = NULL;

    if (pReferralCB->pRwLock)
    {
        pthread_rwlock_destroy(&pReferralCB->RwLock);
        pReferralCB->pRwLock = NULL;
    }

    if (pReferralCB->pwszReferralName)
    {
        LwRtlWC16StringFree(&pReferralCB->pwszReferralName);
    }

    pTargetLink = LwListTraverse(&pReferralCB->TargetList, NULL);

    while (pTargetLink)
    {
        pNextTargetLink = LwListTraverse(
                              &pReferralCB->TargetList,
                              pTargetLink);

        LwListRemove(pTargetLink);

        pTarget = LW_STRUCT_FROM_FIELD(
                      pTargetLink,
                      DFS_REFERRAL_TARGET,
                      ReferralLink);

        DfsFreeReferralTarget(pTarget);

        pTargetLink = pNextTargetLink;
    }

    DfsFreeMemory((PVOID*)&pReferralCB);

    return;
}

/***********************************************************************
 **********************************************************************/

VOID
DfsReleaseReferralCB(
    PDFS_REFERRAL_CONTROL_BLOCK *ppReferralCB
    )
{
    PDFS_REFERRAL_CONTROL_BLOCK pReferralCB = *ppReferralCB;

    if (InterlockedDecrement(&pReferralCB->RefCount) == 0)
    {
        DfsFreeReferralCB(pReferralCB);
    }

    *ppReferralCB = NULL;

    return;
}

/***********************************************************************
 **********************************************************************/

PDFS_REFERRAL_CONTROL_BLOCK
DfsReferenceReferralCB(
    PDFS_REFERRAL_CONTROL_BLOCK pReferralCB
    )
{
    InterlockedIncrement(&pReferralCB->RefCount);

    return pReferralCB;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsReferralParseTarget(
    PDFS_REFERRAL_CONTROL_BLOCK pReferralCB,
    PWSTR pwszTarget
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszTTL = NULL;
    PSTR pszTTL = NULL;
    PSTR pszNext = NULL;
    ULONG Ttl = 0;
    PDFS_REFERRAL_TARGET pReferralTarget = NULL;

    pwszTTL = DfsWC16StringFindCharacter(pwszTarget, L':');
    if ((pwszTTL == NULL) || (*(pwszTTL+1) == 0x0))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    *pwszTTL = 0x0;
    pwszTTL++;
    ntStatus = LwRtlCStringAllocateFromWC16String(&pszTTL, pwszTTL);
    BAIL_ON_NT_STATUS(ntStatus);

    Ttl = strtoll(pszTTL, &pszNext, 10);

    if ((Ttl == 0) || (*pszNext != '\0'))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = DfsAllocateReferralTarget(
                   &pReferralTarget,
                   pwszTarget,
                   Ttl);
    BAIL_ON_NT_STATUS(ntStatus);

    LwListInsertAfter(
        &pReferralCB->TargetList,
        &pReferralTarget->ReferralLink);

cleanup:
    if (pszTTL)
    {
        LwRtlCStringFree(&pszTTL);
    }

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
