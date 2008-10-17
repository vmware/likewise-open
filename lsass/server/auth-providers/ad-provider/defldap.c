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
DefaultModeFindUserByName(    
    PCSTR pszDomainName,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PAD_SECURITY_OBJECT *ppUserInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PSTR pszCellDN = NULL;
    ADConfigurationMode adConfMode = NonSchemaMode;
    PSTR pszDirectoryRoot = NULL;
    
    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomainName,
                                               &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaLdapConvertDomainToDN(
                    pszDomainName,
                    &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaAllocateStringPrintf(&pszCellDN,"CN=$LikewiseIdentityCell,%s", pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);      
    
    //In default modes, if the trusted domain executes in unprovisional mode, 
    //we ignore it, and return LSA_ERROR_NO_SUCH_USER.
    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    if (dwError == LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS){
        dwError = LSA_ERROR_NO_SUCH_USER;       
    }    
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (pUserNameInfo->nameType){
       case NameType_UPN:
       case NameType_NT4:
               
           dwError = ADFindUserByNameNonAlias(
                                   hDirectory,
                                   hDirectory, 
                                   pszCellDN,
                                   DEFAULT_MODE,
                                   adConfMode,
                                   pUserNameInfo,
                                   &pUserInfo);
           
           break;
               
       case NameType_Alias:           
          
           dwError = LSA_ERROR_INVALID_PARAMETER;
               
           break;
               
        default:
           dwError = LSA_ERROR_INVALID_PARAMETER;               
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppUserInfo = pUserInfo;
    
cleanup:

    if (hDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hDirectory);
    }
    
    LSA_SAFE_FREE_STRING(pszCellDN);
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    
    return dwError;
    
error:

    *ppUserInfo = NULL;
    
    ADCacheDB_SafeFreeObject(&pUserInfo);
    
    goto cleanup;
}

DWORD
DefaultModeGetUserGroupMembership(
    HANDLE  hDirectory,
    PCSTR   pszCellDN,
    PCSTR   pszNetBIOSDomainName,
    DWORD   dwUID,    
    int     *piPrimaryGroupIndex,
    PDWORD  pdwGroupsFound,
    PAD_SECURITY_OBJECT** pppGroupInfoList
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    DWORD dwGroupsFound = 0;
    PAD_SECURITY_OBJECT* ppGroupInfoList = NULL;
    int    iPrimaryGroupIndex = -1;
    
    ADConfigurationMode adConfMode = NonSchemaMode;
                
    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (adConfMode)
    {
    case SchemaMode:
        dwError = DefaultModeSchemaGetUserGroupMembership(
                        hDirectory,
                        pszCellDN,
                        pszNetBIOSDomainName,
                        dwUID,                        
                        &iPrimaryGroupIndex,
                        &dwGroupsFound,
                        &ppGroupInfoList
                        );
        BAIL_ON_LSA_ERROR(dwError);
        break;
        
    case NonSchemaMode:
        dwError = DefaultModeNonSchemaGetUserGroupMembership(
                        hDirectory,
                        pszCellDN,
                        pszNetBIOSDomainName,
                        dwUID,                        
                        &iPrimaryGroupIndex,
                        &dwGroupsFound,
                        &ppGroupInfoList
                        );
        BAIL_ON_LSA_ERROR(dwError);
        break;       
    case UnknownMode:
        break; 
    }
    
    *pdwGroupsFound = dwGroupsFound;
    *pppGroupInfoList = ppGroupInfoList;
    *piPrimaryGroupIndex = iPrimaryGroupIndex;
    
cleanup:
    
    return dwError;
    
error:

    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;
    *piPrimaryGroupIndex = -1;
    
    ADCacheDB_SafeFreeObjectList(dwGroupsFound, &ppGroupInfoList);
    
    goto cleanup;
}

DWORD
DefaultModeSchemaGetUserGroupMembership(
    HANDLE  hDirectory,
    PCSTR   pszCellDN,
    PCSTR   pszNetBIOSDomainName,
    DWORD   dwUID,   
    int     *piPrimaryGroupIndex,
    PDWORD  pdwGroupsFound,
    PAD_SECURITY_OBJECT** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    LDAP *pLd = NULL;
    int    iPrimaryGroupIndex = -1;
    
    PAD_SECURITY_OBJECT* ppGroupInfoList = NULL;
    
    PSTR pszDirectoryRoot = NULL;

    PSTR szAttributeListGroups[] = 
        {
            AD_LDAP_SAM_NAME_TAG,
            AD_LDAP_GID_TAG,
            AD_LDAP_OBJECTSID_TAG,
            AD_LDAP_PASSWD_TAG,
            AD_LDAP_MEMBER_TAG,
            AD_LDAP_KEYWORDS_TAG,
            NULL
        };
    PSTR pszQuery = NULL;
    LDAPMessage *pMessage = NULL;
    LDAPMessage *pGroupMessage = NULL;
    DWORD dwCount = 0;
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    PSTR pszUserDN = NULL;
    PSTR pszEscapedUserDN = NULL;
    DWORD dwGroupsFound = 0;
    INT iGroup = 0;
    PLSA_SECURITY_IDENTIFIER pUserSID = NULL;
    PLSA_SECURITY_IDENTIFIER pDomainSID = NULL;
    PSTR pszDomainSID = NULL;
    PSTR pszPrimaryGroupSID = NULL;
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
                        pszDomainName,
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
        pszUserSid,
        //pUserInfo->pszObjectSid,
        &pUserSID);
    BAIL_ON_LSA_ERROR(dwError);
                     
    dwError = ADGetUserPrimaryGroupSID(
                hDirectory,
                pLd,
                pszDirectoryRoot,
                pszUserDN,
                pUserSID,
                &pszPrimaryGroupSID);
    BAIL_ON_LSA_ERROR(dwError);    

    dwError = LsaLdapEscapeString(
                &pszEscapedUserDN,
                pszUserDN);
    BAIL_ON_LSA_ERROR(dwError);
               
    //search for those real groups which have the current user as a member.
    dwError = LsaAllocateStringPrintf(
                &pszQuery,
                "(&(objectClass=group)(gidNumber=*)(|(member=%s)(objectSid=%s)))", 
                pszEscapedUserDN,
                pszPrimaryGroupSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapDirectorySearch(
                    hDirectory,
                    pszDirectoryRoot,
                    LDAP_SCOPE_SUBTREE,
                    pszQuery,
                    szAttributeListGroups,
                    &pMessage);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwCount = ldap_count_entries(
                      pLd,
                      pMessage);    
    if (dwCount < 0) 
    {
        dwError = LSA_ERROR_LDAP_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    } 

    dwGroupsFound = dwCount;

    if (!dwGroupsFound)
    {
        goto cleanup;
    } 
    
    dwError = LsaAllocateMemory(
                    sizeof(PAD_SECURITY_OBJECT) * dwGroupsFound,
                    (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
    pGroupMessage = pMessage;
    while (pGroupMessage)
    {   
        dwError = ADMarshalToGroupCache(
                           hDirectory,
                           DEFAULT_MODE,
                           SchemaMode,
                           pszNetBIOSDomainName,
                           pGroupMessage,
                           pGroupMessage,
                           &ppGroupInfoList[iGroup]);
        BAIL_ON_LSA_ERROR(dwError);

        if ( strncasecmp(
                 ppGroupInfoList[iGroup]->pszObjectSid,
                 AD_BUILTIN_GROUP_SID_PREFIX,
                 strlen(AD_BUILTIN_GROUP_SID_PREFIX)-1) )
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
    
    if (pDomainSID)
    {
        LsaFreeSecurityIdentifier(pDomainSID);
    }
    
    LSA_SAFE_FREE_STRING(pszDomainSID);
    LSA_SAFE_FREE_STRING(pszPrimaryGroupSID);
    LSA_SAFE_FREE_MEMORY(pszUserDN);
    LSA_SAFE_FREE_STRING(pszEscapedUserDN);
    LSA_SAFE_FREE_STRING(pszUserSid);
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    LSA_SAFE_FREE_STRING(pszFullDomainName);
    LSA_SAFE_FREE_STRING(pszQuery);
    
    return dwError;
    
error:

    ADCacheDB_SafeFreeObjectList(dwGroupsFound, &ppGroupInfoList);    
    
    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;
    *piPrimaryGroupIndex = -1;

    goto cleanup;
}

DWORD
DefaultModeNonSchemaGetUserGroupMembership(
    HANDLE  hDirectory,
    PCSTR   pszCellDN,
    PCSTR   pszNetBIOSDomainName,
    DWORD   dwUID,    
    int     *piPrimaryGroupIndex,
    PDWORD  pdwGroupsFound,
    PAD_SECURITY_OBJECT** pppGroupInfoList
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszUserDN = NULL;
    PLSA_SECURITY_IDENTIFIER pUserSID = NULL;
    PAD_SECURITY_OBJECT  pUserInfo = NULL;
    PAD_SECURITY_OBJECT* ppGroupInfoList = NULL;
    DWORD dwGroupsFound = 0;
    int    iPrimaryGroupIndex = -1;
    PSTR pszUserSid = NULL;
    
   /* dwError = ADGenericFindUserById(
                        hDirectory, 
                        pszCellDN,
                        DEFAULT_MODE,
                        NonSchemaMode,
                        pszDomainName,
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
 
    dwError = ADGetUserPseudoGroupMembership(
                hDirectory,
                DEFAULT_MODE,
                NonSchemaMode,
                pszCellDN,
                pszNetBIOSDomainName,
                pszUserDN,
                pUserSID,                
                &iPrimaryGroupIndex,
                &dwGroupsFound,
                &ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    *pppGroupInfoList = ppGroupInfoList;
    *pdwGroupsFound = dwGroupsFound; 
    *piPrimaryGroupIndex = iPrimaryGroupIndex;
        
cleanup:

    ADCacheDB_SafeFreeObject(&pUserInfo);
    
    if (pUserSID)
    {
        LsaFreeSecurityIdentifier(pUserSID);
    }

    LSA_SAFE_FREE_MEMORY(pszUserDN);
    LSA_SAFE_FREE_MEMORY(pszUserSid);
    
    return dwError;
    
error:

    *pppGroupInfoList = NULL;
    *pdwGroupsFound = 0;
    *piPrimaryGroupIndex = -1;
    
    ADCacheDB_SafeFreeObjectList(dwGroupsFound, &ppGroupInfoList);

    goto cleanup;
}

DWORD
DefaultModeFindGroupByName(    
    PCSTR pszDomainName,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PAD_SECURITY_OBJECT *ppGroupInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PAD_SECURITY_OBJECT pGroupInfo = NULL;
    HANDLE hDirectory = (HANDLE)NULL;
    PSTR pszCellDN = NULL;
    ADConfigurationMode adConfMode = NonSchemaMode;
    PSTR pszDirectoryRoot = NULL;
    
    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszDomainName,
                                               &hDirectory);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaLdapConvertDomainToDN(
                    pszDomainName,
                    &pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaAllocateStringPrintf(&pszCellDN,"CN=$LikewiseIdentityCell,%s", pszDirectoryRoot);
    BAIL_ON_LSA_ERROR(dwError);      
    
    //In default modes, if the trusted domain executes in unprovisional mode, 
    //we ignore it, and return LSA_ERROR_NO_SUCH_USER.
    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    if (dwError == LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS){
        dwError = LSA_ERROR_NO_SUCH_GROUP;       
    }    
    BAIL_ON_LSA_ERROR(dwError);
    
    switch (pGroupNameInfo->nameType){       
       case NameType_NT4:
               
           dwError = ADFindGroupByNameNT4(
                                   hDirectory,
                                   hDirectory, 
                                   pszCellDN,
                                   DEFAULT_MODE,
                                   adConfMode,
                                   pGroupNameInfo,
                                   &pGroupInfo);
           
           break;
               
       case NameType_Alias:
           
           dwError = LSA_ERROR_INVALID_PARAMETER;           
               
          break;
               
        default:
           dwError = LSA_ERROR_INVALID_PARAMETER;
               
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppGroupInfo = pGroupInfo;
    
cleanup:

    if (hDirectory != (HANDLE)NULL) {
        LsaLdapCloseDirectory(hDirectory);
    }
    
    LSA_SAFE_FREE_STRING(pszCellDN);
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    
    return dwError;
    
error:

    *ppGroupInfo = NULL;
    
    ADCacheDB_SafeFreeObject(&pGroupInfo);
    
    goto cleanup;
}

DWORD
DefaultModeEnumUsers(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumUsers,
    PDWORD         pdwUsersFound,
    PVOID**        pppUserInfoList
    )
{
    DWORD  dwError = 0;    
    DWORD  dwNumUsersFound = 0;
    PVOID* ppUserInfoList = NULL;
    
    ADConfigurationMode adConfMode = NonSchemaMode;
                
    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    BAIL_ON_LSA_ERROR(dwError);
   
    switch (adConfMode)
    {
       case SchemaMode:
           dwError = DefaultModeSchemaEnumUsers(
                       hDirectory,
                       pszCellDN,
                       pszDomainName,
                       pEnumState,
                       dwMaxNumUsers,
                       &dwNumUsersFound,
                       &ppUserInfoList
                       );
           BAIL_ON_LSA_ERROR(dwError);
           break;
       
       case NonSchemaMode:
           dwError = DefaultModeNonSchemaEnumUsers(
                       hDirectory,
                       pszCellDN,
                       pszDomainName,
                       pEnumState,
                       dwMaxNumUsers,
                       &dwNumUsersFound,
                       &ppUserInfoList
                       );
           BAIL_ON_LSA_ERROR(dwError);
           break;      
       case UnknownMode:
           break;  
    }
   
    *pppUserInfoList = ppUserInfoList;
    *pdwUsersFound = dwNumUsersFound;
   
cleanup:
   
    return dwError;
   
error:

    *pppUserInfoList = NULL;
    *pdwUsersFound = 0;
   
    if (ppUserInfoList) {
      LsaFreeUserInfoList(pEnumState->dwInfoLevel, ppUserInfoList, dwNumUsersFound);
    }
   
    goto cleanup;
}

DWORD
DefaultModeSchemaEnumUsers(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumUsers,
    PDWORD         pdwNumUsersFound,
    PVOID**        pppUserInfoList
    )
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    CHAR szQuery[1024];
    PVOID* ppUserInfoList = NULL;
    DWORD  dwNumUsersFound = 0;
    PSTR   szAttributeList[] = 
            {
                     AD_LDAP_UID_TAG,
                     AD_LDAP_GID_TAG,
                     AD_LDAP_SAM_NAME_TAG,
                     AD_LDAP_PASSWD_TAG,
                     AD_LDAP_HOMEDIR_TAG,
                     AD_LDAP_SHELL_TAG,
                     AD_LDAP_GECOS_TAG,
                     AD_LDAP_SEC_DESC_TAG,
                     AD_LDAP_UPN_TAG,
                     AD_LDAP_USER_CTRL_TAG,
                     AD_LDAP_PWD_LASTSET_TAG,
                     AD_LDAP_ACCOUT_EXP_TAG,
                     NULL
            };   

    LDAPMessage *pMessagePseudo = NULL;
    PSTR pszDomain = NULL;
    PSTR pszDirectoryRoot = NULL;
    LDAP *pLd = LsaLdapGetSession(hDirectory);
    
    
    pszDomain = gpADProviderData->szDomain;
    dwError = LsaLdapConvertDomainToDN(pszDomain, &pszDirectoryRoot);
    sprintf(szQuery, "(&(objectClass=User)(!(objectClass=computer))(uidNumber=*))");
    
    if (!pEnumState->bMorePages){
        dwError = LSA_ERROR_NO_MORE_USERS;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaLdapDirectoryOnePagedSearch(
                       hDirectory,
                       pszDirectoryRoot,
                       szQuery,
                       szAttributeList,
                       dwMaxNumUsers,                       
                       &pEnumState->pCookie,        
                       LDAP_SCOPE_SUBTREE,
                       &pMessagePseudo,
                       &pEnumState->bMorePages);
    
    BAIL_ON_LSA_ERROR(dwError);
    
    dwCount = ldap_count_entries(
                          pLd,
                          pMessagePseudo
                          );    
    if (dwCount < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
        dwError = LSA_ERROR_NO_MORE_USERS;
    } 
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = ADSchemaMarshalUserInfoList(
                    hDirectory,
                    pszNetBIOSDomainName,
                    pMessagePseudo,                    
                    pEnumState->dwInfoLevel,
                    &ppUserInfoList,
                    &dwNumUsersFound);
    BAIL_ON_LSA_ERROR(dwError);       
    
    *pppUserInfoList = ppUserInfoList;
    *pdwNumUsersFound = dwNumUsersFound;
    
cleanup:

    if (pMessagePseudo)
    {
        ldap_msgfree(pMessagePseudo);
    }
    
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    
    return dwError;
    
error:

    *pppUserInfoList = NULL;
    *pdwNumUsersFound = 0;
    
    if (ppUserInfoList) {
          LsaFreeUserInfoList(pEnumState->dwInfoLevel, ppUserInfoList, dwNumUsersFound);
       }

    goto cleanup;
}

DWORD
DefaultModeNonSchemaEnumUsers(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumUsers,
    PDWORD         pdwNumUsersFound,
    PVOID**        pppUserInfoList
    )
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    CHAR szQuery[1024];
    CHAR szBuffer[1024]; 
    PVOID* ppUserInfoList = NULL;
    PVOID* ppUserInfoList_accumulate = NULL;        
    DWORD  dwTotalNumUsersFound = 0;
    DWORD  dwNumUsersFound = 0;
    DWORD  dwUserInfoLevel = 0;  
    PSTR szAttributeList[] = 
               {
                 AD_LDAP_NAME_TAG,
                 AD_LDAP_KEYWORDS_TAG,                       
                 NULL
               }; 

    LDAPMessage *pMessagePseudo = NULL;
    LDAP *pLd = LsaLdapGetSession(hDirectory);
    DWORD dwNumUsersWanted = dwMaxNumUsers;
    
    dwUserInfoLevel = pEnumState->dwInfoLevel;
    sprintf(szBuffer,"CN=Users,%s", pszCellDN);
    sprintf(szQuery, "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseUser)(keywords=uidNumber=*))");    
    
    if (!pEnumState->bMorePages){
          dwError = LSA_ERROR_NO_MORE_USERS;
          BAIL_ON_LSA_ERROR(dwError);
    }
    
    do
    {        
        dwError = LsaLdapDirectoryOnePagedSearch(
                       hDirectory,
                       szBuffer,
                       szQuery,
                       szAttributeList,
                       dwNumUsersWanted,                       
                       &pEnumState->pCookie,        
                       LDAP_SCOPE_ONELEVEL,
                       &pMessagePseudo,
                       &pEnumState->bMorePages);    
        BAIL_ON_LSA_ERROR(dwError);
    
        dwCount = ldap_count_entries(
                          pLd,
                          pMessagePseudo);    
        if (dwCount < 0) {
           dwError = LSA_ERROR_LDAP_ERROR;
        } else if (dwCount == 0) {
           dwError = LSA_ERROR_NO_MORE_USERS;
        } 
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = ADNonSchemaMarshalUserInfoList(
                        hDirectory,
                        pszNetBIOSDomainName,
                        pMessagePseudo,                    
                        dwUserInfoLevel,
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
           
        if (pMessagePseudo) {
               ldap_msgfree(pMessagePseudo);
               pMessagePseudo = NULL;            
        }
    } while (pEnumState->bMorePages && dwNumUsersWanted);    
    
    *pppUserInfoList = ppUserInfoList_accumulate;
    *pdwNumUsersFound = dwTotalNumUsersFound;
    
cleanup:
        
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
DefaultModeEnumGroups(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumGroups,
    PDWORD         pdwNumGroupsFound,
    PVOID**        pppGroupInfoList
    )
{
    DWORD  dwError = 0;    
    DWORD  dwNumGroupsFound = 0;
    PVOID* ppGroupInfoList = NULL;
    
    ADConfigurationMode adConfMode = NonSchemaMode;
                
    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    BAIL_ON_LSA_ERROR(dwError);
   
    switch (adConfMode)
    {
       case SchemaMode:
           dwError = DefaultModeSchemaEnumGroups(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pEnumState,
                       dwMaxNumGroups,
                       &dwNumGroupsFound,
                       &ppGroupInfoList
                       );
           BAIL_ON_LSA_ERROR(dwError);
           break;
       
       case NonSchemaMode:
           dwError = DefaultModeNonSchemaEnumGroups(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pEnumState,
                       dwMaxNumGroups,
                       &dwNumGroupsFound,
                       &ppGroupInfoList
                       );
           BAIL_ON_LSA_ERROR(dwError);
           break;       
       case UnknownMode:
           break; 
    }
   
    *pppGroupInfoList = ppGroupInfoList;
    *pdwNumGroupsFound = dwNumGroupsFound;
   
cleanup:
   
    return dwError;
   
error:

    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = 0;
   
    if (ppGroupInfoList) {
      LsaFreeGroupInfoList(pEnumState->dwInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }
   
    goto cleanup;
}

DWORD
DefaultModeSchemaEnumGroups(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumGroups,
    PDWORD         pdwNumGroupsFound,
    PVOID**        pppGroupInfoList
    )
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    CHAR szQuery[1024];
    PVOID* ppGroupInfoList = NULL;
    DWORD  dwNumGroupsFound = 0;
    PSTR szAttributeList[] = 
                {
                        AD_LDAP_SAM_NAME_TAG,
                        AD_LDAP_GID_TAG,
                        AD_LDAP_PASSWD_TAG,
                        AD_LDAP_MEMBER_TAG,
                        NULL
                };
    LDAPMessage *pMessagePseudo = NULL;
    PSTR pszDomain = NULL;
    PSTR pszDirectoryRoot = NULL;
    LDAP *pLd = LsaLdapGetSession(hDirectory);
    
    
    pszDomain = gpADProviderData->szDomain;
    dwError = LsaLdapConvertDomainToDN(pszDomain, &pszDirectoryRoot);
    sprintf(szQuery, "(&(objectClass=Group)(gidNumber=*))");
    
    if (!pEnumState->bMorePages){
        dwError = LSA_ERROR_NO_MORE_GROUPS;
        BAIL_ON_LSA_ERROR(dwError);       
    }
    
    dwError = LsaLdapDirectoryOnePagedSearch(
                       hDirectory,
                       pszDirectoryRoot,
                       szQuery,
                       szAttributeList,
                       dwMaxNumGroups,                       
                       &pEnumState->pCookie,        
                       LDAP_SCOPE_SUBTREE,
                       &pMessagePseudo,
                       &pEnumState->bMorePages);
    
    BAIL_ON_LSA_ERROR(dwError);
    
    dwCount = ldap_count_entries(
                          pLd,
                          pMessagePseudo
                          );    
    if (dwCount < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
        dwError = LSA_ERROR_NO_MORE_GROUPS;
    } 
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = ADSchemaMarshalGroupInfoList(
                    hDirectory,
                    pszNetBIOSDomainName,            
                    pMessagePseudo,
                    pEnumState->dwInfoLevel,
                    &ppGroupInfoList,
                    &dwNumGroupsFound
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    *pppGroupInfoList = ppGroupInfoList;
    *pdwNumGroupsFound = dwNumGroupsFound;
    
cleanup:

    if (pMessagePseudo)
    {
        ldap_msgfree(pMessagePseudo);
    }
    
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    
    return dwError;
    
error:

    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = 0;
    
    if (ppGroupInfoList) {
          LsaFreeGroupInfoList(pEnumState->dwInfoLevel, ppGroupInfoList, dwNumGroupsFound);
       }

    goto cleanup;
}

DWORD
DefaultModeNonSchemaEnumGroups(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumGroups,
    PDWORD         pdwNumGroupsFound,
    PVOID**        pppGroupInfoList
    )
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    CHAR szQuery[1024];
    CHAR szBuffer[1024]; 
    PVOID* ppGroupInfoList = NULL;
    PVOID* ppGroupInfoList_accumulate = NULL;        
    DWORD  dwTotalNumGroupsFound = 0;
    DWORD  dwNumGroupsFound = 0;
    DWORD  dwGroupInfoLevel = 0;  
    PSTR szAttributeList[] = 
               {
                 AD_LDAP_NAME_TAG,
                 AD_LDAP_KEYWORDS_TAG,                       
                 NULL
               };   

    LDAPMessage *pMessagePseudo = NULL;
    LDAP *pLd = LsaLdapGetSession(hDirectory);
    DWORD dwNumGroupsWanted = dwMaxNumGroups;
    
    dwGroupInfoLevel = pEnumState->dwInfoLevel;
    sprintf(szBuffer,"CN=Groups,%s", pszCellDN);
    sprintf(szQuery, "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseGroup)(keywords=gidNumber=*))");
    
    if (!pEnumState->bMorePages){
            dwError = LSA_ERROR_NO_MORE_GROUPS;
            BAIL_ON_LSA_ERROR(dwError);       
    }    
    
    do
    {
        dwError = LsaLdapDirectoryOnePagedSearch(
                       hDirectory,
                       szBuffer,
                       szQuery,
                       szAttributeList,
                       dwNumGroupsWanted,                       
                       &pEnumState->pCookie,        
                       LDAP_SCOPE_ONELEVEL,
                       &pMessagePseudo,
                       &pEnumState->bMorePages);    
        BAIL_ON_LSA_ERROR(dwError);
    
        dwCount = ldap_count_entries(
                          pLd,
                          pMessagePseudo);    
        if (dwCount < 0) {
           dwError = LSA_ERROR_LDAP_ERROR;
        } else if (dwCount == 0) {
           dwError = LSA_ERROR_NO_MORE_GROUPS;
        } 
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = ADNonSchemaMarshalGroupInfoList(
                        hDirectory,
                        pszNetBIOSDomainName,
                        pMessagePseudo,                    
                        dwGroupInfoLevel,
                        &ppGroupInfoList,
                        &dwNumGroupsFound
                        );
        BAIL_ON_LSA_ERROR(dwError);
       
        dwNumGroupsWanted -= dwNumGroupsFound;
        
        dwError = LsaCoalesceGroupInfoList(
                        &ppGroupInfoList,
                        &dwNumGroupsFound,
                        &ppGroupInfoList_accumulate,
                        &dwTotalNumGroupsFound
                        );
        BAIL_ON_LSA_ERROR(dwError);
           
        if (pMessagePseudo) {
               ldap_msgfree(pMessagePseudo);
               pMessagePseudo = NULL;            
        }
    } while (pEnumState->bMorePages && dwNumGroupsWanted);

    *pppGroupInfoList = ppGroupInfoList_accumulate;
    *pdwNumGroupsFound = dwTotalNumGroupsFound;
    
cleanup:
        
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



DWORD
DefaultModeEnumNSSArtefacts(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumNSSArtefacts,
    PDWORD         pdwNumNSSArtefactsFound,
    PVOID**        pppNSSArtefactInfoList
    )
{
    DWORD  dwError = 0;    
    DWORD  dwNumNSSArtefactsFound = 0;
    PVOID* ppNSSArtefactInfoList = NULL;
    
    ADConfigurationMode adConfMode = NonSchemaMode;
                
    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    BAIL_ON_LSA_ERROR(dwError);
   
    switch (adConfMode)
    {
       case SchemaMode:
           dwError = DefaultModeSchemaEnumNSSArtefacts(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pEnumState,
                       dwMaxNumNSSArtefacts,
                       &dwNumNSSArtefactsFound,
                       &ppNSSArtefactInfoList
                       );
           BAIL_ON_LSA_ERROR(dwError);
           break;
       
       case NonSchemaMode:
           dwError = DefaultModeNonSchemaEnumNSSArtefacts(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pEnumState,
                       dwMaxNumNSSArtefacts,
                       &dwNumNSSArtefactsFound,
                       &ppNSSArtefactInfoList
                       );
           BAIL_ON_LSA_ERROR(dwError);
           break;       
       case UnknownMode:
           break; 
    }
   
    *pppNSSArtefactInfoList = ppNSSArtefactInfoList;
    *pdwNumNSSArtefactsFound = dwNumNSSArtefactsFound;
   
cleanup:
   
    return dwError;
   
error:

    *pppNSSArtefactInfoList = NULL;
    *pdwNumNSSArtefactsFound = 0;
   
    if (ppNSSArtefactInfoList) {
      LsaFreeNSSArtefactInfoList(pEnumState->dwInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }
   
    goto cleanup;
}

DWORD
DefaultModeSchemaEnumNSSArtefacts(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumNSSArtefacts,
    PDWORD         pdwNumNSSArtefactsFound,
    PVOID**        pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    CHAR szQuery[1024];
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD  dwNumNSSArtefactsFound = 0;
    PSTR szAttributeList[] = 
                {
                        AD_LDAP_SAM_NAME_TAG,
                        AD_LDAP_GID_TAG,
                        AD_LDAP_PASSWD_TAG,
                        AD_LDAP_MEMBER_TAG,
                        NULL
                };
    LDAPMessage *pMessagePseudo = NULL;
    PSTR pszDomain = NULL;
    PSTR pszDirectoryRoot = NULL;
    LDAP *pLd = LsaLdapGetSession(hDirectory);
    
    
    pszDomain = gpADProviderData->szDomain;
    dwError = LsaLdapConvertDomainToDN(pszDomain, &pszDirectoryRoot);
    sprintf(szQuery, "(&(objectClass=NSSArtefact)(gidNumber=*))");
    
    if (!pEnumState->bMorePages){
        dwError = LSA_ERROR_NO_MORE_GROUPS;
        BAIL_ON_LSA_ERROR(dwError);       
    }
    
    dwError = LsaLdapDirectoryOnePagedSearch(
                       hDirectory,
                       pszDirectoryRoot,
                       szQuery,
                       szAttributeList,
                       dwMaxNumNSSArtefacts,                       
                       &pEnumState->pCookie,        
                       LDAP_SCOPE_SUBTREE,
                       &pMessagePseudo,
                       &pEnumState->bMorePages);
    
    BAIL_ON_LSA_ERROR(dwError);
    
    dwCount = ldap_count_entries(
                          pLd,
                          pMessagePseudo
                          );    
    if (dwCount < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
        dwError = LSA_ERROR_NO_MORE_GROUPS;
    } 
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = ADSchemaMarshalNSSArtefactInfoList(
                    hDirectory,
                    pszNetBIOSDomainName,            
                    pMessagePseudo,
                    pEnumState->dwInfoLevel,
                    &ppNSSArtefactInfoList,
                    &dwNumNSSArtefactsFound
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    *pppNSSArtefactInfoList = ppNSSArtefactInfoList;
    *pdwNumNSSArtefactsFound = dwNumNSSArtefactsFound;
    
cleanup:
    if (pMessagePseudo)
    {
        ldap_msgfree(pMessagePseudo);
    }
    
    return dwError;
    
error:

    *pppNSSArtefactInfoList = NULL;
    *pdwNumNSSArtefactsFound = 0;
    
    if (ppNSSArtefactInfoList) {
          LsaFreeNSSArtefactInfoList(pEnumState->dwInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
       }

    goto cleanup;
}

DWORD
DefaultModeNonSchemaEnumNSSArtefacts(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumNSSArtefacts,
    PDWORD         pdwNumNSSArtefactsFound,
    PVOID**        pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    CHAR szQuery[1024];
    CHAR szBuffer[1024]; 
    PVOID* ppNSSArtefactInfoList = NULL;
    PVOID* ppNSSArtefactInfoList_accumulate = NULL;        
    DWORD  dwTotalNumNSSArtefactsFound = 0;
    DWORD  dwNumNSSArtefactsFound = 0;
    DWORD  dwNSSArtefactInfoLevel = 0;  
    PSTR szAttributeList[] = 
               {
                 AD_LDAP_NAME_TAG,
                 AD_LDAP_KEYWORDS_TAG,                       
                 NULL
               };   

    LDAPMessage *pMessagePseudo = NULL;
    LDAP *pLd = LsaLdapGetSession(hDirectory);
    DWORD dwNumNSSArtefactsWanted = dwMaxNumNSSArtefacts;
    
    dwNSSArtefactInfoLevel = pEnumState->dwInfoLevel;
    sprintf(szBuffer,"CN=NSSArtefacts,%s", pszCellDN);
    sprintf(szQuery, "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseNSSArtefact)(keywords=gidNumber=*))");
    
    if (!pEnumState->bMorePages){
            dwError = LSA_ERROR_NO_MORE_GROUPS;
            BAIL_ON_LSA_ERROR(dwError);       
    }    
    
    do
    {
        dwError = LsaLdapDirectoryOnePagedSearch(
                       hDirectory,
                       szBuffer,
                       szQuery,
                       szAttributeList,
                       dwNumNSSArtefactsWanted,                       
                       &pEnumState->pCookie,        
                       LDAP_SCOPE_ONELEVEL,
                       &pMessagePseudo,
                       &pEnumState->bMorePages);    
        BAIL_ON_LSA_ERROR(dwError);
    
        dwCount = ldap_count_entries(
                          pLd,
                          pMessagePseudo);    
        if (dwCount < 0) {
           dwError = LSA_ERROR_LDAP_ERROR;
        } else if (dwCount == 0) {
           dwError = LSA_ERROR_NO_MORE_GROUPS;
        } 
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = ADNonSchemaMarshalNSSArtefactInfoList(
                        hDirectory,
                        pszNetBIOSDomainName,
                        pMessagePseudo,                    
                        dwNSSArtefactInfoLevel,
                        &ppNSSArtefactInfoList,
                        &dwNumNSSArtefactsFound
                        );
        BAIL_ON_LSA_ERROR(dwError);
       
        dwNumNSSArtefactsWanted -= dwNumNSSArtefactsFound;
        
        dwError = LsaCoalesceNSSArtefactInfoList(
                        &ppNSSArtefactInfoList,
                        &dwNumNSSArtefactsFound,
                        &ppNSSArtefactInfoList_accumulate,
                        &dwTotalNumNSSArtefactsFound
                        );
        BAIL_ON_LSA_ERROR(dwError);
           
        if (pMessagePseudo) {
               ldap_msgfree(pMessagePseudo);
               pMessagePseudo = NULL;            
        }
    } while (pEnumState->bMorePages && dwNumNSSArtefactsWanted);

    *pppNSSArtefactInfoList = ppNSSArtefactInfoList_accumulate;
    *pdwNumNSSArtefactsFound = dwTotalNumNSSArtefactsFound;
    
cleanup:
        
    return dwError;
        
error:
    *pppNSSArtefactInfoList = NULL;
    *pdwNumNSSArtefactsFound = 0;
       
    if (ppNSSArtefactInfoList) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList, dwNumNSSArtefactsFound);
    }
        
    if (ppNSSArtefactInfoList_accumulate) {
        LsaFreeNSSArtefactInfoList(dwNSSArtefactInfoLevel, ppNSSArtefactInfoList_accumulate, dwTotalNumNSSArtefactsFound);
    }  

    goto cleanup;
}
