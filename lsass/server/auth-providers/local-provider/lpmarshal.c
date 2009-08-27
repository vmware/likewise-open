/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lpmarshal.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider (Defines)
 *
 *        Marshal from DIRECTORY structures to lsass info levels
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
DWORD
LocalFindAttribute(
    PDIRECTORY_ENTRY      pEntry,
    PWSTR                 pwszAttrName,
    PDIRECTORY_ATTRIBUTE* ppAttr
    );

DWORD
LocalMarshalEntryToUserInfo_0(
    PDIRECTORY_ENTRY  pEntry,
    PWSTR*            ppwszUserDN,
    PLSA_USER_INFO_0* ppUserInfo
    )
{
    DWORD dwError = 0;
    wchar16_t wszAttrNameUID[]            = LOCAL_DIR_ATTR_UID;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_PRIMARY_GROUP;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNamePassword[]       = LOCAL_DIR_ATTR_PASSWORD;
    wchar16_t wszAttrNameGecos[]          = LOCAL_DIR_ATTR_GECOS;
    wchar16_t wszAttrNameShell[]          = LOCAL_DIR_ATTR_SHELL;
    wchar16_t wszAttrNameHomedir[]        = LOCAL_DIR_ATTR_HOME_DIR;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    wchar16_t wszAttrNameNetBIOSDomain[]  = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    PWSTR pwszUserDN = NULL;
    PSTR  pszNetBIOSDomain = NULL;
    PSTR  pszName = NULL;
    DWORD dwInfoLevel = 0;
    DWORD dwUid = 0;
    DWORD dwGid = 0;

    dwError = LwAllocateMemory(
                        sizeof(LSA_USER_INFO_0),
                        (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToInteger(
                    pEntry,
                    &wszAttrNameUID[0],
                    &dwUid);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->uid = dwUid;

    dwError = LocalMarshalAttrToInteger(
                    pEntry,
                    &wszAttrNameGID[0],
                    &dwGid);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->gid = dwGid;

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameSamAccountName[0],
                    &pszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNamePassword[0],
                    &pUserInfo->pszPasswd);
    if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameGecos[0],
                    &pUserInfo->pszGecos);
    if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameShell[0],
                    &pUserInfo->pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameHomedir[0],
                    &pUserInfo->pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameObjectSID[0],
                    &pUserInfo->pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameNetBIOSDomain[0],
                    &pszNetBIOSDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pUserInfo->pszName,
                    "%s\\%s",
                    pszNetBIOSDomain,
                    pszName);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppwszUserDN)
    {
        dwError = LocalMarshalAttrToUnicodeString(
                        pEntry,
                        &wszAttrNameDN[0],
                        &pwszUserDN);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (ppwszUserDN)
    {
        *ppwszUserDN = pwszUserDN;
    }
    *ppUserInfo = pUserInfo;

cleanup:

    LW_SAFE_FREE_STRING(pszNetBIOSDomain);
    LW_SAFE_FREE_STRING(pszName);

    return dwError;

error:

    if (ppwszUserDN)
    {
        *ppwszUserDN = NULL;
    }
    *ppUserInfo = NULL;

    LW_SAFE_FREE_MEMORY(pwszUserDN);

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
LocalMarshalEntryToUserInfo_1(
    PDIRECTORY_ENTRY  pEntry,
    PCSTR             pszDomainName,
    PWSTR*            ppwszUserDN,
    PLSA_USER_INFO_1* ppUserInfo
    )
{
    DWORD dwError = 0;
    wchar16_t wszAttrNameUID[]            = LOCAL_DIR_ATTR_UID;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_PRIMARY_GROUP;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNamePassword[]       = LOCAL_DIR_ATTR_PASSWORD;
    wchar16_t wszAttrNameGecos[]          = LOCAL_DIR_ATTR_GECOS;
    wchar16_t wszAttrNameShell[]          = LOCAL_DIR_ATTR_SHELL;
    wchar16_t wszAttrNameHomedir[]        = LOCAL_DIR_ATTR_HOME_DIR;
    wchar16_t wszAttrNameUPN[]            = LOCAL_DIR_ATTR_USER_PRINCIPAL_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    wchar16_t wszAttrNameNetBIOSDomain[]      = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PLSA_USER_INFO_1 pUserInfo = NULL;
    PWSTR  pwszUserDN = NULL;
    PSTR   pszNetBIOSDomain = NULL;
    PSTR   pszName = NULL;
    DWORD dwInfoLevel = 1;
    DWORD dwUid = 0;
    DWORD dwGid = 0;

    dwError = LwAllocateMemory(
                    sizeof(LSA_USER_INFO_1),
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToInteger(
                    pEntry,
                    &wszAttrNameUID[0],
                    &dwUid);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->uid = dwUid;

    dwError = LocalMarshalAttrToInteger(
                    pEntry,
                    &wszAttrNameGID[0],
                    &dwGid);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->gid = dwGid;

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameSamAccountName[0],
                    &pszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNamePassword[0],
                    &pUserInfo->pszPasswd);
    if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameGecos[0],
                    &pUserInfo->pszGecos);
    if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameShell[0],
                    &pUserInfo->pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameHomedir[0],
                    &pUserInfo->pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameObjectSID[0],
                    &pUserInfo->pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameUPN[0],
                    &pUserInfo->pszUPN);
    if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameNetBIOSDomain[0],
                    &pszNetBIOSDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pUserInfo->pszName,
                    "%s\\%s",
                    pszNetBIOSDomain,
                    pszName);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppwszUserDN)
    {
        dwError = LocalMarshalAttrToUnicodeString(
                        pEntry,
                        &wszAttrNameDN[0],
                        &pwszUserDN);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszUPN))
    {
        dwError = LwAllocateStringPrintf(
                        &pUserInfo->pszUPN,
                        "%s@%s",
                        pszName,
                        pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        LwStrToUpper(pUserInfo->pszUPN + strlen(pszName) + 1);

        pUserInfo->bIsGeneratedUPN = TRUE;
    }

    pUserInfo->bIsLocalUser = TRUE;

    if (ppwszUserDN)
    {
        *ppwszUserDN = pwszUserDN;
    }
    *ppUserInfo = pUserInfo;

cleanup:

    LW_SAFE_FREE_STRING(pszNetBIOSDomain);
    LW_SAFE_FREE_STRING(pszName);

    return dwError;

error:

    if (ppwszUserDN)
    {
        *ppwszUserDN = NULL;
    }
    *ppUserInfo = NULL;

    LW_SAFE_FREE_MEMORY(pwszUserDN);

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
LocalMarshalEntryToUserInfo_2(
    PDIRECTORY_ENTRY  pEntry,
    PCSTR             pszDomainName,
    PWSTR*            ppwszUserDN,
    PLSA_USER_INFO_2* ppUserInfo
    )
{
    DWORD dwError = 0;
    wchar16_t wszAttrNameUID[]            = LOCAL_DIR_ATTR_UID;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_PRIMARY_GROUP;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNamePassword[]       = LOCAL_DIR_ATTR_PASSWORD;
    wchar16_t wszAttrNameNTHash[]         = LOCAL_DIR_ATTR_NT_HASH;
    wchar16_t wszAttrNameLMHash[]         = LOCAL_DIR_ATTR_LM_HASH;
    wchar16_t wszAttrNameGecos[]          = LOCAL_DIR_ATTR_GECOS;
    wchar16_t wszAttrNameShell[]          = LOCAL_DIR_ATTR_SHELL;
    wchar16_t wszAttrNameHomedir[]        = LOCAL_DIR_ATTR_HOME_DIR;
    wchar16_t wszAttrNameUPN[]            = LOCAL_DIR_ATTR_USER_PRINCIPAL_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameUserInfoFlags[]  = LOCAL_DIR_ATTR_ACCOUNT_FLAGS;
    wchar16_t wszAttrNameAccountExpiry[]  = LOCAL_DIR_ATTR_ACCOUNT_EXPIRY;
    wchar16_t wszAttrNamePasswdLastSet[]  = LOCAL_DIR_ATTR_PASSWORD_LAST_SET;
    wchar16_t wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    wchar16_t wszAttrNameNetBIOSDomain[]      = LOCAL_DIR_ATTR_NETBIOS_NAME;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    PWSTR pwszUserDN = NULL;
    PSTR  pszName = NULL;
    PSTR  pszNetBIOSDomain = NULL;
    DWORD  dwInfoLevel = 2;
    DWORD  dwUserInfoFlags = 0;
    LONG64 llAccountExpiry = 0;
    LONG64 llPasswordLastSet = 0;
    DWORD  dwUid = 0;
    DWORD  dwGid = 0;

    dwError = LwAllocateMemory(
                    sizeof(LSA_USER_INFO_2),
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToInteger(
                    pEntry,
                    &wszAttrNameUID[0],
                    &dwUid);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->uid = dwUid;

    dwError = LocalMarshalAttrToInteger(
                    pEntry,
                    &wszAttrNameGID[0],
                    &dwGid);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->gid = dwGid;

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameSamAccountName[0],
                    &pszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNamePassword[0],
                    &pUserInfo->pszPasswd);
    if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameGecos[0],
                    &pUserInfo->pszGecos);
    if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameShell[0],
                    &pUserInfo->pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameHomedir[0],
                    &pUserInfo->pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameObjectSID[0],
                    &pUserInfo->pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameUPN[0],
                    &pUserInfo->pszUPN);
    if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameNetBIOSDomain[0],
                    &pszNetBIOSDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pUserInfo->pszName,
                    "%s\\%s",
                    pszNetBIOSDomain,
                    pszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToInteger(
                    pEntry,
                    &wszAttrNameUserInfoFlags[0],
                    &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToLargeInteger(
                        pEntry,
                        &wszAttrNameAccountExpiry[0],
                        &llAccountExpiry);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToLargeInteger(
                        pEntry,
                        &wszAttrNamePasswdLastSet[0],
                        &llPasswordLastSet);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToOctetStream(
                        pEntry,
                        &wszAttrNameNTHash[0],
                        &pUserInfo->pNTHash,
                        &pUserInfo->dwNTHashLen);
    if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToOctetStream(
                        pEntry,
                        &wszAttrNameLMHash[0],
                        &pUserInfo->pLMHash,
                        &pUserInfo->dwLMHashLen);
    if (dwError == LW_ERROR_NO_ATTRIBUTE_VALUE)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (ppwszUserDN)
    {
        dwError = LocalMarshalAttrToUnicodeString(
                        pEntry,
                        &wszAttrNameDN[0],
                        &pwszUserDN);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (LW_IS_NULL_OR_EMPTY_STR(pUserInfo->pszUPN))
    {
        dwError = LwAllocateStringPrintf(
                        &pUserInfo->pszUPN,
                        "%s@%s",
                        pszName,
                        pszDomainName);
        BAIL_ON_LSA_ERROR(dwError);

        LwStrToUpper(pUserInfo->pszUPN + strlen(pszName) + 1);

        pUserInfo->bIsGeneratedUPN = TRUE;
    }

    pUserInfo->bIsLocalUser = TRUE;

    dwError = LocalMarshallAccountFlags(
                    pUserInfo,
                    dwUserInfoFlags,
                    llPasswordLastSet,
                    llAccountExpiry);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppwszUserDN)
    {
        *ppwszUserDN = pwszUserDN;
    }
    *ppUserInfo = pUserInfo;

cleanup:

    LW_SAFE_FREE_STRING(pszNetBIOSDomain);
    LW_SAFE_FREE_STRING(pszName);

    return dwError;

error:

    if (ppwszUserDN)
    {
        *ppwszUserDN = NULL;
    }
    *ppUserInfo = NULL;

    LW_SAFE_FREE_MEMORY(pwszUserDN);

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
LocalMarshallAccountFlags(
    PLSA_USER_INFO_2 pUserInfo,
    DWORD            dwUserInfoFlags,
    LONG64           llPwdLastSet,
    LONG64           llAcctExpiry
    )
{
    DWORD dwError = 0;
    BOOLEAN bPasswordNeverExpires = FALSE;
    BOOLEAN bAccountDisabled = FALSE;
    BOOLEAN bUserCanChangePassword = FALSE;
    BOOLEAN bAccountLocked = FALSE;
    BOOLEAN bAccountExpired = FALSE;
    BOOLEAN bPasswordExpired = FALSE;
    BOOLEAN bPromptPasswordChange = FALSE;
    DWORD   dwDaysToPasswordExpiry = 0;

    if (dwUserInfoFlags & LOCAL_ACB_PWNOEXP)
    {
        bPasswordNeverExpires = TRUE;
    }

    if (dwUserInfoFlags & LOCAL_ACB_DISABLED)
    {
        bAccountDisabled = TRUE;
    }

    if (dwUserInfoFlags & LOCAL_ACB_DOMTRUST)
    {
        bUserCanChangePassword = TRUE;
    }

    if (dwUserInfoFlags & LOCAL_ACB_NORMAL)
    {
        bAccountLocked = TRUE;
    }

    if (!bPasswordNeverExpires)
    {
        // Password expires

        if (dwUserInfoFlags & LOCAL_ACB_PW_EXPIRED)
        {
            bPasswordExpired = TRUE;
            dwDaysToPasswordExpiry = 0;
            bPromptPasswordChange = TRUE;
        }
        else
        {
            // Account has not expired yet

            LONG64 llMaxPwdAge = 0;
            LONG64 llPwdChangeTime = 0;
            LONG64 llCurTime = 0;
            LONG64 llTimeToExpiry = 0;

            dwError = LocalCfgGetMaxPasswordAge(&llMaxPwdAge);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LocalCfgGetPasswordChangeWarningTime(&llPwdChangeTime);
            BAIL_ON_LSA_ERROR(dwError);

            bPasswordExpired = FALSE;

            llCurTime = LocalGetNTTime(time(NULL));
            llTimeToExpiry = llMaxPwdAge - (llCurTime - llPwdLastSet);

            if (llTimeToExpiry <= llPwdChangeTime)
            {
                bPromptPasswordChange = TRUE;
            }
            else
            {
                bPromptPasswordChange = FALSE;
            }

            dwDaysToPasswordExpiry = llTimeToExpiry/(24 * 60 * 60 * 10000000LL);
        }
    }
    else
    {
        // Password never expires
        bPasswordExpired = FALSE;
        dwDaysToPasswordExpiry = 0;
        bPromptPasswordChange = FALSE;
    }

    if (llAcctExpiry)
    {
        LONG64 llCurTime = LocalGetNTTime(time(NULL));

        bAccountExpired = (llCurTime > llAcctExpiry) ? TRUE : FALSE;
    }

    pUserInfo->bAccountDisabled       = bAccountDisabled;
    pUserInfo->bAccountExpired        = bAccountExpired;
    pUserInfo->bAccountLocked         = bAccountLocked;
    pUserInfo->bPasswordExpired       = bPasswordExpired;
    pUserInfo->bPasswordNeverExpires  = bPasswordNeverExpires;
    pUserInfo->bPromptPasswordChange  = bPromptPasswordChange;
    pUserInfo->bUserCanChangePassword = bUserCanChangePassword;
    pUserInfo->dwDaysToPasswordExpiry = dwDaysToPasswordExpiry;

error:

    return dwError;
}

DWORD
LocalMarshalEntryToGroupInfo_0(
    PDIRECTORY_ENTRY   pEntry,
    PWSTR*             ppwszGroupDN,
    PLSA_GROUP_INFO_0* ppGroupInfo
    )
{
    DWORD dwError = 0;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameNetBIOSDomain[]      = LOCAL_DIR_ATTR_NETBIOS_NAME;
    DWORD dwInfoLevel = 0;
    PSTR  pszNetBIOSDomain = NULL;
    PSTR  pszName = NULL;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    PWSTR pwszGroupDN = NULL;
    DWORD dwGid = 0;

    dwError = LwAllocateMemory(
                        sizeof(LSA_GROUP_INFO_0),
                        (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToInteger(
                    pEntry,
                    &wszAttrNameGID[0],
                    &dwGid);
    BAIL_ON_LSA_ERROR(dwError);

    pGroupInfo->gid = dwGid;

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameSamAccountName[0],
                    &pszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameObjectSID[0],
                    &pGroupInfo->pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameNetBIOSDomain[0],
                    &pszNetBIOSDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pGroupInfo->pszName,
                    "%s\\%s",
                    pszNetBIOSDomain,
                    pszName);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppwszGroupDN)
    {
        dwError = LocalMarshalAttrToUnicodeString(
                        pEntry,
                        &wszAttrNameDN[0],
                        &pwszGroupDN);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (ppwszGroupDN)
    {
        *ppwszGroupDN = pwszGroupDN;
    }
    *ppGroupInfo = pGroupInfo;

cleanup:

    LW_SAFE_FREE_STRING(pszNetBIOSDomain);
    LW_SAFE_FREE_STRING(pszName);

    return dwError;

error:

    if (ppwszGroupDN)
    {
        *ppwszGroupDN = NULL;
    }
    *ppGroupInfo = NULL;

    LW_SAFE_FREE_MEMORY(pwszGroupDN);

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
LocalMarshalEntryToGroupInfo_1(
    PDIRECTORY_ENTRY   pEntry,
    PWSTR*             ppwszGroupDN,
    PLSA_GROUP_INFO_1* ppGroupInfo
    )
{
    DWORD dwError = 0;
    wchar16_t wszAttrNameGID[]            = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameDN[]             = LOCAL_DIR_ATTR_DISTINGUISHED_NAME;
    wchar16_t wszAttrNameObjectSID[]      = LOCAL_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameNetBIOSDomain[]      = LOCAL_DIR_ATTR_NETBIOS_NAME;
    DWORD dwInfoLevel = 1;
    PSTR  pszName = NULL;
    PSTR  pszNetBIOSDomain = NULL;
    PLSA_GROUP_INFO_1 pGroupInfo = NULL;
    PWSTR pwszGroupDN = NULL;
    DWORD dwGid = 0;

    dwError = LwAllocateMemory(
                        sizeof(LSA_GROUP_INFO_1),
                        (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToInteger(
                    pEntry,
                    &wszAttrNameGID[0],
                    &dwGid);
    BAIL_ON_LSA_ERROR(dwError);

    pGroupInfo->gid = dwGid;

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameSamAccountName[0],
                    &pszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
		    &wszAttrNameDN[0],
		    &pGroupInfo->pszDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameObjectSID[0],
                    &pGroupInfo->pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalMarshalAttrToANSIFromUnicodeString(
                    pEntry,
                    &wszAttrNameNetBIOSDomain[0],
                    &pszNetBIOSDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pGroupInfo->pszName,
                    "%s\\%s",
                    pszNetBIOSDomain,
                    pszName);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppwszGroupDN)
    {
        dwError = LocalMarshalAttrToUnicodeString(
                        pEntry,
                        &wszAttrNameDN[0],
                        &pwszGroupDN);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (ppwszGroupDN)
    {
        *ppwszGroupDN = pwszGroupDN;
    }
    *ppGroupInfo = pGroupInfo;

cleanup:

    LW_SAFE_FREE_STRING(pszNetBIOSDomain);
    LW_SAFE_FREE_STRING(pszName);

    return dwError;

error:

    if (ppwszGroupDN)
    {
        *ppwszGroupDN = NULL;
    }
    *ppGroupInfo = NULL;

    LW_SAFE_FREE_MEMORY(pwszGroupDN);

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
LocalMarshalEntryToGroupInfoMembers_1(
    PLOCAL_PROVIDER_GROUP_MEMBER* ppMemberEntries,
    DWORD                         dwNumMemberEntries,
    PSTR**                        pppszMembers
    )
{
    DWORD dwError = 0;
    PSTR* ppszMembers = NULL;
    DWORD iMember = 0;

    dwError = LwAllocateMemory(
                    (dwNumMemberEntries + 1) * sizeof(PSTR),
                    (PVOID*)&ppszMembers);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iMember < dwNumMemberEntries; iMember++)
    {
        dwError = LwAllocateStringPrintf(
                        &ppszMembers[iMember],
                        "%s\\%s",
                        ppMemberEntries[iMember]->pszNetbiosDomain,
                        ppMemberEntries[iMember]->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppszMembers = ppszMembers;

cleanup:

    return dwError;

error:

    *ppszMembers = NULL;

    if (ppszMembers)
    {
        LwFreeStringArray(ppszMembers, dwNumMemberEntries);
    }

    goto cleanup;
}

DWORD
LocalMarshalAttrToInteger(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PDWORD           pdwValue
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_INTEGER)
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    *pdwValue = pAttrValue->data.ulValue;

cleanup:

    return dwError;

error:

    *pdwValue = 0;

    goto cleanup;
}

DWORD
LocalMarshalAttrToLargeInteger(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PLONG64          pllValue
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_LARGE_INTEGER)
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    *pllValue = pAttrValue->data.llValue;

cleanup:

    return dwError;

error:

    *pllValue = 0;

    goto cleanup;
}

DWORD
LocalMarshalAttrToANSIString(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PSTR*            ppszValue
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;
    PSTR             pszValue = NULL;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_ANSI_STRING)
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
        else if (!pAttrValue->data.pszStringValue)
        {
            dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    pAttrValue->data.pszStringValue,
                    &pszValue);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszValue = pszValue;

cleanup:

    return dwError;

error:

    *ppszValue = NULL;

    LW_SAFE_FREE_STRING(pszValue);

    goto cleanup;
}

DWORD
LocalMarshalAttrToUnicodeString(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PWSTR*           ppwszValue
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;
    PWSTR            pwszValue = NULL;
    DWORD            dwLen = 0;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
        else if (!pAttrValue->data.pwszStringValue)
        {
            dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwLen = wc16slen(pAttrValue->data.pwszStringValue);

    dwError = LwAllocateMemory(
                    (dwLen + 1) * sizeof(wchar16_t),
                    (PVOID*)&pwszValue);
    BAIL_ON_LSA_ERROR(dwError);

    memcpy((PBYTE)pwszValue,
           (PBYTE)pAttrValue->data.pwszStringValue,
           dwLen * sizeof(wchar16_t));

    *ppwszValue = pwszValue;

cleanup:

    return dwError;

error:

    *ppwszValue = NULL;

    LW_SAFE_FREE_MEMORY(pwszValue);

    goto cleanup;
}

DWORD
LocalMarshalAttrToANSIFromUnicodeString(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PSTR*            ppszValue
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;
    PSTR             pszValue = NULL;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
        else if (!pAttrValue->data.pwszStringValue)
        {
            dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaWc16sToMbs(
                    pAttrValue->data.pwszStringValue,
                    &pszValue);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszValue = pszValue;

cleanup:

    return dwError;

error:

    *ppszValue = NULL;

    LW_SAFE_FREE_STRING(pszValue);

    goto cleanup;
}

DWORD
LocalMarshalAttrToOctetStream(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PBYTE*           ppData,
    PDWORD           pdwDataLen
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;
    PBYTE            pData = NULL;
    DWORD            dwDataLen = 0;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_OCTET_STREAM)
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
        else if (!pAttrValue->data.pOctetString ||
                 !pAttrValue->data.pOctetString->pBytes ||
                 !pAttrValue->data.pOctetString->ulNumBytes)
        {
            dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    pAttrValue->data.pOctetString->ulNumBytes,
                    (PVOID*)&pData);
    BAIL_ON_LSA_ERROR(dwError);

    dwDataLen = pAttrValue->data.pOctetString->ulNumBytes;

    memcpy(pData, pAttrValue->data.pOctetString->pBytes, dwDataLen);

    *ppData = pData;
    *pdwDataLen = dwDataLen;

cleanup:

    return dwError;

error:

    *ppData = NULL;
    *pdwDataLen = 0;

    LW_SAFE_FREE_MEMORY(pData);

    goto cleanup;
}

DWORD
LocalMarshalAttrToBOOLEAN(
    PDIRECTORY_ENTRY pEntry,
    PWSTR            pwszAttrName,
    PBOOLEAN         pbValue
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrValue = NULL;

    BAIL_ON_INVALID_POINTER(pEntry);

    dwError = LocalFindAttribute(
                    pEntry,
                    pwszAttrName,
                    &pAttr);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAttr->ulNumValues > 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
    }
    else if (pAttr->ulNumValues == 0)
    {
        dwError = LW_ERROR_NO_ATTRIBUTE_VALUE;
    }
    else
    {
        pAttrValue = &pAttr->pValues[0];

        if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_BOOLEAN)
        {
            dwError = LW_ERROR_INVALID_ATTRIBUTE_VALUE;
        }
    }
    BAIL_ON_LSA_ERROR(dwError);

    *pbValue = pAttrValue->data.bBooleanValue;

cleanup:

    return dwError;

error:

    *pbValue = FALSE;

    goto cleanup;
}

static
DWORD
LocalFindAttribute(
    PDIRECTORY_ENTRY      pEntry,
    PWSTR                 pwszAttrName,
    PDIRECTORY_ATTRIBUTE* ppAttr
    )
{
    DWORD dwError = 0;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    DWORD iAttr = 0;

    for (; iAttr < pEntry->ulNumAttributes; iAttr++)
    {
        pAttr = &pEntry->pAttributes[iAttr];

        if (!wc16scasecmp(pAttr->pwszName, pwszAttrName))
        {
            break;
        }

        pAttr = NULL;
    }

    if (!pAttr)
    {
        dwError = LW_ERROR_NO_SUCH_ATTRIBUTE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppAttr = pAttr;

cleanup:

    return dwError;

error:

    *ppAttr = NULL;

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
