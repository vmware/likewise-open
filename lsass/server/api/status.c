/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        status.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Status (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "api.h"

DWORD
LsaSrvGetStatus(
    HANDLE hServer,
    PLSASTATUS* ppLsaStatus
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    DWORD dwProviderCount = 0;
    DWORD iCount = 0;
    HANDLE hProvider = (HANDLE)NULL;
    PLSASTATUS pLsaStatus = NULL;
    PLSA_AUTH_PROVIDER_STATUS pProviderOwnedStatus = NULL;

    BAIL_ON_INVALID_POINTER(ppLsaStatus);

    dwError = LsaAllocateMemory(
                  sizeof(LSASTATUS),
                  (PVOID*)&pLsaStatus);
    BAIL_ON_LSA_ERROR(dwError);

    pLsaStatus->dwUptime = (DWORD)difftime(time(NULL), gServerStartTime);
    
    dwError = LsaSrvGetVersion(
                    &pLsaStatus->version);
    BAIL_ON_LSA_ERROR(dwError);
    
    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);
    
    dwProviderCount = LsaGetNumberOfProviders_inlock();
    
    if (!dwProviderCount)
    {
        goto done;
    }
    
    dwError = LsaAllocateMemory(
                    dwProviderCount * sizeof(LSA_AUTH_PROVIDER_STATUS),
                    (PVOID*)&pLsaStatus->pAuthProviderStatusList);
    BAIL_ON_LSA_ERROR(dwError);
    
    pLsaStatus->dwCount = dwProviderCount;
        
    dwError = LSA_ERROR_NOT_HANDLED;
    
    for (pProvider = gpAuthProviderList, iCount = 0;
         pProvider;
         pProvider = pProvider->pNext, iCount++)
    {
        PLSA_AUTH_PROVIDER_STATUS pAuthProviderStatus = NULL;
        
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);
        
        pAuthProviderStatus = &pLsaStatus->pAuthProviderStatusList[iCount];
        
        dwError = LsaAllocateString(
                        pProvider->pszName,
                        &pAuthProviderStatus->pszId);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = pProvider->pFnTable->pfnGetStatus(
                                            hProvider,
                                            &pProviderOwnedStatus);
        if (dwError == LSA_ERROR_NOT_HANDLED)
        {
           LsaSrvCloseProvider(pProvider, hProvider);
           hProvider = (HANDLE)NULL;
           continue;
        }

        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaSrvCopyProviderStatus(
                        pProviderOwnedStatus,
                        pAuthProviderStatus);
        BAIL_ON_LSA_ERROR(dwError);
        
        pProvider->pFnTable->pfnFreeStatus(
                        pProviderOwnedStatus);
        
        pProviderOwnedStatus = NULL;
    }
    
done:

    *ppLsaStatus = pLsaStatus;

cleanup:

    if (hProvider != (HANDLE)NULL) {
        
        if (pProviderOwnedStatus)
        {
            pProvider->pFnTable->pfnFreeStatus(
                            pProviderOwnedStatus);
        }
        
        LsaSrvCloseProvider(pProvider, hProvider);
    }
    
    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    return dwError;

error:

    if (ppLsaStatus)
    {
        *ppLsaStatus = NULL;
    }
    
    if (pLsaStatus)
    {
        LsaFreeStatus(pLsaStatus);
    }

   goto cleanup;
}

DWORD
LsaSrvCopyProviderStatus(
    PLSA_AUTH_PROVIDER_STATUS pProviderOwnedStatus,
    PLSA_AUTH_PROVIDER_STATUS pTargetStatus
    )
{
    DWORD dwError = 0;
    
    pTargetStatus->mode = pProviderOwnedStatus->mode;
    
    LSA_SAFE_FREE_STRING(pTargetStatus->pszCell);
    
    if (!IsNullOrEmptyString(pProviderOwnedStatus->pszCell))
    {
        dwError = LsaAllocateString(
                        pProviderOwnedStatus->pszCell,
                        &pTargetStatus->pszCell);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    LSA_SAFE_FREE_STRING(pTargetStatus->pszDomain);
    
    if (!IsNullOrEmptyString(pProviderOwnedStatus->pszDomain))
    {
        dwError = LsaAllocateString(
                        pProviderOwnedStatus->pszDomain,
                        &pTargetStatus->pszDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    LSA_SAFE_FREE_STRING(pTargetStatus->pszForest);
    
    if (!IsNullOrEmptyString(pProviderOwnedStatus->pszForest))
    {
        dwError = LsaAllocateString(
                        pProviderOwnedStatus->pszForest,
                        &pTargetStatus->pszForest);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    LSA_SAFE_FREE_STRING(pTargetStatus->pszId);
    
    if (!IsNullOrEmptyString(pProviderOwnedStatus->pszId))
    {
        dwError = LsaAllocateString(
                    pProviderOwnedStatus->pszId,
                    &pTargetStatus->pszId);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    LSA_SAFE_FREE_STRING(pTargetStatus->pszSite);
    
    if (!IsNullOrEmptyString(pProviderOwnedStatus->pszSite))
    {
        dwError = LsaAllocateString(
                    pProviderOwnedStatus->pszSite,
                    &pTargetStatus->pszSite);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    pTargetStatus->status = pProviderOwnedStatus->status;
    pTargetStatus->subMode = pProviderOwnedStatus->subMode;
    pTargetStatus->dwNetworkCheckInterval = pProviderOwnedStatus->dwNetworkCheckInterval;
    
    if (pProviderOwnedStatus->pTrustedDomainInfoArray)
    {
        dwError = LsaSrvCopyTrustedDomainInfoArray(
                        pProviderOwnedStatus->dwNumTrustedDomains,
                        pProviderOwnedStatus->pTrustedDomainInfoArray,
                        &pTargetStatus->pTrustedDomainInfoArray);
        BAIL_ON_LSA_ERROR(dwError);
        
        pTargetStatus->dwNumTrustedDomains = pProviderOwnedStatus->dwNumTrustedDomains;
    }
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaSrvCopyTrustedDomainInfoArray(
    DWORD                     dwNumDomains,
    PLSA_TRUSTED_DOMAIN_INFO  pSrcDomainInfoArray,
    PLSA_TRUSTED_DOMAIN_INFO* ppDomainInfoArray
    )
{
    DWORD dwError = 0;
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfoArray = NULL;
    DWORD iDomain = 0;
    
    dwError = LsaAllocateMemory(
                    dwNumDomains * sizeof(LSA_TRUSTED_DOMAIN_INFO),
                    (PVOID*)&pDomainInfoArray);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (; iDomain < dwNumDomains; iDomain++)
    {
        PLSA_TRUSTED_DOMAIN_INFO pSrcDomainInfo =
            &pSrcDomainInfoArray[iDomain];
        PLSA_TRUSTED_DOMAIN_INFO pDestDomainInfo =
            &pDomainInfoArray[iDomain];
        
        dwError = LsaStrDupOrNull(
                        pSrcDomainInfo->pszDnsDomain,
                        &pDestDomainInfo->pszDnsDomain);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaStrDupOrNull(
                        pSrcDomainInfo->pszNetbiosDomain,
                        &pDestDomainInfo->pszNetbiosDomain);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaStrDupOrNull(
                        pSrcDomainInfo->pszDomainSID,
                        &pDestDomainInfo->pszDomainSID);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaStrDupOrNull(
                        pSrcDomainInfo->pszDomainGUID,
                        &pDestDomainInfo->pszDomainGUID);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaStrDupOrNull(
                        pSrcDomainInfo->pszTrusteeDnsDomain,
                        &pDestDomainInfo->pszTrusteeDnsDomain);
        BAIL_ON_LSA_ERROR(dwError);
        
        pDestDomainInfo->dwTrustFlags = pSrcDomainInfo->dwTrustFlags;
        pDestDomainInfo->dwTrustType = pSrcDomainInfo->dwTrustType;
        pDestDomainInfo->dwTrustAttributes = pSrcDomainInfo->dwTrustAttributes;
        pDestDomainInfo->dwTrustDirection = pSrcDomainInfo->dwTrustDirection;
        pDestDomainInfo->dwTrustMode = pSrcDomainInfo->dwTrustMode;
        
        dwError = LsaStrDupOrNull(
                        pSrcDomainInfo->pszForestName,
                        &pDestDomainInfo->pszForestName);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaStrDupOrNull(
                        pSrcDomainInfo->pszClientSiteName,
                        &pDestDomainInfo->pszClientSiteName);
        BAIL_ON_LSA_ERROR(dwError);
        
        pDestDomainInfo->dwDomainFlags = pSrcDomainInfo->dwDomainFlags;
        
        if (pSrcDomainInfo->pDCInfo)
        {
            dwError = LsaSrvCopyDCInfo(
                            pSrcDomainInfo->pDCInfo,
                            &pDestDomainInfo->pDCInfo);
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        if (pSrcDomainInfo->pGCInfo)
        {
            dwError = LsaSrvCopyDCInfo(
                            pSrcDomainInfo->pGCInfo,
                            &pDestDomainInfo->pGCInfo);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    *ppDomainInfoArray = pDomainInfoArray;
    
cleanup:

    return dwError;
    
error:

    *ppDomainInfoArray = NULL;
    
    if (pDomainInfoArray)
    {
        LsaFreeDomainInfoArray(dwNumDomains, pDomainInfoArray);
    }

    goto cleanup;
}

DWORD
LsaSrvCopyDCInfo(
    PLSA_DC_INFO  pSrcInfo,
    PLSA_DC_INFO* ppDCInfo
    )
{
    DWORD dwError = 0;
    PLSA_DC_INFO pDCInfo = NULL;
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_DC_INFO),
                    (PVOID*)&pDCInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaStrDupOrNull(
                    pSrcInfo->pszName,
                    &pDCInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaStrDupOrNull(
                    pSrcInfo->pszAddress,
                    &pDCInfo->pszAddress);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaStrDupOrNull(
                    pSrcInfo->pszSiteName,
                    &pDCInfo->pszSiteName);
    BAIL_ON_LSA_ERROR(dwError);
    
    pDCInfo->dwFlags = pSrcInfo->dwFlags;
    
    *ppDCInfo = pDCInfo;
    
cleanup:

    return dwError;
    
error:

    *ppDCInfo = NULL;
    
    if (pDCInfo)
    {
        LsaFreeDCInfo(pDCInfo);
    }
    goto cleanup;
}

DWORD
LsaSrvGetVersion(
    PLSA_VERSION pVersion
    )
{  
    DWORD dwError = 0;
    PSTR pszVersion = NULL;
    DWORD iVerComp = 0;
    PSTR  pszToken = NULL;
    PSTR  pszTokenState = NULL;
    DWORD dwMajor = 0;
    DWORD dwMinor = 0;
    DWORD dwBuild = 0;
    
    if (IsNullOrEmptyString(PKG_VERSION))
    {
        dwError = LSA_ERROR_INVALID_AGENT_VERSION;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaAllocateString(
                    PKG_VERSION,
                    &pszVersion);
    BAIL_ON_LSA_ERROR(dwError);
    
    pszToken = strtok_r(pszVersion, ".",  &pszTokenState);
    
    while (!IsNullOrEmptyString(pszVersion) && (iVerComp < 3))
    {
        int i = 0;
        
        for (; i < strlen(pszVersion); i++)
        {
            if (!isdigit((int)pszVersion[i]))
            {
                dwError = LSA_ERROR_INVALID_AGENT_VERSION;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        
        switch (iVerComp++)
        {
            case 0:
                
                dwMajor = atoi(pszToken);
                break;
                
            case 1:
                
                dwMinor = atoi(pszToken);
                break;
                
            case 2:
                
                dwBuild = atoi(pszToken);
                break;
                
            default:
                
                dwError = LSA_ERROR_INTERNAL;
                BAIL_ON_LSA_ERROR(dwError);
        }
        
        pszToken = strtok_r(NULL, ".", &pszTokenState);
    }
    
    if (iVerComp < 3)
    {
        dwError = LSA_ERROR_INVALID_AGENT_VERSION;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    pVersion->dwMajor = dwMajor;
    pVersion->dwMinor = dwMinor;
    pVersion->dwBuild = dwBuild;
    
cleanup:

    LSA_SAFE_FREE_MEMORY(pszVersion);

    return dwError;
    
error:

    memset(pVersion, 0, sizeof(*pVersion));
    
    goto cleanup;
}
