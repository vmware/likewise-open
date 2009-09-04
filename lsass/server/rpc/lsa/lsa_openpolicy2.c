/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        lsa_openpolicy2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaOpenPolicy2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvOpenPolicy2(
    /* [in] */ handle_t hBinding,
    /* [in] */ wchar16_t *system_name,
    /* [in] */ ObjectAttribute *attrib,
    /* [in] */ uint32 access_mask,
    /* [out] */ POLICY_HANDLE *hPolicy
    )
{
    const DWORD dwSamrConnAccess     = SAMR_ACCESS_CONNECT_TO_SERVER;
    const DWORD dwSamrDomainAccess   = DOMAIN_ACCESS_LOOKUP_INFO_1 |
                                       DOMAIN_ACCESS_LOOKUP_ALIAS;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    RPCSTATUS rpcStatus = 0;
    PPOLICY_CONTEXT pPolCtx = NULL;
    HANDLE hDirectory = NULL;
    PSTR pszSamrLpcSocketPath = NULL;
    handle_t hSamrBinding = NULL;
    CHAR szHostname[64] = {0};
    PWSTR pwszSystemName = NULL;
    PolicyHandle hConn;
    DWORD dwResume = 0;
    DWORD dwMaxSize = -1;
    DWORD dwCount = 0;
    PWSTR *ppwszLocalDomainNames = NULL;
    DWORD i = 0;
    PWSTR pwszDomainName = NULL;
    PSID pDomainSid = NULL;
    PolicyHandle hDomain;
    SAM_DOMAIN_ENTRY Domain = {0};

    ntStatus = RTL_ALLOCATE(&pPolCtx, POLICY_CONTEXT, sizeof(*pPolCtx));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = DirectoryOpen(&hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    pPolCtx->Type        = LsaContextPolicy;
    pPolCtx->refcount    = 1;
    pPolCtx->hDirectory  = hDirectory;

    ntStatus = LsaSrvCreateSamDomainsTable(&pPolCtx->pDomains);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Connect samr rpc server and get basic domain info
     */

    dwError = LsaSrvConfigGetSamrLpcSocketPath(&pszSamrLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    rpcStatus = InitSamrBindingFull(&hSamrBinding,
                                    "ncalrpc",
                                    szHostname,
                                    pszSamrLpcSocketPath,
                                    NULL,
                                    NULL,
                                    NULL);
    if (rpcStatus) {
        dwError = LW_ERROR_RPC_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pPolCtx->hSamrBinding = hSamrBinding;

    dwError = gethostname(szHostname, sizeof(szHostname));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s((PSTR)szHostname, &pwszSystemName);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrConnect2(hSamrBinding,
                            pwszSystemName,
                            dwSamrConnAccess,
                            &hConn);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pPolCtx->hConn = hConn;

    /* Get local and builtin domains info */
    ntStatus = SamrEnumDomains(hSamrBinding,
                               &hConn,
                               &dwResume,
                               dwMaxSize,
                               &ppwszLocalDomainNames,
                               &dwCount);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    for (i = 0; i < dwCount; i++)
    {
        pwszDomainName = ppwszLocalDomainNames[i];
        memset(&Domain, 0, sizeof(Domain));

        ntStatus = SamrLookupDomain(hSamrBinding,
                                    &hConn,
                                    pwszDomainName,
                                    &pDomainSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = SamrOpenDomain(hSamrBinding,
                                  &hConn,
                                  dwSamrDomainAccess,
                                  pDomainSid,
                                  &hDomain);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        Domain.pwszName = pwszDomainName;
        Domain.pSid     = pDomainSid;
        Domain.bLocal   = TRUE;
        Domain.hDomain  = hDomain;

        ntStatus = LsaSrvSetSamDomain(pPolCtx,
                                      &Domain);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        SamrFreeMemory(pDomainSid);
        pDomainSid = NULL;
    }

    /* Increase ref count because DCE/RPC runtime is about to use this
       pointer as well */
    InterlockedIncrement(&pPolCtx->refcount);

    *hPolicy = (POLICY_HANDLE)pPolCtx;

cleanup:
    if (pDomainSid)
    {
        SamrFreeMemory(pDomainSid);
    }

    if (ppwszLocalDomainNames)
    {
        SamrFreeMemory(ppwszLocalDomainNames);
    }

    LW_SAFE_FREE_MEMORY(pwszSystemName);
    LW_SAFE_FREE_MEMORY(pszSamrLpcSocketPath);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pPolCtx) {
        InterlockedDecrement(&pPolCtx->refcount);
        POLICY_HANDLE_rundown((POLICY_HANDLE)pPolCtx);
    }

    *hPolicy = NULL;
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
