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
 *        Local Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "adprovider.h"

DWORD
UnprovisionedModeFindUserByName(
    PCSTR                pszRealDomainName,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PAD_SECURITY_OBJECT *ppUserInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    HANDLE hRealDirectory = (HANDLE)NULL;
    
    if (!IsNullOrEmptyString(pszRealDomainName))
    {
        dwError = LsaDmWrapLdapOpenDirectoryDomain(pszRealDomainName,
                                                   &hRealDirectory);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = ADFindUserByNameNonAlias(
                        (HANDLE)NULL,                        
                        hRealDirectory,
                        NULL,
                        UNPROVISIONED_MODE,
                        UnknownMode,
                        pUserNameInfo,
                        &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppUserInfo = pUserInfo;
    
cleanup:

    if (hRealDirectory != (HANDLE)NULL)
    {
        LsaLdapCloseDirectory(hRealDirectory);
    }
        
    return dwError;
        
error:
    *ppUserInfo = NULL;
        
    ADCacheDB_SafeFreeObject(&pUserInfo);
        
    goto cleanup;
}

DWORD
UnprovisionedModeFindUserByNameInOneWayTrust(
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PAD_SECURITY_OBJECT *ppUserInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    
    dwError = ADUnprovisionedMarshalToUserCacheInOneWayTrust(
                             pUserNameInfo,
                             &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppUserInfo = pUserInfo;
    
cleanup:
     
    return dwError;
    
error:
    
    *ppUserInfo = NULL;
        
    ADCacheDB_SafeFreeObject(&pUserInfo);
        
    goto cleanup;
}

DWORD
UnprovisionedModeGetUserGroupMembership(
    HANDLE  hDirectory,
    PCSTR   pszNetBIOSDomainName,
    DWORD   dwUID,    
    int     *piPrimaryGroupIndex,
    PDWORD  pdwGroupsFound,
    PAD_SECURITY_OBJECT** pppGroupInfoList
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    LDAP *pLd = NULL;
    PSTR pszDirectoryRoot = NULL;
    PAD_SECURITY_OBJECT* ppGroupInfoList = NULL;
    PSTR szAttributeListGroups[] = 
    {
        AD_LDAP_OBJECTSID_TAG,
        AD_LDAP_NAME_TAG, 
        AD_LDAP_DISPLAY_NAME_TAG,
        AD_LDAP_SAM_NAME_TAG,
        AD_LDAP_MEMBER_TAG, 
        NULL
    };
    PSTR pszQuery = NULL;
    LDAPMessage *pMessage = NULL;
    LDAPMessage *pGroupMessage = NULL;
    DWORD dwCount = 0;
    PSTR pszUserDN = NULL;
    PSTR pszEscapedUserDN = NULL;
    DWORD dwGroupsFound = 0;    
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    PLSA_SECURITY_IDENTIFIER pUserSID = NULL;
    PSTR pszPrimaryGroupSID = NULL;    
    INT iGroup = 0;
    int    iPrimaryGroupIndex = -1;
    PSTR pszUserSid = NULL;
    PSTR pszFullDomainName = NULL;
    
    
    pLd = LsaLdapGetSession(hDirectory);
    
    dwError = AD_DetermineTrustModeandDomainName(
                        pszNetBIOSDomainName,
                        NULL,
                        NULL,
                        &pszFullDomainName,
                        NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapConvertDomainToDN(pszFullDomainName, &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);
    
    /* dwError = ADGenericFindUserById(
                         hDirectory, 
                         pszCellDN,
                         DEFAULT_MODE,
                         NonSchemaMode,
                         pszNetBIOSDomainName,
                         dwUID,
                         &pUserInfo,
                         &pszUserDN);
     BAIL_ON_LSA_ERROR(dwError);*/
     
     dwError = ADLdap_FindUserSidDNById( 
                  dwUID,
                  &pszUserSid,
                  &pszUserDN);
     BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocSecurityIdentifierFromString(
        //pUserInfo->pszObjectSid,
        pszUserSid,
        &pUserSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADGetUserPrimaryGroupSID(
                hDirectory,
                pLd,
                pszDirectoryRoot,
                pszUserDN,
                pUserSID,
                &pszPrimaryGroupSID
                );
    BAIL_ON_LSA_ERROR(dwError);   

    dwError = LsaLdapEscapeString(
                &pszEscapedUserDN,
                pszUserDN);
    BAIL_ON_LSA_ERROR(dwError);
 
    //search for those real groups which have the current user as a member.
    dwError = LsaAllocateStringPrintf(
                &pszQuery,
                "(&(objectClass=group)(|(member=%s)(objectSid=%s)))", 
                pszEscapedUserDN,
                pszPrimaryGroupSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapDirectorySearch(
                    hDirectory,
                    pszDirectoryRoot,
                    LDAP_SCOPE_SUBTREE,
                    pszQuery,
                    szAttributeListGroups,
                    &pMessage
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    dwCount = ldap_count_entries(
                      pLd,
                      pMessage);    
    if (dwCount < 0) 
    {
        dwError = LSA_ERROR_LDAP_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    } 
    else if (dwCount == 0) 
    {
        goto cleanup;
    }
    dwGroupsFound = dwCount;  
    
    //Get the GID from objectSID.
    dwError = LsaAllocateMemory(
                dwGroupsFound * sizeof(PVOID),
                (PVOID*)&ppGroupInfoList
                );
    BAIL_ON_LSA_ERROR(dwError);
    
    pGroupMessage = ldap_first_entry(
                        pLd,
                        pMessage);
    iGroup = 0;
    while (pGroupMessage)
    {
        dwError = ADMarshalToGroupCache(
                           hDirectory,
                           UNPROVISIONED_MODE,
                           UnknownMode,
                           pszNetBIOSDomainName,
                           pGroupMessage,
                           NULL,
                           &ppGroupInfoList[iGroup]);
        BAIL_ON_LSA_ERROR(dwError);

        if ( strncasecmp(
                 ppGroupInfoList[iGroup]->pszObjectSid,
                 AD_BUILTIN_GROUP_SID_PREFIX,
                 sizeof(AD_BUILTIN_GROUP_SID_PREFIX)-1) )
        {
            if (!strcmp(ppGroupInfoList[iGroup]->pszObjectSid, pszPrimaryGroupSID))
            {
                iPrimaryGroupIndex = iGroup;
            }
                         
            iGroup++;                   
        }
        else
        {
            ADCacheDB_SafeFreeObject(&ppGroupInfoList[iGroup]);
        }
        
        pGroupMessage = ldap_next_entry(
                         pLd, 
                         pGroupMessage);
    }          
       
    *pppGroupInfoList = ppGroupInfoList;
    *pdwGroupsFound = iGroup;      
    *piPrimaryGroupIndex = iPrimaryGroupIndex;
        
cleanup:

    if (pMessage) 
    {
        ldap_msgfree(pMessage);
    }

    ADCacheDB_SafeFreeObject(&pUserInfo);

    if (pUserSID)
    {
        LsaFreeSecurityIdentifier(pUserSID);
    }

    LSA_SAFE_FREE_STRING(pszPrimaryGroupSID);
    LSA_SAFE_FREE_STRING(pszUserSid);
    LSA_SAFE_FREE_MEMORY(pszUserDN);
    LSA_SAFE_FREE_MEMORY(pszEscapedUserDN);
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    LSA_SAFE_FREE_STRING(pszQuery);
    LSA_SAFE_FREE_STRING(pszFullDomainName);
    
    return dwError;
    
error:

    ADCacheDB_SafeFreeObjectList(dwGroupsFound, &ppGroupInfoList);

    *pdwGroupsFound = 0;
    *piPrimaryGroupIndex = -1;

    goto cleanup;
}

DWORD
UnprovisionedModeEnumUsers(
    HANDLE  hDirectory,
    PCSTR   pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD   dwMaxNumUsers,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD  dwError = 0;
    DWORD  dwCount = 0;    
    PVOID* ppUserInfoList_accumulate = NULL;
    PVOID* ppUserInfoList = NULL;    
    DWORD  dwTotalNumUsersFound = 0;
    DWORD  dwNumUsersFound = 0;
    DWORD  dwUserInfoLevel = 0;
    PSTR   pszDirectoryRoot = NULL;
    PCSTR  pszQuery = "(&(objectClass=user)(!(objectClass=computer))(sAMAccountName=*))";
    PSTR szAttributeList[] = 
        {
             AD_LDAP_OBJECTSID_TAG,
             AD_LDAP_NAME_TAG, 
             AD_LDAP_DISPLAY_NAME_TAG,
             AD_LDAP_SAM_NAME_TAG,
             AD_LDAP_UPN_TAG,
             AD_LDAP_USER_CTRL_TAG,
             AD_LDAP_PWD_LASTSET_TAG,
             AD_LDAP_ACCOUT_EXP_TAG,
             NULL
        }; 

    LDAPMessage *pMessage = NULL;
    LDAP *pLd = LsaLdapGetSession(hDirectory);
    
    DWORD dwNumUsersWanted = dwMaxNumUsers;
    
    if (!pEnumState->bMorePages){
           dwError = LSA_ERROR_NO_MORE_USERS;
           BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwUserInfoLevel = pEnumState->dwInfoLevel;
    
    dwError = LsaLdapConvertDomainToDN(
                    gpADProviderData->szDomain,
                    &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);
    
    do
    {       
        dwError = LsaLdapDirectoryOnePagedSearch(
                       hDirectory,
                       pszDirectoryRoot,
                       pszQuery,
                       szAttributeList,
                       dwNumUsersWanted,                       
                       &pEnumState->pCookie,        
                       LDAP_SCOPE_SUBTREE,
                       &pMessage,
                       &pEnumState->bMorePages);    
        BAIL_ON_LSA_ERROR(dwError);
    
        dwCount = ldap_count_entries(
                          pLd,
                          pMessage);    
        if (dwCount < 0) {
           dwError = LSA_ERROR_LDAP_ERROR;
        } else if (dwCount == 0) {
           dwError = LSA_ERROR_NO_MORE_USERS;
        } 
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = ADUnprovisionedMarshalUserInfoList(
                        hDirectory,
                        pszNetBIOSDomainName,
                        pMessage,
                        pEnumState->dwInfoLevel,
                        &ppUserInfoList,
                        &dwNumUsersFound);
        BAIL_ON_LSA_ERROR(dwError);
       
        dwNumUsersWanted -= dwNumUsersFound;
        
        dwError = LsaCoalesceUserInfoList(
                        &ppUserInfoList,
                        &dwNumUsersFound,
                        &ppUserInfoList_accumulate,
                        &dwTotalNumUsersFound);
        BAIL_ON_LSA_ERROR(dwError);
           
        if (pMessage) {
            ldap_msgfree(pMessage);
            pMessage = NULL;            
        }
    } while (pEnumState->bMorePages && dwNumUsersWanted);     
    
    *pppUserInfoList = ppUserInfoList_accumulate;
    *pdwNumUsersFound = dwTotalNumUsersFound;
    
cleanup:

    if (pMessage) {
        ldap_msgfree(pMessage);
    }

    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    
    return dwError;
    
error:

    *pppUserInfoList = NULL;
    *pdwNumUsersFound = 0;
    
    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsersFound);
    }
    
    if (ppUserInfoList_accumulate) {
        LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList_accumulate, dwTotalNumUsersFound);
    }  

    goto cleanup;
}

DWORD
UnprovisionedModeFindGroupByName(
    PCSTR                pszRealDomainName,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PAD_SECURITY_OBJECT *ppGroupInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PAD_SECURITY_OBJECT pGroupInfo = NULL;
    HANDLE hRealDirectory = (HANDLE)NULL;
    
    if (!IsNullOrEmptyString(pszRealDomainName))
    {
        dwError = LsaDmWrapLdapOpenDirectoryDomain(pszRealDomainName,
                                                   &hRealDirectory);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = ADFindGroupByNameNT4(
                        (HANDLE)NULL,                        
                        hRealDirectory,
                        NULL,
                        UNPROVISIONED_MODE,
                        UnknownMode,
                        pGroupNameInfo,
                        &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppGroupInfo = pGroupInfo;
    
cleanup:

    if (hRealDirectory != (HANDLE)NULL)
    {
        LsaLdapCloseDirectory(hRealDirectory);
    }
        
    return dwError;
        
error:
    *ppGroupInfo = NULL;
        
    ADCacheDB_SafeFreeObject(&pGroupInfo);
        
    goto cleanup;
}

DWORD
UnprovisionedModeFindGroupByNameInOneWayTrust(
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PAD_SECURITY_OBJECT *ppGroupInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PAD_SECURITY_OBJECT pGroupInfo = NULL;
    
    dwError = ADUnprovisionedMarshalToGroupCacheInOneWayTrust(
                             pGroupNameInfo,
                             &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppGroupInfo = pGroupInfo;
    
cleanup:
     
    return dwError;
    
error:
    
    *ppGroupInfo = NULL;
        
    ADCacheDB_SafeFreeObject(&pGroupInfo);
        
    goto cleanup;
}

DWORD
UnprovisionedModeEnumGroups(
    HANDLE  hDirectory,
    PCSTR   pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD   dwMaxNumGroups,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD  dwError = 0;
    DWORD  dwCount = 0;
    PCSTR  pszQuery = "(&(objectClass=group)(sAMAccountName=*))";    
    PVOID* ppGroupInfoList_accumulate = NULL;
    PVOID* ppGroupInfoList = NULL;    
    DWORD  dwTotalNumGroupsFound = 0;
    DWORD  dwNumGroupsFound = 0;
    DWORD  dwGroupInfoLevel = 0;    
    PSTR szAttributeList[] =
        {
             AD_LDAP_OBJECTSID_TAG,
             AD_LDAP_NAME_TAG, 
             AD_LDAP_DISPLAY_NAME_TAG,
             AD_LDAP_SAM_NAME_TAG,
             AD_LDAP_MEMBER_TAG, 
             NULL
        };
    LDAPMessage *pMessage = NULL;
    LDAP *pLd = LsaLdapGetSession(hDirectory);
    PSTR pszDirectoryRoot = NULL;
    
    DWORD dwNumGroupsWanted = dwMaxNumGroups;
    
    if (!pEnumState->bMorePages){
        dwError = LSA_ERROR_NO_MORE_GROUPS;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwGroupInfoLevel = pEnumState->dwInfoLevel;
    
    dwError = LsaLdapConvertDomainToDN(
                    gpADProviderData->szDomain,
                    &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);
    
    do
    {       
        dwError = LsaLdapDirectoryOnePagedSearch(
                       hDirectory,
                       pszDirectoryRoot,
                       pszQuery,
                       szAttributeList,
                       dwNumGroupsWanted,                       
                       &pEnumState->pCookie,        
                       LDAP_SCOPE_SUBTREE,
                       &pMessage,
                       &pEnumState->bMorePages);    
        BAIL_ON_LSA_ERROR(dwError);
    
        dwCount = ldap_count_entries(
                          pLd,
                          pMessage);    
        if (dwCount < 0) {
           dwError = LSA_ERROR_LDAP_ERROR;
        } else if (dwCount == 0) {
           dwError = LSA_ERROR_NO_MORE_GROUPS;
        } 
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = ADUnprovisionedMarshalGroupInfoList(
                        hDirectory,
                        pszNetBIOSDomainName,            
                        pMessage,
                        pEnumState->dwInfoLevel,
                        &ppGroupInfoList,
                        &dwNumGroupsFound);
        BAIL_ON_LSA_ERROR(dwError);
       
        dwNumGroupsWanted -= dwNumGroupsFound;
        
        dwError = LsaCoalesceGroupInfoList(
                        &ppGroupInfoList,
                        &dwNumGroupsFound,
                        &ppGroupInfoList_accumulate,
                        &dwTotalNumGroupsFound);
        BAIL_ON_LSA_ERROR(dwError);
           
        if (pMessage) {
               ldap_msgfree(pMessage);
               pMessage = NULL;            
        }
    } while (pEnumState->bMorePages && dwNumGroupsWanted);      
    
    *pppGroupInfoList = ppGroupInfoList_accumulate;
    *pdwNumGroupsFound = dwTotalNumGroupsFound;
        
cleanup:

    if (pMessage) {
       ldap_msgfree(pMessage);         
    }

    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    
    return dwError;
    
error:

    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = 0;
    
    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }
    
    if (ppGroupInfoList_accumulate) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList_accumulate, dwTotalNumGroupsFound);
    }  

    goto cleanup;
}
