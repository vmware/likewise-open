/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        userinfo.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Authentication Provider Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "includes.h"


static
void
LsaFreeUserInfoContents_0(
    PLSA_USER_INFO_0 pUserInfo
    );

static
void
LsaFreeUserInfoContents_1(
    PLSA_USER_INFO_1 pUserInfo
    );

static
void
LsaFreeUserInfoContents_2(
    PLSA_USER_INFO_2 pUserInfo
    );

static
DWORD
LsaModifyUser_SetPasswordHash(
    PLW_LSA_DATA_BLOB *ppHashBlob,
    PCSTR pszHash
    );


static
void
LsaFreeUserInfoContents_0(
    PLSA_USER_INFO_0 pUserInfo
    )
{
    LSA_SAFE_FREE_STRING(pUserInfo->pszName);
    LSA_SAFE_FREE_STRING(pUserInfo->pszPasswd);
    LSA_SAFE_FREE_STRING(pUserInfo->pszGecos);
    LSA_SAFE_FREE_STRING(pUserInfo->pszShell);
    LSA_SAFE_FREE_STRING(pUserInfo->pszHomedir);
    LSA_SAFE_FREE_STRING(pUserInfo->pszSid);
}

static
void
LsaFreeUserInfoContents_1(
    PLSA_USER_INFO_1 pUserInfo
    )
{
    LsaFreeUserInfoContents_0(&pUserInfo->info0);
    LSA_SAFE_FREE_STRING(pUserInfo->pszDN);
    LSA_SAFE_FREE_STRING(pUserInfo->pszUPN);
    LSA_SAFE_FREE_MEMORY(pUserInfo->pLMHash);
    LSA_SAFE_FREE_MEMORY(pUserInfo->pNTHash);
}

static
void
LsaFreeUserInfoContents_2(
    PLSA_USER_INFO_2 pUserInfo
    )
{
    LsaFreeUserInfoContents_1(&pUserInfo->info1);
}

CHAR
LsaGetDomainSeparator(
    VOID
    )
{
    return gchDomainSeparator;
}

DWORD
LsaSetDomainSeparator(
    CHAR chValue
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    if (!ispunct((int)chValue))
    {
        LSA_LOG_ERROR(
                "Error: the domain separator must be set to a punctuation character; the value provided is '%c'.",
                chValue);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    gchDomainSeparator = chValue;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaCrackDomainQualifiedName(
    PCSTR pszId,
    PCSTR pszDefaultDomain,
    PLSA_LOGIN_NAME_INFO* ppNameInfo
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pNameInfo = NULL;
    PCSTR pszIndex = NULL;
    int idx = 0;

    dwError = LsaAllocateMemory(
                    sizeof(LSA_LOGIN_NAME_INFO),
                    (PVOID*)&pNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if ((pszIndex = strchr(pszId, LsaGetDomainSeparator())) != NULL) {
        idx = pszIndex-pszId;
        dwError = LsaStrndup(pszId, idx, &pNameInfo->pszDomainNetBiosName);
        BAIL_ON_LSA_ERROR(dwError);

        if (!IsNullOrEmptyString(pszId+idx+1)) {
            dwError = LsaAllocateString(pszId+idx+1, &pNameInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        pNameInfo->nameType = NameType_NT4;
    }
    else if ((pszIndex = strchr(pszId, '@')) != NULL) {
        idx = pszIndex-pszId;
        dwError = LsaStrndup(pszId, idx, &pNameInfo->pszName);
        BAIL_ON_LSA_ERROR(dwError);

        if (!IsNullOrEmptyString(pszId+idx+1)) {
            dwError = LsaAllocateString(pszId+idx+1, &pNameInfo->pszDomainNetBiosName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        pNameInfo->nameType = NameType_UPN;
    }
    else {
        dwError = LsaAllocateString(pszId, &pNameInfo->pszName);
        BAIL_ON_LSA_ERROR(dwError);
        pNameInfo->nameType = NameType_Alias;
    }

    if (IsNullOrEmptyString(pNameInfo->pszDomainNetBiosName) &&
        !IsNullOrEmptyString(pszDefaultDomain)) {
       dwError = LsaAllocateString(pszDefaultDomain, &pNameInfo->pszDomainNetBiosName);
       BAIL_ON_LSA_ERROR(dwError);
    }

    *ppNameInfo = pNameInfo;

cleanup:

    return(dwError);

error:

    *ppNameInfo = NULL;

    if (pNameInfo)
    {
        LsaFreeNameInfo(pNameInfo);
    }

    goto cleanup;
}

void
LsaFreeNameInfo(
    PLSA_LOGIN_NAME_INFO pNameInfo
    )
{
    LSA_SAFE_FREE_STRING(pNameInfo->pszDomainNetBiosName);
    LSA_SAFE_FREE_STRING(pNameInfo->pszName);
    LSA_SAFE_FREE_STRING(pNameInfo->pszFullDomainName);
    LSA_SAFE_FREE_STRING(pNameInfo->pszObjectSid);
    LsaFreeMemory(pNameInfo);
}

void
LsaFreeUserInfo(
    DWORD dwLevel,
    PVOID pUserInfo
    )
{
    switch(dwLevel)
    {
        case 0:
        {
            LsaFreeUserInfoContents_0((PLSA_USER_INFO_0)pUserInfo);
            break;
        }
        case 1:
        {
            LsaFreeUserInfoContents_1((PLSA_USER_INFO_1)pUserInfo);
            break;
        }
        case 2:
        {
            LsaFreeUserInfoContents_2((PLSA_USER_INFO_2)pUserInfo);
            break;
        }
        default:
        {
            LSA_LOG_ERROR("Unsupported User Info Level [%d]", dwLevel);
        }
    }
    if (dwLevel < 3)
    {
        LSA_SAFE_FREE_MEMORY(pUserInfo);
    }
}

DWORD
LsaBuildUserModInfo(
    uid_t uid,
    PLSA_USER_MOD_INFO* ppUserModInfo
    )
{
    DWORD dwError = 0;
    PLSA_USER_MOD_INFO pUserModInfo = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(LSA_USER_MOD_INFO),
                    (PVOID*)&pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);

    pUserModInfo->uid = uid;

    *ppUserModInfo = pUserModInfo;

cleanup:

    return dwError;

error:

    *ppUserModInfo = NULL;

    if (pUserModInfo) {
       LsaFreeUserModInfo(pUserModInfo);
    }

    goto cleanup;
}

DWORD
LsaModifyUser_EnableUser(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    pUserModInfo->actions.bEnableUser = bValue;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_DisableUser(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    pUserModInfo->actions.bDisableUser = bValue;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_Unlock(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    pUserModInfo->actions.bUnlockUser = bValue;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_AddToGroups(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszGroupList
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    LSA_SAFE_FREE_STRING(pUserModInfo->pszAddToGroups);

    if (!IsNullOrEmptyString(pszGroupList))
    {
       dwError = LsaAllocateString(
                   pszGroupList,
                   &pUserModInfo->pszAddToGroups);
       BAIL_ON_LSA_ERROR(dwError);

       pUserModInfo->actions.bAddToGroups = TRUE;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_RemoveFromGroups(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszGroupList
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    LSA_SAFE_FREE_STRING(pUserModInfo->pszRemoveFromGroups);

    if (!IsNullOrEmptyString(pszGroupList))
    {
       dwError = LsaAllocateString(
                   pszGroupList,
                   &pUserModInfo->pszRemoveFromGroups);
       BAIL_ON_LSA_ERROR(dwError);

       pUserModInfo->actions.bRemoveFromGroups = TRUE;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_ChangePasswordAtNextLogon(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    pUserModInfo->actions.bSetChangePasswordOnNextLogon = bValue;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_SetPasswordNeverExpires(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    pUserModInfo->actions.bSetPasswordNeverExpires = bValue;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_SetPasswordMustExpire(
    PLSA_USER_MOD_INFO pUserModInfo,
    BOOLEAN bValue
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    pUserModInfo->actions.bSetPasswordMustExpire = bValue;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_SetAccountExpiryDate(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszDate
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserModInfo);

    LSA_SAFE_FREE_STRING(pUserModInfo->pszExpiryDate);

    if (!IsNullOrEmptyString(pszDate)) {
       struct tm timebuf;

       if (NULL == strptime(pszDate, "%Y-%m-%d", &timebuf)) {
          dwError = LW_ERROR_FAILED_TIME_CONVERSION;
          BAIL_ON_LSA_ERROR(dwError);
       }

       dwError = LsaAllocateString(pszDate, &pUserModInfo->pszExpiryDate);
       BAIL_ON_LSA_ERROR(dwError);

       pUserModInfo->actions.bSetAccountExpiryDate = TRUE;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaModifyUser_SetNtPasswordHash(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszHash
    )
{
    DWORD dwError = 0;

    dwError = LsaModifyUser_SetPasswordHash(
                   &pUserModInfo->pNtPasswordHash,
                   pszHash);
    BAIL_ON_LSA_ERROR(dwError);

    pUserModInfo->actions.bSetNtPasswordHash = TRUE;

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaModifyUser_SetLmPasswordHash(
    PLSA_USER_MOD_INFO pUserModInfo,
    PCSTR pszHash
    )
{
    DWORD dwError = 0;

    dwError = LsaModifyUser_SetPasswordHash(
                   &pUserModInfo->pLmPasswordHash,
                   pszHash);
    BAIL_ON_LSA_ERROR(dwError);

    pUserModInfo->actions.bSetLmPasswordHash = TRUE;

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaModifyUser_SetPasswordHash(
    PLW_LSA_DATA_BLOB *ppHashBlob,
    PCSTR pszHash
    )
{
    DWORD dwError = 0;
    BYTE Hash[16] = {0};
    PCSTR pszHashCursor = NULL;
    DWORD i = 0;
    int ret = 0;
    PLW_LSA_DATA_BLOB pHashBlob = NULL;

    BAIL_ON_INVALID_POINTER(ppHashBlob);
    BAIL_ON_INVALID_POINTER(pszHash);

    if (!IsNullOrEmptyString(pszHash)) {
        for (i = 0, pszHashCursor = pszHash;
             pszHashCursor[0] && pszHashCursor[1] && i < sizeof(Hash);
             i++, pszHashCursor += 2)
        {
            ret = sscanf(pszHashCursor, "%02hhx", &Hash[i]);
            if (ret == 0) {
                dwError = LW_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    dwError = LsaAllocateMemory(sizeof(*pHashBlob),
                                (PVOID*)&pHashBlob);
    BAIL_ON_LSA_ERROR(dwError);

    pHashBlob->dwLen = sizeof(Hash);

    dwError = LsaAllocateMemory(pHashBlob->dwLen,
                                (PVOID*)&pHashBlob->pData);
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(pHashBlob->pData, Hash, pHashBlob->dwLen);

    *ppHashBlob = pHashBlob;

cleanup:
    return dwError;

error:
    if (pHashBlob &&
        pHashBlob->pData) {
        LSA_SAFE_FREE_MEMORY(pHashBlob->pData);
    }

    if (pHashBlob) {
        LSA_SAFE_FREE_MEMORY(pHashBlob);
    }

    *ppHashBlob = NULL;

    goto cleanup;
}

void
LsaFreeUserModInfo(
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    LSA_SAFE_FREE_STRING(pUserModInfo->pszAddToGroups);
    LSA_SAFE_FREE_STRING(pUserModInfo->pszRemoveFromGroups);
    LSA_SAFE_FREE_STRING(pUserModInfo->pszExpiryDate);

    if (pUserModInfo->pNtPasswordHash) {
        LSA_SAFE_FREE_MEMORY(pUserModInfo->pNtPasswordHash->pData);
    }
    LSA_SAFE_FREE_MEMORY(pUserModInfo->pNtPasswordHash);

    if (pUserModInfo->pLmPasswordHash) {
        LSA_SAFE_FREE_MEMORY(pUserModInfo->pLmPasswordHash->pData);
    }
    LSA_SAFE_FREE_MEMORY(pUserModInfo->pLmPasswordHash);

    LsaFreeMemory(pUserModInfo);
}

void
LsaFreeUserInfoList(
    DWORD  dwLevel,
    PVOID* ppUserInfoList,
    DWORD  dwNumUsers
    )
{
    int iUser = 0;
    for (; iUser < dwNumUsers; iUser++) {
        PVOID pUserInfo = *(ppUserInfoList+iUser);
        if (pUserInfo) {
           LsaFreeUserInfo(dwLevel, pUserInfo);
        }
    }
    LsaFreeMemory(ppUserInfoList);
}

void
LsaFreeIpcUserInfoList(
    PLSA_USER_INFO_LIST pUserIpcInfoList
    )
{
    if (pUserIpcInfoList)
    {
        switch (pUserIpcInfoList->dwUserInfoLevel)
        {
            case 0:
                LsaFreeUserInfoList(
                        0,
                        (PVOID*)pUserIpcInfoList->ppUserInfoList.ppInfoList0,
                        pUserIpcInfoList->dwNumUsers);
                break;
            case 1:
                LsaFreeUserInfoList(
                        1,
                        (PVOID*)pUserIpcInfoList->ppUserInfoList.ppInfoList1,
                        pUserIpcInfoList->dwNumUsers);
                break;
            case 2:
                LsaFreeUserInfoList(
                        2,
                        (PVOID*)pUserIpcInfoList->ppUserInfoList.ppInfoList2,
                        pUserIpcInfoList->dwNumUsers);
                break;

            default:
            {
                LSA_LOG_ERROR(
                        "Unsupported User Info Level [%d]",
                        pUserIpcInfoList->dwUserInfoLevel);
            }
        }
        LsaFreeMemory(pUserIpcInfoList);
    }
}

DWORD
LsaValidateUserName(
    PCSTR pszName
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pParsedName = NULL;
    size_t sNameLen = 0;

    dwError = LsaCrackDomainQualifiedName(
                pszName,
                "unset",
                &pParsedName);
    BAIL_ON_LSA_ERROR(dwError);

    if (pParsedName->pszName == NULL)
    {
        dwError = LW_ERROR_INVALID_USER_NAME;
        BAIL_ON_LSA_ERROR(dwError);
    }

    sNameLen = strlen(pParsedName->pszName);
    if (sNameLen > LSA_MAX_USER_NAME_LENGTH || sNameLen == 0)
    {
        dwError = LW_ERROR_INVALID_USER_NAME;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pParsedName != NULL)
    {
        LsaFreeNameInfo(pParsedName);
    }
    return dwError;

error:

    goto cleanup;
}

DWORD
LsaValidateUserInfoLevel(
    DWORD dwUserInfoLevel
    )
{
    return ((dwUserInfoLevel >= 0) &&
            (dwUserInfoLevel <= 2)) ? 0 : LW_ERROR_INVALID_USER_INFO_LEVEL;
}

DWORD
LsaValidateUserInfo(
    PVOID pUserInfo,
    DWORD dwUserInfoLevel
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_POINTER(pUserInfo);

    dwError = LsaValidateUserInfoLevel(dwUserInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

    switch (dwUserInfoLevel)
    {
        case 0:

            {
                PLSA_USER_INFO_0 pUserInfo_0 = (PLSA_USER_INFO_0)pUserInfo;

                dwError = LsaValidateUserName(pUserInfo_0->pszName);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }

        case 1:

            {
                PLSA_USER_INFO_1 pUserInfo_1 = (PLSA_USER_INFO_1)pUserInfo;

                dwError = LsaValidateUserName(pUserInfo_1->pszName);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }

        case 2:

            {
                PLSA_USER_INFO_2 pUserInfo_2 = (PLSA_USER_INFO_2)pUserInfo;

                dwError = LsaValidateUserName(pUserInfo_2->pszName);
                BAIL_ON_LSA_ERROR(dwError);

                break;
            }

        default:

            dwError = LW_ERROR_UNSUPPORTED_USER_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);

            break;
    }

cleanup:

    return dwError;

error:

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
