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
 *        lsa_queryinfopolicy2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaQueryInfoPolicy2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
LsaQueryDomainInfo(
    handle_t hBinding,
    PPOLICY_CONTEXT pPolCtx,
    LsaDomainInfo *pInfo
    );


static
NTSTATUS
LsaQueryDnsDomainInfo(
    handle_t hBinding,
    PPOLICY_CONTEXT pPolCtx,
    DnsDomainInfo *pInfo
    );


NTSTATUS
LsaSrvQueryInfoPolicy2(
    handle_t hBinding,
    POLICY_HANDLE hPolicy,
    uint16 level,
    LsaPolicyInformation **ppInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PPOLICY_CONTEXT pPolCtx = NULL;
    LsaPolicyInformation *pInfo = NULL;

    pPolCtx = (PPOLICY_CONTEXT)hPolicy;

    if (pPolCtx == NULL || pPolCtx->Type != LsaContextPolicy) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    status = LsaSrvAllocateMemory((void**)&pInfo,
                                  sizeof(*pInfo),
                                  pPolCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    switch (level) {
    case LSA_POLICY_INFO_AUDIT_LOG:
    case LSA_POLICY_INFO_AUDIT_EVENTS:
        status = STATUS_INVALID_PARAMETER;
        break;

    case LSA_POLICY_INFO_DOMAIN:
        status = LsaQueryDomainInfo(hBinding, pPolCtx, &pInfo->domain);
        break;

    case LSA_POLICY_INFO_PD:
    case LSA_POLICY_INFO_ACCOUNT_DOMAIN:
    case LSA_POLICY_INFO_ROLE:
    case LSA_POLICY_INFO_REPLICA:
    case LSA_POLICY_INFO_QUOTA:
    case LSA_POLICY_INFO_DB:
    case LSA_POLICY_INFO_AUDIT_FULL_SET:
    case LSA_POLICY_INFO_AUDIT_FULL_QUERY:
        status = STATUS_INVALID_PARAMETER;
        break;

    case LSA_POLICY_INFO_DNS:
        status = LsaQueryDnsDomainInfo(hBinding, pPolCtx, &pInfo->dns);
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
    }

    BAIL_ON_NTSTATUS_ERROR(status);

    *ppInfo = pInfo;

cleanup:
    return status;

error:
    if (pInfo) {
        LsaSrvFreeMemory(pInfo);
    }

    pInfo = NULL;
    goto cleanup;
}


#if !defined(MAXHOSTNAMELEN)
#define MAXHOSTNAMELEN (256)
#endif

static
NTSTATUS
LsaQueryDomainInfo(
    handle_t hBinding,
    PPOLICY_CONTEXT pPolCtx,
    LsaDomainInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE hStore = NULL;
    PLWPS_PASSWORD_INFO pPassInfo = NULL;
    char pszLocalname[MAXHOSTNAMELEN];

    memset(pszLocalname, 0, sizeof(pszLocalname));

    if (gethostname((char*)pszLocalname, sizeof(pszLocalname)) < 0) {
        status = STATUS_INTERNAL_ERROR;
        goto error;
    }

    status = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT,
                                   &hStore);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = LwpsGetPasswordByHostName(hStore,
                                       pszLocalname,
                                       &pPassInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    if (pPassInfo) {
        status = LsaSrvInitUnicodeStringEx(&pInfo->name,
                                           pPassInfo->pwszDomainName,
                                           pPolCtx);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = LsaSrvAllocateSidFromWC16String(&pInfo->sid,
                                                 pPassInfo->pwszSID,
                                                 pPolCtx);
        BAIL_ON_NTSTATUS_ERROR(status);

        LwpsFreePasswordInfo(hStore, pPassInfo);
        pPassInfo = NULL;
    }

    if (hStore != NULL) {
        status = LwpsClosePasswordStore(hStore);
        BAIL_ON_NTSTATUS_ERROR(status);
    }

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
LsaQueryDnsDomainInfo(
    handle_t hBinding,
    PPOLICY_CONTEXT pPolCtx,
    DnsDomainInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    HANDLE hStore = NULL;
    PLWPS_PASSWORD_INFO pPassInfo = NULL;
    char pszLocalname[MAXHOSTNAMELEN];
    PWSTR pwszDnsForest = NULL;
    PSTR pszDomainFqdn = NULL;
    PSTR pszDcFqdn = NULL;
    PSTR pszSiteName = NULL;
    DWORD dwFlags = 0;
    PLWNET_DC_INFO pDcInfo = NULL;

    memset(pszLocalname, 0, sizeof(pszLocalname));

    if (gethostname((char*)pszLocalname, sizeof(pszLocalname)) < 0) {
        status = STATUS_INTERNAL_ERROR;
        goto error;
    }

    status = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT,
                                   &hStore);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = LwpsGetPasswordByHostName(hStore,
                                       pszLocalname,
                                       &pPassInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    if (pPassInfo) {
        status = LsaSrvInitUnicodeStringEx(&pInfo->name,
                                           pPassInfo->pwszDomainName,
                                           pPolCtx);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = LsaSrvInitUnicodeStringEx(&pInfo->dns_domain,
                                           pPassInfo->pwszDnsDomainName,
                                           pPolCtx);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = LsaSrvAllocateSidFromWC16String(&pInfo->sid,
                                                 pPassInfo->pwszSID,
                                                 pPolCtx);
        BAIL_ON_NTSTATUS_ERROR(status);

        pszDomainFqdn = awc16stombs(pPassInfo->pwszDnsDomainName);
        BAIL_ON_NO_MEMORY(pszDomainFqdn);

        dwError = LWNetGetDCName(pszDcFqdn,
                                 pszDomainFqdn,
                                 pszSiteName,
                                 dwFlags,
                                 &pDcInfo);
        BAIL_ON_LSA_ERROR(dwError);

        pwszDnsForest = ambstowc16s(pDcInfo->pszDnsForestName);
        BAIL_ON_NO_MEMORY(pwszDnsForest);

        status = LsaSrvInitUnicodeStringEx(&pInfo->dns_forest,
                                           pwszDnsForest,
                                           pPolCtx);
        BAIL_ON_NTSTATUS_ERROR(status);

        memcpy(&pInfo->domain_guid, pDcInfo->pucDomainGUID,
               sizeof(pInfo->domain_guid));
    }

cleanup:
    if (pPassInfo) {
        LwpsFreePasswordInfo(hStore, pPassInfo);
    }

    if (hStore) {
        LwpsClosePasswordStore(hStore);
    }

    if (pDcInfo) {
        LWNetFreeDCInfo(pDcInfo);
    }

    if (pszDomainFqdn) {
        RTL_FREE(&pszDomainFqdn);
    }

    if (pwszDnsForest) {
        RTL_FREE(&pwszDnsForest);
    }

    return status;

error:
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
