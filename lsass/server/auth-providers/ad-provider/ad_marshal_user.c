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
 *        AD LDAP User Marshalling
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "adprovider.h"

DWORD
ADMarshalUserInfoListDefaultNonSchemaOrCell(
    HANDLE hProvider,
    HANDLE hDirectory,    
    LDAPMessage* pMessagePseudo,
    DWORD dwUserInfoLevel,
    PVOID** pppUserInfoList,
    PDWORD pdwNumUsers
    )
{
    DWORD dwError = 0;
    PSTR* ppSidsList = NULL;
    DWORD dwNumSidsFound = 0;
    size_t sNumUsersFound = 0;    
    PVOID* ppUserInfoList = NULL;
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
                    &sNumUsersFound,
                    &ppObjects);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaAllocateMemory(
                    sizeof(*ppUserInfoList) * sNumUsersFound,
                    (PVOID*)&ppUserInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (iValue = 0; iValue < sNumUsersFound; iValue++)
    {
        dwError = ADMarshalFromUserCache(
                                ppObjects[iValue],
                                dwUserInfoLevel,
                                &ppUserInfoList[iValue]);
        BAIL_ON_LSA_ERROR(dwError);            
    }
    
    *pppUserInfoList = ppUserInfoList;
    *pdwNumUsers = sNumUsersFound;
    
cleanup:
    LsaFreeStringArray(ppSidsList, dwNumSidsFound);
    ADCacheDB_SafeFreeObjectList(sNumUsersFound, &ppObjects);
    
    return dwError;

error:
    if (ppUserInfoList) 
    {
        LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, sNumUsersFound);
    }
    *pppUserInfoList = NULL;
    *pdwNumUsers = 0;
    
    goto cleanup;
}

DWORD
ADMarshalUserInfoListDefaultSchemaOrUnprovision(
    HANDLE hDirectory,
    DWORD dwDirectoryMode,
    ADConfigurationMode adConfMode,
    PCSTR pszDomainDnsName,
    LDAPMessage* pMessageReal,
    DWORD dwUserInfoLevel,
    PVOID** pppUserInfoList,
    PDWORD pdwNumUsers
    )
{
    DWORD dwError = 0;
    DWORD dwMessageCount = 0;
    PAD_SECURITY_OBJECT pObject = NULL;
    PVOID* ppUserInfoList = NULL;
    // Do not free
    LDAPMessage *pUserMessage = NULL;    
    PSTR pszUserName = NULL;
    PSTR pszUserSid = NULL;
    PLSA_LOGIN_NAME_INFO pLoginNameInfo = NULL;
    LDAP *pLd = NULL;
    DWORD dwNumUsers = 0;    

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
                    sizeof(*ppUserInfoList) * dwMessageCount,
                    (PVOID*)&ppUserInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    pUserMessage = ldap_first_entry(pLd, pMessageReal);
    while (pUserMessage)
    {   
        LSA_SAFE_FREE_STRING(pszUserName);
        dwError = LsaLdapGetString(
                       hDirectory,
                       pUserMessage,
                       AD_LDAP_SAM_NAME_TAG,
                       &pszUserName);
        BAIL_ON_LSA_ERROR(dwError);
        
        LSA_SAFE_FREE_STRING(pszUserSid);
        dwError = ADLdap_GetObjectSid(
                        hDirectory,
                        pUserMessage,
                        &pszUserSid);
        BAIL_ON_LSA_ERROR(dwError);
   
        LSA_SAFE_FREE_LOGIN_NAME_INFO(pLoginNameInfo);
        dwError = CreateObjectLoginNameInfo(
                        &pLoginNameInfo,
                        pszDomainDnsName,
                        pszUserName,
                        pszUserSid);
        BAIL_ON_LSA_ERROR(dwError);
        
        ADCacheDB_SafeFreeObject(&pObject);
        dwError = ADMarshalToUserCache(
                    // hPseudoDirectory should be NULL for unprovision mode
                    // hPseudoDirectory should be hRealDirectory for default schem mode
                    dwDirectoryMode == UNPROVISIONED_MODE ? (HANDLE)NULL : hDirectory,
                    hDirectory,
                    dwDirectoryMode,
                    adConfMode,
                    pLoginNameInfo,                    
                    pUserMessage,
                    // pMessagePseudo should be NULL for unprovision mode
                    // pMessagePseudo should be pRealMessage for default schema mode
                    dwDirectoryMode == UNPROVISIONED_MODE ? NULL : pUserMessage,
                    &pObject);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = ADMarshalFromUserCache(
                    pObject,
                    dwUserInfoLevel,
                    &ppUserInfoList[dwNumUsers++]);
        BAIL_ON_LSA_ERROR(dwError);   
        
        pUserMessage = ldap_next_entry(pLd, pUserMessage);
    }   
    
cleanup:
    // We handle error here instead of error label
    // because there is a goto cleanup above.
    if (dwError)
    {
        if (ppUserInfoList) 
        {
            LsaFreeUserInfoList(dwUserInfoLevel, ppUserInfoList, dwNumUsers);
        }
        ppUserInfoList = NULL;
        dwNumUsers = 0;;
    }
    
    *pppUserInfoList = ppUserInfoList;
    *pdwNumUsers = dwNumUsers;
   
    ADCacheDB_SafeFreeObject(&pObject);
    LSA_SAFE_FREE_LOGIN_NAME_INFO(pLoginNameInfo);
    LSA_SAFE_FREE_STRING(pszUserSid);
    LSA_SAFE_FREE_STRING(pszUserName);
    
    
    return dwError;

error:
    // Do not actually handle any errors here.
    goto cleanup;
}

DWORD
ADMarshalUserInfoList(
    IN OPTIONAL HANDLE hProvider,
    IN HANDLE hDirectory,    
    IN OPTIONAL PCSTR pszDomainDnsName,
    IN LSA_AD_MARSHAL_INFO_LIST_MODE InfoListMarshalMode,
    IN LDAPMessage* pMessagePseudo,
    IN DWORD dwUserInfoLevel,
    OUT PVOID** pppUserInfoList,
    OUT PDWORD pdwNumUsers
    )
{
    switch (InfoListMarshalMode)
    {
        case LSA_AD_MARSHAL_MODE_DEFAULT_SCHEMA:
            return ADMarshalUserInfoListDefaultSchemaOrUnprovision(
                     hDirectory,
                     DEFAULT_MODE,
                     SchemaMode,
                     pszDomainDnsName,
                     pMessagePseudo,
                     dwUserInfoLevel,
                     pppUserInfoList,
                     pdwNumUsers);
            
        case LSA_AD_MARSHAL_MODE_UNPROVISIONED:
            return ADMarshalUserInfoListDefaultSchemaOrUnprovision(
                     hDirectory,
                     UNPROVISIONED_MODE,
                     UnknownMode,
                     pszDomainDnsName,
                     pMessagePseudo,
                     dwUserInfoLevel,
                     pppUserInfoList,
                     pdwNumUsers);
            
        case LSA_AD_MARSHAL_MODE_OTHER:
            return ADMarshalUserInfoListDefaultNonSchemaOrCell(
                     hProvider,
                     hDirectory,    
                     pMessagePseudo,
                     dwUserInfoLevel,
                     pppUserInfoList,
                     pdwNumUsers);        
    }
    
    return LSA_ERROR_NOT_SUPPORTED;
}

DWORD
ADMarshalToUserCache(
    HANDLE                  hPseudoDirectory,
    HANDLE                  hRealDirectory,
    DWORD                   dwDirectoryMode,
    ADConfigurationMode     adConfMode,
    PLSA_LOGIN_NAME_INFO    pUserNameInfo,
    LDAPMessage*            pMessageReal,
    LDAPMessage*            pMessagePseudo,
    PAD_SECURITY_OBJECT*    ppUserInfo
    )
{
    DWORD dwError = 0;
    PAD_SECURITY_OBJECT pUserInfo = NULL;

    switch (dwDirectoryMode){

        case UNPROVISIONED_MODE:
            if (pMessagePseudo){
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = ADUnprovisionedMarshalToUserCache(
                            hRealDirectory,
                            pUserNameInfo,
                            pMessageReal,
                            &pUserInfo);
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

                     dwError = ADSchemaMarshalToUserCache(
                                   hPseudoDirectory,
                                   hRealDirectory,
                                   pUserNameInfo,
                                   pMessageReal,
                                   pMessagePseudo,
                                   &pUserInfo);
                     BAIL_ON_LSA_ERROR(dwError);

                     break;

                 case NonSchemaMode:

                     dwError = ADNonSchemaMarshalToUserCache(
                                   hPseudoDirectory,
                                   hRealDirectory,
                                   pUserNameInfo,
                                   pMessageReal,
                                   pMessagePseudo,
                                   &pUserInfo);
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

    *ppUserInfo = pUserInfo;

cleanup:

    return dwError;

error:

    *ppUserInfo = NULL;

    ADCacheDB_SafeFreeObject(&pUserInfo);

    goto cleanup;
}

DWORD
ADMarshalFromUserCache(
    PAD_SECURITY_OBJECT pUser,
    DWORD       dwUserInfoLevel,
    PVOID*      ppUserInfo
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    /* These variables represent pUserInfo casted to different types. Do not
     * free these values directly, free pUserInfo instead.
     */
    PLSA_USER_INFO_0 pUserInfo0 = NULL;
    PLSA_USER_INFO_1 pUserInfo1 = NULL;
    PLSA_USER_INFO_2 pUserInfo2 = NULL;

    *ppUserInfo = NULL;

    BAIL_ON_INVALID_POINTER(pUser);

    if (pUser->type != AccountType_User)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch(dwUserInfoLevel)
    {
        case 0:
            dwError = LsaAllocateMemory(
                            sizeof(LSA_USER_INFO_0),
                            (PVOID*)&pUserInfo);
            BAIL_ON_LSA_ERROR(dwError);
            pUserInfo0 = (PLSA_USER_INFO_0) pUserInfo;
            break;
        case 1:
            dwError = LsaAllocateMemory(
                            sizeof(LSA_USER_INFO_1),
                            (PVOID*)&pUserInfo);
            BAIL_ON_LSA_ERROR(dwError);
            pUserInfo0 = (PLSA_USER_INFO_0) pUserInfo;
            pUserInfo1 = (PLSA_USER_INFO_1) pUserInfo;
            break;
        case 2:
            dwError = LsaAllocateMemory(
                            sizeof(LSA_USER_INFO_2),
                            (PVOID*)&pUserInfo);
            BAIL_ON_LSA_ERROR(dwError);
            pUserInfo0 = (PLSA_USER_INFO_0) pUserInfo;
            pUserInfo1 = (PLSA_USER_INFO_1) pUserInfo;
            pUserInfo2 = (PLSA_USER_INFO_2) pUserInfo;
            break;
        default:
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }

    if (pUserInfo0 != NULL)
    {
        pUserInfo0->uid = pUser->userInfo.uid;
        pUserInfo0->gid = pUser->userInfo.gid;
        dwError = ADMarshalGetCanonicalName(
                pUser,
                &pUserInfo0->pszName);
        BAIL_ON_LSA_ERROR(dwError);

        // Optional values use LsaStrDupOrNull. Required values use
        // LsaAllocateString.
        dwError = LsaStrDupOrNull(
                    pUser->userInfo.pszPasswd,
                    &pUserInfo0->pszPasswd);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaStrDupOrNull(
                    pUser->userInfo.pszGecos,
                    &pUserInfo0->pszGecos);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAllocateString(
                    pUser->userInfo.pszShell,
                    &pUserInfo0->pszShell);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAllocateString(
                    pUser->userInfo.pszHomedir,
                    &pUserInfo0->pszHomedir);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAllocateString(
                    pUser->pszObjectSid,
                    &pUserInfo0->pszSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo1 != NULL)
    {
        dwError = LsaStrDupOrNull(
                      pUser->userInfo.pszUPN,
                      &pUserInfo1->pszUPN);
        BAIL_ON_LSA_ERROR(dwError);

        pUserInfo1->bIsGeneratedUPN = pUser->userInfo.bIsGeneratedUPN;
        pUserInfo1->bIsLocalUser = FALSE;
        pUserInfo1->pLMHash = NULL;
        pUserInfo1->dwLMHashLen = 0;
        pUserInfo1->pNTHash = NULL;
        pUserInfo1->dwNTHashLen = 0;
    }

    if (pUserInfo2 != NULL)
    {
        struct timeval current_tv;
        UINT64 u64current_NTtime = 0;
        int64_t qwNanosecsToPasswordExpiry;

        if (gettimeofday(&current_tv, NULL) < 0)
        {
            dwError = errno;
            BAIL_ON_LSA_ERROR(dwError);
        }
        ADConvertTimeUnix2Nt(current_tv.tv_sec,
                             &u64current_NTtime);

        qwNanosecsToPasswordExpiry = gpADProviderData->adMaxPwdAge -
            (u64current_NTtime - pUser->userInfo.qwPwdLastSet);

        dwError = AD_UpdateUserObjectFlags(pUser);
        BAIL_ON_LSA_ERROR(dwError);

        if (pUser->userInfo.bPasswordNeverExpires ||
            gpADProviderData->adMaxPwdAge == 0)
        {
            //password never expires
            pUserInfo2->dwDaysToPasswordExpiry = 0LL;
        }
        else if (pUser->userInfo.bPasswordExpired)
        {
            //password is expired already
            pUserInfo2->dwDaysToPasswordExpiry = 0LL;
        }
        else
        {
            pUserInfo2->dwDaysToPasswordExpiry = qwNanosecsToPasswordExpiry /
                (10000000LL * 24*60*60);
        }
        pUserInfo2->bPasswordExpired = pUser->userInfo.bPasswordExpired;
        pUserInfo2->bPasswordNeverExpires = pUser->userInfo.bPasswordNeverExpires;
        pUserInfo2->bPromptPasswordChange = pUser->userInfo.bPromptPasswordChange;
        pUserInfo2->bUserCanChangePassword = pUser->userInfo.bUserCanChangePassword;
        pUserInfo2->bAccountDisabled = pUser->userInfo.bAccountDisabled;
        pUserInfo2->bAccountExpired = pUser->userInfo.bAccountExpired;
        pUserInfo2->bAccountLocked = pUser->userInfo.bAccountLocked;
    }

    *ppUserInfo = pUserInfo;

cleanup:

    return dwError;


error:
    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
        pUserInfo = NULL;
    }

    *ppUserInfo = NULL;

    goto cleanup;
}

// This function fillls in all of the booleans in pUserInfo except for
// bPromptPasswordChange and bAccountExpired
DWORD
ADParseUserCtrlToCache(
    DWORD            dwUserAccountCtrl,
    PAD_SECURITY_OBJECT pUserInfo)
{
    pUserInfo->userInfo.bPasswordNeverExpires =
        ((dwUserAccountCtrl & LSA_AD_UF_DONT_EXPIRE_PASSWD) != 0);
    if (pUserInfo->userInfo.bPasswordNeverExpires)
    {
        pUserInfo->userInfo.bPasswordExpired = FALSE;
    } else
    {
        pUserInfo->userInfo.bPasswordExpired =
            ((dwUserAccountCtrl & LSA_AD_UF_PASSWORD_EXPIRED) != 0);
    }
    pUserInfo->userInfo.bUserCanChangePassword =
        ((dwUserAccountCtrl & LSA_AD_UF_CANT_CHANGE_PASSWD) == 0);
    pUserInfo->userInfo.bAccountDisabled =
        ((dwUserAccountCtrl & LSA_AD_UF_ACCOUNTDISABLE) != 0);
    pUserInfo->userInfo.bAccountLocked =
        ((dwUserAccountCtrl & LSA_AD_UF_LOCKOUT) != 0);

    return 0;
}

DWORD ADGetCurrentNtTime(UINT64 *qwResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    struct timeval current_tv;

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }
    dwError = ADConvertTimeUnix2Nt(current_tv.tv_sec,
        qwResult);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    *qwResult = 0;
    goto cleanup;
}

DWORD
ADParsePasswdInfoToCache(
    HANDLE            hDirectory,
    LDAPMessage*      pMessageReal,
    PAD_SECURITY_OBJECT pUserInfo)
{
    DWORD dwError = 0;

    PSTR  pszPwdLastSet = NULL;
    UINT64 u64PwdLastSet = 0;
    PSTR pszAccountExpired = NULL;
    UINT64 u64AccountExpired = 0;
    UINT64 u64current_NTtime = 0;
    int64_t qwNanosecsToPasswordExpiry;

    dwError = ADGetCurrentNtTime(&u64current_NTtime);
    BAIL_ON_LSA_ERROR(dwError);

    //process "accountExpires"
    dwError = LsaLdapGetString(
                hDirectory,
                pMessageReal,
                AD_LDAP_ACCOUT_EXP_TAG,
                &pszAccountExpired);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADStr2UINT64(
                pszAccountExpired,
                &u64AccountExpired);
    if (u64AccountExpired == 0LL || u64AccountExpired == 9223372036854775807LL)//this means the account will never be expired
        pUserInfo->userInfo.bAccountExpired = FALSE;
    else{
        if (u64current_NTtime <= u64AccountExpired)
            pUserInfo->userInfo.bAccountExpired = FALSE;
        else
            pUserInfo->userInfo.bAccountExpired = TRUE;
    }
    pUserInfo->userInfo.qwAccountExpires = u64AccountExpired;

    //process "pwdLastSet"
    dwError = LsaLdapGetString(
                hDirectory,
                pMessageReal,
                AD_LDAP_PWD_LASTSET_TAG,
                &pszPwdLastSet);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADStr2UINT64(
                pszPwdLastSet,
                &u64PwdLastSet);

    pUserInfo->userInfo.qwPwdLastSet = u64PwdLastSet;

    qwNanosecsToPasswordExpiry = gpADProviderData->adMaxPwdAge -
        (u64current_NTtime - pUserInfo->userInfo.qwPwdLastSet);
    if (qwNanosecsToPasswordExpiry / (10000000LL * 24*60*60) <= 14)
    {
        //The password will expire in 14 days or less
        pUserInfo->userInfo.bPromptPasswordChange = TRUE;
    }
    else
        pUserInfo->userInfo.bPromptPasswordChange = FALSE;

cleanup:
    LSA_SAFE_FREE_STRING(pszPwdLastSet);
    LSA_SAFE_FREE_STRING(pszAccountExpired);

    return dwError;

error:

    goto cleanup;
}

DWORD
ADSchemaMarshalToUserCache(
    HANDLE                  hPseudoDirectory,
    HANDLE                  hRealDirectory,
    PLSA_LOGIN_NAME_INFO    pUserNameInfo,
    LDAPMessage*            pMessageReal,
    LDAPMessage*            pMessagePseudo,
    PAD_SECURITY_OBJECT*    ppUserInfo
    )
{
    DWORD dwError = 0;
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    struct timeval current_tv;
    UCHAR* pucSIDBytes = NULL;
    DWORD dwSIDByteLength = 0;
    PSTR  pszUserDomainFQDN = NULL;
    PSTR  pszHomedir = NULL;

    dwError = LsaDmWrapGetDomainName(pUserNameInfo->pszDomainNetBiosName,
                                     &pszUserDomainFQDN,
                                     NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sizeof(AD_SECURITY_OBJECT),
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pUserInfo->cache.qwCacheId = -1;
    pUserInfo->cache.tLastUpdated = current_tv.tv_sec;

    pUserInfo->type = AccountType_User;

    if (pMessageReal && hRealDirectory)
    {
        DWORD dwUserAccountCtrl = 0;

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
                    &pUserInfo->pszObjectSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapGetDN(
                hRealDirectory,
                pMessageReal,
                &pUserInfo->pszDN);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapGetString(
                    hRealDirectory,
                    pMessageReal,
                    AD_LDAP_SAM_NAME_TAG,
                    &pUserInfo->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pUserInfo->pszSamAccountName);

        dwError = LsaAllocateString(
                    pUserNameInfo->pszDomainNetBiosName,
                    &pUserInfo->pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapGetUInt32(
                    hRealDirectory,
                    pMessageReal,
                    AD_LDAP_USER_CTRL_TAG,
                    &dwUserAccountCtrl);
        if (dwError == LSA_ERROR_INVALID_LDAP_ATTR_VALUE)
        {
            LSA_LOG_ERROR(
                    "User %s has an invalid value for the userAccountControl"
                    " attribute. Please check that it is set and that the "
                    "machine account has permission to read it.",
                    pUserInfo->pszDN);
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwError =  ADParseUserCtrlToCache(
                    dwUserAccountCtrl,
                    pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);

        dwError =  ADParsePasswdInfoToCache(
                     hRealDirectory,
                     pMessageReal,
                     pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        //at least objectSid is in pUserNameInfo
        dwError = LsaAllocateString(
                    pUserNameInfo->pszObjectSid,
                    &pUserInfo->pszObjectSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAllocateString(
                    pUserNameInfo->pszDomainNetBiosName,
                    &pUserInfo->pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAllocateString(
                    pUserNameInfo->pszName,
                    &pUserInfo->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pUserInfo->pszSamAccountName);
    }

    dwError = ADGetLDAPUPNString(
                        hRealDirectory,
                        pMessageReal,
                        pszUserDomainFQDN,
                        pUserInfo->pszSamAccountName,
                        &pUserInfo->userInfo.pszUPN,
                        &pUserInfo->userInfo.bIsGeneratedUPN);
    BAIL_ON_LSA_ERROR(dwError);

    if (pMessagePseudo){
        pUserInfo->enabled = TRUE;

        dwError = LsaLdapGetUInt32(
                    hPseudoDirectory,
                    pMessagePseudo,
                    AD_LDAP_UID_TAG,
                    (PDWORD)&pUserInfo->userInfo.uid);
        if(dwError == LSA_ERROR_INVALID_LDAP_ATTR_VALUE)
        {
            pUserInfo->enabled = FALSE;
            dwError = LSA_ERROR_SUCCESS;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (pUserInfo->enabled)
    {
        dwError = LsaLdapGetUInt32(
                    hPseudoDirectory,
                    pMessagePseudo,
                    AD_LDAP_GID_TAG,
                    (PDWORD)&pUserInfo->userInfo.gid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapGetString(
                    hPseudoDirectory,
                    pMessagePseudo,
                    AD_LDAP_ALIAS_TAG,
                    &pUserInfo->userInfo.pszAliasName
                    );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapGetString(
                    hPseudoDirectory,
                    pMessagePseudo,
                    AD_LDAP_PASSWD_TAG,
                    &pUserInfo->userInfo.pszPasswd
                    );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapGetString(
                    hPseudoDirectory,
                    pMessagePseudo,
                    AD_LDAP_GECOS_TAG,
                    &pUserInfo->userInfo.pszGecos
                    );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapGetString(
                    hPseudoDirectory,
                    pMessagePseudo,
                    AD_LDAP_SHELL_TAG,
                    &pUserInfo->userInfo.pszShell
                    );
        BAIL_ON_LSA_ERROR(dwError);
        if (!pUserInfo->userInfo.pszShell){
            dwError = AD_GetUnprovisionedModeShell(
                            &pUserInfo->userInfo.pszShell);
            BAIL_ON_LSA_ERROR(dwError);
        }
        BAIL_ON_INVALID_STRING(pUserInfo->userInfo.pszShell);

        dwError = LsaLdapGetString(
                    hPseudoDirectory,
                    pMessagePseudo,
                    AD_LDAP_HOMEDIR_TAG,
                    &pUserInfo->userInfo.pszHomedir
                    );
        BAIL_ON_LSA_ERROR(dwError);

        if (!pUserInfo->userInfo.pszHomedir){
            dwError = AD_GetUnprovisionedModeHomedirTemplate(
                              &pUserInfo->userInfo.pszHomedir);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (strstr(pUserInfo->userInfo.pszHomedir, "%"))
        {
            dwError = AD_BuildHomeDirFromTemplate(
                            pUserInfo->userInfo.pszHomedir,
                            pUserNameInfo->pszDomainNetBiosName,
                            pUserInfo->pszSamAccountName,
                            &pszHomedir);
            BAIL_ON_LSA_ERROR(dwError);

            LSA_SAFE_FREE_STRING(pUserInfo->userInfo.pszHomedir);
            pUserInfo->userInfo.pszHomedir = pszHomedir;
            pszHomedir = NULL;
        }

        LsaStrCharReplace(pUserInfo->userInfo.pszHomedir, ' ', '_');

        BAIL_ON_INVALID_STRING(pUserInfo->userInfo.pszHomedir);
    }

    *ppUserInfo = (PVOID)pUserInfo;

cleanup:

    LSA_SAFE_FREE_MEMORY(pucSIDBytes);
    LSA_SAFE_FREE_STRING(pszUserDomainFQDN);
    LSA_SAFE_FREE_STRING(pszHomedir);

    return dwError;

error:

    *ppUserInfo = NULL;

    ADCacheDB_SafeFreeObject(&pUserInfo);

    goto cleanup;
}

DWORD
ADUnprovisionedMarshalToUserCache(
    HANDLE                  hDirectory,
    PLSA_LOGIN_NAME_INFO    pUserNameInfo,
    LDAPMessage*            pMessage,
    PAD_SECURITY_OBJECT*    ppUserInfo
    )
{
    DWORD dwError = 0;
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    PSTR pszUnprovisionedModeHomedirTemplate = NULL;
    UCHAR* pucSIDBytes = NULL;
    DWORD dwSIDByteLength = 0;
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier = NULL;
    DWORD dwDomainUsersHashedRID = 0;
    DWORD dwDomainUsersUnhashedRID = 0;
    DWORD dwUserAccountCtrl = 0;
    struct timeval current_tv;
    PSTR  pszUserDomainFQDN = NULL;

    if ((hDirectory == (HANDLE)NULL && pMessage) ||
        (hDirectory != (HANDLE)NULL && !pMessage))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    BAIL_ON_INVALID_POINTER(pUserNameInfo);

    dwError = LsaDmWrapGetDomainName(pUserNameInfo->pszDomainNetBiosName,
                                     &pszUserDomainFQDN,
                                     NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sizeof(AD_SECURITY_OBJECT),
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pUserInfo->cache.qwCacheId = -1;
    pUserInfo->cache.tLastUpdated = current_tv.tv_sec;

    pUserInfo->type = AccountType_User;

    pUserInfo->enabled = TRUE;
    
    if (hDirectory == (HANDLE)NULL && !pMessage)
    {
        dwError = ADUnprovisionedMarshalToUserCacheInOneWayTrust(
                                 pUserNameInfo,
                                 &pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        goto cleanup;
    }

    dwError = LsaLdapGetString(
                hDirectory,
                pMessage,
                AD_LDAP_SAM_NAME_TAG,
                &pUserInfo->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);
    BAIL_ON_INVALID_STRING(pUserInfo->pszSamAccountName);

    pUserInfo->userInfo.pszAliasName = NULL;
    pUserInfo->userInfo.pszPasswd = NULL;

    dwError = LsaAllocateString(
                pUserNameInfo->pszDomainNetBiosName,
                &pUserInfo->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    //gecos (display name)
    dwError = LsaLdapGetString(
                hDirectory,
                pMessage,
                AD_LDAP_DISPLAY_NAME_TAG,
                &pUserInfo->userInfo.pszGecos);
    BAIL_ON_LSA_ERROR(dwError);

    //shell
    dwError = AD_GetUnprovisionedModeShell(&pUserInfo->userInfo.pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    //homedir
    dwError = AD_GetUnprovisionedModeHomedirTemplate(
                    &pszUnprovisionedModeHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_BuildHomeDirFromTemplate(
                    pszUnprovisionedModeHomedirTemplate,
                    pUserNameInfo->pszDomainNetBiosName,
                    pUserInfo->pszSamAccountName,
                    &pUserInfo->userInfo.pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrCharReplace(pUserInfo->userInfo.pszHomedir, ' ', '_');

    //UID
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
                &pUserInfo->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetDN(
            hDirectory,
            pMessage,
            &pUserInfo->pszDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetSecurityIdentifierHashedRid(
        pSecurityIdentifier,
        (PDWORD)&pUserInfo->userInfo.uid);
    BAIL_ON_LSA_ERROR(dwError);

    //GID
    dwError = LsaLdapGetUInt32(
                hDirectory,
                pMessage,
                AD_LDAP_PRIMEGID_TAG,
                &dwDomainUsersUnhashedRID);
    if (dwError != 0)
    {
        dwDomainUsersUnhashedRID = WELLKNOWN_SID_DOMAIN_USER_GROUP_RID;
        dwError = 0;
    }

    //Change pSecurityIdentifier to hold domain user group SID
    dwError = LsaSetSecurityIdentifierRid(
                  pSecurityIdentifier,
                  dwDomainUsersUnhashedRID);
    BAIL_ON_LSA_ERROR(dwError);

    //get the domain user group hashed RID
    dwError = LsaGetSecurityIdentifierHashedRid(
                  pSecurityIdentifier,
                  &dwDomainUsersHashedRID);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->userInfo.gid = dwDomainUsersHashedRID;

    dwError = ADGetLDAPUPNString(
                        hDirectory,
                        pMessage,
                        pszUserDomainFQDN,
                        pUserInfo->pszSamAccountName,
                        &pUserInfo->userInfo.pszUPN,
                        &pUserInfo->userInfo.bIsGeneratedUPN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLdapGetUInt32(
                        hDirectory,
                        pMessage,
                        AD_LDAP_USER_CTRL_TAG,
                        &dwUserAccountCtrl);
    if (dwError == LSA_ERROR_INVALID_LDAP_ATTR_VALUE)
    {
        LSA_LOG_ERROR(
                "User %s has an invalid value for the userAccountControl "
                "attribute. Please check that it is set and that the "
                "machine account has permission to read it.",
                pUserInfo->pszDN);
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError =  ADParseUserCtrlToCache(
                dwUserAccountCtrl,
                pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError =  ADParsePasswdInfoToCache(
                 hDirectory,
                 pMessage,
                 pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    *ppUserInfo = pUserInfo;

    if (pSecurityIdentifier)
    {
        LsaFreeSecurityIdentifier(pSecurityIdentifier);
    }

    LSA_SAFE_FREE_STRING(pszUnprovisionedModeHomedirTemplate);
    LSA_SAFE_FREE_MEMORY(pucSIDBytes);
    LSA_SAFE_FREE_STRING(pszUserDomainFQDN);

    return dwError;

error:
    // Output parameter has been explicitly set in cleanup due to use of "goto cleanup"    

    ADCacheDB_SafeFreeObject(&pUserInfo);

    goto cleanup;
}

DWORD
ADUnprovisionedMarshalToUserCacheInOneWayTrust(
    PLSA_LOGIN_NAME_INFO    pUserNameInfo,
    PAD_SECURITY_OBJECT*    ppUserInfo
    )
{
    DWORD dwError = 0;
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    PSTR pszUnprovisionedModeHomedirTemplate = NULL;
    PLSA_SECURITY_IDENTIFIER pSecurityIdentifier = NULL;
    DWORD dwDomainUsersHashedRID = 0;
    DWORD dwDomainUsersUnhashedRID = 0;
    struct timeval current_tv;
    PSTR  pszUserDomainFQDN = NULL;

    dwError = LsaDmWrapGetDomainName(pUserNameInfo->pszDomainNetBiosName,
                                     &pszUserDomainFQDN,
                                     NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sizeof(AD_SECURITY_OBJECT),
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pUserInfo->cache.qwCacheId = -1;
    pUserInfo->cache.tLastUpdated = current_tv.tv_sec;

    pUserInfo->type = AccountType_User;

    pUserInfo->enabled = TRUE;

    // User SamAccountName (generate using user name)
    dwError = LsaAllocateString(
                    pUserNameInfo->pszName,
                    &pUserInfo->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->userInfo.pszAliasName = NULL;
    pUserInfo->userInfo.pszPasswd = NULL;

    dwError = LsaAllocateString(
                pUserNameInfo->pszDomainNetBiosName,
                &pUserInfo->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    // gecos (display name N/A)
    pUserInfo->userInfo.pszGecos = NULL;

    // shell
    dwError = AD_GetUnprovisionedModeShell(
        &pUserInfo->userInfo.pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    // homedir
    dwError = AD_GetUnprovisionedModeHomedirTemplate(
                    &pszUnprovisionedModeHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_BuildHomeDirFromTemplate(
                    pszUnprovisionedModeHomedirTemplate,
                    pUserNameInfo->pszDomainNetBiosName,
                    pUserInfo->pszSamAccountName,
                    &pUserInfo->userInfo.pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrCharReplace(pUserInfo->userInfo.pszHomedir, ' ', '_');

    // ObjectSid
    dwError = LsaAllocateString(
                pUserNameInfo->pszObjectSid,
                &pUserInfo->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    // User DN
    pUserInfo->pszDN = NULL;

    // UID
    dwError = LsaAllocSecurityIdentifierFromString(
                    pUserNameInfo->pszObjectSid,
                    &pSecurityIdentifier);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetSecurityIdentifierHashedRid(
                    pSecurityIdentifier,
                   (PDWORD)&pUserInfo->userInfo.uid);
    BAIL_ON_LSA_ERROR(dwError);

    // GID
     dwDomainUsersUnhashedRID = WELLKNOWN_SID_DOMAIN_USER_GROUP_RID;

    // Change pSecurityIdentifier to hold domain user group SID
    dwError = LsaSetSecurityIdentifierRid(
                  pSecurityIdentifier,
                  dwDomainUsersUnhashedRID);
    BAIL_ON_LSA_ERROR(dwError);

    // Get the domain user group hashed RID
    dwError = LsaGetSecurityIdentifierHashedRid(
                  pSecurityIdentifier,
                  &dwDomainUsersHashedRID);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->userInfo.gid = dwDomainUsersHashedRID;

    // UPN (generated)
    dwError = ADGetLDAPUPNString(
                        (HANDLE)NULL,
                        NULL,
                        pszUserDomainFQDN,
                        pUserInfo->pszSamAccountName,
                        &pUserInfo->userInfo.pszUPN,
                        &pUserInfo->userInfo.bIsGeneratedUPN);
    BAIL_ON_LSA_ERROR(dwError);

    // UserAccountCtrl (N/A)
    // Password (N/A)

    *ppUserInfo = pUserInfo;

cleanup:

    if (pSecurityIdentifier)
    {
        LsaFreeSecurityIdentifier(pSecurityIdentifier);
    }

    LSA_SAFE_FREE_STRING(pszUnprovisionedModeHomedirTemplate);
    LSA_SAFE_FREE_STRING(pszUserDomainFQDN);

    return dwError;

error:

    *ppUserInfo = NULL;

    ADCacheDB_SafeFreeObject(&pUserInfo);

    goto cleanup;
}

DWORD
ADParseUserCtrl(
    DWORD            dwUserAccountCtrl,
    PLSA_USER_INFO_2 pUserInfo)
{
    pUserInfo->bAccountDisabled = ((dwUserAccountCtrl & LSA_AD_UF_ACCOUNTDISABLE) != 0);

    pUserInfo->bUserCanChangePassword = ((dwUserAccountCtrl & LSA_AD_UF_CANT_CHANGE_PASSWD) == 0);

    pUserInfo->bAccountLocked = ((dwUserAccountCtrl & LSA_AD_UF_LOCKOUT) != 0);

    pUserInfo->bPasswordNeverExpires = ((dwUserAccountCtrl & LSA_AD_UF_DONT_EXPIRE_PASSWD) != 0);

    if (pUserInfo->bPasswordNeverExpires) {

        pUserInfo->bPasswordExpired = FALSE;

    } else {

        pUserInfo->bPasswordExpired = ((dwUserAccountCtrl & LSA_AD_UF_PASSWORD_EXPIRED) != 0);

    }

    return 0;
}

DWORD
ADParsePasswdInfo(
    HANDLE            hDirectory,
    LDAPMessage*      pMessageReal,
    PLSA_USER_INFO_2  pUserInfo)
{
    DWORD dwError = 0;

    PSTR  pszPwdLastSet = NULL;
    UINT64 u64PwdLastSet = 0;
    PSTR pszAccountExpired = NULL;
    UINT64 u64AccountExpired = 0;
    struct timeval current_tv;
    UINT64 u64current_NTtime = 0;

    gettimeofday(&current_tv, NULL);
    ADConvertTimeUnix2Nt(current_tv.tv_sec,
                         &u64current_NTtime);

    //process "accountExpires"
    dwError = LsaLdapGetString(
                hDirectory,
                pMessageReal,
                AD_LDAP_ACCOUT_EXP_TAG,
                &pszAccountExpired);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADStr2UINT64(
                pszAccountExpired,
                &u64AccountExpired);

    if (u64AccountExpired == 0LL || u64AccountExpired == 9223372036854775807LL)//this means the account will never be expired
        pUserInfo->bAccountExpired = FALSE;
    else{
        if (u64current_NTtime <= u64AccountExpired)
            pUserInfo->bAccountExpired = FALSE;
        else
            pUserInfo->bAccountExpired = TRUE;
    }

    //process "pwdLastSet"
    dwError = LsaLdapGetString(
                hDirectory,
                pMessageReal,
                AD_LDAP_PWD_LASTSET_TAG,
                &pszPwdLastSet);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADStr2UINT64(
                pszPwdLastSet,
                &u64PwdLastSet);

    if (pUserInfo->bPasswordNeverExpires ||
        pUserInfo->bPasswordExpired ||
        gpADProviderData->adMaxPwdAge == 0)//password never expires
        pUserInfo->dwDaysToPasswordExpiry = 0LL;
    else{
        if ((u64current_NTtime-u64PwdLastSet) >= gpADProviderData->adMaxPwdAge)//password is expired already
            pUserInfo->dwDaysToPasswordExpiry = 0LL;
        else {
            UINT64 NanosecsToPasswordExpiry = gpADProviderData->adMaxPwdAge - (u64current_NTtime-u64PwdLastSet);

            pUserInfo->dwDaysToPasswordExpiry = (NanosecsToPasswordExpiry/10000000LL)/(24*60*60);
        }
    }

cleanup:
    LSA_SAFE_FREE_STRING(pszPwdLastSet);
    LSA_SAFE_FREE_STRING(pszAccountExpired);

    return dwError;

error:

    goto cleanup;
}

DWORD
ADConvertTimeNt2Unix(
    UINT64 ntTime,
    PUINT64 pUnixTime
        )
{
    UINT64 unixTime = 0;

    unixTime = ntTime/10000000LL - 11644473600LL;

    *pUnixTime = unixTime;

    return 0;
}

DWORD
ADConvertTimeUnix2Nt(
    UINT64 unixTime,
    PUINT64 pNtTime
        )
{
    UINT64 ntTime = 0;

    ntTime = (unixTime+11644473600LL)*10000000LL;

    *pNtTime = ntTime;

    return 0;
}

DWORD
ADStr2UINT64(
    PSTR pszStr,
    PUINT64 pResult)
{
    UINT64 result = 0;

    if (pszStr){
        while (*pszStr != '\0')
        {
            result *= 10 ;
            result += *pszStr - '0';
            pszStr ++ ;
        }
    }

    *pResult = result;

    return 0;
}

DWORD
ADNonSchemaKeywordGetString(
    PSTR *ppszValues,
    DWORD dwNumValues,
    PCSTR pszAttributeName,
    PSTR *ppszResult
    )
{
    DWORD dwError = 0;
    size_t i;
    size_t sNameLen = strlen(pszAttributeName);
    PSTR pszResult = NULL;

    for (i = 0; i < dwNumValues; i++)
    {
        PCSTR pszValue = ppszValues[i];

        // Look for ldap values which are in the form <attributename>=<value>
        if (!strncasecmp(pszValue, pszAttributeName, sNameLen) &&
                pszValue[sNameLen] == '=')
        {
            dwError = LsaAllocateString(
                        pszValue + sNameLen + 1,
                        &pszResult);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
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
ADNonSchemaKeywordGetUInt32(
    PSTR *ppszValues,
    DWORD dwNumValues,
    PCSTR pszAttributeName,
    DWORD *pdwResult
    )
{
    size_t i;
    size_t sNameLen = strlen(pszAttributeName);

    for (i = 0; i < dwNumValues; i++)
    {
        PCSTR pszValue = ppszValues[i];
        // Don't free this
        PSTR pszEndPtr = NULL;

        // Look for ldap values which are in the form <attributename>=<value>
        if (!strncasecmp(pszValue, pszAttributeName, sNameLen) &&
                pszValue[sNameLen] == '=')
        {
            pszValue += sNameLen + 1;
            *pdwResult = strtoul(pszValue, &pszEndPtr, 10);
            if (pszEndPtr == NULL || *pszEndPtr != '\0' || pszEndPtr == pszValue)
            {
                // Couldn't parse the whole number
                return LSA_ERROR_INVALID_LDAP_ATTR_VALUE;
            }
            return LSA_ERROR_SUCCESS;
        }
    }

    // Couldn't find the attribute
    return LSA_ERROR_INVALID_LDAP_ATTR_VALUE;
}

DWORD
ADNonSchemaMarshalToUserCache(
    HANDLE                  hPseudoDirectory,
    HANDLE                  hRealDirectory,
    PLSA_LOGIN_NAME_INFO    pUserNameInfo,
    LDAPMessage*            pMessageReal,
    LDAPMessage*            pMessagePseudo,
    PAD_SECURITY_OBJECT*    ppUserInfo
    )
{
    DWORD dwError = 0;
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    struct timeval current_tv;
    PSTR* ppszValues = NULL;
    DWORD dwNumValues = 0;
    UCHAR* pucSIDBytes = NULL;
    DWORD dwSIDByteLength = 0;
    PSTR  pszUserDomainFQDN = NULL;
    PSTR  pszHomedir = NULL;

    dwError = LsaDmWrapGetDomainName(pUserNameInfo->pszDomainNetBiosName,
                                     &pszUserDomainFQDN,
                                     NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sizeof(AD_SECURITY_OBJECT),
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (gettimeofday(&current_tv, NULL) < 0)
    {
        dwError = errno;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pUserInfo->cache.qwCacheId = -1;
    pUserInfo->cache.tLastUpdated = current_tv.tv_sec;

    pUserInfo->type = AccountType_User;

    if (pMessageReal && hRealDirectory)
    {
        DWORD dwUserAccountCtrl = 0;

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
                    &pUserInfo->pszObjectSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapGetDN(
                hRealDirectory,
                pMessageReal,
                &pUserInfo->pszDN);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapGetString(
                    hRealDirectory,
                    pMessageReal,
                    AD_LDAP_SAM_NAME_TAG,
                    &pUserInfo->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pUserInfo->pszSamAccountName);

        dwError = LsaAllocateString(
                    pUserNameInfo->pszDomainNetBiosName,
                    &pUserInfo->pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaLdapGetUInt32(
                    hRealDirectory,
                    pMessageReal,
                    AD_LDAP_USER_CTRL_TAG,
                    &dwUserAccountCtrl);
        if (dwError == LSA_ERROR_INVALID_LDAP_ATTR_VALUE)
        {
            LSA_LOG_ERROR(
                    "User %s has an invalid value for the userAccountControl "
                    "attribute. Please check that it is set and that the "
                    "machine account has permission to read it.",
                    pUserInfo->pszDN);
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwError =  ADParseUserCtrlToCache(
                    dwUserAccountCtrl,
                    pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);

        dwError =  ADParsePasswdInfoToCache(
                     hRealDirectory,
                     pMessageReal,
                     pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        //at least objectSid is in pUserNameInfo
        dwError = LsaAllocateString(
                    pUserNameInfo->pszObjectSid,
                    &pUserInfo->pszObjectSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAllocateString(
                    pUserNameInfo->pszDomainNetBiosName,
                    &pUserInfo->pszNetbiosDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAllocateString(
                    pUserNameInfo->pszName,
                    &pUserInfo->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pUserInfo->pszSamAccountName);
    }

    dwError = ADGetLDAPUPNString(
                        hRealDirectory,
                        pMessageReal,
                        pszUserDomainFQDN,
                        pUserInfo->pszSamAccountName,
                        &pUserInfo->userInfo.pszUPN,
                        &pUserInfo->userInfo.bIsGeneratedUPN);
    BAIL_ON_LSA_ERROR(dwError);

    if (pMessagePseudo)
    {
        pUserInfo->enabled = TRUE;

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
                    AD_LDAP_UID_TAG,
                    (DWORD*)&pUserInfo->userInfo.uid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADNonSchemaKeywordGetUInt32(
                    ppszValues,
                    dwNumValues,
                    AD_LDAP_GID_TAG,
                    (DWORD*)&pUserInfo->userInfo.gid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADNonSchemaKeywordGetString(
                    ppszValues,
                    dwNumValues,
                    AD_LDAP_ALIAS_TAG,
                    &pUserInfo->userInfo.pszAliasName
                    );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADNonSchemaKeywordGetString(
                    ppszValues,
                    dwNumValues,
                    AD_LDAP_PASSWD_TAG,
                    &pUserInfo->userInfo.pszPasswd
                    );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADNonSchemaKeywordGetString(
                    ppszValues,
                    dwNumValues,
                    AD_LDAP_GECOS_TAG,
                    &pUserInfo->userInfo.pszGecos
                    );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADNonSchemaKeywordGetString(
                    ppszValues,
                    dwNumValues,
                    AD_LDAP_SHELL_TAG,
                    &pUserInfo->userInfo.pszShell
                    );
        BAIL_ON_LSA_ERROR(dwError);
        BAIL_ON_INVALID_STRING(pUserInfo->userInfo.pszShell);

        dwError = ADNonSchemaKeywordGetString(
                    ppszValues,
                    dwNumValues,
                    AD_LDAP_HOMEDIR_TAG,
                    &pUserInfo->userInfo.pszHomedir
                    );
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pUserInfo->userInfo.pszHomedir)
    {
        dwError = AD_GetUnprovisionedModeHomedirTemplate(
                    &pUserInfo->userInfo.pszHomedir);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (strstr(pUserInfo->userInfo.pszHomedir, "%"))
    {
        dwError = AD_BuildHomeDirFromTemplate(
                    pUserInfo->userInfo.pszHomedir,
                    pUserInfo->pszNetbiosDomainName,
                    pUserInfo->pszSamAccountName,
                    &pszHomedir);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_SAFE_FREE_STRING(pUserInfo->userInfo.pszHomedir);
        pUserInfo->userInfo.pszHomedir = pszHomedir;
        pszHomedir = NULL;
    }

    LsaStrCharReplace(pUserInfo->userInfo.pszHomedir, ' ', '_');

    BAIL_ON_INVALID_STRING(pUserInfo->userInfo.pszHomedir);

    *ppUserInfo = (PVOID)pUserInfo;

cleanup:

    LSA_SAFE_FREE_MEMORY(pucSIDBytes);
    LSA_SAFE_FREE_STRING(pszUserDomainFQDN);
    LSA_SAFE_FREE_STRING(pszHomedir);
    LsaFreeStringArray(ppszValues, dwNumValues);

    return dwError;

error:

    *ppUserInfo = NULL;

    ADCacheDB_SafeFreeObject(&pUserInfo);

    goto cleanup;
}

DWORD
AD_BuildHomeDirFromTemplate(
    PCSTR pszHomedirTemplate,
    PCSTR pszNetBIOSDomainName,
    PCSTR pszSamAccountName,
    PSTR* ppszHomedir
    )
{
    DWORD dwError = 0;
    PSTR pszHomedirPrefix = NULL;
    PSTR pszHomedir = NULL;
    DWORD dwOffset = 0;
    PCSTR pszIterTemplate = pszHomedirTemplate;
    DWORD dwBytesAllocated = 0;
    DWORD dwNetBIOSDomainNameLength = 0;
    DWORD dwSamAccountNameLength = 0;
    DWORD dwHomedirPrefixLength = 0;

    BAIL_ON_INVALID_STRING(pszHomedirTemplate);
    BAIL_ON_INVALID_STRING(pszNetBIOSDomainName);
    BAIL_ON_INVALID_STRING(pszSamAccountName);

    if (strstr(pszHomedirTemplate, "%H"))
    {
        dwError = AD_GetHomedirPrefixPath(&pszHomedirPrefix);
        BAIL_ON_LSA_ERROR(dwError);

        BAIL_ON_INVALID_STRING(pszHomedirPrefix);

        dwHomedirPrefixLength = strlen(pszHomedirPrefix);
    }

    dwNetBIOSDomainNameLength = strlen(pszNetBIOSDomainName);
    dwSamAccountNameLength = strlen(pszSamAccountName);

    // Handle common case where we might use all replacements.
    dwBytesAllocated = (strlen(pszHomedirTemplate) +
                        dwNetBIOSDomainNameLength +
                        dwSamAccountNameLength +
                        dwHomedirPrefixLength +
                        1);

    dwError = LsaAllocateMemory(
                    sizeof(CHAR) * dwBytesAllocated,
                    (PVOID*)&pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    while (pszIterTemplate[0])
    {
        // Do not count the terminating NULL as "available".
        DWORD dwBytesRemaining = dwBytesAllocated - dwOffset - 1;
        PCSTR pszInsert = NULL;
        DWORD dwInsertLength = 0;
        BOOLEAN bNeedUpper = FALSE;
        BOOLEAN bNeedLower = FALSE;

        LSA_ASSERT(dwOffset < dwBytesAllocated);

        if (pszIterTemplate[0] == '%')
        {
            switch (pszIterTemplate[1])
            {
                case 'D':
                    pszInsert = pszNetBIOSDomainName;
                    dwInsertLength = dwNetBIOSDomainNameLength;
                    bNeedUpper = TRUE;
                    break;
                case 'U':
                    pszInsert = pszSamAccountName;
                    dwInsertLength = dwSamAccountNameLength;
                    bNeedLower = TRUE;
                    break;
                case 'H':
                    pszInsert = pszHomedirPrefix;
                    dwInsertLength = dwHomedirPrefixLength;
                    break;
                default:
                    dwError = LSA_ERROR_INVALID_HOMEDIR_TEMPLATE;
                    BAIL_ON_LSA_ERROR(dwError);
            }
            LSA_ASSERT(!(bNeedUpper && bNeedLower));
            pszIterTemplate += 2;
        }
        else
        {
            PCSTR pszEnd = strchr(pszIterTemplate, '%');
            if (!pszEnd)
            {
                dwInsertLength = strlen(pszIterTemplate);
            }
            else
            {
                dwInsertLength = pszEnd - pszIterTemplate;
            }
            pszInsert = pszIterTemplate;
            pszIterTemplate += dwInsertLength;
        }

        if (dwBytesRemaining < dwInsertLength)
        {
            // We will increment by at least a minimum amount.
            DWORD dwAllocate = LSA_MAX(dwInsertLength - dwBytesRemaining, 64);
            PSTR pszNewHomedir = NULL;
            dwError = LsaReallocMemory(
                            pszHomedir,
                            (PVOID*)&pszNewHomedir,
                            dwBytesAllocated + dwAllocate);
            BAIL_ON_LSA_ERROR(dwError);
            pszHomedir = pszNewHomedir;
            dwBytesAllocated += dwAllocate;
        }
        memcpy(pszHomedir + dwOffset,
               pszInsert,
               dwInsertLength);
        if (bNeedUpper)
        {
            LsaStrnToUpper(pszHomedir + dwOffset, dwInsertLength);
        }
        else if (bNeedLower)
        {
            LsaStrnToLower(pszHomedir + dwOffset, dwInsertLength);
        }
        dwOffset += dwInsertLength;
    }

    // We should still have enough room for NULL.
    LSA_ASSERT(dwOffset < dwBytesAllocated);

    pszHomedir[dwOffset] = 0;
    dwOffset++;

    *ppszHomedir = pszHomedir;

cleanup:
    LSA_SAFE_FREE_STRING(pszHomedirPrefix);

    return dwError;

error:
    *ppszHomedir = NULL;
    LSA_SAFE_FREE_MEMORY(pszHomedir);

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
