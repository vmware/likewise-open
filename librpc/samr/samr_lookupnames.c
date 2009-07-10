/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samr_lookupnames.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrLookupNames function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrLookupNames(
    IN  handle_t      hSamrBinding,
    IN  PolicyHandle *phDomain,
    IN  UINT32        NumNames,
    IN  PWSTR        *ppwszNames,
    OUT UINT32      **ppRids,
    OUT UINT32      **ppTypes,
    OUT UINT32       *pRidsCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UnicodeString *pNames = NULL;
    Ids Rids = {0};
    Ids Types = {0};
    UINT32 *pRids = NULL;
    uint32 *pTypes = NULL;

    BAIL_ON_INVALID_PTR(hSamrBinding, ntStatus);
    BAIL_ON_INVALID_PTR(phDomain, ntStatus);
    BAIL_ON_INVALID_PTR(ppwszNames, ntStatus);
    BAIL_ON_INVALID_PTR(ppRids, ntStatus);
    BAIL_ON_INVALID_PTR(ppTypes, ntStatus);
    /* pRidsCount can be NULL, in which case the number of returned rids must
       match num_names. */

    pNames = InitUnicodeStringArray(ppwszNames, NumNames);
    BAIL_ON_NULL_PTR(pNames, ntStatus);

    DCERPC_CALL(ntStatus, _SamrLookupNames(hSamrBinding,
                                           phDomain,
                                           NumNames,
                                           pNames,
                                           &Rids,
                                           &Types));
    BAIL_ON_NT_STATUS(ntStatus);

    if (Rids.count != Types.count)
    {
        ntStatus = STATUS_REPLY_MESSAGE_MISMATCH;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SamrAllocateIds(&pRids, &Rids);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrAllocateIds(&pTypes, &Types);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pRidsCount)
    {
        *pRidsCount = Rids.count;
    }
    else if (Rids.count != NumNames)
    {
        ntStatus = STATUS_REPLY_MESSAGE_MISMATCH;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppRids  = pRids;
    *ppTypes = pTypes;

cleanup:
    SamrCleanStubIds(&Rids);
    SamrCleanStubIds(&Types);

    FreeUnicodeStringArray(pNames, NumNames);

    return ntStatus;

error:
    if (pRids) {
        SamrFreeMemory((void*)pRids);
    }

    if (pTypes) {
        SamrFreeMemory((void*)pTypes);
    }

    if (pRidsCount) {
        *pRidsCount = 0;
    }

    *ppRids  = NULL;
    *ppTypes = NULL;

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
