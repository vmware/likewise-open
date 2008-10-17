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
ADMarshalGroupInfo(
    HANDLE              hDirectory,
    DWORD               dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PCSTR               pszNetBIOSDomainName,
    LDAPMessage*        pMessageReal,
    LDAPMessage*        pMessagePseudo,
    DWORD               dwGroupInfoLevel,
    PVOID*              ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;

    switch (dwDirectoryMode){
    
        case UNPROVISIONED_MODE:
            if (pMessagePseudo){
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError); 
            }
            
            dwError = ADUnprovisionedMarshalGroupInfo(
                            hDirectory,
                            pszNetBIOSDomainName,
                            pMessageReal,
                            dwGroupInfoLevel,
                            &pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
        
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

                     dwError = ADSchemaMarshalGroupInfo(
                                   hDirectory,
                                   pszNetBIOSDomainName,
                                   pMessageReal,
                                   pMessagePseudo,
                                   dwGroupInfoLevel,
                                   &pGroupInfo);
                     BAIL_ON_LSA_ERROR(dwError);

                     break;

                 case NonSchemaMode:

                     dwError = ADNonSchemaMarshalGroupInfo(
                                   hDirectory,
                                   pszNetBIOSDomainName,
                                   pMessageReal,
                                   pMessagePseudo,
                                   dwGroupInfoLevel,
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
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }        

    goto cleanup;
}

DWORD
ADMarshalToGroupCache(
    HANDLE                  hDirectory,
    DWORD                   dwDirectoryMode,
    ADConfigurationMode     adConfMode,
    PCSTR                   pszNetBIOSDomainName,
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
                            hDirectory,
                            pszNetBIOSDomainName,
                            pMessageReal,
                            &pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
        
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

                     dwError = ADSchemaMarshalToGroupCache(
                                   hDirectory,
                                   pszNetBIOSDomainName,
                                   pMessageReal,
                                   pMessagePseudo,
                                   &pGroupInfo);
                     BAIL_ON_LSA_ERROR(dwError);

                     break;

                 case NonSchemaMode:

                     dwError = ADNonSchemaMarshalToGroupCache(
                                   hDirectory,
                                   pszNetBIOSDomainName,
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
ADMarshalToGroupCacheEx(
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
                            pGroupNameInfo->pszDomainNetBiosName,
                            pMessageReal,
                            &pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
        
            break;
        
        case DEFAULT_MODE:
        case CELL_MODE:
            
             switch (adConfMode) 
             {
                 case SchemaMode:

                     dwError = ADSchemaMarshalToGroupCacheEx(
                                   hPseudoDirectory,
                                   hRealDirectory,
                                   pGroupNameInfo,
                                   pMessageReal,
                                   pMessagePseudo,
                                   &pGroupInfo);
                     BAIL_ON_LSA_ERROR(dwError);

                     break;

                 case NonSchemaMode:

                     dwError = ADNonSchemaMarshalToGroupCacheEx(
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
            "%s\\%s",
            pObject->pszNetbiosDomainName,
            pObject->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);

        LsaStrCharReplace(
            pszResult,
            ' ',
            AD_GetSeparator());

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
    HANDLE                  hDirectory,
    PCSTR                   pszNetBIOSDomainName,
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

    if (pMessageReal){
    
        dwError = LsaLdapGetBytes(
                    hDirectory,
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
                    hDirectory,
                    pMessageReal,
                    AD_LDAP_SAM_NAME_TAG,
                    &pGroupInfo->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pGroupInfo->pszSamAccountName);
    
        dwError = LsaAllocateString(
                    pszNetBIOSDomainName,
                    &pGroupInfo->pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = LsaLdapGetDN(
                    hDirectory,
                    pMessageReal,
                    &pGroupInfo->pszDN);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (pMessagePseudo){
        pGroupInfo->enabled = TRUE;
    
        dwError = LsaLdapGetUInt32(
                    hDirectory,
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
                    hDirectory,
                    pMessagePseudo,
                    AD_LDAP_DISPLAY_NAME_TAG,
                    &pGroupInfo->groupInfo.pszAliasName
                    );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapGetString(
                    hDirectory,
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
ADSchemaMarshalToGroupCacheEx(
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
ADSchemaMarshalGroupInfo_0(
    HANDLE       hDirectory,
    PCSTR        pszNetBIOSDomainName,
    LDAPMessage* pMessageReal,
    LDAPMessage* pMessagePseudo,
    PVOID*       ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 0;
    PSTR  pszGroupName = NULL;
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_GROUP_INFO_0),
                    (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (pMessagePseudo) {
    
        dwError = LsaLdapGetUInt32(
                    hDirectory,
                    pMessagePseudo,
                    AD_LDAP_GID_TAG,
                    (PDWORD)&pGroupInfo->gid);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (pMessageReal) {

            dwError = LsaLdapGetString(
                        hDirectory,
                        pMessageReal,
                        AD_LDAP_SAM_NAME_TAG,
                        &pszGroupName);
            BAIL_ON_LSA_ERROR(dwError);

            BAIL_ON_INVALID_STRING(pszGroupName);
        
            dwError = ADGetDomainQualifiedString(
                        pszNetBIOSDomainName,
                        pszGroupName,
                        &pGroupInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
            
            LsaStrCharReplace(pGroupInfo->pszName, ' ', AD_GetSeparator());
    }
    
    *ppGroupInfo = (PVOID)pGroupInfo;
    
cleanup:

    LSA_SAFE_FREE_STRING(pszGroupName);

    return dwError;
    
error:

    *ppGroupInfo = NULL;
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
ADSchemaMarshalGroupInfo_1(
    HANDLE       hDirectory,
    PCSTR        pszNetBIOSDomainName,
    LDAPMessage* pMessageReal,
    LDAPMessage* pMessagePseudo,
    PVOID*       ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1 pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 1;
    DWORD dwNumGroups = 0;
    PSTR  pszGroupName = NULL;
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_GROUP_INFO_1),
                    (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (pMessagePseudo) {
    
        dwError = LsaLdapGetUInt32(
                    hDirectory,
                    pMessagePseudo,
                    AD_LDAP_GID_TAG,
                    (PDWORD)&pGroupInfo->gid);
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = LsaLdapGetString(
                    hDirectory,
                    pMessagePseudo,
                    AD_LDAP_PASSWD_TAG,
                    &pGroupInfo->pszPasswd);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (pMessageReal) {

        dwError = LsaLdapGetString(
                    hDirectory,
                    pMessageReal,
                    AD_LDAP_SAM_NAME_TAG,
                    &pszGroupName);
        BAIL_ON_LSA_ERROR(dwError);

        BAIL_ON_INVALID_STRING(pszGroupName);
    
        dwError = ADGetDomainQualifiedString(
                    pszNetBIOSDomainName,
                    pszGroupName,
                    &pGroupInfo->pszName);
        BAIL_ON_LSA_ERROR(dwError);
        
        LsaStrCharReplace(pGroupInfo->pszName, ' ', AD_GetSeparator());
        
        dwError = ADGetGroupMembers(
                    hDirectory,
                    pMessageReal,                    
                    pszNetBIOSDomainName,
                    gpADProviderData->szDomain,
                    &pGroupInfo->ppszMembers,
                    &dwNumGroups);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppGroupInfo = (PVOID)pGroupInfo;
    
cleanup:

    LSA_SAFE_FREE_STRING(pszGroupName);

    return dwError;
    
error:

    *ppGroupInfo = NULL;
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
ADSchemaMarshalGroupInfo(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,
    LDAPMessage *pMessageReal,
    LDAPMessage *pMessagePseudo,
    DWORD       dwGroupInfoLevel,
    PVOID*      ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    
    switch(dwGroupInfoLevel)
    {
        case 0:
            dwError = ADSchemaMarshalGroupInfo_0(
                            hDirectory,
                            pszNetBIOSDomainName,
                            pMessageReal,
                            pMessagePseudo,
                            &pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case 1:
            dwError = ADSchemaMarshalGroupInfo_1(
                            hDirectory,
                            pszNetBIOSDomainName,
                            pMessageReal,
                            pMessagePseudo,
                            &pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }
    
    *ppGroupInfo = pGroupInfo;
    
cleanup:

    return dwError;
    
error:

    *ppGroupInfo = NULL;
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
ADNonSchemaMarshalToGroupCache(
    HANDLE                  hDirectory,
    PCSTR                   pszNetBIOSDomainName,
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

    if (pMessageReal){
    
        dwError = LsaLdapGetBytes(
                    hDirectory,
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
                    hDirectory,
                    pMessageReal,
                    AD_LDAP_SAM_NAME_TAG,
                    &pGroupInfo->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pGroupInfo->pszSamAccountName);
    
        dwError = LsaAllocateString(
                    pszNetBIOSDomainName,
                    &pGroupInfo->pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);      
        
        dwError = LsaLdapGetDN(
                    hDirectory,
                    pMessageReal,
                    &pGroupInfo->pszDN);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (pMessagePseudo){
        pGroupInfo->enabled = TRUE;

        dwError = LsaLdapGetStrings(hDirectory,
                               pMessagePseudo,
                               AD_LDAP_KEYWORDS_TAG,
                               &ppszValues,
                               &dwNumValues);
        BAIL_ON_LSA_ERROR(dwError);
    
        dwError = ADNonSchemaKeywordGetUInt32(
                    ppszValues,
                    dwNumValues,
                    AD_LDAP_GID_TAG,
                    (PDWORD)&pGroupInfo->groupInfo.gid);
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
ADNonSchemaMarshalToGroupCacheEx(
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
ADNonSchemaMarshalGroupInfo_0(
    HANDLE       hDirectory,
    PCSTR        pszNetBIOSDomainName,
    LDAPMessage* pMessageReal,
    LDAPMessage* pMessagePseudo,
    PVOID*       ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 0;
    PSTR  pszGroupName = NULL;
    
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    DWORD i = 0;   
    
    dwError = LsaAllocateMemory(
                      sizeof(LSA_GROUP_INFO_0),
                      (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);    
    
    if (pMessagePseudo){    
        dwError = LsaLdapGetStrings(
                    hDirectory,
                    pMessagePseudo,
                    AD_LDAP_KEYWORDS_TAG,
                    &ppszValues,
                    &dwNumValues);
        BAIL_ON_LSA_ERROR(dwError);
        
        for (i = 0; i < dwNumValues; i++)
        {
            PSTR pszValue = NULL;
        
            if (!strncasecmp(ppszValues[i], "gidNumber=", sizeof("gidNumber=")-1))
            {
                pszValue = ppszValues[i] + sizeof("gidNumber=") - 1;
            
                if (!IsNullOrEmptyString(pszValue)){
                    pGroupInfo->gid = atoi(pszValue);
                 }
            }
         }
    }
    
    if (pMessageReal) {
        dwError = LsaLdapGetString(
                    hDirectory,
                    pMessageReal,
                    AD_LDAP_SAM_NAME_TAG,
                    &pszGroupName);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pszGroupName);
    
        dwError = ADGetDomainQualifiedString(
                    pszNetBIOSDomainName,
                    pszGroupName,
                    &pGroupInfo->pszName);
        BAIL_ON_LSA_ERROR(dwError);
        
        LsaStrCharReplace(pGroupInfo->pszName, ' ', AD_GetSeparator());
    }
    
    *ppGroupInfo = (PVOID)pGroupInfo;
    
cleanup:

    LSA_SAFE_FREE_STRING(pszGroupName);

    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }

    return dwError;
    
error:

    *ppGroupInfo = NULL;
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
ADNonSchemaMarshalGroupInfo_1(
    HANDLE       hDirectory,
    PCSTR        pszNetBIOSDomainName,
    LDAPMessage* pMessageReal,
    LDAPMessage* pMessagePseudo,
    PVOID*       ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1 pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 1;
    DWORD dwNumGroups = 0;
    PSTR  pszGroupName = NULL;
    
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    DWORD i = 0;   
    
    dwError = LsaAllocateMemory(
                      sizeof(LSA_GROUP_INFO_1),
                      (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);    
    
    if (pMessagePseudo){    
        dwError = LsaLdapGetStrings(
                    hDirectory,
                    pMessagePseudo,
                    AD_LDAP_KEYWORDS_TAG,
                    &ppszValues,
                    &dwNumValues);
        BAIL_ON_LSA_ERROR(dwError);
        
        for (i = 0; i < dwNumValues; i++)
        {
            PSTR pszValue = NULL;
        
            if (!strncasecmp(ppszValues[i], "gidNumber=", sizeof("gidNumber=")-1))
            {
                pszValue = ppszValues[i] + sizeof("gidNumber=") - 1;
            
                if (!IsNullOrEmptyString(pszValue)){
                    pGroupInfo->gid = atoi(pszValue);
                 }
            }
         }
    }
    
    if (pMessageReal){
        dwError = LsaLdapGetString(
                    hDirectory,
                    pMessageReal,
                    AD_LDAP_SAM_NAME_TAG,
                    &pszGroupName);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pszGroupName);
    
        dwError = ADGetDomainQualifiedString(
                    pszNetBIOSDomainName,
                    pszGroupName,
                    &pGroupInfo->pszName);
        BAIL_ON_LSA_ERROR(dwError);
        
        LsaStrCharReplace(pGroupInfo->pszName, ' ', AD_GetSeparator());
        
        dwError = ADGetGroupMembers(
                    hDirectory,
                    pMessageReal,                    
                    pszNetBIOSDomainName,
                    gpADProviderData->szDomain,
                    &pGroupInfo->ppszMembers,
                    &dwNumGroups);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppGroupInfo = (PVOID)pGroupInfo;
    
cleanup:

    LSA_SAFE_FREE_STRING(pszGroupName);

    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }

    return dwError;
    
error:

    *ppGroupInfo = NULL;
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
ADNonSchemaMarshalGroupInfo(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,
    LDAPMessage *pMessageReal,
    LDAPMessage *pMessagePseudo,
    DWORD       dwGroupInfoLevel,
    PVOID*      ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    
    switch(dwGroupInfoLevel)
    {
        case 0:
            dwError = ADNonSchemaMarshalGroupInfo_0(
                            hDirectory,
                            pszNetBIOSDomainName,
                            pMessageReal,
                            pMessagePseudo,
                            &pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case 1:
            dwError = ADNonSchemaMarshalGroupInfo_1(
                            hDirectory,
                            pszNetBIOSDomainName,
                            pMessageReal,
                            pMessagePseudo,
                            &pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }
    
    *ppGroupInfo = pGroupInfo;
    
cleanup:

    return dwError;
    
error:

    *ppGroupInfo = NULL;
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
ADUnprovisionedMarshalToGroupCache(
    HANDLE                  hDirectory,
    PCSTR                   pszNetBIOSDomainName,
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
    
    if (hDirectory == (HANDLE)NULL){
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

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

    pGroupInfo->enabled = TRUE;
    
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
                gpADProviderData->szShortDomain,
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
    
    *ppGroupInfo = pGroupInfo;    
    
cleanup:

    if (pSecurityIdentifier)
    {
        LsaFreeSecurityIdentifier(pSecurityIdentifier);
    }
    
    LSA_SAFE_FREE_MEMORY(pucSIDBytes);
    
    return dwError;
    
error:

    *ppGroupInfo = NULL;
    
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


DWORD
ADUnprovisionedMarshalGroupInfo_0(
    HANDLE       hDirectory,
    PCSTR        pszNetBIOSDomainName,
    LDAPMessage* pMessage,
    PVOID*       ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 0;
    PSTR  pszGroupName = NULL;
    
    UCHAR* pucSIDBytes = NULL;
    DWORD dwSIDByteLength = 0;
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier = NULL;
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_GROUP_INFO_0),
                    (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_SAM_NAME_TAG,
                    &pszGroupName);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pszGroupName);
    
    dwError = ADGetDomainQualifiedString(
                    pszNetBIOSDomainName,
                    pszGroupName,
                    &pGroupInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaStrCharReplace(pGroupInfo->pszName, ' ', AD_GetSeparator());
    
    //Get the GID from objectSID.
    dwError = LsaLdapGetBytes(
                hDirectory,
                pMessage,
                AD_LDAP_OBJECTSID_TAG,
                &pucSIDBytes,
                &dwSIDByteLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocSecurityIdentifierFromBinary(
                pucSIDBytes,
                dwSIDByteLength,
                &pSecurityIdentifier);
    BAIL_ON_LSA_ERROR(dwError);  
    
    dwError = LsaGetSecurityIdentifierHashedRid(
                pSecurityIdentifier,
                (PDWORD)&(((PLSA_GROUP_INFO_0)pGroupInfo)->gid));
    BAIL_ON_LSA_ERROR(dwError);

    *ppGroupInfo = (PVOID)pGroupInfo;
    
cleanup:

    if(pSecurityIdentifier)
    {
        LsaFreeSecurityIdentifier(pSecurityIdentifier);
    }
    
    LSA_SAFE_FREE_MEMORY(pucSIDBytes);
    LSA_SAFE_FREE_STRING(pszGroupName);

    return dwError;
    
error:

    *ppGroupInfo = NULL;
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
ADUnprovisionedMarshalGroupInfo_1(
    HANDLE       hDirectory,
    PCSTR        pszNetBIOSDomainName,
    LDAPMessage* pMessage,
    PVOID*       ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1 pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 1;
    DWORD dwNumGroups = 0;
    PSTR  pszGroupName = NULL;
    
    UCHAR* pucSIDBytes = NULL;
    DWORD dwSIDByteLength = 0;
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier = NULL;
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_GROUP_INFO_1),
                    (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaLdapGetString(
                    hDirectory,
                    pMessage,
                    AD_LDAP_SAM_NAME_TAG,
                    &pszGroupName);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pszGroupName);
    
    dwError = ADGetDomainQualifiedString(
                    pszNetBIOSDomainName,
                    pszGroupName,
                    &pGroupInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaStrCharReplace(pGroupInfo->pszName, ' ', AD_GetSeparator());

    dwError = ADGetGroupMembers(
                hDirectory,
                pMessage,                    
                pszNetBIOSDomainName,
                gpADProviderData->szDomain,
                &pGroupInfo->ppszMembers,
                &dwNumGroups);
    BAIL_ON_LSA_ERROR(dwError);
    
    //Get the GID from objectSID.
    dwError = LsaLdapGetBytes(
                hDirectory,
                pMessage,
                AD_LDAP_OBJECTSID_TAG,
                &pucSIDBytes,
                &dwSIDByteLength);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocSecurityIdentifierFromBinary(
                pucSIDBytes,
                dwSIDByteLength,
                &pSecurityIdentifier);
    BAIL_ON_LSA_ERROR(dwError);  
    
    dwError = LsaGetSecurityIdentifierHashedRid(
                pSecurityIdentifier,
                (PDWORD)&(((PLSA_GROUP_INFO_1)pGroupInfo)->gid));
    BAIL_ON_LSA_ERROR(dwError);

    *ppGroupInfo = (PVOID)pGroupInfo;
    
cleanup:

    if(pSecurityIdentifier)
    {
        LsaFreeSecurityIdentifier(pSecurityIdentifier);
    }
    LSA_SAFE_FREE_MEMORY(pucSIDBytes);
    LSA_SAFE_FREE_STRING(pszGroupName);

    return dwError;
    
error:

    *ppGroupInfo = NULL;
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
ADUnprovisionedMarshalGroupInfo(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,
    LDAPMessage *pMessage,
    DWORD       dwGroupInfoLevel,
    PVOID*      ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    
    switch(dwGroupInfoLevel)
    {
        case 0:
            dwError = ADUnprovisionedMarshalGroupInfo_0(
                hDirectory, 
                pszNetBIOSDomainName, 
                pMessage, 
                &pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case 1:
            dwError = ADUnprovisionedMarshalGroupInfo_1(
                hDirectory, 
                pszNetBIOSDomainName, 
                pMessage, 
                &pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }
    
    *ppGroupInfo = pGroupInfo;
    
cleanup:

    return dwError;
    
error:

    *ppGroupInfo = NULL;
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
ADSchemaMarshalGroupInfoList_0(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,    
    LDAPMessage *pMessagePseudo,
    PVOID**     pppGroupInfoList,
    PDWORD      pwdNumGroups
    )    
{
    
    DWORD dwError = 0;
    PLSA_GROUP_INFO_0* ppGroupInfoList = NULL;
    PLSA_GROUP_INFO_0  pGroupInfo = NULL;
    DWORD iGroup = 0;
    DWORD nGroup = 0;
    DWORD dwGroupInfoLevel = 0;
    // Do not free
    LDAPMessage *pGroupMessage = NULL;
    LDAPMessage *pGroupMessageReal = NULL;
    PSTR pszGroupName = NULL;
    LDAP *pLd = LsaLdapGetSession(hDirectory);
    
    PSTR  pszObjectSID = NULL;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    PSTR pszDirectoryRoot = NULL;
    CHAR szQuery[1024];    
        
    PSTR szAttributeListName[] = 
                    {AD_LDAP_SAM_NAME_TAG,
                     AD_LDAP_UPN_TAG,
                     AD_LDAP_MEMBER_TAG,
                     NULL
                    };   

    if (!pMessagePseudo)
    { 
        goto done;
    }
    
    nGroup = ldap_count_entries(
                    pLd, 
                    pMessagePseudo);
    if (nGroup < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
    } else if (nGroup == 0) {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaAllocateMemory(
                    sizeof(PLSA_GROUP_INFO_0) * nGroup,
                    (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);
       
    pGroupMessage = ldap_first_entry(
                           pLd,
                           pMessagePseudo);
    BAIL_ON_LSA_ERROR(dwError);   
           
    while (pGroupMessage)
    {
        BOOLEAN bValidADEntry = false;
        
        dwError = LsaLdapIsValidADEntry(
                        hDirectory,
                        pGroupMessage,
                        &bValidADEntry);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (!bValidADEntry){
            dwError = LSA_ERROR_LDAP_FAILED_GETDN;
            BAIL_ON_LSA_ERROR(dwError);            
        } 
        
        dwError = LsaLdapGetString(
                       hDirectory,
                       pGroupMessage,
                       AD_LDAP_SAM_NAME_TAG,
                       &pszGroupName);
        BAIL_ON_LSA_ERROR(dwError);
           
        //if group's Pseudo attribute is stored in the real group object(default schema mode)
        if (!IsNullOrEmptyString(pszGroupName)){               

            dwError = ADSchemaMarshalGroupInfo(
                            hDirectory, 
                            pszNetBIOSDomainName, 
                            pGroupMessage, 
                            pGroupMessage, 
                            dwGroupInfoLevel, 
                            (PVOID*)&pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
            
            *(ppGroupInfoList + iGroup++) = pGroupInfo;
            pGroupInfo = NULL;
        }
        else
        {
            // Otherwise, use group backlink to locate the real group object
            // and grab the sAMAccountName etc. attributes
            
            DWORD iValue = 0;
            DWORD dwCount = 0;
            
            dwError = LsaLdapGetStrings(
                            hDirectory,
                            pGroupMessage,
                            AD_LDAP_KEYWORDS_TAG,
                            &ppszValues,
                            &dwNumValues);
            BAIL_ON_LSA_ERROR(dwError);
               
            for ( iValue = 0; iValue < dwNumValues; iValue++)
            {
                 if (!strncasecmp(ppszValues[iValue],
                                  "backLink=",
                                  sizeof("backLink=")-1))
                 {
                     pszObjectSID = ppszValues[iValue] + sizeof("backLink=") - 1;           
                     break;
                 }        
            }
               
            if (IsNullOrEmptyString(pszObjectSID)) {
                dwError = LSA_ERROR_INVALID_SID;
                BAIL_ON_LSA_ERROR(dwError);
            }
               
            sprintf(szQuery, "(objectSid=%s)", pszObjectSID);    

            if (ppszValues) {
                LsaFreeStringArray(ppszValues, dwNumValues);
                ppszValues = NULL;
                dwNumValues = 0;
            }

            dwError = LsaLdapConvertDomainToDN(
                            gpADProviderData->szDomain,
                            &pszDirectoryRoot);
            BAIL_ON_LSA_ERROR(dwError);
               
            dwError = LsaLdapDirectorySearch(
                                  hDirectory,
                                  pszDirectoryRoot,
                                  LDAP_SCOPE_SUBTREE,
                                  szQuery,
                                  szAttributeListName,
                                  &pGroupMessageReal);
            BAIL_ON_LSA_ERROR(dwError);

            dwCount = ldap_count_entries(
                                  pLd,
                                  pGroupMessageReal);               
            if (dwCount < 0) {
                dwError = LSA_ERROR_LDAP_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            } else if (dwCount == 0) {
                //this means the current user is an "orphan"
                dwError = 0;
            } else if (dwCount > 1) {
                dwError = LSA_ERROR_DUPLICATE_GROUPNAME;
                BAIL_ON_LSA_ERROR(dwError);
            } else {
                  
                  dwError = ADSchemaMarshalGroupInfo(
                                  hDirectory,
                                  pszNetBIOSDomainName,
                                  pGroupMessageReal,
                                  pGroupMessage,
                                  dwGroupInfoLevel,
                                  (PVOID*)&pGroupInfo);
                  BAIL_ON_LSA_ERROR(dwError);
                  
                  *(ppGroupInfoList + iGroup++) = pGroupInfo;               
                  pGroupInfo = NULL;
           }
            
           if (pGroupMessageReal) {
               ldap_msgfree(pGroupMessageReal);
               pGroupMessageReal = NULL;
           }

           LSA_SAFE_FREE_STRING(pszDirectoryRoot);
       }               
       pGroupMessage = ldap_next_entry(
                                     pLd, 
                                     pGroupMessage);     
    }
    
done:
    
    *pppGroupInfoList = (PVOID*)ppGroupInfoList;
    *pwdNumGroups = iGroup;
    
cleanup:
    
    if (pGroupMessageReal) {
        ldap_msgfree(pGroupMessageReal);    
    }
    
    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }

    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    LSA_SAFE_FREE_STRING(pszGroupName);
    
    return dwError;

error:

    *pppGroupInfoList = NULL;
    *pwdNumGroups = 0;

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, nGroup);        
    }

    goto cleanup;
}

DWORD
ADSchemaMarshalGroupInfoList_1(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,    
    LDAPMessage *pMessagePseudo,
    PVOID**     pppGroupInfoList,
    PDWORD      pwdNumGroups
    )    
{
    
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1* ppGroupInfoList = NULL;
    PLSA_GROUP_INFO_1  pGroupInfo = NULL;
    DWORD iGroup = 0;
    DWORD nGroup = 0;
    DWORD dwGroupInfoLevel = 1;
    // Do not free
    LDAPMessage *pGroupMessage = NULL;
    LDAPMessage *pGroupMessageReal = NULL;
    PSTR pszGroupName = NULL;
    LDAP *pLd = LsaLdapGetSession(hDirectory);
    
    PSTR  pszObjectSID = NULL;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    PSTR pszDirectoryRoot = NULL;
    CHAR szQuery[1024];    
        
    PSTR szAttributeListName[] = 
                    {AD_LDAP_SAM_NAME_TAG,
                     AD_LDAP_UPN_TAG,
                     AD_LDAP_MEMBER_TAG,
                     NULL
                    };   

    if (!pMessagePseudo)
    { 
        goto done;
    }
    
    nGroup = ldap_count_entries(
                    pLd, 
                    pMessagePseudo);
    if (nGroup < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
    } else if (nGroup == 0) {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaAllocateMemory(
                    sizeof(PLSA_GROUP_INFO_1) * nGroup,
                    (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);
       
    pGroupMessage = ldap_first_entry(
                           pLd,
                           pMessagePseudo);
    BAIL_ON_LSA_ERROR(dwError);   
           
    while (pGroupMessage)
    {
        BOOLEAN bValidADEntry = false;
        
        dwError = LsaLdapIsValidADEntry(
                        hDirectory,
                        pGroupMessage,
                        &bValidADEntry);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (!bValidADEntry){
            dwError = LSA_ERROR_LDAP_FAILED_GETDN;
            BAIL_ON_LSA_ERROR(dwError);            
        }
        
        LSA_SAFE_FREE_STRING(pszGroupName);
        
        dwError = LsaLdapGetString(
                       hDirectory,
                       pGroupMessage,
                       AD_LDAP_SAM_NAME_TAG,
                       &pszGroupName);
        BAIL_ON_LSA_ERROR(dwError);
           
        //if group's Pseudo attribute is stored in the real group object(default schema mode)
        if (!IsNullOrEmptyString(pszGroupName)){               

            dwError = ADSchemaMarshalGroupInfo(
                            hDirectory, 
                            pszNetBIOSDomainName, 
                            pGroupMessage, 
                            pGroupMessage, 
                            dwGroupInfoLevel, 
                            (PVOID*)&pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
            
            *(ppGroupInfoList + iGroup++) = pGroupInfo;
            pGroupInfo = NULL;
        }
        else
        {
            // Otherwise, use group backlink to locate the real group object
            // and grab the sAMAccountName etc. attributes
            
            DWORD iValue = 0;
            DWORD dwCount = 0;
            
            dwError = LsaLdapGetStrings(
                            hDirectory,
                            pGroupMessage,
                            AD_LDAP_KEYWORDS_TAG,
                            &ppszValues,
                            &dwNumValues);
            BAIL_ON_LSA_ERROR(dwError);
               
            for ( iValue = 0; iValue < dwNumValues; iValue++)
            {
                 if (!strncasecmp(ppszValues[iValue],
                                  "backLink=",
                                  sizeof("backLink=")-1))
                 {
                     pszObjectSID = ppszValues[iValue] + sizeof("backLink=") - 1;           
                     break;
                 }        
            }
               
            if (IsNullOrEmptyString(pszObjectSID)) {
                dwError = LSA_ERROR_INVALID_SID;
                BAIL_ON_LSA_ERROR(dwError);
            }
               
            sprintf(szQuery, "(objectSid=%s)", pszObjectSID);    

            if (ppszValues) {
                LsaFreeStringArray(ppszValues, dwNumValues);
                ppszValues = NULL;
                dwNumValues = 0;
            }

            dwError = LsaLdapConvertDomainToDN(
                            gpADProviderData->szDomain,
                            &pszDirectoryRoot);
            BAIL_ON_LSA_ERROR(dwError);
               
            dwError = LsaLdapDirectorySearch(
                                  hDirectory,
                                  pszDirectoryRoot,
                                  LDAP_SCOPE_SUBTREE,
                                  szQuery,
                                  szAttributeListName,
                                  &pGroupMessageReal);
            BAIL_ON_LSA_ERROR(dwError);

            dwCount = ldap_count_entries(
                                  pLd,
                                  pGroupMessageReal);               
            if (dwCount < 0) {
                dwError = LSA_ERROR_LDAP_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            } else if (dwCount == 0) {
                //this means the current user is an "orphan"
                dwError = 0;
            } else if (dwCount > 1) {
                dwError = LSA_ERROR_DUPLICATE_GROUPNAME;
                BAIL_ON_LSA_ERROR(dwError);
            } else {
                  
                  dwError = ADSchemaMarshalGroupInfo(
                                  hDirectory,
                                  pszNetBIOSDomainName,
                                  pGroupMessageReal,
                                  pGroupMessage,
                                  dwGroupInfoLevel,
                                  (PVOID*)&pGroupInfo);
                  BAIL_ON_LSA_ERROR(dwError);
                  
                  *(ppGroupInfoList + iGroup++) = pGroupInfo;               
                  pGroupInfo = NULL;
           }
            
           if (pGroupMessageReal) {
               ldap_msgfree(pGroupMessageReal);
               pGroupMessageReal = NULL;
           }

           LSA_SAFE_FREE_STRING(pszDirectoryRoot);
       }               
       pGroupMessage = ldap_next_entry(
                                     pLd, 
                                     pGroupMessage);     
    }
    
done:
    
    *pppGroupInfoList = (PVOID*)ppGroupInfoList;
    *pwdNumGroups = iGroup;
    
cleanup:
    
    if (pGroupMessageReal) {
        ldap_msgfree(pGroupMessageReal);    
    }
    
    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }

    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    LSA_SAFE_FREE_STRING(pszGroupName);
    
    return dwError;

error:

    *pppGroupInfoList = NULL;
    *pwdNumGroups = 0;

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, nGroup);        
    }

    goto cleanup;
}

DWORD
ADSchemaMarshalGroupInfoList(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,    
    LDAPMessage *pMessagePseudo,
    DWORD       dwGroupInfoLevel,
    PVOID**     pppGroupInfoList,
    PDWORD      pNumGroups
    )
{
    DWORD dwError = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD NumGroups = 0;
    
    switch(dwGroupInfoLevel)
    {
        case 0:
            dwError = ADSchemaMarshalGroupInfoList_0(
                            hDirectory,
                            pszNetBIOSDomainName,
                            pMessagePseudo,
                            &ppGroupInfoList,
                            &NumGroups);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case 1:
            dwError = ADSchemaMarshalGroupInfoList_1(
                            hDirectory,
                            pszNetBIOSDomainName,
                            pMessagePseudo,
                            &ppGroupInfoList,
                            &NumGroups);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }
    
    *pppGroupInfoList = ppGroupInfoList;    
    *pNumGroups = NumGroups;
    
cleanup:

    return dwError;
    
error:

    *pppGroupInfoList = NULL; 
    *pNumGroups = 0;
    
    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, NumGroups);
    }
    goto cleanup;
}


DWORD
ADNonSchemaMarshalGroupInfoList_0(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,    
    LDAPMessage *pMessagePseudo,
    PVOID**     pppGroupInfoList,
    PDWORD      pwdNumGroups
    )    
{
    
    DWORD dwError = 0;
    PLSA_GROUP_INFO_0* ppGroupInfoList = NULL;
    PLSA_GROUP_INFO_0  pGroupInfo = NULL;
    DWORD iGroup = 0;
    DWORD nGroup = 0;
    DWORD dwGroupInfoLevel = 0;
    // Do not free
    LDAPMessage *pGroupMessage = NULL;
    LDAPMessage *pGroupMessageReal = NULL;
    LDAP *pLd = LsaLdapGetSession(hDirectory);
    
    PSTR  pszObjectSID = NULL;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    PSTR pszDirectoryRoot = NULL;
    CHAR szQuery[1024];
    DWORD iValue = 0;
    DWORD dwCount = 0;    
    
    PSTR szAttributeListName[] = 
                    {AD_LDAP_SAM_NAME_TAG,
                     AD_LDAP_UPN_TAG,
                     AD_LDAP_MEMBER_TAG,
                     NULL
                    };   

    if (!pMessagePseudo)
    {
        goto done;
    }
    
    nGroup = ldap_count_entries(
                    pLd, 
                    pMessagePseudo);
    if (nGroup < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
    } else if (nGroup == 0) {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaAllocateMemory(
                    sizeof(PLSA_GROUP_INFO_0) * nGroup,
                    (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);
       
    pGroupMessage = ldap_first_entry(
                           pLd,
                           pMessagePseudo);
    BAIL_ON_LSA_ERROR(dwError);   
           
    while (pGroupMessage)
    {
        BOOLEAN bValidADEntry = false;
        
        dwError = LsaLdapIsValidADEntry(
                        hDirectory,
                        pGroupMessage,
                        &bValidADEntry);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (!bValidADEntry){
            dwError = LSA_ERROR_LDAP_FAILED_GETDN;
            BAIL_ON_LSA_ERROR(dwError);            
        }

        dwError = LsaLdapGetStrings(
                       hDirectory,
                       pGroupMessage,
                       AD_LDAP_KEYWORDS_TAG,
                       &ppszValues,
                       &dwNumValues);
        BAIL_ON_LSA_ERROR(dwError);
             
        for ( iValue = 0; iValue < dwNumValues; iValue++)
        {
             if (!strncasecmp(ppszValues[iValue], "backLink=", sizeof("backLink=")-1))
             {
                 pszObjectSID = ppszValues[iValue] + sizeof("backLink=") - 1;           
                 break;
             }        
        }
               
        if (IsNullOrEmptyString(pszObjectSID)) {
            dwError = LSA_ERROR_INVALID_SID;
            BAIL_ON_LSA_ERROR(dwError);
        }
               
        sprintf(szQuery, "(objectSid=%s)", pszObjectSID);    
            
        dwError = LsaLdapConvertDomainToDN(
                        gpADProviderData->szDomain,
                        &pszDirectoryRoot);
        BAIL_ON_LSA_ERROR(dwError);
               
        dwError = LsaLdapDirectorySearch(
                         hDirectory,
                         pszDirectoryRoot,
                         LDAP_SCOPE_SUBTREE,
                         szQuery,
                         szAttributeListName,
                         &pGroupMessageReal);
                  
        BAIL_ON_LSA_ERROR(dwError);

        dwCount = ldap_count_entries(
                         pLd,
                         pGroupMessageReal);               
        if (dwCount < 0) {
           dwError = LSA_ERROR_LDAP_ERROR;
           BAIL_ON_LSA_ERROR(dwError);
        } else if (dwCount == 0) {
                //this means the current user is an "orphan"
                dwError = 0;
        } else if (dwCount > 1){
            dwError = LSA_ERROR_DUPLICATE_GROUPNAME;
            BAIL_ON_LSA_ERROR(dwError);
        } else {                  
            dwError = ADNonSchemaMarshalGroupInfo(
                        hDirectory,
                        pszNetBIOSDomainName,
                        pGroupMessageReal,
                        pGroupMessage,
                        dwGroupInfoLevel,
                        (PVOID*)&pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
                  
            *(ppGroupInfoList + iGroup++) = pGroupInfo;
            pGroupInfo = NULL;
        }
            
       if (pGroupMessageReal) {
           ldap_msgfree(pGroupMessageReal);
           pGroupMessageReal = NULL;
       }
       
       pGroupMessage = ldap_next_entry(
           pLd, 
           pGroupMessage);     
    }
    
done:
    
    *pppGroupInfoList = (PVOID*)ppGroupInfoList;
    *pwdNumGroups = iGroup;
    
cleanup:

    if (pGroupMessageReal)
            ldap_msgfree(pGroupMessageReal);    
    
    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }
    
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    
    return dwError;

error:

    *pppGroupInfoList = NULL;
    *pwdNumGroups = 0;

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, nGroup);        
    }

    goto cleanup;
}

DWORD
ADNonSchemaMarshalGroupInfoList_1(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,    
    LDAPMessage *pMessagePseudo,
    PVOID**     pppGroupInfoList,
    PDWORD      pwdNumGroups
    )    
{
    
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1* ppGroupInfoList = NULL;
    PLSA_GROUP_INFO_1  pGroupInfo = NULL;
    DWORD iGroup = 0;
    DWORD nGroup = 0;
    DWORD dwGroupInfoLevel = 1;
    // Do not free
    LDAPMessage *pGroupMessage = NULL;
    LDAPMessage *pGroupMessageReal = NULL;
    LDAP *pLd = LsaLdapGetSession(hDirectory);
    
    PSTR  pszObjectSID = NULL;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    PSTR pszDirectoryRoot = NULL;
    CHAR szQuery[1024];
    DWORD iValue = 0;
    DWORD dwCount = 0;    
    
    PSTR szAttributeListName[] = 
                    {AD_LDAP_SAM_NAME_TAG,
                     AD_LDAP_UPN_TAG,
                     AD_LDAP_MEMBER_TAG,
                     NULL
                    };   

    if (!pMessagePseudo)
    {
        goto done;
    }
    
    nGroup = ldap_count_entries(
                    pLd, 
                    pMessagePseudo);
    if (nGroup < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
    } else if (nGroup == 0) {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
    }
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaAllocateMemory(
                    sizeof(PLSA_GROUP_INFO_1) * nGroup,
                    (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);
       
    pGroupMessage = ldap_first_entry(
                           pLd,
                           pMessagePseudo);
    BAIL_ON_LSA_ERROR(dwError);   
           
    while (pGroupMessage)
    {
        BOOLEAN bValidADEntry = false;
        
        dwError = LsaLdapIsValidADEntry(
                        hDirectory,
                        pGroupMessage,
                        &bValidADEntry);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (!bValidADEntry){
            dwError = LSA_ERROR_LDAP_FAILED_GETDN;
            BAIL_ON_LSA_ERROR(dwError);            
        }

        dwError = LsaLdapGetStrings(
                       hDirectory,
                       pGroupMessage,
                       AD_LDAP_KEYWORDS_TAG,
                       &ppszValues,
                       &dwNumValues);
        BAIL_ON_LSA_ERROR(dwError);
             
        for ( iValue = 0; iValue < dwNumValues; iValue++)
        {
             if (!strncasecmp(ppszValues[iValue], "backLink=", sizeof("backLink=")-1))
             {
                 pszObjectSID = ppszValues[iValue] + sizeof("backLink=") - 1;           
                 break;
             }        
        }
               
        if (IsNullOrEmptyString(pszObjectSID)) {
            dwError = LSA_ERROR_INVALID_SID;
            BAIL_ON_LSA_ERROR(dwError);
        }
               
        sprintf(szQuery, "(objectSid=%s)", pszObjectSID);    
            
        dwError = LsaLdapConvertDomainToDN(
                        gpADProviderData->szDomain,
                        &pszDirectoryRoot);
        BAIL_ON_LSA_ERROR(dwError);
               
        dwError = LsaLdapDirectorySearch(
                         hDirectory,
                         pszDirectoryRoot,
                         LDAP_SCOPE_SUBTREE,
                         szQuery,
                         szAttributeListName,
                         &pGroupMessageReal);
                  
        BAIL_ON_LSA_ERROR(dwError);

        dwCount = ldap_count_entries(
                         pLd,
                         pGroupMessageReal);               
        if (dwCount < 0) {
           dwError = LSA_ERROR_LDAP_ERROR;
           BAIL_ON_LSA_ERROR(dwError);
        } else if (dwCount == 0) {
                //this means the current user is an "orphan"
                dwError = 0;
        } else if (dwCount > 1){
            dwError = LSA_ERROR_DUPLICATE_GROUPNAME;
            BAIL_ON_LSA_ERROR(dwError);
        } else {                  
            dwError = ADNonSchemaMarshalGroupInfo(
                        hDirectory,
                        pszNetBIOSDomainName,
                        pGroupMessageReal,
                        pGroupMessage,
                        dwGroupInfoLevel,
                        (PVOID*)&pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
                  
            *(ppGroupInfoList + iGroup++) = pGroupInfo;
            pGroupInfo = NULL;
        }
            
       if (pGroupMessageReal) {
           ldap_msgfree(pGroupMessageReal);
           pGroupMessageReal = NULL;
       }
       
       pGroupMessage = ldap_next_entry(
           pLd, 
           pGroupMessage);     
    }
    
done:
    
    *pppGroupInfoList = (PVOID*)ppGroupInfoList;
    *pwdNumGroups = iGroup;
    
cleanup:

    if (pGroupMessageReal)
            ldap_msgfree(pGroupMessageReal);    
    
    if (ppszValues) {
        LsaFreeStringArray(ppszValues, dwNumValues);
    }
    
    LSA_SAFE_FREE_STRING(pszDirectoryRoot);
    
    return dwError;

error:

    *pppGroupInfoList = NULL;
    *pwdNumGroups = 0;

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, nGroup);        
    }

    goto cleanup;
}

DWORD
ADNonSchemaMarshalGroupInfoList(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,    
    LDAPMessage *pMessagePseudo,
    DWORD       dwGroupInfoLevel,
    PVOID**     pppGroupInfoList,
    PDWORD      pNumGroups
    )
{
    DWORD dwError = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD NumGroups = 0;
    
    switch(dwGroupInfoLevel)
    {
        case 0:
            dwError = ADNonSchemaMarshalGroupInfoList_0(
                            hDirectory,
                            pszNetBIOSDomainName,
                            pMessagePseudo,
                            &ppGroupInfoList,
                            &NumGroups);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case 1:
            dwError = ADNonSchemaMarshalGroupInfoList_1(
                            hDirectory,
                            pszNetBIOSDomainName,
                            pMessagePseudo,
                            &ppGroupInfoList,
                            &NumGroups);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }
    
    *pppGroupInfoList = ppGroupInfoList;    
    *pNumGroups = NumGroups;
    
cleanup:

    return dwError;
    
error:

    *pppGroupInfoList = NULL; 
    *pNumGroups = 0;
    
    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, NumGroups);
    }
    goto cleanup;
}



DWORD
ADUnprovisionedMarshalGroupInfoList_0(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,
    LDAPMessage *pMessage,
    PVOID**     pppGroupInfoList,
    PDWORD      pdwNumGroupsFound
    )    
{   
    DWORD dwError = 0;
    PLSA_GROUP_INFO_0* ppGroupInfoList = NULL;
    DWORD iGroup = 0;
    DWORD dwGroupInfoLevel = 0;
    DWORD dwCount = 0;
    // Do not free
    LDAPMessage *pGroupMessage = NULL;    
    LDAP *pLd = LsaLdapGetSession(hDirectory);
    
    dwCount = ldap_count_entries(pLd, 
                                 pMessage);
    if (dwCount < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
        dwError = LSA_ERROR_NO_MORE_GROUPS;
    } 
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaAllocateMemory(dwCount * sizeof(PLSA_GROUP_INFO_0),
                                (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
    pGroupMessage = ldap_first_entry(
                        pLd,
                        pMessage);
    BAIL_ON_LSA_ERROR(dwError);   
        
    while (pGroupMessage)
    {
        BOOLEAN bValidADEntry = false;
        
        dwError = LsaLdapIsValidADEntry(
                        hDirectory,
                        pGroupMessage,
                        &bValidADEntry);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (!bValidADEntry){
            dwError = LSA_ERROR_LDAP_FAILED_GETDN;
            BAIL_ON_LSA_ERROR(dwError);            
        } 
        
        dwError = ADUnprovisionedMarshalGroupInfo(
                    hDirectory, 
                    pszNetBIOSDomainName,
                    pGroupMessage, 
                    dwGroupInfoLevel, 
                    (PVOID)(&(ppGroupInfoList[iGroup])));
        BAIL_ON_LSA_ERROR(dwError);          
        
        pGroupMessage = ldap_next_entry(
                         pLd, 
                         pGroupMessage);
        iGroup++;                   
    }  

    *pppGroupInfoList = (PVOID)ppGroupInfoList;
    *pdwNumGroupsFound = dwCount;
    
cleanup:

    return dwError;

error:

    *pppGroupInfoList = NULL;

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwCount);        
    }

    goto cleanup;
}

DWORD
ADUnprovisionedMarshalGroupInfoList_1(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,
    LDAPMessage *pMessage,
    PVOID**     pppGroupInfoList,
    PDWORD      pdwNumGroupsFound
    )    
{   
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1* ppGroupInfoList = NULL;
    DWORD iGroup = 0;
    DWORD dwGroupInfoLevel = 1;
    DWORD dwCount = 0;
    // Do not free
    LDAPMessage *pGroupMessage = NULL;    
    LDAP *pLd = LsaLdapGetSession(hDirectory);
    
    dwCount = ldap_count_entries(pLd, 
                                 pMessage);
    if (dwCount < 0) {
        dwError = LSA_ERROR_LDAP_ERROR;
    } else if (dwCount == 0) {
        dwError = LSA_ERROR_NO_MORE_GROUPS;
    } 
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaAllocateMemory(dwCount * sizeof(PLSA_GROUP_INFO_1),
                                (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
    pGroupMessage = ldap_first_entry(
                        pLd,
                        pMessage);
    BAIL_ON_LSA_ERROR(dwError);   
        
    while (pGroupMessage)
    {
        BOOLEAN bValidADEntry = false;
        
        dwError = LsaLdapIsValidADEntry(
                        hDirectory,
                        pGroupMessage,
                        &bValidADEntry);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (!bValidADEntry){
            dwError = LSA_ERROR_LDAP_FAILED_GETDN;
            BAIL_ON_LSA_ERROR(dwError);            
        } 
        
        dwError = ADUnprovisionedMarshalGroupInfo(
                    hDirectory, 
                    pszNetBIOSDomainName,
                    pGroupMessage, 
                    dwGroupInfoLevel, 
                    (PVOID)(&(ppGroupInfoList[iGroup])));
        BAIL_ON_LSA_ERROR(dwError);          
        
        pGroupMessage = ldap_next_entry(
                         pLd, 
                         pGroupMessage);
        iGroup++;                   
    }  

    *pppGroupInfoList = (PVOID)ppGroupInfoList;
    *pdwNumGroupsFound = dwCount;
    
cleanup:

    return dwError;

error:

    *pppGroupInfoList = NULL;

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwCount);        
    }

    goto cleanup;
}

DWORD
ADUnprovisionedMarshalGroupInfoList(
    HANDLE      hDirectory,
    PCSTR       pszNetBIOSDomainName,    
    LDAPMessage *pMessage,
    DWORD       dwGroupInfoLevel,
    PVOID**     pppGroupInfoList,
    PDWORD      pdwNumGroupsFound
    )
{
    DWORD  dwError = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD  dwNumGroupsFound = 0;
    
    switch(dwGroupInfoLevel)
    {
        case 0:
            dwError = ADUnprovisionedMarshalGroupInfoList_0(
                hDirectory, 
                pszNetBIOSDomainName, 
                pMessage, 
                &ppGroupInfoList, 
                &dwNumGroupsFound);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        case 1:
            dwError = ADUnprovisionedMarshalGroupInfoList_1(
                hDirectory, 
                pszNetBIOSDomainName, 
                pMessage, 
                &ppGroupInfoList, 
                &dwNumGroupsFound);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }
    
    *pppGroupInfoList = ppGroupInfoList;
    *pdwNumGroupsFound = dwNumGroupsFound;
    
cleanup:

    return dwError;
    
error:

    *pppGroupInfoList = NULL;    
    
    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwNumGroupsFound);
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
