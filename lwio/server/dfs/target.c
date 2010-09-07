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
 *        target.c
 *
 * Abstract:
 *
 *        Likewise Distributed File System Driver (DFS)
 *
 *        Referral Target routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "dfs.h"


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsAllocateReferralTarget(
    OUT PDFS_REFERRAL_TARGET *ppReferralTarget,
    IN PWSTR pwszTarget,
    IN ULONG TTL
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDFS_REFERRAL_TARGET pReferralTarget = NULL;

    ntStatus = DfsAllocateMemory(
                    (PVOID*)&pReferralTarget,
                    sizeof(*pReferralTarget));
    BAIL_ON_NT_STATUS(ntStatus);

    LwListInit(&pReferralTarget->ReferralLink);

    ntStatus = LwRtlWC16StringDuplicate(
                   &pReferralTarget->pwszTargetPath,
                   pwszTarget);
    BAIL_ON_NT_STATUS(ntStatus);

    pReferralTarget->Ttl = TTL;

    *ppReferralTarget = pReferralTarget;
cleanup:
    return ntStatus;

error:
    if (pReferralTarget)
    {
        DfsFreeReferralTarget(pReferralTarget);
    }

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

VOID
DfsFreeReferralTarget(
    PDFS_REFERRAL_TARGET pTarget
    )
{
    if (pTarget->pwszTargetPath)
    {
        LwRtlWC16StringFree(&pTarget->pwszTargetPath);
    }

    return;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
