/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        srvsvc_netsharegetinfo.c
 *
 * Abstract:
 *
 *        Likewise Workstation Service (wkssvc) RPC client and server
 *
 *        NetWkstaGetInfo server API
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "includes.h"


NET_API_STATUS
WksSvcNetWkstaGetInfo(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ wchar16_t *server_name,
    /* [in] */ uint32 level,
    /* [out] */ wkssvc_NetWkstaInfo *info
    )
{
    const DWORD dwPolicyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                                     LSA_ACCESS_VIEW_POLICY_INFO;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    RPCSTATUS rpcStatus = RPC_S_OK;
    DWORD dwError = 0;
    PWKSTA_INFO_100 pInfo100 = NULL;
    PSTR pszDomainFqdn = NULL;
    PSTR pszDcName = NULL;
    PWSTR pwszDcName = NULL;
    PIO_ACCESS_TOKEN pAccessToken = NULL;
    CHAR szHostname[64];
    PSTR pszLsaLpcSocketPath = NULL;
    handle_t hLsaBinding = NULL;
    PolicyHandle hLocalPolicy = {0};
    LsaPolicyInformation *pPolInfo = NULL;
    PWSTR pwszHostname = NULL;
    ULONG HostnameLen = 0;

    dwError = SrvSvcSrvAllocateMemory(sizeof(*pInfo100),
                                      (PVOID*)&pInfo100);
    BAIL_ON_ERROR(dwError);

    dwError = LWNetGetCurrentDomain(&pszDomainFqdn);
    BAIL_ON_ERROR(dwError);

    dwError = LWNetGetDomainController(pszDomainFqdn,
                                       &pszDcName);
    BAIL_ON_ERROR(dwError);

    ntStatus = LwIoGetThreadAccessToken(&pAccessToken);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = gethostname(szHostname, sizeof(szHostname));
    BAIL_ON_ERROR(dwError);

    dwError = SrvSvcConfigGetLsaLpcSocketPath(&pszLsaLpcSocketPath);
    BAIL_ON_ERROR(dwError);

    rpcStatus = InitLsaBindingFull(&hLsaBinding,
                                   "ncalrpc",
                                   szHostname,
                                   pszLsaLpcSocketPath,
                                   NULL,
                                   NULL,
                                   NULL);
    if (rpcStatus) {
        dwError = NERR_InternalError;
        BAIL_ON_ERROR(dwError);
    }

    dwError = LwMbsToWc16s(pszDcName, &pwszDcName);
    BAIL_ON_ERROR(dwError);

    ntStatus = LsaOpenPolicy2(hLsaBinding,
                              pwszDcName,
                              NULL,
                              dwPolicyAccessMask,
                              &hLocalPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaQueryInfoPolicy(hLsaBinding,
                                  &hLocalPolicy,
                                  LSA_POLICY_INFO_DNS,
                                  &pPolInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = SrvSvcSrvGetFromUnicodeStringEx(
                  &pInfo100->wksta100_domain,
                  &pPolInfo->dns.name);
    BAIL_ON_ERROR(dwError);

    dwError = LwMbsToWc16s(szHostname, &pwszHostname);
    BAIL_ON_ERROR(dwError);

    HostnameLen = LwRtlWC16StringNumChars(pwszHostname);
    dwError = SrvSvcSrvAllocateMemory(
                  sizeof(WCHAR)*(HostnameLen+1),
                  (PVOID*)&pInfo100->wksta100_name);
    BAIL_ON_ERROR(dwError);

    memcpy(pInfo100->wksta100_name, pwszHostname, sizeof(WCHAR)*(HostnameLen));

    pInfo100->wksta100_version_major = 5;
    pInfo100->wksta100_version_minor = 1;
    pInfo100->wksta100_platform_id   = 500;

    info->info100 = pInfo100;

    ntStatus = LsaClose(hLsaBinding, &hLocalPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    if (pPolInfo)
    {
        LsaRpcFreeMemory(pPolInfo);
    }

    if (pszDomainFqdn)
    {
        LWNetFreeString(pszDomainFqdn);
    }

    if (pszDcName)
    {
        LWNetFreeString(pszDcName);
    }

    LW_SAFE_FREE_MEMORY(pwszDcName);
    LW_SAFE_FREE_MEMORY(pwszHostname);

    FreeLsaBinding(&hLsaBinding);

    return dwError;

error:
    if (pInfo100)
    {
        SrvSvcSrvFreeMemory(pInfo100);
    }

    switch (level) {
    case 100:
        info->info100 = NULL;
        break;
    }

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
