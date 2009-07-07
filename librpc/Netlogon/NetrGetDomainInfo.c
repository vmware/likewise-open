/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4:
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
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
NetrGetDomainInfo(
    IN  handle_t          hNetrBinding,
    IN  NetrCredentials  *pCreds,
    IN  PCWSTR            pwszServer,
    IN  PCWSTR            pwszComputer,
    IN  UINT32            Level,
    IN  NetrDomainQuery  *pQuery,
    OUT NetrDomainInfo  **ppDomainInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszServerName = NULL;
    PWSTR pwszComputerName = NULL;
    NetrAuth *pAuth = NULL;
    NetrAuth *pReturnedAuth = NULL;
    NetrDomainInfo *pDomainInfo = NULL;
    NetrDomainInfo DomainInfo;

    memset((void*)&DomainInfo, 0, sizeof(DomainInfo));

    BAIL_ON_INVALID_PTR(hNetrBinding, ntStatus);
    BAIL_ON_INVALID_PTR(pCreds, ntStatus);
    BAIL_ON_INVALID_PTR(pwszServer, ntStatus);
    BAIL_ON_INVALID_PTR(pwszComputer, ntStatus);
    BAIL_ON_INVALID_PTR(pQuery, ntStatus);
    BAIL_ON_INVALID_PTR(ppDomainInfo, ntStatus);

    ntStatus = NetrAllocateUniString(&pwszServerName,
                                     pwszServer,
                                     NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateUniString(&pwszComputerName,
                                     pwszComputer,
                                     NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Create authenticator info with credentials chain */
    ntStatus = NetrAllocateMemory((void**)&pAuth,
                                  sizeof(NetrAuth),
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    pCreds->sequence += 2;
    NetrCredentialsCliStep(pCreds);

    pAuth->timestamp = pCreds->sequence;
    memcpy(pAuth->cred.data,
           pCreds->cli_chal.data,
           sizeof(pAuth->cred.data));

    /* Allocate returned authenticator */
    ntStatus = NetrAllocateMemory((void**)&pReturnedAuth,
                                  sizeof(NetrAuth),
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    DCERPC_CALL(ntStatus, _NetrLogonGetDomainInfo(hNetrBinding,
                                                  pwszServerName,
                                                  pwszComputerName,
                                                  pAuth,
                                                  pReturnedAuth,
                                                  Level,
                                                  pQuery,
                                                  &DomainInfo));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NetrAllocateDomainInfo(&pDomainInfo,
                                      &DomainInfo,
                                      Level);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppDomainInfo = pDomainInfo;

cleanup:
    NetrCleanStubDomainInfo(&DomainInfo, Level);

    if (pwszServerName) {
        NetrFreeMemory((void*)pwszServerName);
    }

    if (pwszComputerName) {
        NetrFreeMemory((void*)pwszComputerName);
    }

    if (pAuth) {
        NetrFreeMemory((void*)pAuth);
    }

    return ntStatus;

error:
    if (pDomainInfo) {
        NetrFreeMemory((void*)pDomainInfo);
    }

    *ppDomainInfo = NULL;
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
