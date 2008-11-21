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
ADMarshalGroupInfoListDefaultNonSchemaOrCell(
    HANDLE hProvider,
    HANDLE hDirectory,
    LDAPMessage* pMessagePseudo,
    DWORD dwGroupInfoLevel,
    PVOID** pppGroupInfoList,
    PDWORD pdwNumGroups
    )
{
    DWORD dwError = 0;
    PSTR* ppSidsList = NULL;
    DWORD dwNumSidsFound = 0;
    size_t sNumGroupsFound = 0;
    PVOID* ppGroupInfoList = NULL;
    PAD_SECURITY_OBJECT* ppObjects = NULL;
    DWORD iValue  = 0;

    dwError = ADMarshalObjectSidListFromPseudo(
                    hDirectory,
                    pMessagePseudo,
                    &ppSidsList,
                    &dwNumSidsFound);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = AD_FindObjectsBySidList(
                    hProvider,
                    dwNumSidsFound,
                    ppSidsList,
                    &sNumGroupsFound,
                    &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sizeof(*ppGroupInfoList) * sNumGroupsFound,
                    (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    for (iValue = 0; iValue < sNumGroupsFound; iValue++)
    {
        dwError = AD_GroupObjectToGroupInfo(
                     hProvider,
                     ppObjects[iValue],
                     TRUE,
                     dwGroupInfoLevel,
                     &ppGroupInfoList[iValue]);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *pppGroupInfoList = ppGroupInfoList;
    *pdwNumGroups = sNumGroupsFound;

cleanup:
    LsaFreeStringArray(ppSidsList, dwNumSidsFound);
    ADCacheDB_SafeFreeObjectList(sNumGroupsFound, &ppObjects);

    return dwError;

error:
    if (ppGroupInfoList) 
    {
        LsaFreeUserInfoList(dwGroupInfoLevel, ppGroupInfoList, sNumGroupsFound);
    }
    *pppGroupInfoList = NULL;
    *pdwNumGroups = 0;

    goto cleanup;
}

DWORD
ADMarshalGroupInfoListDefaultSchemaOrUnprovision(
    HANDLE hProvider,
    HANDLE hDirectory,
    DWORD dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PCSTR pszDomainDnsName,
    LDAPMessage* pMessageReal,
    DWORD dwGroupInfoLevel,
    PVOID** pppGroupInfoList,
    PDWORD pdwNumGroups
    )
{
    DWORD dwError = 0;
    DWORD dwMessageCount = 0;
    PAD_SECURITY_OBJECT pObject = NULL;
    PVOID* ppGroupInfoList = NULL;
    // Do not free
    LDAPMessage *pGroupMessage = NULL;    
    PSTR pszGroupName = NULL;
    PSTR pszGroupSid = NULL;
    PLSA_LOGIN_NAME_INFO pLoginNameInfo = NULL;
    LDAP *pLd = NULL;
    DWORD dwNumGroups = 0;    

    if (!pMessageReal)
    {
        goto cleanup;
    }

    pLd = LsaLdapGetSession(hDirectory);
    dwMessageCount = ldap_count_entries(
                    pLd,
                    pMessageReal);
    if ((INT)dwMessageCount < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    } else if (dwMessageCount == 0) {
        goto cleanup;
    }    
    
    dwError = LsaAllocateMemory(
                    sizeof(*ppGroupInfoList) * dwMessageCount,
                    (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    pGroupMessage = ldap_first_entry(pLd, pMessageReal);
    while (pGroupMessage)
    {   
        LSA_SAFE_FREE_STRING(pszGroupName);
        dwError = LsaLdapGetString(
                       hDirectory,
                       pGroupMessage,
                       AD_LDAP_SAM_NAME_TAG,
                       &pszGroupName);
        BAIL_ON_LSA_ERROR(dwError);
        
        LSA_SAFE_FREE_STRING(pszGroupSid);
        dwError = ADLdap_GetObjectSid(
                        hDirectory,
                        pGroupMessage,
                        &pszGroupSid);
        BAIL_ON_LSA_ERROR(dwError);
   
        LSA_SAFE_FREE_LOGIN_NAME_INFO(pLoginNameInfo);
        dwError = CreateObjectLoginNameInfo(
                        &pLoginNameInfo,
                        pszDomainDnsName,
                        pszGroupName,
                        pszGroupSid);
        BAIL_ON_LSA_ERROR(dwError);
        
        ADCacheDB_SafeFreeObject(&pObject);
        dwError = ADMarshalToGroupCache(
                    // hPseudoDirectory should be NULL for unprovision mode
                    // hPseudoDirectory should be hRealDirectory for default schem mode
                    dwDirectoryMode == UNPROVISIONED_MODE ? (HANDLE)NULL : hDirectory,
                    hDirectory,
                    dwDirectoryMode,
                    adConfMode,
                    pLoginNameInfo,                    
                    pGroupMessage,
                    // pMessagePseudo should be NULL for unprovision mode
                    // pMessagePseudo should be pRealMessage for default schema mode
                    dwDirectoryMode == UNPROVISIONED_MODE ? NULL : pGroupMessage,
                    &pObject);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = AD_GroupObjectToGroupInfo(
                     hProvider,
                     pObject,                     
                     TRUE,
                     dwGroupInfoLevel,
                     &ppGroupInfoList[dwNumGroups]);        
        BAIL_ON_LSA_ERROR(dwError);
        dwNumGroups++;
        
        pGroupMessage = ldap_next_entry(pLd, pGroupMessage);
    }   
    
cleanup:
    // We handle error here instead of error label
    // because there is a goto cleanup above.
    if (dwError)
    {
        if (ppGroupInfoList) 
        {
            LsaFreeGroupInfoList(dwGroupInfoLevel, ppGroupInfoList, dwNumGroups);
        }
        ppGroupInfoList = NULL;
        dwNumGroups = 0;;
    }
    
    *pppGroupInfoList = ppGroupInfoList;
    *pdwNumGroups = dwNumGroups;
   
    ADCacheDB_SafeFreeObject(&pObject);
    LSA_SAFE_FREE_LOGIN_NAME_INFO(pLoginNameInfo);
    LSA_SAFE_FREE_STRING(pszGroupSid);
    LSA_SAFE_FREE_STRING(pszGroupName);
    
    
    return dwError;

error:
    // Do not actually handle any errors here.
    goto cleanup;
}

DWORD
ADMarshalGroupInfoList(
    IN OPTIONAL HANDLE hProvider,
    IN HANDLE hDirectory,    
    IN OPTIONAL PCSTR pszDomainDnsName,
    IN LSA_AD_MARSHAL_INFO_LIST_MODE InfoListMarshalMode,
    IN LDAPMessage* pMessagePseudo,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID** pppGroupInfoList,
    OUT PDWORD pdwNumGroups
    )
{
    switch (InfoListMarshalMode)
    {
        case LSA_AD_MARSHAL_MODE_DEFAULT_SCHEMA:
            return ADMarshalGroupInfoListDefaultSchemaOrUnprovision(
                     hProvider,
                     hDirectory,
                     DEFAULT_MODE,
                     SchemaMode,
                     pszDomainDnsName,
                     pMessagePseudo,
                     dwGroupInfoLevel,
                     pppGroupInfoList,
                     pdwNumGroups);
            
        case LSA_AD_MARSHAL_MODE_UNPROVISIONED:
            return ADMarshalGroupInfoListDefaultSchemaOrUnprovision(
                     hProvider,
                     hDirectory,
                     UNPROVISIONED_MODE,
                     UnknownMode,
                     pszDomainDnsName,
                     pMessagePseudo,
                     dwGroupInfoLevel,
                     pppGroupInfoList,
                     pdwNumGroups);
            
        case LSA_AD_MARSHAL_MODE_OTHER:
            return ADMarshalGroupInfoListDefaultNonSchemaOrCell(
                     hProvider,
                     hDirectory,    
                     pMessagePseudo,
                     dwGroupInfoLevel,
                     pppGroupInfoList,
                     pdwNumGroups);        
    }
    
    return LSA_ERROR_NOT_SUPPORTED;
}

DWORD
ADMarshalToGroupCache(
    HANDLE                  hPseudoDirectory,
    HANDLE                  hRealDirectory,
    DWORD                   dwDirectoryMode,
    ADConfigurationMode     adConfMode,
    PLSA_LOGIN_NAME_INFO    pGroupNameInfo,
    LDAPMessage*            pMessageReal,
    LDAPMessage*            pMessagePseudo,
    PAD_SECURITY_OBJECT*    ppGroupInfo
    )
{
    DWORD dwError = 0;
    PAD_SECURITY_OBJECT pGroupInfo = NULL;    

    switch (dwDirectoryMode){
    
        case UNPROVISIONED_MODE:
            if (pMessagePseudo){
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError); 
            }
            
            dwError = ADUnprovisionedMarshalToGroupCache(
                            hRealDirectory,
                            pGroupNameInfo,
                            pMessageReal,
                            &pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
        
            break;
        
        case DEFAULT_MODE:
        case CELL_MODE:
            
             switch (adConfMode) 
             {
                 case SchemaMode:

                     dwError = ADSchemaMarshalToGroupCache(
                                   hPseudoDirectory,
                                   hRealDirectory,
                                   pGroupNameInfo,
                                   pMessageReal,
                                   pMessagePseudo,
                                   &pGroupInfo);
                     BAIL_ON_LSA_ERROR(dwError);

                     break;

                 case NonSchemaMode:

                     dwError = ADNonSchemaMarshalToGroupCache(
                                   hPseudoDirectory,
                                   hRealDirectory,
                                   pGroupNameInfo,
                                   pMessageReal,
                                   pMessagePseudo,
                                   &pGroupInfo);
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
    
    *ppGroupInfo = pGroupInfo;   

cleanup:

    return dwError;
    
error:

    *ppGroupInfo = NULL;
    
    ADCacheDB_SafeFreeObject(&pGroupInfo);

    goto cleanup;
}

DWORD
ADMarshalGetCanonicalName(
    PAD_SECURITY_OBJECT     pObject,
    PSTR*                   ppszResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR    pszResult = NULL;

    if(pObject->type == AccountType_Group &&
            !IsNullOrEmptyString(pObject->groupInfo.pszAliasName))
    {
        dwError = LsaAllocateString(
            pObject->groupInfo.pszAliasName,
            &pszResult);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if(pObject->type == AccountType_User &&
            !IsNullOrEmptyString(pObject->userInfo.pszAliasName))
    {
        dwError = LsaAllocateString(
            pObject->userInfo.pszAliasName,
            &pszResult);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LsaAllocateStringPrintf(
            &pszResult,
            "%s%c%s",
            pObject->pszNetbiosDomainName,
            LsaGetDomainSeparator(),
            pObject->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);

        LsaStrCharReplace(
            pszResult,
            ' ',
            AD_GetSpaceReplacement());

        LsaStrnToUpper(
            pszResult,
            strlen(pObject->pszNetbiosDomainName));

        LsaStrToLower(
            pszResult + strlen(pObject->pszNetbiosDomainName) + 1);
    }

    *ppszResult = pszResult;

cleanup:
    return dwError;

error:
    *ppszResult = NULL;
    LSA_SAFE_FREE_STRING(pszResult);
    goto cleanup;
}

DWORD
ADMarshalFromGroupCache(
    PAD_SECURITY_OBJECT     pGroup,
    size_t                  sMembers,
    PAD_SECURITY_OBJECT*    ppMembers,
    DWORD                   dwGroupInfoLevel,
    PVOID*                  ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    /* The variable represents pGroupInfo casted to different types. Do not
     * free these values directly, free pGroupInfo instead.
     */   
    size_t sIndex = 0;
    size_t sEnabled = 0;

    *ppGroupInfo = NULL;

    BAIL_ON_INVALID_POINTER(pGroup);

    if(pGroup->type != AccountType_Group)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (!pGroup->enabled)
    {
        /* Unenabled groups can be represented in cache format, but not in
         * group info format. So when marshalling an unenabled group, pretend
         * like it doesn't exist.
         */
        dwError = LSA_ERROR_OBJECT_NOT_ENABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch(dwGroupInfoLevel)
    {
        case 0:
        {
            PLSA_GROUP_INFO_0 pGroupInfo0 = NULL;

            dwError = LsaAllocateMemory(
                            sizeof(LSA_GROUP_INFO_0),
                            (PVOID*)&pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
            
            pGroupInfo0 = (PLSA_GROUP_INFO_0) pGroupInfo;
            
            pGroupInfo0->gid = pGroup->groupInfo.gid;
            
            dwError = ADMarshalGetCanonicalName(
                            pGroup,
                            &pGroupInfo0->pszName);
            BAIL_ON_LSA_ERROR(dwError);
            
            dwError = LsaAllocateString(
                                pGroup->pszObjectSid,
                                &pGroupInfo0->pszSid);
            BAIL_ON_LSA_ERROR(dwError);
            
            break;
        }
        case 1:
        {
            PLSA_GROUP_INFO_1 pGroupInfo1 = NULL; 

            dwError = LsaAllocateMemory(
                            sizeof(LSA_GROUP_INFO_1),
                            (PVOID*)&pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
            
            pGroupInfo1 = (PLSA_GROUP_INFO_1) pGroupInfo;
     
            pGroupInfo1->gid = pGroup->groupInfo.gid;
            dwError = ADMarshalGetCanonicalName(
                            pGroup,
                            &pGroupInfo1->pszName);
            BAIL_ON_LSA_ERROR(dwError);

            // Optional values use LsaStrDupOrNull. Required values use
            // LsaAllocateString.
            dwError = LsaStrDupOrNull(
                        pGroup->groupInfo.pszPasswd,
                        &pGroupInfo1->pszPasswd);
            BAIL_ON_LSA_ERROR(dwError);        

            dwError = LsaAllocateString(
                                pGroup->pszObjectSid,
                                &pGroupInfo1->pszSid);
            BAIL_ON_LSA_ERROR(dwError);

            for (sIndex = 0; sIndex < sMembers; sIndex++)
            {
                if (ppMembers[sIndex]->enabled)
                {
                    sEnabled++;
                }

                if (ppMembers[sIndex]->type != AccountType_User)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(dwError);
                }
            }

            dwError = LsaAllocateMemory(
                            //Leave room for terminating null pointer
                            sizeof(PSTR) * (sEnabled+1),
                            (PVOID*)&pGroupInfo1->ppszMembers);
            BAIL_ON_LSA_ERROR(dwError);

            sEnabled = 0;

            for (sIndex = 0; sIndex < sMembers; sIndex++)
            {
                if (ppMembers[sIndex]->enabled)
                {
                    dwError = ADMarshalGetCanonicalName(
                            ppMembers[sIndex],
                            &pGroupInfo1->ppszMembers[sEnabled++]);
                    BAIL_ON_LSA_ERROR(dwError);
                }
            }
            
            break;
        }

        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }
    
    *ppGroupInfo = pGroupInfo;
    
cleanup:
    
    return dwError;
    
error:
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
        pGroupInfo = NULL;
    }
    
    *ppGroupInfo = NULL;
    
    goto cleanup;
}

DWORD
ADSchemaMarshalToGroupCache(
    HANDLE                  hPseudoDirectory,
    HANDLE                  hRealDirectory,
    PLSA_LOGIN_NAME_INFO    pGroupNameInfo,
    LDAPMessage*            pMessageReal,
    LDAPMessage*            pMessagePseudo,
    PAD_SECURITY_OBJECT*    ppGroupInfo    
    )
{
    DWORD dwError = 0;
    PAD_SECURITY_OBJECT pGroupInfo = NULL;
    
    struct timeval current_tv;
    UCHAR* pucSIDBytes = NULL;
    DWORD dwSIDByteLength = 0;

    dwError = LsaAllocateMemory(
                    sizeof(AD_SECURITY_OBJECT),
                    (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pGroupInfo->cache.qwCacheId = -1;
    pGroupInfo->cache.tLastUpdated = current_tv.tv_sec;

    pGroupInfo->type = AccountType_Group;

    if (pMessageReal && hRealDirectory != (HANDLE)NULL){
    
        dwError = LsaLdapGetBytes(
                    hRealDirectory,
                    pMessageReal,
                    AD_LDAP_OBJECTSID_TAG,
                    &pucSIDBytes,
                    &dwSIDByteLength);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_POINTER(pucSIDBytes);

        dwError = LsaSidBytesToString(
                    pucSIDBytes,
                    dwSIDByteLength,
                    &pGroupInfo->pszObjectSid);

        dwError = LsaLdapGetString(
                    hRealDirectory,
                    pMessageReal,
                    AD_LDAP_SAM_NAME_TAG,
                    &pGroupInfo->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pGroupInfo->pszSamAccountName);
    
        dwError = LsaAllocateString(
                    pGroupNameInfo->pszDomainNetBiosName,
                    &pGroupInfo->pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaLdapGetDN(
                    hRealDirectory,
                    pMessageReal,
                    &pGroupInfo->pszDN);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else{//at least objectSid is in pGroupNameInfo
        dwError = LsaAllocateString(
                    pGroupNameInfo->pszObjectSid,                    
                    &pGroupInfo->pszObjectSid);
        BAIL_ON_LSA_ERROR(dwError);     
        
        dwError = LsaAllocateString(
                    pGroupNameInfo->pszDomainNetBiosName,
                    &pGroupInfo->pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaAllocateString(
                    pGroupNameInfo->pszName,
                    &pGroupInfo->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pGroupInfo->pszSamAccountName);
    }
    
    if (pMessagePseudo){
        pGroupInfo->enabled = TRUE;
    
        dwError = LsaLdapGetUInt32(
                    hPseudoDirectory,
                    pMessagePseudo,
                    AD_LDAP_GID_TAG,
                    (PDWORD)&pGroupInfo->groupInfo.gid);
        if(dwError == LSA_ERROR_INVALID_LDAP_ATTR_VALUE)
        {
            pGroupInfo->enabled = FALSE;
            dwError = LSA_ERROR_SUCCESS;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    if (pGroupInfo->enabled)
    {
        dwError = LsaLdapGetString(
                    hPseudoDirectory,
                    pMessagePseudo,
                    AD_LDAP_DISPLAY_NAME_TAG,
                    &pGroupInfo->groupInfo.pszAliasName
                    );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapGetString(
                    hPseudoDirectory,
                    pMessagePseudo,
                    AD_LDAP_PASSWD_TAG,
                    &pGroupInfo->groupInfo.pszPasswd
                    );
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppGroupInfo = pGroupInfo;
    
cleanup:
    LSA_SAFE_FREE_MEMORY(pucSIDBytes);

    return dwError;
    
error:

    *ppGroupInfo = NULL;
    
    ADCacheDB_SafeFreeObject(&pGroupInfo);

    goto cleanup;
}

DWORD
ADNonSchemaMarshalToGroupCache(
    HANDLE                  hPseudoDirectory,
    HANDLE                  hRealDirectory,    
    PLSA_LOGIN_NAME_INFO    pGroupNameInfo,
    LDAPMessage*            pMessageReal,
    LDAPMessage*            pMessagePseudo,
    PAD_SECURITY_OBJECT*    ppGroupInfo    
    )
{
    DWORD dwError = 0;
    PAD_SECURITY_OBJECT pGroupInfo = NULL;    
    DWORD dwNumValues = 0;
    struct timeval current_tv;
    PSTR* ppszValues = NULL;
    UCHAR* pucSIDBytes = NULL;
    DWORD dwSIDByteLength = 0;

    dwError = LsaAllocateMemory(
                    sizeof(AD_SECURITY_OBJECT),
                    (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pGroupInfo->cache.qwCacheId = -1;
    pGroupInfo->cache.tLastUpdated = current_tv.tv_sec;

    pGroupInfo->type = AccountType_Group;

    if (pMessageReal && hRealDirectory != (HANDLE)NULL){
    
        dwError = LsaLdapGetBytes(
                    hRealDirectory,
                    pMessageReal,
                    AD_LDAP_OBJECTSID_TAG,
                    &pucSIDBytes,
                    &dwSIDByteLength);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_POINTER(pucSIDBytes);

        dwError = LsaSidBytesToString(
                    pucSIDBytes,
                    dwSIDByteLength,
                    &pGroupInfo->pszObjectSid);

        dwError = LsaLdapGetString(
                    hRealDirectory,
                    pMessageReal,
                    AD_LDAP_SAM_NAME_TAG,
                    &pGroupInfo->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pGroupInfo->pszSamAccountName);
    
        dwError = LsaAllocateString(
                    pGroupNameInfo->pszDomainNetBiosName,
                    &pGroupInfo->pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);      
        
        dwError = LsaLdapGetDN(
                    hRealDirectory,
                    pMessageReal,
                    &pGroupInfo->pszDN);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else{//at least objectSid is in pGroupNameInfo
        dwError = LsaAllocateString(
                    pGroupNameInfo->pszObjectSid,                    
                    &pGroupInfo->pszObjectSid);
        BAIL_ON_LSA_ERROR(dwError);     
        
        dwError = LsaAllocateString(
                    pGroupNameInfo->pszDomainNetBiosName,
                    &pGroupInfo->pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaAllocateString(
                    pGroupNameInfo->pszName,
                    &pGroupInfo->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pGroupInfo->pszSamAccountName);      
    }
    
    if (pMessagePseudo){
        pGroupInfo->enabled = TRUE;

        dwError = LsaLdapGetStrings(
                         hPseudoDirectory,
                         pMessagePseudo,
                         AD_LDAP_KEYWORDS_TAG,
                         &ppszValues,
                         &dwNumValues);
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = ADNonSchemaKeywordGetUInt32(
                    ppszValues,
                    dwNumValues,
                    AD_LDAP_GID_TAG,
                    (DWORD*)&pGroupInfo->groupInfo.gid);
        BAIL_ON_LSA_ERROR(dwError);    

        dwError = ADNonSchemaKeywordGetString(
                    ppszValues,
                    dwNumValues,
                    AD_LDAP_DISPLAY_NAME_TAG,
                    &pGroupInfo->groupInfo.pszAliasName
                    );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADNonSchemaKeywordGetString(
                    ppszValues,
                    dwNumValues,
                    AD_LDAP_PASSWD_TAG,
                    &pGroupInfo->groupInfo.pszPasswd
                    );
        BAIL_ON_LSA_ERROR(dwError);
        
    }
    
    *ppGroupInfo = pGroupInfo;    
    
cleanup:
    LSA_SAFE_FREE_MEMORY(pucSIDBytes);
    LsaFreeStringArray(ppszValues, dwNumValues);

    return dwError;
    
error:

    *ppGroupInfo = NULL;    
    
    ADCacheDB_SafeFreeObject(&pGroupInfo);

    goto cleanup;
}

DWORD
ADUnprovisionedMarshalToGroupCache(
    HANDLE                  hDirectory,
    PLSA_LOGIN_NAME_INFO    pGroupNameInfo,
    LDAPMessage*            pMessage,
    PAD_SECURITY_OBJECT*    ppGroupInfo
    )
{
    DWORD dwError = 0;
    PAD_SECURITY_OBJECT pGroupInfo = NULL;
    UCHAR* pucSIDBytes = NULL;
    DWORD dwSIDByteLength = 0;
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier = NULL;    
    struct timeval current_tv;
    
    if ((hDirectory == (HANDLE)NULL && pMessage) ||
        (hDirectory != (HANDLE)NULL && !pMessage))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    BAIL_ON_INVALID_POINTER(pGroupNameInfo);

    dwError = LsaAllocateMemory(
                    sizeof(*pGroupInfo),
                    (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    pGroupInfo->cache.qwCacheId = -1;
    pGroupInfo->cache.tLastUpdated = current_tv.tv_sec;

    pGroupInfo->type = AccountType_Group;

    pGroupInfo->enabled = TRUE;
    
    if (hDirectory == (HANDLE)NULL && !pMessage)
    {
        dwError = ADUnprovisionedMarshalToGroupCacheInOneWayTrust(
                                 pGroupNameInfo,
                                 &pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        goto cleanup;
    }
    
    dwError = LsaLdapGetString(
                hDirectory,
                pMessage,
                AD_LDAP_SAM_NAME_TAG,
                &pGroupInfo->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pGroupInfo->pszSamAccountName);

    pGroupInfo->groupInfo.pszAliasName = NULL;
    pGroupInfo->groupInfo.pszPasswd = NULL;

    dwError = LsaAllocateString(
                pGroupNameInfo->pszDomainNetBiosName,
                &pGroupInfo->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaLdapGetDN(
                hDirectory,
                pMessage,
                &pGroupInfo->pszDN);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaLdapGetBytes(
                hDirectory,
                pMessage,
                AD_LDAP_OBJECTSID_TAG,
                &pucSIDBytes,
                &dwSIDByteLength);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_POINTER(pucSIDBytes);

    dwError = LsaAllocSecurityIdentifierFromBinary(
        pucSIDBytes,
        dwSIDByteLength,
        &pSecurityIdentifier);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = LsaGetSecurityIdentifierString(
                pSecurityIdentifier,
                &pGroupInfo->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);
    
    //Get the GID from objectSID.    
    dwError = LsaGetSecurityIdentifierHashedRid(
        pSecurityIdentifier,
        (PDWORD)&pGroupInfo->groupInfo.gid);
    BAIL_ON_LSA_ERROR(dwError);
    
cleanup:
    *ppGroupInfo = pGroupInfo;

    if (pSecurityIdentifier)
    {
        LsaFreeSecurityIdentifier(pSecurityIdentifier);
    }
    
    LSA_SAFE_FREE_MEMORY(pucSIDBytes);
    
    return dwError;
    
error:
    // Output parameter has been explicitly set in cleanup due to use of "goto cleanup"    
    
    ADCacheDB_SafeFreeObject(&pGroupInfo);

    goto cleanup;
}

DWORD
ADUnprovisionedMarshalToGroupCacheInOneWayTrust(
    PLSA_LOGIN_NAME_INFO    pGroupNameInfo,
    PAD_SECURITY_OBJECT*    ppGroupInfo
    )
{
    DWORD dwError = 0;
    PAD_SECURITY_OBJECT pGroupInfo = NULL;    
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier = NULL;    
    struct timeval current_tv;    
    
    dwError = LsaAllocateMemory(
                    sizeof(*pGroupInfo),
                    (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    pGroupInfo->cache.qwCacheId = -1;
    pGroupInfo->cache.tLastUpdated = current_tv.tv_sec;

    pGroupInfo->type = AccountType_Group;

    pGroupInfo->enabled = TRUE;
    
    //Group SamAccountName (generate using group name)
    dwError = LsaAllocateString(
                    pGroupNameInfo->pszName,
                    &pGroupInfo->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    pGroupInfo->groupInfo.pszAliasName = NULL;
    pGroupInfo->groupInfo.pszPasswd = NULL;

    dwError = LsaAllocateString(
                pGroupNameInfo->pszDomainNetBiosName,
                &pGroupInfo->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);
    
    pGroupInfo->pszDN = NULL;
    
    //ObjectSid
    dwError = LsaAllocateString(
                pGroupNameInfo->pszObjectSid,
                &pGroupInfo->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);
    
    //GID
    dwError = LsaAllocSecurityIdentifierFromString(
                    pGroupNameInfo->pszObjectSid,
                    &pSecurityIdentifier);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaGetSecurityIdentifierHashedRid(
                    pSecurityIdentifier,
                   (PDWORD)&pGroupInfo->groupInfo.gid);
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppGroupInfo = pGroupInfo;    
    
cleanup:

    if (pSecurityIdentifier)
    {
        LsaFreeSecurityIdentifier(pSecurityIdentifier);
    }
    
    return dwError;
    
error:

    *ppGroupInfo = NULL;
    
    ADCacheDB_SafeFreeObject(&pGroupInfo);

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
