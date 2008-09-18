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
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) 
 *        
 *        Tool to get status from LSA Server
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#define _POSIX_PTHREAD_SEMANTICS 1

#include "config.h"
#include "lsasystem.h"
#include "lsadef.h"
#include "lsa/lsa.h"

#include "lsaclient.h"
#include "lsaipc.h"

#define LSA_MODE_STRING_UNKNOWN          "Unknown"
#define LSA_MODE_STRING_UNPROVISIONED    "Un-provisioned"
#define LSA_MODE_STRING_DEFAULT_CELL     "Default Cell"
#define LSA_MODE_STRING_NON_DEFAULT_CELL "Non-default Cell"
#define LSA_MODE_STRING_LOCAL            "Local system"

#define LSA_SUBMODE_STRING_UNKNOWN       "Unknown"
#define LSA_SUBMODE_STRING_SCHEMA        "Schema"
#define LSA_SUBMODE_STRING_NON_SCHEMA    "Non-schema"

#define LSA_STATUS_STRING_UNKNOWN        "Unknown"
#define LSA_STATUS_STRING_ONLINE         "Online"
#define LSA_STATUS_STRING_OFFLINE        "Offline"

#define LSA_TRUST_TYPE_STRING_UNKNOWN    "Unknown"
#define LSA_TRUST_TYPE_STRING_DOWNLEVEL  "Down Level"
#define LSA_TRUST_TYPE_STRING_UPLEVEL    "Up Level"
#define LSA_TRUST_TYPE_STRING_MIT        "MIT"
#define LSA_TRUST_TYPE_STRING_DCE        "DCE"

VOID
ParseArgs(
    int    argc,
    char*  argv[]
    );

VOID
ShowUsage();

VOID
PrintStatus(
    PLSASTATUS pStatus
    );

PCSTR
GetStatusString(
    LsaAuthProviderStatus status
    );

PCSTR
GetModeString(
    LsaAuthProviderMode mode
    );

PCSTR
GetSubmodeString(
    LsaAuthProviderSubMode subMode
    );

PCSTR
GetTrustTypeString(
    DWORD dwTrustType
    );

DWORD
MapErrorCode(
    DWORD dwError
    );


int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    PLSASTATUS pLsaStatus = NULL;
    HANDLE hLsaConnection = (HANDLE)NULL;
    size_t dwErrorBufferSize = 0;
    BOOLEAN bPrintOrigError = TRUE;
    
    ParseArgs(argc, argv);
    
    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaGetStatus(
                    hLsaConnection,
                    &pLsaStatus);
    BAIL_ON_LSA_ERROR(dwError);
    
    PrintStatus(pLsaStatus);

cleanup:

    if (pLsaStatus) {
       LsaFreeStatus(pLsaStatus);
    }
    
    if (hLsaConnection != (HANDLE)NULL) {
        LsaCloseServer(hLsaConnection);
    }

    return (dwError);

error:

    dwError = MapErrorCode(dwError);
    
    dwErrorBufferSize = LsaGetErrorString(dwError, NULL, 0);
    
    if (dwErrorBufferSize > 0)
    {
        DWORD dwError2 = 0;
        PSTR   pszErrorBuffer = NULL;
        
        dwError2 = LsaAllocateMemory(
                    dwErrorBufferSize,
                    (PVOID*)&pszErrorBuffer);
        
        if (!dwError2)
        {
            DWORD dwLen = LsaGetErrorString(dwError, pszErrorBuffer, dwErrorBufferSize);
            
            if ((dwLen == dwErrorBufferSize) && !IsNullOrEmptyString(pszErrorBuffer))
            {
                fprintf(stderr, "Failed to query status from LSA service.  %s\n", pszErrorBuffer);
                bPrintOrigError = FALSE;
            }
        }
        
        LSA_SAFE_FREE_STRING(pszErrorBuffer);
    }
    
    if (bPrintOrigError)
    {
        fprintf(stderr, "Failed to query status from LSA service. Error code [%d]\n", dwError);
    }

    goto cleanup;
}

VOID
ParseArgs(
    int    argc,
    char*  argv[]
    )
{
    typedef enum {
            PARSE_MODE_OPEN = 0
        } ParseMode;
        
    int iArg = 1;
    PSTR pszArg = NULL;
    ParseMode parseMode = PARSE_MODE_OPEN;

    do {
        pszArg = argv[iArg++];
        if (pszArg == NULL || *pszArg == '\0')
        {
            break;
        }
        
        switch (parseMode)
        {
            case PARSE_MODE_OPEN:
        
                if ((strcmp(pszArg, "--help") == 0) ||
                    (strcmp(pszArg, "-h") == 0))
                {
                    ShowUsage();
                    exit(0);
                }
                else
                {
                    ShowUsage();
                    exit(1);
                }

                break;
        }
        
    } while (iArg < argc);
}

void
ShowUsage()
{
    printf("Usage: lw-get-status\n");
}

VOID
PrintStatus(
    PLSASTATUS pStatus
    )
{
    DWORD iCount = 0;
    DWORD dwDays = pStatus->dwUptime/LSA_SECONDS_IN_DAY;
    DWORD dwHours = (pStatus->dwUptime - (dwDays * LSA_SECONDS_IN_DAY))/LSA_SECONDS_IN_HOUR;
    DWORD dwMins = (pStatus->dwUptime - 
                    (dwDays * LSA_SECONDS_IN_DAY) - 
                    (dwHours * LSA_SECONDS_IN_HOUR))/LSA_SECONDS_IN_MINUTE;
    DWORD dwSecs = (pStatus->dwUptime - 
                    (dwDays * LSA_SECONDS_IN_DAY) - 
                    (dwHours * LSA_SECONDS_IN_HOUR) - 
                    (dwMins * LSA_SECONDS_IN_MINUTE));

    printf("LSA Server Status:\n\n");
    printf("Agent version: %u.%u.%u\n",
                    pStatus->version.dwMajor,
                    pStatus->version.dwMinor,
                    pStatus->version.dwBuild);
    printf("Uptime:        %u days %u hours %u minutes %u seconds\n", dwDays, dwHours, dwMins, dwSecs);
    
    for (iCount = 0; iCount < pStatus->dwCount; iCount++)
    {
        PLSA_AUTH_PROVIDER_STATUS pProviderStatus =
            &pStatus->pAuthProviderStatusList[iCount];
        
        printf("\n[Authentication provider: %s]\n\n", 
                        IsNullOrEmptyString(pProviderStatus->pszId) ? "" : pProviderStatus->pszId);
        
        printf("\tStatus:                %s\n", GetStatusString(pProviderStatus->status));
        printf("\tMode:                  %s\n", GetModeString(pProviderStatus->mode));

        switch (pProviderStatus->mode)
        {
            case LSA_PROVIDER_MODE_LOCAL_SYSTEM:
                break;
            
            case LSA_PROVIDER_MODE_UNPROVISIONED:
            case LSA_PROVIDER_MODE_DEFAULT_CELL:
            case LSA_PROVIDER_MODE_NON_DEFAULT_CELL:
                
                printf("\tDomain:        %s\n", IsNullOrEmptyString(pProviderStatus->pszDomain) ? "" : pProviderStatus->pszDomain);
                printf("\tForest:        %s\n", IsNullOrEmptyString(pProviderStatus->pszForest) ? "" : pProviderStatus->pszForest);
                printf("\tSite:          %s\n", IsNullOrEmptyString(pProviderStatus->pszSite) ? "" : pProviderStatus->pszSite);
                printf("\tOnline check interval:  %d seconds\n", pProviderStatus->dwNetworkCheckInterval);

                break;
                
            default:
                
                break;
        }
        
        switch (pProviderStatus->mode)
        {
            case LSA_PROVIDER_MODE_DEFAULT_CELL:
                
                printf("\tSub mode:      %s\n", GetSubmodeString(pProviderStatus->subMode));
                
                break;
                
            case LSA_PROVIDER_MODE_NON_DEFAULT_CELL:
                
                printf("\tSub mode:      %s\n", GetSubmodeString(pProviderStatus->subMode));
                
                printf("\tCell:          %s\n", IsNullOrEmptyString(pProviderStatus->pszCell) ? "" : pProviderStatus->pszCell);
                
                break;
                
            default:
                
                break;
        }
        
        if (pProviderStatus->pTrustedDomainInfoArray)
        {
            DWORD iDomain = 0;
            
            printf("\t[Trusted Domains: %d]\n\n", pProviderStatus->dwNumTrustedDomains);
            
            for (; iDomain < pProviderStatus->dwNumTrustedDomains; iDomain++)
            {
                PLSA_TRUSTED_DOMAIN_INFO pDomainInfo =
                    &pProviderStatus->pTrustedDomainInfoArray[iDomain];
                
                printf("\n\t[Domain: %s]\n\n", IsNullOrEmptyString(pDomainInfo->pszNetbiosDomain) ? "" : pDomainInfo->pszNetbiosDomain);
                
                printf("\t\tDNS Domain:       %s\n", IsNullOrEmptyString(pDomainInfo->pszDnsDomain) ? "" : pDomainInfo->pszDnsDomain);
                printf("\t\tNetbios name:     %s\n", IsNullOrEmptyString(pDomainInfo->pszNetbiosDomain) ? "" : pDomainInfo->pszNetbiosDomain);
                printf("\t\tForest name:      %s\n", IsNullOrEmptyString(pDomainInfo->pszForestName) ? "" : pDomainInfo->pszForestName);
                printf("\t\tTrustee DNS name: %s\n", IsNullOrEmptyString(pDomainInfo->pszTrusteeDnsDomain) ? "" : pDomainInfo->pszTrusteeDnsDomain);
                printf("\t\tClient site name: %s\n", IsNullOrEmptyString(pDomainInfo->pszClientSiteName) ? "" : pDomainInfo->pszClientSiteName);
                printf("\t\tDomain SID:       %s\n", IsNullOrEmptyString(pDomainInfo->pszDomainSID) ? "" : pDomainInfo->pszDomainSID);
                printf("\t\tDomain GUID:      %s\n", IsNullOrEmptyString(pDomainInfo->pszDomainGUID) ? "" : pDomainInfo->pszDomainGUID);
                printf("\t\tTrust Flags:      0x%x\n", pDomainInfo->dwTrustFlags);
                printf("\t\tTrust type:       %s\n", GetTrustTypeString(pDomainInfo->dwTrustType));
                printf("\t\tTrust Attributes: 0x%x\n", pDomainInfo->dwTrustAttributes);
                printf("\t\tDomain flags:     0x%x\n", pDomainInfo->dwDomainFlags);
                
                if (pDomainInfo->pDCInfo)
                {
                    printf("\n\t\t[Domain Controller (DC) Information]\n\n");
                    printf("\t\t\tDC Name:              %s\n", IsNullOrEmptyString(pDomainInfo->pDCInfo->pszName) ? "" : pDomainInfo->pDCInfo->pszName);
                    printf("\t\t\tDC Address:           %s\n", IsNullOrEmptyString(pDomainInfo->pDCInfo->pszAddress) ? "" : pDomainInfo->pDCInfo->pszAddress);
                    printf("\t\t\tDC Site:              %s\n", IsNullOrEmptyString(pDomainInfo->pDCInfo->pszSiteName) ? "" : pDomainInfo->pDCInfo->pszSiteName);
                    printf("\t\t\tDC Flags:             0x%x\n", pDomainInfo->pDCInfo->dwFlags);
                    printf("\t\t\tDC Is PDC:            %s\n",
                                    (pDomainInfo->pDCInfo->dwFlags & LSA_DS_PDC_FLAG) ? "yes" : "no");
                    printf("\t\t\tDC is time server:    %s\n",
                                    (pDomainInfo->pDCInfo->dwFlags & LSA_DS_TIMESERV_FLAG) ? "yes" : "no");
                    printf("\t\t\tDC has writeable DS:  %s\n",
                                    (pDomainInfo->pDCInfo->dwFlags & LSA_DS_WRITABLE_FLAG) ? "yes" : "no");
                    printf("\t\t\tDC is Global Catalog: %s\n",
                                    (pDomainInfo->pDCInfo->dwFlags & LSA_DS_GC_FLAG) ? "yes" : "no");
                    printf("\t\t\tDC is running KDC:    %s\n",
                                    (pDomainInfo->pDCInfo->dwFlags & LSA_DS_KDC_FLAG) ? "yes" : "no");         
                }
                
                if (pDomainInfo->pGCInfo)
                {
                    printf("\n\t\t[Global Catalog (GC) Information]\n\n");
                    printf("\t\t\tGC Name:              %s\n", IsNullOrEmptyString(pDomainInfo->pGCInfo->pszName) ? "" : pDomainInfo->pGCInfo->pszName);
                    printf("\t\t\tGC Address:           %s\n", IsNullOrEmptyString(pDomainInfo->pGCInfo->pszAddress) ? "" : pDomainInfo->pGCInfo->pszAddress);
                    printf("\t\t\tGC Site:              %s\n", IsNullOrEmptyString(pDomainInfo->pGCInfo->pszSiteName) ? "" : pDomainInfo->pGCInfo->pszSiteName);
                    printf("\t\t\tGC Flags:             0x%x\n", pDomainInfo->pGCInfo->dwFlags);
                    printf("\t\t\tGC Is PDC:            %s\n",
                                    (pDomainInfo->pGCInfo->dwFlags & LSA_DS_PDC_FLAG) ? "yes" : "no");
                    printf("\t\t\tGC is time server:    %s\n",
                                    (pDomainInfo->pGCInfo->dwFlags & LSA_DS_TIMESERV_FLAG) ? "yes" : "no");
                    printf("\t\t\tGC has writeable DS:  %s\n",
                                    (pDomainInfo->pGCInfo->dwFlags & LSA_DS_WRITABLE_FLAG) ? "yes" : "no");
                    printf("\t\t\tGC is running KDC:    %s\n",
                                    (pDomainInfo->pGCInfo->dwFlags & LSA_DS_KDC_FLAG) ? "yes" : "no");  
                }
            }
        }
    }
}

PCSTR
GetStatusString(
    LsaAuthProviderStatus status
    )
{
    PCSTR pszStatusString = NULL;
    
    switch (status)
    {
        case LSA_AUTH_PROVIDER_STATUS_ONLINE:
            
            pszStatusString = LSA_STATUS_STRING_ONLINE;
            
            break;
            
        case LSA_AUTH_PROVIDER_STATUS_OFFLINE:
            
            pszStatusString = LSA_STATUS_STRING_OFFLINE;
            
            break;
            
        default:
            
            pszStatusString = LSA_STATUS_STRING_UNKNOWN;
            
            break;
    }
    
    return pszStatusString;
}

PCSTR
GetModeString(
    LsaAuthProviderMode mode
    )
{
    PCSTR pszModeString = NULL;
    
    switch (mode)
    {    
        case LSA_PROVIDER_MODE_UNPROVISIONED:
            
            pszModeString = LSA_MODE_STRING_UNPROVISIONED;
            
            break;
            
        case LSA_PROVIDER_MODE_DEFAULT_CELL:
            
            pszModeString = LSA_MODE_STRING_DEFAULT_CELL;
            
            break;
            
        case LSA_PROVIDER_MODE_NON_DEFAULT_CELL:
            
            pszModeString = LSA_MODE_STRING_NON_DEFAULT_CELL;
            
            break;
            
        case LSA_PROVIDER_MODE_LOCAL_SYSTEM:
            
            pszModeString = LSA_MODE_STRING_LOCAL;
            
            break;
            
        default:
            
            pszModeString = LSA_MODE_STRING_UNKNOWN;
            
            break;
    }
    
    return pszModeString;
}

PCSTR
GetSubmodeString(
    LsaAuthProviderSubMode subMode
    )
{
    PCSTR pszSubmodeString = NULL;
    
    switch(subMode)
    {
        case LSA_AUTH_PROVIDER_SUBMODE_SCHEMA:
            
            pszSubmodeString = LSA_SUBMODE_STRING_SCHEMA;
            
            break;
            
        case LSA_AUTH_PROVIDER_SUBMODE_NONSCHEMA:
            
            pszSubmodeString = LSA_SUBMODE_STRING_NON_SCHEMA;
            
            break;
            
        default:
            
            pszSubmodeString = LSA_SUBMODE_STRING_UNKNOWN;
            
            break;
    }

    return pszSubmodeString;
}

PCSTR
GetTrustTypeString(
    DWORD dwTrustType
    )
{
    PCSTR pszTrustTypeString = NULL;
    
    switch (dwTrustType)
    {
        case LSA_TRUST_TYPE_DOWNLEVEL:
            pszTrustTypeString = LSA_TRUST_TYPE_STRING_DOWNLEVEL;
            break;
             
        case LSA_TRUST_TYPE_UPLEVEL:
            pszTrustTypeString = LSA_TRUST_TYPE_STRING_UPLEVEL;
            break;
            
        case LSA_TRUST_TYPE_MIT:
            pszTrustTypeString = LSA_TRUST_TYPE_STRING_MIT;
            break;
            
        case LSA_TRUST_TYPE_DCE:
            pszTrustTypeString = LSA_TRUST_TYPE_STRING_DCE;
            break;
            
        default:
            pszTrustTypeString = LSA_TRUST_TYPE_STRING_UNKNOWN;
            break;
    }
    
    return pszTrustTypeString;
}

DWORD
MapErrorCode(
    DWORD dwError
    )
{
    DWORD dwError2 = dwError;
    
    switch (dwError)
    {
        case ECONNREFUSED:
        case ENETUNREACH:
        case ETIMEDOUT:
            
            dwError2 = LSA_ERROR_LSA_SERVER_UNREACHABLE;
            
            break;
            
        default:
            
            break;
    }
    
    return dwError2;
}
