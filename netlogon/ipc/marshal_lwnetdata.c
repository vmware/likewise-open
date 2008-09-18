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
 *        marshal_lwnetdata.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        Marshal/Unmarshal API for Site Data
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "includes.h"

DWORD
LWNetMarshalDCNameReq(
    PCSTR           pszServerFQDN,
    PCSTR           pszDomainFQDN,
    PCSTR           pszSiteName,
    DWORD           dwFlags,
    PSTR            pszBuf,
    PDWORD          pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwRequiredLength = 0;
    DWORD dwOffset = 0;
    LWNET_DC_NAME_REQ_HEADER header;
    
    dwRequiredLength = 
        LWNetComputeDCNameReqLength(
                pszServerFQDN,
                pszDomainFQDN,
                pszSiteName
                );
    
    if (!pszBuf) {
       *pdwBufLen = dwRequiredLength;
       goto error;
    }

    if (*pdwBufLen < dwRequiredLength) {
        dwError = LWNET_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    memset(&header, 0, sizeof(header));
    dwOffset = sizeof(header);

    if (!IsNullOrEmptyString(pszServerFQDN))
    {  
        header.serverFQDN.length = strlen(pszServerFQDN);
        header.serverFQDN.offset = dwOffset;
        memcpy(pszBuf + dwOffset,
               pszServerFQDN,
               header.serverFQDN.length);
        dwOffset += header.serverFQDN.length;
    }
    
    if (!IsNullOrEmptyString(pszDomainFQDN))
    {  
        header.domainFQDN.length = strlen(pszDomainFQDN);
        header.domainFQDN.offset = dwOffset;
        memcpy(pszBuf + dwOffset,
               pszDomainFQDN,
               header.domainFQDN.length);
        dwOffset += header.domainFQDN.length;
    }
    
    if (!IsNullOrEmptyString(pszSiteName))
    {  
        header.siteName.length = strlen(pszSiteName);
        header.siteName.offset = dwOffset;
        memcpy(pszBuf + dwOffset,
               pszSiteName,
               header.siteName.length);
        dwOffset += header.siteName.length;
    }
    
    header.flags = dwFlags;
    memcpy(pszBuf, &header, sizeof(header)); 
    
error:
    return dwError;
}

DWORD
LWNetComputeDCNameReqLength(
    PCSTR           pszServerFQDN,
    PCSTR           pszDomainFQDN,
    PCSTR           pszSiteName
    )
{
    DWORD dwRequiredLength = sizeof(LWNET_DC_NAME_REQ_HEADER);

    if (!IsNullOrEmptyString(pszServerFQDN))
       dwRequiredLength += strlen(pszServerFQDN);
    
    if (!IsNullOrEmptyString(pszDomainFQDN))
       dwRequiredLength += strlen(pszDomainFQDN);

    if (!IsNullOrEmptyString(pszSiteName))
       dwRequiredLength += strlen(pszSiteName);
    
    return dwRequiredLength;
}

DWORD
LWNetUnmarshalDCNameReq(
    PCSTR pszMsgBuf,
    DWORD dwMsgLen,
    PSTR* ppszServerFQDN,
    PSTR* ppszDomainFQDN,
    PSTR* ppszSiteName,
    PDWORD pdwFlags
    )
{
    DWORD dwError = 0;
    LWNET_DC_NAME_REQ_HEADER header;
    PSTR pszDomainFQDN = NULL;
    PSTR pszServerFQDN = NULL;
    PSTR pszSiteName = NULL;
    
    if (dwMsgLen < sizeof(header)) {
       dwError = LWNET_ERROR_INVALID_MESSAGE;
       BAIL_ON_LWNET_ERROR(dwError);
    }

    memcpy(&header, pszMsgBuf, sizeof(header));

    if (header.serverFQDN.length)
    {  
        dwError = LWNetStrndup(
                        pszMsgBuf + header.serverFQDN.offset,
                        header.serverFQDN.length,
                        &pszServerFQDN);
        BAIL_ON_LWNET_ERROR(dwError);   
    }
    
    if (header.domainFQDN.length)
    {  
        dwError = LWNetStrndup(
                        pszMsgBuf + header.domainFQDN.offset,
                        header.domainFQDN.length,
                        &pszDomainFQDN);
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    if (header.siteName.length)
    {  
        dwError = LWNetStrndup(
                        pszMsgBuf + header.siteName.offset,
                        header.siteName.length,
                        &pszSiteName);
        BAIL_ON_LWNET_ERROR(dwError);   
    }

error:
    if (dwError)
    {
        LWNET_SAFE_FREE_STRING(pszServerFQDN);
        LWNET_SAFE_FREE_STRING(pszDomainFQDN);
        LWNET_SAFE_FREE_STRING(pszSiteName);
        header.flags =0;
    }

    *ppszServerFQDN = pszServerFQDN;
    *ppszDomainFQDN = pszDomainFQDN;
    *ppszSiteName = pszSiteName;
    *pdwFlags = header.flags;

    return dwError;
}

DWORD
LWNetMarshalDCInfo(
    PLWNET_DC_INFO pDCInfo,
    PSTR            pszBuf,
    PDWORD          pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwOffset = 0;
    DWORD dwRequiredBufLength = 0;
    LWNET_DC_INFO_HEADER header = {0};
  
    BAIL_ON_INVALID_POINTER(pDCInfo);
    
    dwRequiredBufLength = LWNetComputeBufferLength(pDCInfo);

    if (!pszBuf) {
       *pdwBufLen = dwRequiredBufLength;
       goto error;
    }

    if (*pdwBufLen < dwRequiredBufLength) {
        dwError = LWNET_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    header.domainControllerAddressType = pDCInfo->dwDomainControllerAddressType;
    header.flags   = pDCInfo->dwFlags;
    header.version = pDCInfo->dwVersion;
    header.LMToken = pDCInfo->wLMToken;
    header.NTToken = pDCInfo->wNTToken;

    memcpy(header.domainGUID, pDCInfo->pucDomainGUID, LWNET_GUID_SIZE);

    dwOffset = sizeof(header);

    if (!IsNullOrEmptyString(pDCInfo->pszDomainControllerName))
    {  
        header.domainControllerName.length = 
                  strlen(pDCInfo->pszDomainControllerName);
        header.domainControllerName.offset = dwOffset;
        memcpy(pszBuf+dwOffset,
               pDCInfo->pszDomainControllerName,
               header.domainControllerName.length);
        dwOffset += header.domainControllerName.length;
    }
    
    if (!IsNullOrEmptyString(pDCInfo->pszDomainControllerAddress))
    {  
        header.domainControllerAddress.length =
                  strlen(pDCInfo->pszDomainControllerAddress);
        header.domainControllerAddress.offset = dwOffset;
        memcpy(pszBuf+dwOffset,
               pDCInfo->pszDomainControllerAddress,
               header.domainControllerAddress.length);
        dwOffset += header.domainControllerAddress.length;
    }
    
    if (!IsNullOrEmptyString(pDCInfo->pszNetBIOSDomainName))
    {  
        header.netBIOSDomainName.length = strlen(pDCInfo->pszNetBIOSDomainName);
        header.netBIOSDomainName.offset = dwOffset;
        memcpy(pszBuf+dwOffset,
               pDCInfo->pszNetBIOSDomainName,
               header.netBIOSDomainName.length);
        dwOffset += header.netBIOSDomainName.length;
    }

    if (!IsNullOrEmptyString(pDCInfo->pszFullyQualifiedDomainName))
    {  
        header.fullyQualifiedDomainName.length =
                 strlen(pDCInfo->pszFullyQualifiedDomainName);
        header.fullyQualifiedDomainName.offset = dwOffset;
        memcpy(pszBuf+dwOffset,
               pDCInfo->pszFullyQualifiedDomainName,
               header.fullyQualifiedDomainName.length);
        dwOffset += header.fullyQualifiedDomainName.length;
    }

    if (!IsNullOrEmptyString(pDCInfo->pszDnsForestName))
    {  
        header.DNSForestName.length = strlen(pDCInfo->pszDnsForestName);
        header.DNSForestName.offset = dwOffset;
        memcpy(pszBuf+dwOffset,
               pDCInfo->pszDnsForestName,
               header.DNSForestName.length);
        dwOffset += header.DNSForestName.length;
    }

    if (!IsNullOrEmptyString(pDCInfo->pszDCSiteName))
    {  
        header.DCSiteName.length = strlen(pDCInfo->pszDCSiteName);
        header.DCSiteName.offset = dwOffset;
        memcpy(pszBuf+dwOffset,
               pDCInfo->pszDCSiteName,
               header.DCSiteName.length);
        dwOffset += header.DCSiteName.length;
    }
    
    if (!IsNullOrEmptyString(pDCInfo->pszClientSiteName))
    {  
        header.clientSiteName.length = strlen(pDCInfo->pszClientSiteName);
        header.clientSiteName.offset = dwOffset;
        memcpy(pszBuf+dwOffset,
               pDCInfo->pszClientSiteName,
               header.clientSiteName.length);
        dwOffset += header.clientSiteName.length;
    }
    
    if (!IsNullOrEmptyString(pDCInfo->pszNetBIOSHostName))
    {  
        header.netBIOSHostName.length = strlen(pDCInfo->pszNetBIOSHostName);
        header.netBIOSHostName.offset = dwOffset;
        memcpy(pszBuf+dwOffset,
               pDCInfo->pszNetBIOSHostName,
               header.netBIOSHostName.length);
        dwOffset += header.netBIOSHostName.length;
    }

    if (!IsNullOrEmptyString(pDCInfo->pszUserName))
    {  
        header.userName.length = strlen(pDCInfo->pszUserName);
        header.userName.offset = dwOffset;
        memcpy(pszBuf+dwOffset,
               pDCInfo->pszUserName,
               header.userName.length);
        dwOffset += header.userName.length;
    }
    memcpy(pszBuf, &header, sizeof(header));     
    
error:
    return dwError;
}

DWORD
LWNetComputeBufferLength(
    PLWNET_DC_INFO pDCInfo
    )
{
    DWORD dwRequiredLength = sizeof(LWNET_DC_INFO_HEADER);
    
    if (!IsNullOrEmptyString(pDCInfo->pszDomainControllerName))
    {  
        dwRequiredLength += strlen(pDCInfo->pszDomainControllerName);
    }
    
    if (!IsNullOrEmptyString(pDCInfo->pszDomainControllerAddress))
    {  
        dwRequiredLength += strlen(pDCInfo->pszDomainControllerAddress);
    }
    
    if (!IsNullOrEmptyString(pDCInfo->pszNetBIOSDomainName))
    {  
        dwRequiredLength += strlen(pDCInfo->pszNetBIOSDomainName);
    }

    if (!IsNullOrEmptyString(pDCInfo->pszFullyQualifiedDomainName))
    {  
        dwRequiredLength += strlen(pDCInfo->pszFullyQualifiedDomainName);
    }

    if (!IsNullOrEmptyString(pDCInfo->pszDnsForestName))
    {  
        dwRequiredLength += strlen(pDCInfo->pszDnsForestName);
    }

    if (!IsNullOrEmptyString(pDCInfo->pszDCSiteName))
    {  
        dwRequiredLength += strlen(pDCInfo->pszDCSiteName);
    }
    
    if (!IsNullOrEmptyString(pDCInfo->pszClientSiteName))
    {  
        dwRequiredLength += strlen(pDCInfo->pszClientSiteName);
    }
    
    if (!IsNullOrEmptyString(pDCInfo->pszNetBIOSHostName))
    {  
        dwRequiredLength += strlen(pDCInfo->pszNetBIOSHostName);
    }

    if (!IsNullOrEmptyString(pDCInfo->pszUserName))
    {  
        dwRequiredLength += strlen(pDCInfo->pszUserName);
    }

    return dwRequiredLength;
}

DWORD
LWNetUnmarshalDCInfo(
    PCSTR         pszMsgBuf,
    DWORD         dwMsgLen,
    PLWNET_DC_INFO* ppDCInfo
    )
{
    DWORD dwError = 0;
    LWNET_DC_INFO_HEADER header = {0};
    PLWNET_DC_INFO pDCInfo = NULL;
    
    if (dwMsgLen < sizeof(header)) {
        dwError = LWNET_ERROR_INVALID_MESSAGE;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    memcpy(&header, pszMsgBuf, sizeof(header));

    dwError = LWNetAllocateMemory(
                    sizeof(LWNET_DC_INFO),
                    (PVOID*)&pDCInfo);
    BAIL_ON_LWNET_ERROR(dwError);

    pDCInfo->dwDomainControllerAddressType =
              header.domainControllerAddressType;
    pDCInfo->dwFlags = header.flags;
    pDCInfo->dwVersion = header.version;
    pDCInfo->wLMToken = header.LMToken;
    pDCInfo->wNTToken = header.NTToken;    

    memcpy(pDCInfo->pucDomainGUID, header.domainGUID, LWNET_GUID_SIZE);

    if (header.domainControllerName.length)
    {  
        dwError = LWNetStrndup(
                        pszMsgBuf + header.domainControllerName.offset,
                        header.domainControllerName.length, 
                        &pDCInfo->pszDomainControllerName);
        BAIL_ON_LWNET_ERROR(dwError);   
    }
    
    if (header.domainControllerAddress.length)
    {
        dwError = LWNetStrndup(
                        pszMsgBuf + header.domainControllerAddress.offset,
                        header.domainControllerAddress.length,
                        &pDCInfo->pszDomainControllerAddress);
        BAIL_ON_LWNET_ERROR(dwError); 
    }
    
    if (header.netBIOSDomainName.length)
    {  
        dwError = LWNetStrndup(
                        pszMsgBuf + header.netBIOSDomainName.offset,
                        header.netBIOSDomainName.length,
                        &pDCInfo->pszNetBIOSDomainName);
        BAIL_ON_LWNET_ERROR(dwError);   
    }

    if (header.fullyQualifiedDomainName.length)
    {  
        dwError = LWNetStrndup(
                        pszMsgBuf + header.fullyQualifiedDomainName.offset,
                        header.fullyQualifiedDomainName.length,
                        &pDCInfo->pszFullyQualifiedDomainName);
        BAIL_ON_LWNET_ERROR(dwError); 
    }

    if (header.DNSForestName.length)
    {  
        dwError = LWNetStrndup(
                        pszMsgBuf + header.DNSForestName.offset,
                        header.DNSForestName.length,
                        &pDCInfo->pszDnsForestName);
        BAIL_ON_LWNET_ERROR(dwError); 
    }

    if (header.DCSiteName.length)
    {  
        dwError = LWNetStrndup(
                        pszMsgBuf + header.DCSiteName.offset,
                        header.DCSiteName.length,
                        &pDCInfo->pszDCSiteName);
        BAIL_ON_LWNET_ERROR(dwError); 
    }
    
    if (header.clientSiteName.length)
    {  
        dwError = LWNetStrndup(
                        pszMsgBuf + header.clientSiteName.offset,
                        header.clientSiteName.length,
                        &pDCInfo->pszClientSiteName);
        BAIL_ON_LWNET_ERROR(dwError); 
    }
    
    if (header.netBIOSHostName.length)
    {  
        dwError = LWNetStrndup(
                        pszMsgBuf + header.netBIOSHostName.offset,
                        header.netBIOSHostName.length,
                        &pDCInfo->pszNetBIOSHostName);
        BAIL_ON_LWNET_ERROR(dwError); 
    }

    if (header.userName.length)
    {  
        dwError = LWNetStrndup(
                        pszMsgBuf + header.userName.offset,
                        header.userName.length,
                        &pDCInfo->pszUserName);
        BAIL_ON_LWNET_ERROR(dwError); 
    }

error:
    if (dwError)
    {
        LWNET_SAFE_FREE_DC_INFO(pDCInfo);
    }

    *ppDCInfo = pDCInfo;

    return dwError;
}
