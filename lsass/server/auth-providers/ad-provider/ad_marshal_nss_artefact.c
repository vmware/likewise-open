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
 *        provider-main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        AD LDAP Group Marshalling
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "adprovider.h"

DWORD
ADMarshalNSSArtefactInfo(
    HANDLE              hDirectory,
    DWORD               dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PCSTR               pszDomainName,
    LDAPMessage*        pMessageReal,
    LDAPMessage*        pMessagePseudo,
    DWORD               dwNSSArtefactInfoLevel,
    PVOID*              ppNSSArtefactInfo
    )
{
    DWORD dwError = 0;
    PVOID pNSSArtefactInfo = NULL;

    switch (dwDirectoryMode){
    
        case UNPROVISIONED_MODE:
            dwError = LSA_ERROR_NOT_SUPPORTED;
        
            break;
        
        case DEFAULT_MODE:
        case CELL_MODE:
            
            if (!pMessagePseudo){
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError); 
            }
             
             switch (adConfMode) 
             {
                 case SchemaMode:

                     dwError = ADSchemaMarshalNSSArtefactInfo(
                                   hDirectory,
                                   pszDomainName,
                                   pMessageReal,
                                   pMessagePseudo,
                                   dwNSSArtefactInfoLevel,
                                   &pNSSArtefactInfo);
                     BAIL_ON_LSA_ERROR(dwError);

                     break;

                 case NonSchemaMode:

                     dwError = ADNonSchemaMarshalNSSArtefactInfo(
                                   hDirectory,
                                   pszDomainName,
                                   pMessageReal,
                                   pMessagePseudo,
                                   dwNSSArtefactInfoLevel,
                                   &pNSSArtefactInfo);
                     BAIL_ON_LSA_ERROR(dwError);

                     break; 
          
                 default:           
                     dwError = LSA_ERROR_INVALID_PARAMETER;
                     BAIL_ON_LSA_ERROR(dwError); 
             }
             
             break;
        
         default:           
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppNSSArtefactInfo = pNSSArtefactInfo;

cleanup:

    return dwError;
    
error:

    *ppNSSArtefactInfo = NULL;
    
    if (pNSSArtefactInfo) {
        LsaFreeNSSArtefactInfo(dwNSSArtefactInfoLevel, pNSSArtefactInfo);
    }        

    goto cleanup;
}

DWORD
ADSchemaMarshalNSSArtefactInfo(
    HANDLE      hDirectory,
    PCSTR       pszDomainName,
    LDAPMessage *pMessageReal,
    LDAPMessage *pMessagePseudo,
    DWORD       dwNSSArtefactInfoLevel,
    PVOID*      ppNSSArtefactInfo
    )
{
    DWORD dwError = 0;
    PVOID pNSSArtefactInfo = NULL;
    
    switch(dwNSSArtefactInfoLevel)
    {
        case 0:
            dwError = ADSchemaMarshalNSSArtefactInfo_0(
                            hDirectory,
                            pszDomainName,
                            pMessageReal,
                            pMessagePseudo,
                            &pNSSArtefactInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case 1:
            dwError = ADSchemaMarshalNSSArtefactInfo_1(
                            hDirectory,
                            pszDomainName,
                            pMessageReal,
                            pMessagePseudo,
                            &pNSSArtefactInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }
    
    *ppNSSArtefactInfo = pNSSArtefactInfo;
    
cleanup:

    return dwError;
    
error:

    *ppNSSArtefactInfo = NULL;
    
    if (pNSSArtefactInfo) {
        LsaFreeNSSArtefactInfo(dwNSSArtefactInfoLevel, pNSSArtefactInfo);
    }

    goto cleanup;
}

DWORD
ADSchemaMarshalNSSArtefactInfo_0(
    HANDLE       hDirectory,
    PCSTR        pszDomainName,
    LDAPMessage* pMessageReal,
    LDAPMessage* pMessagePseudo,
    PVOID*       ppNSSArtefactInfo
    )
{
    return LSA_ERROR_NOT_IMPLEMENTED;
}

DWORD
ADSchemaMarshalNSSArtefactInfo_1(
    HANDLE       hDirectory,
    PCSTR        pszDomainName,
    LDAPMessage* pMessageReal,
    LDAPMessage* pMessagePseudo,
    PVOID*       ppNSSArtefactInfo
    )
{
    return LSA_ERROR_NOT_IMPLEMENTED;
}

DWORD
ADNonSchemaMarshalNSSArtefactInfo(
    HANDLE      hDirectory,
    PCSTR       pszDomainName,
    LDAPMessage *pMessageReal,
    LDAPMessage *pMessagePseudo,
    DWORD       dwNSSArtefactInfoLevel,
    PVOID*      ppNSSArtefactInfo
    )
{
    DWORD dwError = 0;
    PVOID pNSSArtefactInfo = NULL;
    
    switch(dwNSSArtefactInfoLevel)
    {
        case 0:
            dwError = ADNonSchemaMarshalNSSArtefactInfo_0(
                            hDirectory,
                            pszDomainName,
                            pMessageReal,
                            pMessagePseudo,
                            &pNSSArtefactInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case 1:
            dwError = ADNonSchemaMarshalNSSArtefactInfo_1(
                            hDirectory,
                            pszDomainName,
                            pMessageReal,
                            pMessagePseudo,
                            &pNSSArtefactInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }
    
    *ppNSSArtefactInfo = pNSSArtefactInfo;
    
cleanup:

    return dwError;
    
error:

    *ppNSSArtefactInfo = NULL;
    
    if (pNSSArtefactInfo) {
        LsaFreeNSSArtefactInfo(dwNSSArtefactInfoLevel, pNSSArtefactInfo);
    }

    goto cleanup;
}

DWORD
ADNonSchemaMarshalNSSArtefactInfo_0(
    HANDLE       hDirectory,
    PCSTR        pszDomainName,
    LDAPMessage* pMessageReal,
    LDAPMessage* pMessagePseudo,
    PVOID*       ppNSSArtefactInfo
    )
{
    return LSA_ERROR_NOT_IMPLEMENTED;
}

DWORD
ADNonSchemaMarshalNSSArtefactInfo_1(
    HANDLE       hDirectory,
    PCSTR        pszDomainName,
    LDAPMessage* pMessageReal,
    LDAPMessage* pMessagePseudo,
    PVOID*       ppNSSArtefactInfo
    )
{
    return LSA_ERROR_NOT_IMPLEMENTED;
}

DWORD
ADSchemaMarshalNSSArtefactInfoList(
    HANDLE      hDirectory,
    PCSTR       pszDomainName,    
    LDAPMessage *pMessagePseudo,
    DWORD       dwNSSArtefactInfoLevel,
    PVOID**     pppNSSArtefactInfoList,
    PDWORD      pNumNSSArtefacts
    )
{
    DWORD dwError = 0;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD NumNSSArtefacts = 0;
    
    switch(dwNSSArtefactInfoLevel)
    {
        case 0:
            dwError = ADSchemaMarshalNSSArtefactInfoList_0(
                            hDirectory,
                            pszDomainName,
                            pMessagePseudo,
                            &ppNSSArtefactInfoList,
                            &NumNSSArtefacts);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case 1:
            dwError = ADSchemaMarshalNSSArtefactInfoList_1(
                            hDirectory,
                            pszDomainName,
                            pMessagePseudo,
                            &ppNSSArtefactInfoList,
                            &NumNSSArtefacts);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }
    
    *pppNSSArtefactInfoList = ppNSSArtefactInfoList;    
    *pNumNSSArtefacts = NumNSSArtefacts;
    
cleanup:

    return dwError;
    
error:

    *pppNSSArtefactInfoList = NULL; 
    *pNumNSSArtefacts = 0;
    
    if (ppNSSArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, (PVOID*)ppNSSArtefactInfoList, NumNSSArtefacts);
    }
    goto cleanup;
}

DWORD
ADSchemaMarshalNSSArtefactInfoList_0(
    HANDLE      hDirectory,
    PCSTR       pszDomainName,    
    LDAPMessage *pMessagePseudo,
    PVOID**     pppNSSArtefactInfoList,
    PDWORD      pwdNumNSSArtefacts
    )    
{
    return LSA_ERROR_NOT_IMPLEMENTED;
}

DWORD
ADSchemaMarshalNSSArtefactInfoList_1(
    HANDLE      hDirectory,
    PCSTR       pszDomainName,    
    LDAPMessage *pMessagePseudo,
    PVOID**     pppNSSArtefactInfoList,
    PDWORD      pwdNumNSSArtefacts
    )    
{
    return LSA_ERROR_NOT_IMPLEMENTED;
}

DWORD
ADNonSchemaMarshalNSSArtefactInfoList(
    HANDLE      hDirectory,
    PCSTR       pszDomainName,    
    LDAPMessage *pMessagePseudo,
    DWORD       dwNSSArtefactInfoLevel,
    PVOID**     pppNSSArtefactInfoList,
    PDWORD      pNumArtefacts
    )
{
    DWORD dwError = 0;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD dwNumArtefacts = 0;
    
    switch(dwNSSArtefactInfoLevel)
    {
        case 0:
            dwError = ADNonSchemaMarshalNSSArtefactInfoList_0(
                            hDirectory,
                            pszDomainName,
                            pMessagePseudo,
                            &ppNSSArtefactInfoList,
                            &dwNumArtefacts);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case 1:
            dwError = ADNonSchemaMarshalNSSArtefactInfoList_1(
                            hDirectory,
                            pszDomainName,
                            pMessagePseudo,
                            &ppNSSArtefactInfoList,
                            &dwNumArtefacts);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }
    
    *pppNSSArtefactInfoList = ppNSSArtefactInfoList;    
    *pNumArtefacts = dwNumArtefacts;
    
cleanup:

    return dwError;
    
error:

    *pppNSSArtefactInfoList = NULL; 
    *pNumArtefacts = 0;
    
    if (ppNSSArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, (PVOID*)ppNSSArtefactInfoList, dwNumArtefacts);
    }
    goto cleanup;
}

DWORD
ADNonSchemaMarshalNSSArtefactInfoList_0(
    HANDLE      hDirectory,
    PCSTR       pszDomainName,    
    LDAPMessage *pMessagePseudo,
    PVOID**     pppNSSArtefactInfoList,
    PDWORD      pwdNumNSSArtefacts
    )    
{
    return LSA_ERROR_NOT_IMPLEMENTED;
}

DWORD
ADNonSchemaMarshalNSSArtefactInfoList_1(
    HANDLE      hDirectory,
    PCSTR       pszDomainName,    
    LDAPMessage *pMessagePseudo,
    PVOID**     pppNSSArtefactInfoList,
    PDWORD      pwdNumNSSArtefacts
    )    
{
    return LSA_ERROR_NOT_IMPLEMENTED;
}
