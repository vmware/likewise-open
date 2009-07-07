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

#include "includes.h"


WINERR
DsrEnumerateDomainTrusts(
    IN  handle_t           hNetrBinding,
    IN  PCWSTR             pwszServer,
    IN  UINT32             Flags,
    OUT NetrDomainTrust  **ppTrusts,
    OUT PUINT32            pCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    PWSTR pwszServerName = NULL;
    NetrDomainTrustList TrustList;
    NetrDomainTrust *pTrusts = NULL;

    memset((void*)&TrustList, 0, sizeof(TrustList));

    BAIL_ON_INVALID_PTR(hNetrBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServer, ntStatus);
    BAIL_ON_INVALID_PTR(ppTrusts, ntStatus);
    BAIL_ON_INVALID_PTR(pCount, ntStatus);
    
    pwszServerName = wc16sdup(pwszServer);
    BAIL_ON_NULL_PTR(pwszServerName, ntStatus);

    DCERPC_CALL(err, _DsrEnumerateDomainTrusts(hNetrBinding,
                                               pwszServerName,
                                               Flags,
                                               &TrustList));
    BAIL_ON_WINERR(err);

    ntStatus = NetrAllocateDomainTrusts(&pTrusts,
                                        &TrustList);
    BAIL_ON_NT_STATUS(ntStatus);

    *pCount   = TrustList.count;
    *ppTrusts = pTrusts;

cleanup:
    SAFE_FREE(pwszServerName);
    NetrCleanStubDomainTrustList(&TrustList);

    if (err == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS) {
        err = NtStatusToWin32Error(ntStatus);
    }

    return err;

error:
    if (pTrusts) {
        NetrFreeMemory((void*)pTrusts);
    }

    *pCount   = 0;
    *ppTrusts = NULL;

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
