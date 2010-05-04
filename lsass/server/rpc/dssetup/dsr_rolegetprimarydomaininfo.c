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


WINERROR
DsrSrvRoleGetPrimaryDomainInformation(
    IN  handle_t         hBinding,
    IN  UINT16           usLevel,
    OUT PDS_ROLE_INFO   *ppInfo
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = gpDsrSecDesc;
    GENERIC_MAPPING GenericMapping = {0};
    DWORD dwAccessMask = LSA_ACCESS_VIEW_POLICY_INFO;
    DWORD dwAccessGranted = 0;
    PACCESS_TOKEN pUserToken = NULL;
    PDS_ROLE_INFO pInfo = NULL;

    /*
     * Get an access token and perform access check before
     * handling any info level
     */
    ntStatus = DsrSrvInitAuthInfo(hBinding, &pUserToken);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (pUserToken == NULL)
    {
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    if (!RtlAccessCheck(pSecDesc,
                        pUserToken,
                        dwAccessMask,
                        dwAccessGranted,
                        &GenericMapping,
                        &dwAccessGranted,
                        &ntStatus))
    {
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    ntStatus = DsrSrvAllocateMemory((void**)&pInfo,
                                    sizeof(*pInfo));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    switch (usLevel)
    {
    case DS_ROLE_BASIC_INFORMATION:
        ntStatus = DsrSrvRoleGetPDCInfoBasic(&pInfo->basic,
                                             pInfo);
        break;

    case DS_ROLE_UPGRADE_STATUS:
        ntStatus = DsrSrvRoleGetPDCInfoUpgrade(&pInfo->upgrade);
        break;

    case DS_ROLE_OP_STATUS:
        ntStatus = DsrSrvRoleGetPDCInfoOpStatus(&pInfo->opstatus);
        break;

    default:
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppInfo = pInfo;

cleanup:
    if (pUserToken)
    {
        RtlReleaseAccessToken(&pUserToken);
    }

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return (WINERROR)dwError;

error:
    if (pInfo)
    {
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
    const DWORD dwPolicyAccessMask = LSA_ACCESS_VIEW_POLICY_INFO;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD rpcstatus = 0;
    DWORD dwError = 0;
    PSTR pszDomainFqdn = NULL;
    PSTR pszLsaLpcSocketPath = NULL;
    PSTR pszDcName = NULL;
    PWSTR pwszDcName = NULL;
    PIO_CREDS pCreds = NULL;
    CHAR szHostname[64];
    handle_t hLsaBinding = NULL;
    POLICY_HANDLE hLocalPolicy = NULL;
    LsaPolicyInformation *pPolInfo = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszDnsDomain = NULL;
    PWSTR pwszForest = NULL;

    memset(szHostname, 0, sizeof(szHostname));

    dwError = LWNetGetCurrentDomain(&pszDomainFqdn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LWNetGetDomainController(pszDomainFqdn,
                                       &pszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LwIoGetThreadCreds(&pCreds);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

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
        dwError = LW_ERROR_RPC_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaMbsToWc16s(pszDcName, &pwszDcName);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LsaOpenPolicy2(hLsaBinding,
                              pwszDcName,
                              NULL,
                              dwPolicyAccessMask,
                              &hLocalPolicy);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaQueryInfoPolicy(hLsaBinding,
                                  hLocalPolicy,
                                  LSA_POLICY_INFO_DNS,
                                  &pPolInfo);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = DsrSrvGetFromUnicodeStringEx(&pwszDomain,
                                            &pPolInfo->dns.name);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = DsrSrvGetFromUnicodeStringEx(&pwszDnsDomain,
                                          &pPolInfo->dns.dns_domain);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = DsrSrvGetFromUnicodeStringEx(&pwszForest,
                                          &pPolInfo->dns.dns_forest);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memcpy(&pInfo->DomainGuid, &pPolInfo->dns.domain_guid,
           sizeof(pInfo->DomainGuid));

    ntStatus = LsaClose(hLsaBinding, hLocalPolicy);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pInfo->uiRole        = DS_ROLE_MEMBER_SERVER;
    pInfo->uiFlags       = DS_ROLE_PRIMARY_DOMAIN_GUID_PRESENT;
    pInfo->pwszDomain    = pwszDomain;
    pInfo->pwszDnsDomain = pwszDnsDomain;
    pInfo->pwszForest    = pwszForest;

cleanup:
    if (pInfo)
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

    FreeLsaBinding(&hLsaBinding);

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
DsrSrvRoleGetPDCInfoUpgrade(
    PDS_ROLE_UPGRADE_STATUS pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pInfo->uiUpgradeStatus = DS_ROLE_NOT_UPGRADING;
    pInfo->uiPrevious      = DS_ROLE_PREVIOUS_UNKNOWN;

    return ntStatus;
}


static
NTSTATUS
DsrSrvRoleGetPDCInfoOpStatus(
    PDS_ROLE_OP_STATUS pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pInfo->uiStatus = DS_ROLE_OP_IDLE;

    return ntStatus;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
