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
 *        samr_lookuprids.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        SamrLookupRids function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
SamrLookupRids(
    IN  handle_t      hSamrBinding,
    IN  PolicyHandle *phDomain,
    IN  UINT32        NumRids,
    IN  UINT32       *pRids,
    OUT PWSTR       **pppwszNames,
    OUT UINT32      **ppTypes
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UINT32 iRid = 0;
    UnicodeStringArray Names = {0};
    Ids Types = {0};
    PWSTR *ppwszNames = NULL;
    UINT32 *pTypes = NULL;

    BAIL_ON_INVALID_PTR(hSamrBinding, ntStatus);
    BAIL_ON_INVALID_PTR(phDomain, ntStatus);
    BAIL_ON_INVALID_PTR(pRids, ntStatus);
    BAIL_ON_INVALID_PTR(pppwszNames, ntStatus);
    BAIL_ON_INVALID_PTR(ppTypes, ntStatus);

    DCERPC_CALL(ntStatus, _SamrLookupRids(hSamrBinding,
                                          phDomain,
                                          NumRids,
                                          pRids,
                                          &Names,
                                          &Types));
    BAIL_ON_NT_STATUS(ntStatus);

    if (Names.count > 0) {
        ntStatus = SamrAllocateMemory((void**)&ppwszNames,
                                      sizeof(wchar16_t*) * Names.count,
                                      NULL);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SamrAllocateMemory((void**)&pTypes,
                                      sizeof(UINT32) * Names.count,
                                      NULL);
        BAIL_ON_NT_STATUS(ntStatus);

        for (iRid = 0; iRid < Names.count; iRid++) {
            UnicodeString *pName = &(Names.names[iRid]);

            ppwszNames[iRid] = GetFromUnicodeString(pName);
            BAIL_ON_NULL_PTR(ppwszNames[iRid], ntStatus);

            ntStatus = SamrAddDepMemory(ppwszNames[iRid],
                                        (void*)ppwszNames);
            BAIL_ON_NT_STATUS(ntStatus);

            pTypes[iRid] = Types.ids[iRid];
        }
    }

    *pppwszNames = ppwszNames;
    *ppTypes     = pTypes;

cleanup:
    SamrCleanStubIds(&Types);
    SamrCleanStubUnicodeStringArray(&Names);

    return ntStatus;

error:
    if (ppwszNames) {
        SamrFreeMemory((void*)ppwszNames);
    }

    if (pTypes) {
        SamrFreeMemory((void*)pTypes);
    }

    *pppwszNames = NULL;
    *ppTypes     = NULL;

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
