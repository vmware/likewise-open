/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        lsa_wbc_domain.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 * Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */

#include "wbclient.h"
#include "lsawbclient_p.h"
#include "lsaclient.h"


/********************************************************
 *******************************************************/

static DWORD
FillDomainInfo(
    PCSTR pszDomain,
    struct wbcDomainInfo **ppDomInfo,
    PLSASTATUS pStatus
    );

wbcErr
wbcDomainInfo(
    const char *domain,
    struct wbcDomainInfo **info
    )
{
    wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;
    DWORD dwErr = LSA_ERROR_INTERNAL;
    HANDLE hLsa = (HANDLE)NULL;
    PLSASTATUS pLsaStatus = NULL;

    /* Sanity check */

    BAIL_ON_NULL_PTR_PARAM(domain, dwErr);

    /* Work */

    dwErr = LsaOpenServer(&hLsa);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = LsaGetStatus(hLsa, &pLsaStatus);
    BAIL_ON_LSA_ERR(dwErr);

    dwErr = FillDomainInfo(domain, info, pLsaStatus);
    BAIL_ON_LSA_ERR(dwErr);

done:
    if (hLsa != (HANDLE)NULL) {
        LsaCloseServer(hLsa);
    }

    wbc_status = map_error_to_wbc_status(dwErr);

    return wbc_status;
}


/********************************************************
 *******************************************************/

static int
FreeWbcDomainInfo(
    void* p
    )
{
    struct wbcDomainInfo *pDomain = (struct wbcDomainInfo*)p;

    if (!pDomain) {
        return 0;
    }

    _WBC_FREE(pDomain->short_name);
    _WBC_FREE(pDomain->dns_name);

    return 0;
}

static DWORD
FillDomainInfo(
    PCSTR pszDomain,
    struct wbcDomainInfo **ppDomInfo,
    PLSASTATUS pStatus
    )
{
    DWORD dwErr = LSA_ERROR_INTERNAL;
    PLSA_AUTH_PROVIDER_STATUS pADProvStatus = NULL;
    PLSA_TRUSTED_DOMAIN_INFO pLsaDomInfo = NULL;
    struct wbcDomainInfo *pWbcDomInfo = NULL;
    int i;

    /* Find the AD provider entry */

    for (i=0; i<pStatus->dwCount; i++)
    {
        if (strcmp(pStatus->pAuthProviderStatusList[i].pszId,
                   "lsa-activedirectory-provider") == 0)
        {
            pADProvStatus = &pStatus->pAuthProviderStatusList[i];
            break;
        }
    }

    if (pADProvStatus == NULL) {
        dwErr = LSA_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERR(dwErr);
    }

    /* Find the requested domain */

    for (i=0; i<pADProvStatus->dwNumTrustedDomains; i++)
    {
        PLSA_TRUSTED_DOMAIN_INFO pCursorDomInfo = NULL;

        pCursorDomInfo = &pADProvStatus->pTrustedDomainInfoArray[i];
        if (StrEqual(pCursorDomInfo->pszDnsDomain, pszDomain) ||
            StrEqual(pCursorDomInfo->pszNetbiosDomain, pszDomain))
        {
            pLsaDomInfo = pCursorDomInfo;
            break;
        }
    }

    if (pLsaDomInfo == NULL) {
        dwErr = LSA_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_LSA_ERR(dwErr);
    }

    /* Fill in the domain info */

    pWbcDomInfo = _wbc_malloc_zero(sizeof(struct wbcDomainInfo),
                                   FreeWbcDomainInfo);

    if (pLsaDomInfo->pszDnsDomain) {
        pWbcDomInfo->dns_name = _wbc_strdup(pLsaDomInfo->pszDnsDomain);
        BAIL_ON_NULL_PTR(pLsaDomInfo->pszDnsDomain, dwErr);
    }

    if (pLsaDomInfo->pszNetbiosDomain) {
        pWbcDomInfo->short_name = _wbc_strdup(pLsaDomInfo->pszNetbiosDomain);
        BAIL_ON_NULL_PTR(pLsaDomInfo->pszNetbiosDomain, dwErr);
    }

    if (pLsaDomInfo->pszDomainSID) {
        dwErr = wbcStringToSid(pLsaDomInfo->pszDomainSID, &pWbcDomInfo->sid);
        BAIL_ON_LSA_ERR(dwErr);
    }

    /* Domain flags */
    if (pLsaDomInfo->dwDomainFlags & LSA_DM_DOMAIN_FLAG_PRIMARY) {
        pWbcDomInfo->domain_flags |= WBC_DOMINFO_DOMAIN_PRIMARY;
    }

    /* Trust Flags */
    if (pLsaDomInfo->dwTrustFlags & LSA_TRUST_FLAG_INBOUND) {
        pWbcDomInfo->trust_flags |= WBC_DOMINFO_TRUST_INCOMING;
    }
    if (pLsaDomInfo->dwTrustFlags & LSA_TRUST_FLAG_OUTBOUND) {
        pWbcDomInfo->trust_flags |= WBC_DOMINFO_TRUST_OUTGOING;
    }

    /* Trust Type */
    if (pLsaDomInfo->dwTrustAttributes & LSA_TRUST_ATTRIBUTE_WITHIN_FOREST) {
        pWbcDomInfo->trust_type |= WBC_DOMINFO_TRUSTTYPE_IN_FOREST;
    }
    if (pLsaDomInfo->dwTrustAttributes & LSA_TRUST_ATTRIBUTE_FOREST_TRANSITIVE) {
        pWbcDomInfo->trust_type |= WBC_DOMINFO_TRUSTTYPE_FOREST;
    }
    if (pLsaDomInfo->dwTrustAttributes & LSA_TRUST_ATTRIBUTE_NON_TRANSITIVE) {
        pWbcDomInfo->trust_type |= WBC_DOMINFO_TRUSTTYPE_EXTERNAL;
    }

    *ppDomInfo = pWbcDomInfo;
    pWbcDomInfo = NULL;

done:
    _WBC_FREE(pWbcDomInfo);

    return dwErr;
}


/********************************************************
 *******************************************************/

wbcErr
wbcListTrusts(
    struct wbcDomainInfo **domains,
    size_t *num_domains
    )
{
    return WBC_ERR_NOT_IMPLEMENTED;
}


/********************************************************
 *******************************************************/

wbcErr
wbcCheckTrustCredentials(
    const char *domain,
    struct wbcAuthErrorInfo **error
    )
{
    return WBC_ERR_NOT_IMPLEMENTED;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

