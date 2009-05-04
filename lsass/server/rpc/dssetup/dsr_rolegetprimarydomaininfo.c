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
 *        dsr_rolegetprimarydomaininfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        DsrRoleGetPrimaryDomainInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
DsrSrvRoleGetPDCInfoBasic(
    PDS_ROLE_PRIMARY_DOMAIN_INFO_BASIC pInfo,
    void *pParent
    );


static
NTSTATUS
DsrSrvRoleGetPDCInfoUpgrade(
    PDS_ROLE_UPGRADE_STATUS pInfo
    );


static
NTSTATUS
DsrSrvRoleGetPDCInfoOpStatus(
    PDS_ROLE_OP_STATUS pInfo
    );


DWORD
DsrSrvRoleGetPrimaryDomainInformation(
    handle_t hBinding,
    UINT16 uiLevel,
    PDS_ROLE_INFO *ppInfo
    )
{
    DWORD err = 0;
    NTSTATUS status = STATUS_SUCCESS;
    PDS_ROLE_INFO pInfo = NULL;

    status = DsrSrvAllocateMemory((void**)&pInfo,
                                  sizeof(*pInfo),
                                  NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    switch (uiLevel) {
    case DS_ROLE_BASIC_INFORMATION:
        status = DsrSrvRoleGetPDCInfoBasic(&pInfo->basic, pInfo);
        break;

    case DS_ROLE_UPGRADE_STATUS:
        status = DsrSrvRoleGetPDCInfoUpgrade(&pInfo->upgrade);
        break;

    case DS_ROLE_OP_STATUS:
        status = DsrSrvRoleGetPDCInfoOpStatus(&pInfo->opstatus);
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
    }

    *ppInfo = pInfo;

cleanup:
    if (err == 0) {
        err = NtStatusToWin32Error(status);
    }

    return err;

error:
    if (pInfo) {
        DsrSrvFreeMemory(pInfo);
    }

    *ppInfo = NULL;
    goto cleanup;
}


static
NTSTATUS
DsrSrvRoleGetPDCInfoBasic(
    PDS_ROLE_PRIMARY_DOMAIN_INFO_BASIC pInfo,
    void *pParent
    )
{
    const DWORD dwPolicyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS;

    NTSTATUS status = STATUS_SUCCESS;
    DWORD rpcstatus = 0;
    DWORD dwError = 0;
    PSTR pszDomainFqdn = NULL;
    PSTR pszLsaLpcSocketPath = NULL;
    PSTR pszDcName = NULL;
    PWSTR pwszDcName = NULL;
    PIO_ACCESS_TOKEN pAccessToken = NULL;
    CHAR szHostname[64];
    handle_t hLsaBinding = NULL;
    PolicyHandle hLocalPolicy;
    LsaPolicyInformation *pPolInfo = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszDnsDomain = NULL;
    PWSTR pwszForest = NULL;

    memset(szHostname, 0, sizeof(szHostname));
    memset(&hLocalPolicy, 0, sizeof(hLocalPolicy));

    dwError = LWNetGetCurrentDomain(&pszDomainFqdn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LWNetGetDomainController(pszDomainFqdn,
                                       &pszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    status = LwIoGetThreadAccessToken(&pAccessToken);
    BAIL_ON_NTSTATUS_ERROR(status);

    dwError = gethostname(szHostname, sizeof(szHostname));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DsrSrvConfigGetLsaLpcSocketPath(&pszLsaLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    rpcstatus = InitLsaBindingFull(&hLsaBinding,
                                   "ncalrpc",
                                   szHostname,
                                   pszLsaLpcSocketPath,
                                   NULL,
                                   NULL,
                                   NULL);
    if (rpcstatus) {
        dwError = LSA_ERROR_RPC_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaMbsToWc16s(pszDcName, &pwszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    status = LsaOpenPolicy2(hLsaBinding,
                            pwszDcName,
                            NULL,
                            dwPolicyAccessMask,
                            &hLocalPolicy);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = LsaQueryInfoPolicy(hLsaBinding,
                                &hLocalPolicy,
                                LSA_POLICY_INFO_DNS,
                                &pPolInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = DsrSrvGetFromUnicodeStringEx(&pwszDomain,
                                          &pPolInfo->dns.name,
                                          pParent);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = DsrSrvGetFromUnicodeStringEx(&pwszDnsDomain,
                                          &pPolInfo->dns.dns_domain,
                                          pParent);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = DsrSrvGetFromUnicodeStringEx(&pwszForest,
                                          &pPolInfo->dns.dns_forest,
                                          pParent);
    BAIL_ON_NTSTATUS_ERROR(status);

    memcpy(&pInfo->DomainGuid, &pPolInfo->dns.domain_guid,
           sizeof(pInfo->DomainGuid));

    status = LsaClose(hLsaBinding, &hLocalPolicy);
    BAIL_ON_NTSTATUS_ERROR(status);

    pInfo->uiRole        = DS_ROLE_MEMBER_SERVER;
    pInfo->uiFlags       = 0;
    pInfo->pwszDomain    = pwszDomain;
    pInfo->pwszDnsDomain = pwszDnsDomain;
    pInfo->pwszForest    = pwszForest;

cleanup:
    if (pInfo) {
        LsaRpcFreeMemory(pPolInfo);
    }

    if (pszDomainFqdn) {
        LWNetFreeString(pszDomainFqdn);
    }

    if (pszDcName) {
        LWNetFreeString(pszDcName);
    }

    FreeLsaBinding(&hLsaBinding);

    return status;

error:
    goto cleanup;
}


static
NTSTATUS
DsrSrvRoleGetPDCInfoUpgrade(
    PDS_ROLE_UPGRADE_STATUS pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    pInfo->uiUpgradeStatus = DS_ROLE_NOT_UPGRADING;
    pInfo->uiPrevious      = DS_ROLE_PREVIOUS_UNKNOWN;

    return status;
}


static
NTSTATUS
DsrSrvRoleGetPDCInfoOpStatus(
    PDS_ROLE_OP_STATUS pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    pInfo->uiStatus = DS_ROLE_OP_IDLE;

    return status;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
