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
 *        AD LDAP Enumerate groups or users
 *
 * Authors: Wei Fu (wfu@likewisesoftware.com)
 */

#include "adprovider.h"

DWORD
ADMarshalObjectSidListFromPseudo(
    IN HANDLE hDirectory,
    IN LDAPMessage* pMessagePseudo,
    OUT PSTR** pppSidsList,
    OUT PDWORD pdwNumObjectsFound
    )
{
    DWORD dwError = 0;    
    INT iMessageCount = 0;
    // Do not free pMessage
    LDAPMessage* pMessage = NULL;   
    PSTR* ppSidsList = NULL;
    DWORD dwNumObjectsFound = 0;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    LDAP* pLd = LsaLdapGetSession(hDirectory);
    
    iMessageCount = ldap_count_entries(
                    pLd,
                    pMessagePseudo);
    if (iMessageCount < 0) 
    {
        dwError = LSA_ERROR_LDAP_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (iMessageCount == 0)
    {
        goto cleanup; 
    }

    dwError = LsaAllocateMemory(
                    sizeof(*ppSidsList) * (DWORD)iMessageCount,
                    (PVOID*)&ppSidsList);
    BAIL_ON_LSA_ERROR(dwError);
    
    pMessage = ldap_first_entry(pLd, pMessagePseudo);    
    while (pMessage)
    {               
        DWORD iValue = 0;

        LsaFreeStringArray(ppszValues,dwNumValues);
        dwError = LsaLdapGetStrings(
                       hDirectory,
                       pMessage,
                       AD_LDAP_KEYWORDS_TAG,
                       &ppszValues,
                       &dwNumValues);
        BAIL_ON_LSA_ERROR(dwError);

        for (iValue = 0; iValue < dwNumValues; iValue++)
        {
            if (!IsNullOrEmptyString(ppszValues[iValue]) && 
                !strncasecmp(ppszValues[iValue], "backLink=", sizeof("backLink=")-1))
            {
                dwError = LsaAllocateString((ppszValues[iValue] + sizeof("backLink=")-1),
                                            &ppSidsList[dwNumObjectsFound++]);
                BAIL_ON_LSA_ERROR(dwError);
                
                break;
            }
        }

        pMessage = ldap_next_entry(pLd, pMessage);
    }
   
   *pppSidsList = ppSidsList;
   *pdwNumObjectsFound = dwNumObjectsFound;
  
cleanup:
    LsaFreeStringArray(ppszValues, dwNumValues);

    return dwError;

error:
    *pppSidsList = NULL;
    *pdwNumObjectsFound = 0;
    LsaFreeStringArray(ppSidsList, (DWORD)iMessageCount);    

    goto cleanup;    
}

DWORD
DefaultModeSchemaOrUnprovisionEnumObjects(
    // Enumerate either Groups or Users
    IN LSA_AD_ENUM_TYPE EnumType,
    // AdMode can be DEFAULT_MODE (schema only) or UNPROVISIONED_MODE
    IN DWORD AdMode,
    IN LSA_AD_MARSHAL_OBJECTS_INFOLIST_CALLBACK pMarshalObjectsInfoListCallback,
    IN LSA_FREE_OBJECTS_INFOLIST_CALLBACK pFreeObjectsInfoListCallback,
    IN PCSTR pszDomainDnsName,
    IN PAD_ENUM_STATE pEnumState,
    IN DWORD dwMaxNumObjects,
    OUT PDWORD pdwNumObjectsFound,
    OUT PVOID** pppObjectsInfoList
    )
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    PCSTR pszQuery = NULL;
    PVOID* ppObjectsInfoList = NULL;
    PVOID* ppObjectsInfoList_accumulate = NULL;
    // Do not free "ppObjectInfoListTemp_accumulate"
    PVOID* ppObjectsInfoListTemp_accumulate = NULL;
    DWORD dwNumObjectsFound = 0;
    DWORD dwTotalNumObjectsFound = 0;
    DWORD dwObjectInfoLevel = 0;
    PSTR* ppszAttributeList = NULL;
    LDAPMessage *pMessagePseudo = NULL;
    PSTR pszDirectoryRoot = NULL;
    LDAP* pLd = LsaLdapGetSession(pEnumState->hDirectory);
    DWORD dwNumObjectsWanted = dwMaxNumObjects;

    if (LSA_AD_ENUM_TYPE_GROUP != EnumType && LSA_AD_ENUM_TYPE_USER != EnumType)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (!pEnumState->bMorePages)
    {
        if (LSA_AD_ENUM_TYPE_GROUP == EnumType)
        {
            dwError = LSA_ERROR_NO_MORE_GROUPS;            
        }
        else if (LSA_AD_ENUM_TYPE_USER == EnumType)
        {
            dwError = LSA_ERROR_NO_MORE_USERS;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    switch (AdMode)
    {
        case DEFAULT_MODE:
            if (LSA_AD_ENUM_TYPE_GROUP == EnumType)
            {
                pszQuery = "(&(objectClass=Group)(!(objectClass=Computer))(sAMAccountName=*)(gidNumber=*))";
            }
            else if (LSA_AD_ENUM_TYPE_USER == EnumType)
            {
                pszQuery = "(&(objectClass=User)(!(objectClass=computer))(sAMAccountName=*)(uidNumber=*))";
            }
            
            dwError = ADGetUserOrGroupRealAttributeList(
                              DEFAULT_MODE,
                              SchemaMode,
                              &ppszAttributeList);
            BAIL_ON_LSA_ERROR(dwError);            
            break;
            
        case UNPROVISIONED_MODE:
            if (LSA_AD_ENUM_TYPE_GROUP == EnumType)
            {
                pszQuery = "(&(objectClass=Group)(!(objectClass=Computer))(sAMAccountName=*))";
                
            }
            else if (LSA_AD_ENUM_TYPE_USER == EnumType)
            {
                pszQuery = "(&(objectClass=User)(!(objectClass=Computer))(sAMAccountName=*))";
            }
            
            dwError = ADGetUserOrGroupRealAttributeList(
                              UNPROVISIONED_MODE,
                              UnknownMode,
                              &ppszAttributeList);
            BAIL_ON_LSA_ERROR(dwError);
            break;
            
        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);        
    }

    dwError = LsaLdapConvertDomainToDN(pszDomainDnsName, &pszDirectoryRoot);
    dwObjectInfoLevel = pEnumState->dwInfoLevel;

    do
    {
        dwError = LsaLdapDirectoryOnePagedSearch(
                           pEnumState->hDirectory,
                           pszDirectoryRoot,
                           pszQuery,
                           ppszAttributeList,
                           dwMaxNumObjects,
                           &pEnumState->pCookie,
                           LDAP_SCOPE_SUBTREE,
                           &pMessagePseudo,
                           &pEnumState->bMorePages);
        BAIL_ON_LSA_ERROR(dwError);

        dwCount = ldap_count_entries(pLd, pMessagePseudo);
        if ((INT)dwCount < 0) {
           dwError = LSA_ERROR_LDAP_ERROR;
        } else if (dwCount == 0) {
            if (LSA_AD_ENUM_TYPE_GROUP == EnumType)
            {
                dwError = LSA_ERROR_NO_MORE_GROUPS;            
            }
            else if (LSA_AD_ENUM_TYPE_USER == EnumType)
            {
                dwError = LSA_ERROR_NO_MORE_USERS;
            }
        } else if (dwCount > dwNumObjectsWanted) {
           dwError = LSA_ERROR_LDAP_ERROR;
        }
        BAIL_ON_LSA_ERROR(dwError);
        
        if (DEFAULT_MODE != AdMode && UNPROVISIONED_MODE != AdMode)
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        dwError = pMarshalObjectsInfoListCallback(
                    (HANDLE)NULL,
                    pEnumState->hDirectory,
                    pszDomainDnsName,
                    (DEFAULT_MODE == AdMode) ?  LSA_AD_MARSHAL_MODE_DEFAULT_SCHEMA : LSA_AD_MARSHAL_MODE_UNPROVISIONED,                    
                    pMessagePseudo,
                    pEnumState->dwInfoLevel,
                    &ppObjectsInfoList,
                    &dwNumObjectsFound);
        BAIL_ON_LSA_ERROR(dwError);

        dwNumObjectsWanted -= dwNumObjectsFound;
        
        dwError = LsaReallocMemory(
                    ppObjectsInfoList_accumulate,
                    (PVOID*)&ppObjectsInfoListTemp_accumulate,
                    (dwTotalNumObjectsFound+dwNumObjectsFound)*sizeof(*ppObjectsInfoList_accumulate));
        BAIL_ON_LSA_ERROR(dwError);        
        ppObjectsInfoList_accumulate = ppObjectsInfoListTemp_accumulate;
        // Append the ppObjectsInfoList to ppObjectsInfoList_accumulate list
        memcpy(ppObjectsInfoList_accumulate + dwTotalNumObjectsFound,
               ppObjectsInfoList,
               sizeof(*ppObjectsInfoList) * dwNumObjectsFound);
        memset(ppObjectsInfoList,
               0,
               sizeof(*ppObjectsInfoList) * dwNumObjectsFound);

        dwTotalNumObjectsFound += dwNumObjectsFound;

        if (ppObjectsInfoList)
        {
            pFreeObjectsInfoListCallback(dwObjectInfoLevel, ppObjectsInfoList, dwNumObjectsFound);
            ppObjectsInfoList = NULL;
        }
        if (pMessagePseudo)
        {
            ldap_msgfree(pMessagePseudo);
            pMessagePseudo = NULL;
        }
    } while (pEnumState->bMorePages && dwNumObjectsWanted);

    *pppObjectsInfoList = ppObjectsInfoList_accumulate;
    *pdwNumObjectsFound = dwTotalNumObjectsFound;    
    
cleanup:
    if (pMessagePseudo)
    {
        ldap_msgfree(pMessagePseudo);
    }    
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);

    if (ppObjectsInfoList)
    {
        pFreeObjectsInfoListCallback(dwObjectInfoLevel, ppObjectsInfoList, dwNumObjectsFound);
    }
    LsaFreeNullTerminatedStringArray(ppszAttributeList);

    return dwError;

error:
    *pppObjectsInfoList = NULL;
    *pdwNumObjectsFound = 0;

    if (ppObjectsInfoList_accumulate) 
    {
        pFreeObjectsInfoListCallback(dwObjectInfoLevel, ppObjectsInfoList_accumulate, dwTotalNumObjectsFound);
    }

    goto cleanup;
}

DWORD
CellModeOrDefaultNonSchemaEnumObjects(
    IN HANDLE hProvider,    
    IN ADConfigurationMode adConfMode,
    // Enumerate either Groups or Users
    IN LSA_AD_ENUM_TYPE EnumType,    
    IN LSA_AD_MARSHAL_OBJECTS_INFOLIST_CALLBACK pMarshalObjectsInfoListCallback,
    IN LSA_FREE_OBJECTS_INFOLIST_CALLBACK pFreeObjectsInfoListCallback,
    IN PCSTR pszCellDN,    
    IN PAD_ENUM_STATE pEnumState,
    IN DWORD dwMaxNumObjects,
    OUT PDWORD pdwNumObjectsFound,
    OUT PVOID** pppObjectInfoList
    )
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    PCSTR szQuery = NULL;
    PSTR pszSearchScope = NULL;
    PVOID* ppObjectInfoList_accumulate = NULL;
    PVOID* ppObjectInfoList = NULL;
    PVOID* ppObjectInfoListTemp_accumulate = NULL;
    DWORD dwTotalNumObjectsFound = 0;
    DWORD dwNumObjectsFound = 0;
    DWORD dwObjectInfoLevel = 0;
    PSTR szAttributeList[] =
    {
        AD_LDAP_KEYWORDS_TAG,
        NULL
    };
    LDAPMessage* pMessagePseudo = NULL;
    LDAP* pLd = LsaLdapGetSession(pEnumState->hDirectory);
    DWORD dwNumObjectsWanted = dwMaxNumObjects;

    if (LSA_AD_ENUM_TYPE_GROUP != EnumType && LSA_AD_ENUM_TYPE_USER != EnumType)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (!pEnumState->bMorePages)
    {
        if (LSA_AD_ENUM_TYPE_GROUP == EnumType)
        {
            dwError = LSA_ERROR_NO_MORE_GROUPS;            
        }
        else if (LSA_AD_ENUM_TYPE_USER == EnumType)
        {
            dwError = LSA_ERROR_NO_MORE_USERS;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwObjectInfoLevel = pEnumState->dwInfoLevel;
    
    if (LSA_AD_ENUM_TYPE_GROUP == EnumType)
    {
        dwError = LsaAllocateStringPrintf(
                        &pszSearchScope,
                        "CN=Groups,%s",
                        pszCellDN);
        BAIL_ON_LSA_ERROR(dwError);        
    }
    else if (LSA_AD_ENUM_TYPE_USER == EnumType)
    {
        dwError = LsaAllocateStringPrintf(
                        &pszSearchScope,
                        "CN=Users,%s",
                        pszCellDN);
        BAIL_ON_LSA_ERROR(dwError);
    }    

    switch (adConfMode)
    {
       case SchemaMode:
           if (LSA_AD_ENUM_TYPE_GROUP == EnumType)
           {
               szQuery = "(&(objectClass=posixGroup)(keywords=objectClass=centerisLikewiseGroup)(gidNumber=*))";
           }
           else if (LSA_AD_ENUM_TYPE_USER == EnumType)
           {
               szQuery = "(&(objectClass=posixAccount)(keywords=objectClass=centerisLikewiseUser)(uidNumber=*))";
           }
           break;

       case NonSchemaMode:
           if (LSA_AD_ENUM_TYPE_GROUP == EnumType)
           {
               szQuery = "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseGroup)(keywords=gidNumber=*))";               
           }
           else if (LSA_AD_ENUM_TYPE_USER == EnumType)
           {
               szQuery = "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseUser)(keywords=uidNumber=*))";               
           }
           break;

       case UnknownMode:
           dwError = LSA_ERROR_NOT_SUPPORTED;
           BAIL_ON_LSA_ERROR(dwError);
    }

    do
    {
        dwError = LsaLdapDirectoryOnePagedSearch(
                       pEnumState->hDirectory,
                       pszSearchScope,
                       szQuery,
                       szAttributeList,
                       dwNumObjectsWanted,
                       &pEnumState->pCookie,
                       LDAP_SCOPE_ONELEVEL,
                       &pMessagePseudo,
                       &pEnumState->bMorePages);
        BAIL_ON_LSA_ERROR(dwError);

        dwCount = ldap_count_entries(
                          pLd,
                          pMessagePseudo);
        if ((INT)dwCount < 0) {
           dwError = LSA_ERROR_LDAP_ERROR;
        } else if (dwCount == 0) {
            if (LSA_AD_ENUM_TYPE_GROUP == EnumType)
            {
                dwError = LSA_ERROR_NO_MORE_GROUPS;            
            }
            else if (LSA_AD_ENUM_TYPE_USER == EnumType)
            {
                dwError = LSA_ERROR_NO_MORE_USERS;
            }
        } else if (dwCount > dwNumObjectsWanted) {
           dwError = LSA_ERROR_LDAP_ERROR;
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pMarshalObjectsInfoListCallback(
                        hProvider,
                        pEnumState->hDirectory,
                        NULL,
                        LSA_AD_MARSHAL_MODE_OTHER,
                        pMessagePseudo,
                        dwObjectInfoLevel,
                        &ppObjectInfoList,
                        &dwNumObjectsFound);
        BAIL_ON_LSA_ERROR(dwError);

        dwNumObjectsWanted -= dwNumObjectsFound;

        dwError = LsaReallocMemory(
                    ppObjectInfoList_accumulate,
                    (PVOID*)&ppObjectInfoListTemp_accumulate,
                    (dwTotalNumObjectsFound+dwNumObjectsFound)*sizeof(*ppObjectInfoList_accumulate));
        BAIL_ON_LSA_ERROR(dwError);
        // Do not free "ppObjectInfoListTemp_accumulate"
        ppObjectInfoList_accumulate = ppObjectInfoListTemp_accumulate;
        // Append the ppObjectInfoList to ppObjectInfoList_accumulate list
        memcpy(ppObjectInfoList_accumulate + dwTotalNumObjectsFound,
               ppObjectInfoList,
               sizeof(*ppObjectInfoList) * dwNumObjectsFound);       

        memset(ppObjectInfoList,
               0,
               sizeof(*ppObjectInfoList) * dwNumObjectsFound);
        dwTotalNumObjectsFound += dwNumObjectsFound;

        if (ppObjectInfoList)
        {
            pFreeObjectsInfoListCallback(dwObjectInfoLevel, ppObjectInfoList, dwNumObjectsFound);
            ppObjectInfoList = NULL;
        }

        if (pMessagePseudo) 
        {
               ldap_msgfree(pMessagePseudo);
               pMessagePseudo = NULL;
        }
    } while (pEnumState->bMorePages && dwNumObjectsWanted);

    *pppObjectInfoList = ppObjectInfoList_accumulate;
    *pdwNumObjectsFound = dwTotalNumObjectsFound;

cleanup:
    LSA_SAFE_FREE_STRING(pszSearchScope);
    if (pMessagePseudo) 
    {
        ldap_msgfree(pMessagePseudo);
    }
    if (ppObjectInfoList) 
    {
        pFreeObjectsInfoListCallback(dwObjectInfoLevel, ppObjectInfoList, dwNumObjectsFound);
    }

    return dwError;

error:
    *pppObjectInfoList = NULL;
    *pdwNumObjectsFound = 0;
    if (ppObjectInfoList_accumulate) {
        pFreeObjectsInfoListCallback(dwObjectInfoLevel, ppObjectInfoList_accumulate, dwTotalNumObjectsFound);
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
