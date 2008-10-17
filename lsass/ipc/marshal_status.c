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
 *        marshal_status.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Marshal/Unmarshal API for Messages related to status
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "ipc.h"

DWORD
LsaMarshalStatus(
    PLSASTATUS pLsaStatus,
    PSTR       pszBuffer,
    PDWORD     pdwBufLen
    )
{
    DWORD dwError   = 0;
    DWORD dwOffset  = 0;
    DWORD dwOffset2 = 0;
    DWORD iCount    = 0;
    DWORD dwRequiredBufferLength = 0;
    LSA_STATUS_MSG statusMsg;

    BAIL_ON_INVALID_POINTER(pLsaStatus);
    BAIL_ON_INVALID_POINTER(pdwBufLen);

    dwRequiredBufferLength = LsaComputeStatusBufferLength(
                                 pLsaStatus);

    if (!pszBuffer) {
       *pdwBufLen = dwRequiredBufferLength;
       goto cleanup;
    }

    if (*pdwBufLen < dwRequiredBufferLength)
    {
       dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    memset(&statusMsg, 0, sizeof(statusMsg));

    statusMsg.dwUptime = pLsaStatus->dwUptime;
    memcpy(&statusMsg.version, &pLsaStatus->version, sizeof(pLsaStatus->version));
    statusMsg.dwCount = pLsaStatus->dwCount;

    dwOffset += sizeof(statusMsg);

    dwOffset2 = dwOffset + (pLsaStatus->dwCount * sizeof(LSA_AUTH_PROVIDER_STATUS_MSG));

    for (iCount = 0; iCount < pLsaStatus->dwCount; iCount++)
    {
         LSA_AUTH_PROVIDER_STATUS_MSG authProviderStatus;
         PLSA_AUTH_PROVIDER_STATUS pAuthProviderStatus =
            &pLsaStatus->pAuthProviderStatusList[iCount];

         
         memset(&authProviderStatus, 0, sizeof(authProviderStatus));

         authProviderStatus.dwMode = pAuthProviderStatus->mode;
         authProviderStatus.dwSubmode = pAuthProviderStatus->subMode;
         authProviderStatus.dwStatus = pAuthProviderStatus->status;
         authProviderStatus.dwNetworkCheckInterval = pAuthProviderStatus->dwNetworkCheckInterval;
         authProviderStatus.dwNumTrustedDomains = pAuthProviderStatus->dwNumTrustedDomains;
         
         if (!IsNullOrEmptyString(pAuthProviderStatus->pszId))
         {
            authProviderStatus.id.length = strlen(pAuthProviderStatus->pszId);
            authProviderStatus.id.offset = dwOffset2;

            memcpy(pszBuffer + authProviderStatus.id.offset,
                   pAuthProviderStatus->pszId,
                   authProviderStatus.id.length);

            dwOffset2 += authProviderStatus.id.length;
         }

         if (!IsNullOrEmptyString(pAuthProviderStatus->pszDomain))
         {
            authProviderStatus.domain.length = strlen(pAuthProviderStatus->pszDomain);
            authProviderStatus.domain.offset = dwOffset2;

            memcpy(pszBuffer + authProviderStatus.domain.offset,
                   pAuthProviderStatus->pszDomain,
                   authProviderStatus.domain.length);
 
            dwOffset2 += authProviderStatus.domain.length;
         }
         
         if (!IsNullOrEmptyString(pAuthProviderStatus->pszForest))
         {
            authProviderStatus.forest.length = strlen(pAuthProviderStatus->pszForest);
            authProviderStatus.forest.offset = dwOffset2;

            memcpy(pszBuffer + authProviderStatus.forest.offset,
                   pAuthProviderStatus->pszForest,
                   authProviderStatus.forest.length);
 
            dwOffset2 += authProviderStatus.forest.length;
         }
         
         if (!IsNullOrEmptyString(pAuthProviderStatus->pszSite))
         {
            authProviderStatus.site.length = strlen(pAuthProviderStatus->pszSite);
            authProviderStatus.site.offset = dwOffset2;

            memcpy(pszBuffer + authProviderStatus.site.offset,
                   pAuthProviderStatus->pszSite,
                   authProviderStatus.site.length);
 
            dwOffset2 += authProviderStatus.site.length;
         }

         if (!IsNullOrEmptyString(pAuthProviderStatus->pszCell))
         {
            authProviderStatus.cell.length = strlen(pAuthProviderStatus->pszCell);
            authProviderStatus.cell.offset = dwOffset2;
            
            memcpy(pszBuffer + authProviderStatus.cell.offset,
                   pAuthProviderStatus->pszCell,
                   authProviderStatus.cell.length);

            dwOffset2 += authProviderStatus.cell.length;
         }
         
         if (pAuthProviderStatus->dwNumTrustedDomains)
         {
             DWORD iDomain = 0;
             
             authProviderStatus.dwTrustedDomainOffset = dwOffset2;
             
             for (;iDomain < pAuthProviderStatus->dwNumTrustedDomains; iDomain++)
             {
                 dwOffset2 += LsaMarshalDomainInfo(
                                 &pAuthProviderStatus->pTrustedDomainInfoArray[iDomain],
                                 pszBuffer,
                                 dwOffset2);
             }
         }

         memcpy(pszBuffer + dwOffset, &authProviderStatus, sizeof(authProviderStatus));

         dwOffset += sizeof(authProviderStatus);
    }

    memcpy(pszBuffer, &statusMsg, sizeof(statusMsg));

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaComputeStatusBufferLength(
    PLSASTATUS pLsaStatus
    )
{
    DWORD dwBufLen = sizeof(LSA_STATUS_MSG);
    DWORD iCount = 0;

    for (iCount = 0; iCount < pLsaStatus->dwCount; iCount++)
    {
        DWORD iDomain = 0;
        PLSA_AUTH_PROVIDER_STATUS pStatus =
           &pLsaStatus->pAuthProviderStatusList[iCount];

        dwBufLen += sizeof(LSA_AUTH_PROVIDER_STATUS_MSG);

        if (pStatus->pszId) {
            dwBufLen += strlen(pStatus->pszId);
        }
 
        if (pStatus->pszDomain) {
            dwBufLen += strlen(pStatus->pszDomain);
        }
        
        if (pStatus->pszForest) {
            dwBufLen += strlen(pStatus->pszForest);
        }
        
        if (pStatus->pszSite) {
            dwBufLen += strlen(pStatus->pszSite);
        }

        if (pStatus->pszCell) {
           dwBufLen += strlen(pStatus->pszCell);
        }
        
        for (; iDomain < pStatus->dwNumTrustedDomains; iDomain++)
        {
            dwBufLen += LsaComputeDomainInfoBufferLength(
                            &pStatus->pTrustedDomainInfoArray[iDomain]);
        }
    }

    return dwBufLen;
}

DWORD
LsaComputeDomainInfoBufferLength(
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfo
    )
{
    DWORD dwBufLen = sizeof(LSA_DOMAIN_INFO_MSG);
    
    if (pDomainInfo->pszDnsDomain)
    {
        dwBufLen += strlen(pDomainInfo->pszDnsDomain);
    }
    
    if (pDomainInfo->pszNetbiosDomain)
    {
        dwBufLen += strlen(pDomainInfo->pszNetbiosDomain);
    }
    
    if (pDomainInfo->pszTrusteeDnsDomain)
    {
        dwBufLen += strlen(pDomainInfo->pszTrusteeDnsDomain);
    }
    
    if (pDomainInfo->pszDomainSID)
    {
        dwBufLen += strlen(pDomainInfo->pszDomainSID);
    }
    
    if (pDomainInfo->pszDomainGUID)
    {
        dwBufLen += strlen(pDomainInfo->pszDomainGUID);
    }
    
    if (pDomainInfo->pszForestName)
    {
        dwBufLen += strlen(pDomainInfo->pszForestName);
    }
    
    if (pDomainInfo->pszClientSiteName)
    {
        dwBufLen += strlen(pDomainInfo->pszClientSiteName);
    }
    
    if (pDomainInfo->pDCInfo)
    {
        dwBufLen += LsaComputeDCInfoBufferLength(pDomainInfo->pDCInfo);
    }
    
    if (pDomainInfo->pGCInfo)
    {
        dwBufLen += LsaComputeDCInfoBufferLength(pDomainInfo->pGCInfo);
    }
    
    return dwBufLen;
}

DWORD
LsaComputeDCInfoBufferLength(
    PLSA_DC_INFO pDCInfo
    )
{
    DWORD dwBufLen = sizeof(LSA_DC_INFO_MSG);
    
    if (pDCInfo->pszAddress)
    {
        dwBufLen += strlen(pDCInfo->pszAddress);
    }
    
    if (pDCInfo->pszName)
    {
        dwBufLen += strlen(pDCInfo->pszName);
    }
    
    if (pDCInfo->pszSiteName)
    {
        dwBufLen += strlen(pDCInfo->pszSiteName);
    }
    
    return dwBufLen;
}

DWORD
LsaMarshalDomainInfo(
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfo,
    PSTR   pszBuffer,
    DWORD  dwOffset
    )
{
    DWORD dwBytesWritten = 0;
    LSA_DOMAIN_INFO_MSG domainInfoMsg;
    DWORD dwOffset2 = dwOffset;
    
    memset(&domainInfoMsg, 0, sizeof(domainInfoMsg));
    dwOffset2 += sizeof(domainInfoMsg);
    
    domainInfoMsg.dwDomainFlags = pDomainInfo->dwDomainFlags;
    domainInfoMsg.dwTrustAttributes = pDomainInfo->dwTrustAttributes;
    domainInfoMsg.dwTrustFlags = pDomainInfo->dwTrustFlags;
    domainInfoMsg.dwTrustType = pDomainInfo->dwTrustType;
    domainInfoMsg.dwTrustDirection = pDomainInfo->dwTrustDirection;
    domainInfoMsg.dwTrustMode = pDomainInfo->dwTrustMode;    
    
    if (pDomainInfo->pszClientSiteName)
    {
        domainInfoMsg.clientSiteName.length = strlen(pDomainInfo->pszClientSiteName);
        domainInfoMsg.clientSiteName.offset = dwOffset2;
        memcpy(pszBuffer + dwOffset2,
               pDomainInfo->pszClientSiteName,
               domainInfoMsg.clientSiteName.length);
        dwOffset2 += domainInfoMsg.clientSiteName.length;
        dwBytesWritten += domainInfoMsg.clientSiteName.length;
    }
    
    if (pDomainInfo->pszDnsDomain)
    {
        domainInfoMsg.dnsDomain.length = strlen(pDomainInfo->pszDnsDomain);
        domainInfoMsg.dnsDomain.offset = dwOffset2;
        memcpy(pszBuffer + dwOffset2,
               pDomainInfo->pszDnsDomain,
               domainInfoMsg.dnsDomain.length);
        dwOffset2 += domainInfoMsg.dnsDomain.length;
        dwBytesWritten += domainInfoMsg.dnsDomain.length;
    }
    
    if (pDomainInfo->pszDomainGUID)
    {
        domainInfoMsg.domainGUID.length = strlen(pDomainInfo->pszDomainGUID);
        domainInfoMsg.domainGUID.offset = dwOffset2;
        memcpy(pszBuffer + dwOffset2,
               pDomainInfo->pszDomainGUID,
               domainInfoMsg.domainGUID.length);
        dwOffset2 += domainInfoMsg.domainGUID.length;
        dwBytesWritten += domainInfoMsg.domainGUID.length;
    }
    
    if (pDomainInfo->pszDomainSID)
    {
        domainInfoMsg.domainSID.length = strlen(pDomainInfo->pszDomainSID);
        domainInfoMsg.domainSID.offset = dwOffset2;
        memcpy(pszBuffer + dwOffset2,
               pDomainInfo->pszDomainSID,
               domainInfoMsg.domainSID.length);
        dwOffset2 += domainInfoMsg.domainSID.length;       
        dwBytesWritten += domainInfoMsg.domainSID.length;
    }
    
    if (pDomainInfo->pszForestName)
    {
        domainInfoMsg.forestName.length = strlen(pDomainInfo->pszForestName);
        domainInfoMsg.forestName.offset = dwOffset2;
        memcpy(pszBuffer + dwOffset2,
               pDomainInfo->pszForestName,
               domainInfoMsg.forestName.length);
        dwOffset2 += domainInfoMsg.forestName.length; 
        dwBytesWritten += domainInfoMsg.forestName.length;
    }
    
    if (pDomainInfo->pszNetbiosDomain)
    {
        domainInfoMsg.netbiosDomain.length = strlen(pDomainInfo->pszNetbiosDomain);
        domainInfoMsg.netbiosDomain.offset = dwOffset2;
        memcpy(pszBuffer + dwOffset2,
               pDomainInfo->pszNetbiosDomain,
               domainInfoMsg.netbiosDomain.length);
        dwOffset2 += domainInfoMsg.netbiosDomain.length;
        dwBytesWritten += domainInfoMsg.netbiosDomain.length;
    }
    
    if (pDomainInfo->pszTrusteeDnsDomain)
    {
        domainInfoMsg.trusteeDnsDomain.length = strlen(pDomainInfo->pszTrusteeDnsDomain);
        domainInfoMsg.trusteeDnsDomain.offset = dwOffset2;
        memcpy(pszBuffer + dwOffset2,
               pDomainInfo->pszTrusteeDnsDomain,
               domainInfoMsg.trusteeDnsDomain.length);
        dwOffset2 += domainInfoMsg.trusteeDnsDomain.length;
        dwBytesWritten += domainInfoMsg.trusteeDnsDomain.length;
    }
    
    if (pDomainInfo->pDCInfo)
    {
        DWORD dwBytesWritten2 = 0;
        
        domainInfoMsg.dwDCInfoOffset = dwOffset2;
        
        dwBytesWritten2 = LsaMarshalDCInfo(
                            pDomainInfo->pDCInfo,
                            pszBuffer,
                            dwOffset2);
        dwOffset2 += dwBytesWritten2;
        dwBytesWritten += dwBytesWritten2;
    }
    
    if (pDomainInfo->pGCInfo)
    {
        DWORD dwBytesWritten2 = 0;
        
        domainInfoMsg.dwGCInfoOffset = dwOffset2;
        
        dwBytesWritten2 = LsaMarshalDCInfo(
                            pDomainInfo->pGCInfo,
                            pszBuffer,
                            dwOffset2);
        dwOffset2 += dwBytesWritten2;
        dwBytesWritten += dwBytesWritten2;
    }
    
    memcpy(pszBuffer + dwOffset,
           &domainInfoMsg,
           sizeof(domainInfoMsg));
    dwBytesWritten += sizeof(domainInfoMsg);
    
    return dwBytesWritten;
}

DWORD
LsaMarshalDCInfo(
    PLSA_DC_INFO pDCInfo,
    PSTR  pszBuffer,
    DWORD dwOffset
    )
{
    DWORD dwBytesWritten = 0;
    LSA_DC_INFO_MSG dcInfoMsg;
    DWORD dwOffset2 = dwOffset;
    
    memset(&dcInfoMsg, 0, sizeof(dcInfoMsg));
    dwOffset2 += sizeof(dcInfoMsg);
    
    dcInfoMsg.dwFlags = pDCInfo->dwFlags;
    
    if (pDCInfo->pszAddress)
    {
        dcInfoMsg.address.length = strlen(pDCInfo->pszAddress);
        dcInfoMsg.address.offset = dwOffset2;
        memcpy(pszBuffer + dwOffset2,
               pDCInfo->pszAddress,
               dcInfoMsg.address.length);
        dwOffset2 += dcInfoMsg.address.length;
        dwBytesWritten += dcInfoMsg.address.length;
    }
    
    if (pDCInfo->pszName)
    {
        dcInfoMsg.name.length = strlen(pDCInfo->pszName);
        dcInfoMsg.name.offset = dwOffset2;
        memcpy(pszBuffer + dwOffset2,
               pDCInfo->pszName,
               dcInfoMsg.name.length);
        dwOffset2 += dcInfoMsg.name.length;
        dwBytesWritten += dcInfoMsg.name.length;
    }
    
    if (pDCInfo->pszSiteName)
    {
        dcInfoMsg.siteName.length = strlen(pDCInfo->pszSiteName);
        dcInfoMsg.siteName.offset = dwOffset2;
        memcpy(pszBuffer + dwOffset2,
               pDCInfo->pszSiteName,
               dcInfoMsg.siteName.length);
        dwOffset2 += dcInfoMsg.siteName.length;
        dwBytesWritten += dcInfoMsg.siteName.length;
    }
    
    memcpy(pszBuffer + dwOffset,
           &dcInfoMsg,
           sizeof(dcInfoMsg));
    dwBytesWritten += sizeof(dcInfoMsg);
    
    return dwBytesWritten;
}

DWORD
LsaUnmarshalStatus(
    PCSTR       pszMsgBuf,
    DWORD       dwMsgLen,
    PLSASTATUS* ppLsaStatus
    )
{
    DWORD dwError = 0;
    DWORD dwReadOffset = 0;
    DWORD dwBytesRemaining = dwMsgLen;
    LSA_STATUS_MSG statusMsg;
    PLSASTATUS pLsaStatus = NULL;
    DWORD iCount = 0;

    BAIL_ON_INVALID_POINTER(ppLsaStatus);

    if (dwBytesRemaining < sizeof(LSA_STATUS_MSG)) {
       dwError = LSA_ERROR_INVALID_MESSAGE;
       BAIL_ON_LSA_ERROR(dwError);
    }

    memcpy(&statusMsg, pszMsgBuf, sizeof(statusMsg));
    dwReadOffset += sizeof(statusMsg);
    dwBytesRemaining -= sizeof(statusMsg);

    dwError = LsaAllocateMemory(
                  sizeof(LSASTATUS),
                  (PVOID*)&pLsaStatus);
    BAIL_ON_LSA_ERROR(dwError);

    pLsaStatus->dwUptime = statusMsg.dwUptime;
    pLsaStatus->dwCount = statusMsg.dwCount;
    memcpy(&pLsaStatus->version, &statusMsg.version, sizeof(statusMsg.version));

    if (!pLsaStatus->dwCount) {
       goto done;
    }

    dwError = LsaAllocateMemory(
                  sizeof(LSA_AUTH_PROVIDER_STATUS)*pLsaStatus->dwCount,
                  (PVOID*)&pLsaStatus->pAuthProviderStatusList);
    BAIL_ON_LSA_ERROR(dwError);

    for (iCount = 0; iCount < pLsaStatus->dwCount; iCount++)
    {
       LSA_AUTH_PROVIDER_STATUS_MSG providerStatusMsg;
       PLSA_AUTH_PROVIDER_STATUS pAuthProviderStatus = 
          &pLsaStatus->pAuthProviderStatusList[iCount];
       
       if (dwBytesRemaining < sizeof(providerStatusMsg)) {
          dwError = LSA_ERROR_INVALID_MESSAGE;
          BAIL_ON_LSA_ERROR(dwError);
       }

       memcpy(&providerStatusMsg,
              pszMsgBuf + dwReadOffset,
              sizeof(providerStatusMsg));

       dwReadOffset += sizeof(providerStatusMsg);
       dwBytesRemaining -= sizeof(providerStatusMsg);

       pAuthProviderStatus->mode = providerStatusMsg.dwMode;
       pAuthProviderStatus->subMode = providerStatusMsg.dwSubmode;
       pAuthProviderStatus->status = providerStatusMsg.dwStatus;
       pAuthProviderStatus->dwNetworkCheckInterval = providerStatusMsg.dwNetworkCheckInterval;

       if (providerStatusMsg.id.length) {
           dwError = LsaStrndup(
                         pszMsgBuf + providerStatusMsg.id.offset,
                         providerStatusMsg.id.length,
                         &pAuthProviderStatus->pszId);
           BAIL_ON_LSA_ERROR(dwError);
       }

       if (providerStatusMsg.domain.length) {
          dwError = LsaStrndup(
                         pszMsgBuf + providerStatusMsg.domain.offset,
                         providerStatusMsg.domain.length,
                         &pAuthProviderStatus->pszDomain);
          BAIL_ON_LSA_ERROR(dwError);
       }
       
       if (providerStatusMsg.forest.length) {
          dwError = LsaStrndup(
                         pszMsgBuf + providerStatusMsg.forest.offset,
                         providerStatusMsg.forest.length,
                         &pAuthProviderStatus->pszForest);
          BAIL_ON_LSA_ERROR(dwError);
       }
       
       if (providerStatusMsg.site.length) {
          dwError = LsaStrndup(
                         pszMsgBuf + providerStatusMsg.site.offset,
                         providerStatusMsg.site.length,
                         &pAuthProviderStatus->pszSite);
          BAIL_ON_LSA_ERROR(dwError);
       }

       if (providerStatusMsg.cell.length) {
          dwError = LsaStrndup(
                         pszMsgBuf + providerStatusMsg.cell.offset,
                         providerStatusMsg.cell.length,
                         &pAuthProviderStatus->pszCell);
          BAIL_ON_LSA_ERROR(dwError);
       }
       
       if (providerStatusMsg.dwTrustedDomainOffset)
       {
           pAuthProviderStatus->dwNumTrustedDomains = providerStatusMsg.dwNumTrustedDomains;
           
           dwError = LsaUnmarshalDomainInfoList(
                           pszMsgBuf,
                           dwMsgLen,
                           providerStatusMsg.dwTrustedDomainOffset,
                           pAuthProviderStatus->dwNumTrustedDomains,
                           &pAuthProviderStatus->pTrustedDomainInfoArray);
           BAIL_ON_LSA_ERROR(dwError);
       }
    }

done:

    *ppLsaStatus = pLsaStatus;
    

cleanup:

    return dwError;

error:

    if (ppLsaStatus) {
       *ppLsaStatus = NULL;
    }

    if (pLsaStatus) {
       LsaFreeStatus(pLsaStatus);
    }

    goto cleanup;
}

DWORD
LsaUnmarshalDomainInfoList(
    PCSTR pszMsgBuf,
    DWORD dwMessageLen,
    DWORD dwReadOffset,
    DWORD dwNumDomains,
    PLSA_TRUSTED_DOMAIN_INFO* ppDomainInfoArray
    )
{
    DWORD dwError = 0;
    DWORD iDomain = 0;
    DWORD dwBytesRemaining = 0;
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfoArray = NULL;
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_TRUSTED_DOMAIN_INFO) * dwNumDomains,
                    (PVOID*)&pDomainInfoArray);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwBytesRemaining = dwMessageLen - dwReadOffset;
    
    for (; iDomain < dwNumDomains; iDomain++)
    {
        LSA_DOMAIN_INFO_MSG domainInfoMsg;
        
        PLSA_TRUSTED_DOMAIN_INFO pDomainInfo = 
                &pDomainInfoArray[iDomain];
        
        if (dwBytesRemaining < sizeof(domainInfoMsg))
        {
            dwError = LSA_ERROR_INVALID_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        memcpy(&domainInfoMsg, pszMsgBuf + dwReadOffset, sizeof(domainInfoMsg));
        
        dwReadOffset += sizeof(domainInfoMsg);
        
        pDomainInfo->dwDomainFlags = domainInfoMsg.dwDomainFlags;
        pDomainInfo->dwTrustAttributes = domainInfoMsg.dwTrustAttributes;
        pDomainInfo->dwTrustFlags = domainInfoMsg.dwTrustFlags;
        pDomainInfo->dwTrustType = domainInfoMsg.dwTrustType;
        pDomainInfo->dwTrustDirection = domainInfoMsg.dwTrustDirection;
        pDomainInfo->dwTrustMode = domainInfoMsg.dwTrustMode;
        
        if (domainInfoMsg.clientSiteName.length)
        {
            dwError = LsaStrndup(
                            pszMsgBuf + domainInfoMsg.clientSiteName.offset,
                            domainInfoMsg.clientSiteName.length,
                            &pDomainInfo->pszClientSiteName);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwReadOffset += domainInfoMsg.clientSiteName.length;
        }
        
        if (domainInfoMsg.dnsDomain.length)
        {
            dwError = LsaStrndup(
                            pszMsgBuf + domainInfoMsg.dnsDomain.offset,
                            domainInfoMsg.dnsDomain.length,
                            &pDomainInfo->pszDnsDomain);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwReadOffset += domainInfoMsg.dnsDomain.length;
        }
        
        if (domainInfoMsg.domainGUID.length)
        {
            dwError = LsaStrndup(
                            pszMsgBuf + domainInfoMsg.domainGUID.offset,
                            domainInfoMsg.domainGUID.length,
                            &pDomainInfo->pszDomainGUID);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwReadOffset += domainInfoMsg.domainGUID.length;
        }
        
        if (domainInfoMsg.domainSID.length)
        {
            dwError = LsaStrndup(
                            pszMsgBuf + domainInfoMsg.domainSID.offset,
                            domainInfoMsg.domainSID.length,
                            &pDomainInfo->pszDomainSID);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwReadOffset += domainInfoMsg.domainSID.length;
        }
        
        if (domainInfoMsg.forestName.length)
        {
            dwError = LsaStrndup(
                            pszMsgBuf + domainInfoMsg.forestName.offset,
                            domainInfoMsg.forestName.length,
                            &pDomainInfo->pszForestName);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwReadOffset += domainInfoMsg.forestName.length;
        }
        
        if (domainInfoMsg.netbiosDomain.length)
        {
            dwError = LsaStrndup(
                            pszMsgBuf + domainInfoMsg.netbiosDomain.offset,
                            domainInfoMsg.netbiosDomain.length,
                            &pDomainInfo->pszNetbiosDomain);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwReadOffset += domainInfoMsg.netbiosDomain.length;
        }
        
        if (domainInfoMsg.trusteeDnsDomain.length)
        {
            dwError = LsaStrndup(
                            pszMsgBuf + domainInfoMsg.trusteeDnsDomain.offset,
                            domainInfoMsg.trusteeDnsDomain.length,
                            &pDomainInfo->pszTrusteeDnsDomain);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwReadOffset += domainInfoMsg.trusteeDnsDomain.length;
        }
        
        if (domainInfoMsg.dwDCInfoOffset)
        {
            DWORD dwBytesRead = 0;
            
            dwError = LsaUnmarshalDCInfo(
                            pszMsgBuf,
                            dwMessageLen,
                            domainInfoMsg.dwDCInfoOffset,
                            &dwBytesRead,
                            &pDomainInfo->pDCInfo);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwReadOffset += dwBytesRead;
        }
        
        if (domainInfoMsg.dwGCInfoOffset)
        {
            DWORD dwBytesRead = 0;
            
            dwError = LsaUnmarshalDCInfo(
                            pszMsgBuf,
                            dwMessageLen,
                            domainInfoMsg.dwGCInfoOffset,
                            &dwBytesRead,
                            &pDomainInfo->pGCInfo);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwReadOffset += dwBytesRead;
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
LsaUnmarshalDCInfo(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    DWORD  dwReadOffset,
    PDWORD pdwBytesRead,
    PLSA_DC_INFO* ppDCInfo
    )
{
    DWORD dwError = 0;
    PLSA_DC_INFO pDCInfo = NULL;
    DWORD dwBytesRead = 0;
    LSA_DC_INFO_MSG dcInfoMsg;
    DWORD dwBytesRemaining = dwMsgLen - dwReadOffset;
    
    if (dwBytesRemaining < sizeof(dcInfoMsg))
    {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(&dcInfoMsg, pszMsgBuf + dwReadOffset, sizeof(dcInfoMsg));
    
    dwBytesRead += sizeof(dcInfoMsg);
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_DC_INFO),
                    (PVOID*)&pDCInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    pDCInfo->dwFlags = dcInfoMsg.dwFlags;
    
    if (dcInfoMsg.address.length)
    {
        dwError = LsaStrndup(
                        pszMsgBuf + dcInfoMsg.address.offset,
                        dcInfoMsg.address.length,
                        &pDCInfo->pszAddress);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwBytesRead += dcInfoMsg.address.length;
    }
    
    if (dcInfoMsg.name.length)
    {
        dwError = LsaStrndup(
                        pszMsgBuf + dcInfoMsg.name.offset,
                        dcInfoMsg.name.length,
                        &pDCInfo->pszName);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwBytesRead += dcInfoMsg.name.length;
    }
    
    if (dcInfoMsg.siteName.length)
    {
        dwError = LsaStrndup(
                        pszMsgBuf + dcInfoMsg.siteName.offset,
                        dcInfoMsg.siteName.length,
                        &pDCInfo->pszSiteName);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwBytesRead += dcInfoMsg.siteName.length;
    }
    
    *pdwBytesRead = dwBytesRead;
    *ppDCInfo = pDCInfo;
    
cleanup:

    return dwError;
    
error:

    *pdwBytesRead = 0;
    *ppDCInfo = NULL;
    
    if (pDCInfo)
    {
        LsaFreeDCInfo(pDCInfo);
    }

    goto cleanup;
}
