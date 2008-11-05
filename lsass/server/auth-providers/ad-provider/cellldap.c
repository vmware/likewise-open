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
CellModeFindUserByName(
    PCSTR                pszPseudoDomainName,
    PCSTR                pszPseudoCellDN,
    PCSTR                pszRealDomainName,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PAD_SECURITY_OBJECT *ppUserInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    HANDLE hPseudoDirectory = (HANDLE)NULL;
    HANDLE hRealDirectory = (HANDLE)NULL;
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    ADConfigurationMode  adConfMode = NonSchemaMode;

    //Get current joined domain's ldap directory
    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszPseudoDomainName,
                                               &hPseudoDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    //Get execution mode:schema/non-schema (save in adConfMode)
    dwError = ADGetConfigurationMode(
                         hPseudoDirectory,
                         pszPseudoCellDN,
                         &adConfMode);
    if (dwError == LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS){
        dwError = LSA_ERROR_NO_SUCH_USER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (!IsNullOrEmptyString(pszRealDomainName))
    {
        dwError = LsaDmWrapLdapOpenDirectoryDomain(pszRealDomainName,
                                                   &hRealDirectory);
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch (pUserNameInfo->nameType){
           case NameType_UPN:
           case NameType_NT4:

               dwError = ADFindUserByNameNonAlias(
                                   hPseudoDirectory,
                                   hRealDirectory,
                                   pszPseudoCellDN,
                                   CELL_MODE,
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

    if (hPseudoDirectory != (HANDLE)NULL)
    {
        LsaLdapCloseDirectory(hPseudoDirectory);
    }

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
CellModeGetUserGroupMembership(
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
    ADConfigurationMode adConfMode = NonSchemaMode;
    int    iPrimaryGroupIndex = -1;

    dwError = ADGetConfigurationMode(
                         hDirectory,
                         pszCellDN,
                         &adConfMode);
    BAIL_ON_LSA_ERROR(dwError);

    switch (adConfMode)
    {
    case SchemaMode:
        dwError = CellModeSchemaGetUserGroupMembership(
                        hDirectory,
                        pszCellDN,
                        pszNetBIOSDomainName,
                        dwUID,
                        &iPrimaryGroupIndex,
                        &dwGroupsFound,
                        &ppGroupInfoList);
        BAIL_ON_LSA_ERROR(dwError);
        break;

    case NonSchemaMode:
        dwError = CellModeNonSchemaGetUserGroupMembership(
                        hDirectory,
                        pszCellDN,
                        pszNetBIOSDomainName,
                        dwUID,
                        &iPrimaryGroupIndex,
                        &dwGroupsFound,
                        &ppGroupInfoList);
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
CellModeSchemaGetUserGroupMembership(
    HANDLE  hDirectory,
    PCSTR   pszCellDN,
    PCSTR   pszNetBIOSDomainName,
    DWORD   dwUID,
    int     *piPrimaryGroupIndex,
    PDWORD  pdwGroupsFound,
    PAD_SECURITY_OBJECT** pppGroupInfoList
    )
{
    DWORD  dwError = LSA_ERROR_SUCCESS;
    PSTR   pszUserDN = NULL;
    PAD_SECURITY_OBJECT* ppGroupInfoList = NULL;
    DWORD  dwGroupsFound = 0;
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    PLSA_SECURITY_IDENTIFIER pUserSID = NULL;
    int    iPrimaryGroupIndex = -1;
    PSTR pszUserSid = NULL;

    /*dwError = ADGenericFindUserById(
                hDirectory,
                pszCellDN,
                CELL_MODE,
                SchemaMode,
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
                    pszUserSid,
                    &pUserSID);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADGetUserPseudoGroupMembership(
                hDirectory,
                CELL_MODE,
                SchemaMode,
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

    LSA_SAFE_FREE_MEMORY(pszUserDN);
    LSA_SAFE_FREE_MEMORY(pszUserSid);

    ADCacheDB_SafeFreeObject(&pUserInfo);

    if (pUserSID)
    {
        LsaFreeSecurityIdentifier(pUserSID);
    }

    return dwError;

error:

    *pppGroupInfoList = NULL;
    *pdwGroupsFound = 0;
    *piPrimaryGroupIndex = -1;

    ADCacheDB_SafeFreeObjectList(dwGroupsFound, &ppGroupInfoList);

    goto cleanup;
}

DWORD
CellModeNonSchemaGetUserGroupMembership(
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
    DWORD dwGroupsFound = 0;
    PAD_SECURITY_OBJECT* ppGroupInfoList = NULL;
    int    iPrimaryGroupIndex = -1;

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

    //TODO: If user is not found, search in the possible linked cell in order

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
CellModeFindGroupByName(
    PCSTR                pszPseudoDomainName,
    PCSTR                pszPseudoCellDN,
    PCSTR                pszRealDomainName,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PAD_SECURITY_OBJECT *ppGroupInfo
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    HANDLE hPseudoDirectory = (HANDLE)NULL;
    HANDLE hRealDirectory = (HANDLE)NULL;
    PAD_SECURITY_OBJECT pGroupInfo = NULL;
    ADConfigurationMode  adConfMode = NonSchemaMode;

    //Get current joined domain's ldap directory
    dwError = LsaDmWrapLdapOpenDirectoryDomain(pszPseudoDomainName,
                                               &hPseudoDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    //Get execution mode:schema/non-schema (save in adConfMode)
    dwError = ADGetConfigurationMode(
                         hPseudoDirectory,
                         pszPseudoCellDN,
                         &adConfMode);
    if (dwError == LSA_ERROR_INCOMPATIBLE_MODES_BETWEEN_TRUSTEDDOMAINS){
        dwError = LSA_ERROR_NO_SUCH_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (!IsNullOrEmptyString(pszRealDomainName))
    {
        dwError = LsaDmWrapLdapOpenDirectoryDomain(pszRealDomainName,
                                                   &hRealDirectory);
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch (pGroupNameInfo->nameType){
           case NameType_NT4:

               dwError = ADFindGroupByNameNT4(
                                   hPseudoDirectory,
                                   hRealDirectory,
                                   pszPseudoCellDN,
                                   CELL_MODE,
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

    if (hPseudoDirectory != (HANDLE)NULL)
    {
        LsaLdapCloseDirectory(hPseudoDirectory);
    }

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
CellModeEnumUsers(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
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
           dwError = CellModeSchemaEnumUsers(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pEnumState,
                       dwMaxNumUsers,
                       &dwNumUsersFound,
                       &ppUserInfoList
                       );
           BAIL_ON_LSA_ERROR(dwError);
           break;

       case NonSchemaMode:
           dwError = CellModeNonSchemaEnumUsers(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
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
CellModeSchemaEnumUsers(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumUsers,
    PDWORD         pdwNumUsersFound,
    PVOID**        pppUserInfoList
    )
{
    DWORD  dwError = 0;
    DWORD  dwCount = 0;
    CHAR   szQuery[1024];
    CHAR   szBuffer[1024];
    PVOID* ppUserInfoList_accumulate = NULL;
    PVOID* ppUserInfoList = NULL;
    DWORD  dwTotalNumUsersFound = 0;
    DWORD  dwNumUsersFound = 0;
    DWORD  dwUserInfoLevel = 0;
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
                     AD_LDAP_KEYWORDS_TAG,
                     NULL
            };

    LDAPMessage *pMessagePseudo = NULL;
    LDAP *pLd = LsaLdapGetSession(hDirectory);

    DWORD dwNumUsersWanted = dwMaxNumUsers;

    dwUserInfoLevel = pEnumState->dwInfoLevel;
    sprintf(szBuffer,"CN=Users,%s", pszCellDN);
    sprintf(szQuery, "(&(objectClass=posixAccount)(uidNumber=*))");

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

        dwError = ADSchemaMarshalUserInfoList(
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
CellModeNonSchemaEnumUsers(
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
    PVOID* ppUserInfoList = NULL;
    DWORD  dwNumUsersFound = 0;

    dwError = DefaultModeNonSchemaEnumUsers(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pEnumState,
                       dwMaxNumUsers,
                       &dwNumUsersFound,
                       &ppUserInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    //TODO: enum linked cell users

    *pppUserInfoList = ppUserInfoList;
    *pdwNumUsersFound = dwNumUsersFound;

cleanup:

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
CellModeEnumGroups(
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
           dwError = CellModeSchemaEnumGroups(
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
           dwError = CellModeNonSchemaEnumGroups(
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
CellModeSchemaEnumGroups(
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
    DWORD  dwCount = 0;
    CHAR   szQuery[1024];
    CHAR   szBuffer[1024];
    PVOID* ppGroupInfoList_accumulate = NULL;
    PVOID* ppGroupInfoList = NULL;
    DWORD  dwTotalNumGroupsFound = 0;
    DWORD  dwNumGroupsFound = 0;
    DWORD  dwGroupInfoLevel = 0;
    PSTR   szAttributeList[] =
            {
                     AD_LDAP_KEYWORDS_TAG,
                     AD_LDAP_SAM_NAME_TAG,
                     AD_LDAP_GID_TAG,
                     AD_LDAP_PASSWD_TAG,
                     AD_LDAP_MEMBER_TAG,
                     NULL
            };

    LDAPMessage *pMessagePseudo = NULL;
    LDAP *pLd = NULL;

    DWORD dwNumGroupsWanted = dwMaxNumGroups;

    pLd = LsaLdapGetSession(hDirectory);

    dwGroupInfoLevel = pEnumState->dwInfoLevel;
    sprintf(szBuffer,"CN=Groups,%s", pszCellDN);
    sprintf(szQuery, "(&(objectClass=posixGroup)(gidNumber=*))");

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

        dwError = ADSchemaMarshalGroupInfoList(
                        hDirectory,
                        pszNetBIOSDomainName,
                        pMessagePseudo,
                        dwGroupInfoLevel,
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

        if (pMessagePseudo) {
            ldap_msgfree(pMessagePseudo);
            pMessagePseudo = NULL;
        }
    } while (pEnumState->bMorePages && dwNumGroupsWanted);

    *pppGroupInfoList = ppGroupInfoList_accumulate;
    *pdwNumGroupsFound = dwTotalNumGroupsFound;

cleanup:

    if (pMessagePseudo) {
        ldap_msgfree(pMessagePseudo);
    }

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
CellModeNonSchemaEnumGroups(
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
    PVOID* ppGroupInfoList = NULL;
    DWORD  dwNumGroupsFound = 0;

    dwError = DefaultModeNonSchemaEnumGroups(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pEnumState,
                       dwMaxNumGroups,
                       &dwNumGroupsFound,
                       &ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    //ToDo: enum linked cell groups

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
CellModeEnumNSSArtefacts(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    DWORD          dwMapType,
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
           dwError = CellModeSchemaEnumNSSArtefacts(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       dwMapType,
                       pEnumState,
                       dwMaxNumNSSArtefacts,
                       &dwNumNSSArtefactsFound,
                       &ppNSSArtefactInfoList
                       );
           BAIL_ON_LSA_ERROR(dwError);
           break;

       case NonSchemaMode:
           dwError = CellModeNonSchemaEnumNSSArtefacts(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       dwMapType,
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
CellModeSchemaEnumNSSArtefacts(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    DWORD          dwMapType,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumNSSArtefacts,
    PDWORD         pdwNumNSSArtefactsFound,
    PVOID**        pppNSSArtefactInfoList
    )
{
    DWORD  dwError = 0;
    DWORD  dwCount = 0;
    CHAR   szQuery[1024];
    CHAR   szBuffer[1024];
    PVOID* ppNSSArtefactInfoList_accumulate = NULL;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD  dwTotalNumNSSArtefactsFound = 0;
    DWORD  dwNumNSSArtefactsFound = 0;
    DWORD  dwNSSArtefactInfoLevel = 0;
    PSTR   pszMapType = NULL;
    PSTR szAttributeList[] =
        {
             AD_LDAP_NAME_TAG,
             AD_LDAP_KEYWORDS_TAG,
             NULL
        };

    LDAPMessage *pMessagePseudo = NULL;
    LDAP *pLd = NULL;

    DWORD dwNumNSSArtefactsWanted = dwMaxNumNSSArtefacts;

    pLd = LsaLdapGetSession(hDirectory);

    dwNSSArtefactInfoLevel = pEnumState->dwInfoLevel;

    dwError = ADLdap_GetMapTypeString(
                  dwMapType,
                  &pszMapType);
    BAIL_ON_LSA_ERROR(dwError);

    sprintf(szBuffer,"CN=%s,CN=Maps,%s", pszMapType, pszCellDN);
    sprintf(szQuery, "(&(objectClass=serviceConnectionPoint)(keywords=objectClass=centerisLikewiseMapEntry))");

    if (!pEnumState->bMorePages){
        dwError = LSA_ERROR_NO_MORE_NSS_ARTEFACTS;
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
                       LDAP_SCOPE_SUBTREE,
                       &pMessagePseudo,
                       &pEnumState->bMorePages);
        BAIL_ON_LSA_ERROR(dwError);

        dwCount = ldap_count_entries(
                          pLd,
                          pMessagePseudo);
        if (dwCount < 0) {
           dwError = LSA_ERROR_LDAP_ERROR;
        } else if (dwCount == 0) {
           dwError = LSA_ERROR_NO_MORE_NSS_ARTEFACTS;
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADSchemaMarshalNSSArtefactInfoList(
                        hDirectory,
                        pszNetBIOSDomainName,
                        pMessagePseudo,
                        (LsaNSSMapType)dwMapType,
                        dwNSSArtefactInfoLevel,
                        &ppNSSArtefactInfoList,
                        &dwNumNSSArtefactsFound);
        BAIL_ON_LSA_ERROR(dwError);

        dwNumNSSArtefactsWanted -= dwNumNSSArtefactsFound;

        dwError = LsaCoalesceNSSArtefactInfoList(
                        &ppNSSArtefactInfoList,
                        &dwNumNSSArtefactsFound,
                        &ppNSSArtefactInfoList_accumulate,
                        &dwTotalNumNSSArtefactsFound);
        BAIL_ON_LSA_ERROR(dwError);

        if (pMessagePseudo) {
            ldap_msgfree(pMessagePseudo);
            pMessagePseudo = NULL;
        }
    } while (pEnumState->bMorePages && dwNumNSSArtefactsWanted);

    *pppNSSArtefactInfoList = ppNSSArtefactInfoList_accumulate;
    *pdwNumNSSArtefactsFound = dwTotalNumNSSArtefactsFound;

cleanup:

    if (pMessagePseudo) {
        ldap_msgfree(pMessagePseudo);
    }

    LSA_SAFE_FREE_STRING(pszMapType);

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

    if (dwError == LDAP_NO_SUCH_OBJECT)
    {
        dwError = LSA_ERROR_NO_MORE_NSS_ARTEFACTS;
    }

    goto cleanup;
}

DWORD
CellModeNonSchemaEnumNSSArtefacts(
    HANDLE         hDirectory,
    PCSTR          pszCellDN,
    PCSTR          pszNetBIOSDomainName,
    DWORD          dwMapType,
    PAD_ENUM_STATE pEnumState,
    DWORD          dwMaxNumNSSArtefacts,
    PDWORD         pdwNumNSSArtefactsFound,
    PVOID**        pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;
    PVOID* ppNSSArtefactInfoList = NULL;
    DWORD  dwNumNSSArtefactsFound = 0;

    dwError = DefaultModeNonSchemaEnumNSSArtefacts(
                       hDirectory,
                       pszCellDN,
                       pszNetBIOSDomainName,
                       pEnumState,
                       dwMaxNumNSSArtefacts,
                       &dwNumNSSArtefactsFound,
                       &ppNSSArtefactInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    //ToDo: enum linked cell groups

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

    if (dwError == LDAP_NO_SUCH_OBJECT)
    {
        dwError = LSA_ERROR_NO_MORE_NSS_ARTEFACTS;
    }

    goto cleanup;
}

