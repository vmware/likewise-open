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


NTSTATUS
NetrEnumerateTrustedDomainsEx(
    IN  handle_t          hNetrBinding,
    IN  PCWSTR            pwszServerName,
    OUT NetrDomainTrust **ppTrusts,
    OUT PUINT32           pCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszName = NULL;
    NetrDomainTrustList TrustList = {0};
    NetrDomainTrust *pTrusts = NULL;

    BAIL_ON_INVALID_PTR(hNetrBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServerName, ntStatus);
    BAIL_ON_INVALID_PTR(ppTrusts, ntStatus);
    BAIL_ON_INVALID_PTR(pCount, ntStatus);

    pwszName = wc16sdup(pwszServerName);
    BAIL_ON_NULL_PTR(pwszName, ntStatus);

    DCERPC_CALL(ntStatus, _NetrEnumerateTrustedDomainsEx(hNetrBinding,
                                                         pwszName,
                                                         &TrustList));
    BAIL_ON_NT_STATUS(ntStatus);

    *pCount  = TrustList.count;

    ntStatus = NetrAllocateDomainTrusts(&pTrusts,
                                        &TrustList);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppTrusts = pTrusts;

cleanup:
    SAFE_FREE(pwszName);
    NetrCleanStubDomainTrustList(&TrustList);

    return ntStatus;

error:
    if (pTrusts) {
        NetrFreeMemory((void*)pTrusts);
    }

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
