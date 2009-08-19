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
 *        lpmain.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Gerald Carter <gcarter@likewise.com>
 */

#include "includes.h"

DWORD
LSA_INITIALIZE_PROVIDER(local)(
    PCSTR pszConfigFilePath,
    PSTR* ppszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE* ppFunctionTable)
{
    DWORD dwError = 0;
    LOCAL_CONFIG config;
    BOOLEAN bEventLogEnabled = FALSE;
    PWSTR   pwszUserDN = NULL;
    PWSTR   pwszCredentials = NULL;
    ULONG   ulMethod = 0;

    memset(&config, 0, sizeof(config));

    dwError = LocalCfgInitialize(&config);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalGetDomainInfo(
                    pwszUserDN,
                    pwszCredentials,
                    ulMethod,
                    &gLPGlobals.pszLocalDomain,
                    &gLPGlobals.pszNetBIOSName,
                    &gLPGlobals.pLocalDomainSID,
                    &gLPGlobals.llMaxPwdAge,
                    &gLPGlobals.llPwdChangeTime);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszConfigFilePath))
    {
        dwError = LocalCfgParseFile(
                        pszConfigFilePath,
                        &config);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateString(
                        pszConfigFilePath,
                        &gLPGlobals.pszConfigFilePath);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalCfgTransferContents(
                    &config,
                    &gLPGlobals.cfg);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCfgIsEventlogEnabled(&bEventLogEnabled);
    BAIL_ON_LSA_ERROR(dwError);

    if (bEventLogEnabled)
    {
        LocalEventLogServiceStart(dwError);
    }

    *ppszProviderName = (PSTR)gpszLocalProviderName;
    *ppFunctionTable = &gLocalProviderAPITable;

cleanup:

    return dwError;

error:

    if (bEventLogEnabled)
    {
        LocalEventLogServiceStart(dwError);
    }

    LocalCfgFreeContents(&config);

    *ppszProviderName = NULL;
    *ppFunctionTable = NULL;

    goto cleanup;
}

DWORD
LocalOpenHandle(
    uid_t uid,
    gid_t gid,
    PHANDLE phProvider
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = NULL;

    dwError = LwAllocateMemory(
                    sizeof(LOCAL_PROVIDER_CONTEXT),
                    (PVOID*)&pContext);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_mutex_init(&pContext->mutex, NULL);
    pContext->pMutex = &pContext->mutex;

    pContext->uid = uid;
    pContext->gid = gid;
    pContext->localAdminState = LOCAL_ADMIN_STATE_NOT_DETERMINED;

    dwError = DirectoryOpen(&pContext->hDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    *phProvider = (HANDLE)pContext;

cleanup:

    return dwError;

error:

    *(phProvider) = (HANDLE)NULL;

    if (pContext)
    {
        LocalCloseHandle((HANDLE)pContext);
    }

    goto cleanup;
}

void
LocalCloseHandle(
    HANDLE hProvider
    )
{
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    if (pContext->hDirectory)
    {
        DirectoryClose(pContext->hDirectory);
    }

    if (pContext->pMutex)
    {
        pthread_mutex_destroy(&pContext->mutex);
    }

    if (pContext)
    {
        LwFreeMemory(pContext);
    }
}

BOOLEAN
LocalServicesDomain(
    PCSTR pszDomain
    )
{
    BOOLEAN bResult = FALSE;

    if (!LW_IS_NULL_OR_EMPTY_STR(pszDomain) &&
        (!strcasecmp(pszDomain, gLPGlobals.pszNetBIOSName) ||
         !strcasecmp(pszDomain, gLPGlobals.pszLocalDomain) ||
         !strcasecmp(pszDomain, gLPGlobals.pszBuiltinDomain)))
    {
        bResult = TRUE;
    }

    return bResult;
}

DWORD
LocalAuthenticateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    DWORD dwUserInfoLevel = 2;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    PWSTR pwszUserDN = NULL;
    PWSTR pwszPassword = NULL;

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalFindUserByNameEx(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    &pwszUserDN,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    /* Check for disable, expired, etc..  accounts */

    dwError = LocalCheckAccountFlags(pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszPassword)
    {
        dwError = LsaMbsToWc16s(
                        pszPassword,
                        &pwszPassword);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = DirectoryVerifyPassword(
                    pContext->hDirectory,
                    pwszUserDN,
                    pwszPassword);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    LW_SAFE_FREE_MEMORY(pwszUserDN);
    LW_SAFE_FREE_MEMORY(pwszPassword);

    return dwError;

error:

    goto cleanup;
}


DWORD
LocalAuthenticateUserEx(
    HANDLE                hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO*  ppUserInfo
    )
{
    return LocalAuthenticateUserExInternal(hProvider,
                                           pUserParams,
                                           ppUserInfo);
}

DWORD
LocalValidateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoLevel = 2;
    PLSA_USER_INFO_2 pUserInfo = NULL;

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalFindUserByName(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    /* Map any failures to find the user to 'user not found'. This way if the
     * unknown_ok option is specified for the pam module, this error will be
     * ignored instead of blocking all logins.
     */
    if (dwError != LW_ERROR_SUCCESS)
    {
        LSA_LOG_DEBUG(
                "Failed to find user '%s' while validating login "
                "[error code:%d]",
                pszLoginId,
                dwError);
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bPasswordExpired)
    {
        dwError = LW_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalCheckUserInList(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszListName
    )
{
    // require-membership-of is currently not supported
    // for the local provider.  So just return success here.

    return LW_ERROR_SUCCESS;
}

DWORD
LocalFindUserByName(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = STATUS_SUCCESS;
    PVOID pUserInfo = NULL;

    dwError = LocalFindUserByNameEx(
                hProvider,
                pszLoginId,
                dwUserInfoLevel,
                NULL,
                &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    switch(dwUserInfoLevel)
    {
        /* Add any info levels that contain the NT/LM hash here */
        case 2:
        {
            PLSA_USER_INFO_2 pUserInfo2 = (PLSA_USER_INFO_2)pUserInfo;

            if (pUserInfo2->pNTHash)
            {
                memset(pUserInfo2->pNTHash, 0, pUserInfo2->dwNTHashLen);
            }

            if (pUserInfo2->pLMHash)
            {
                memset(pUserInfo2->pLMHash, 0, pUserInfo2->dwLMHashLen);
            }
        }
        break;
    }

    /* give the memory away now */

    *ppUserInfo = pUserInfo;
    pUserInfo = NULL;

cleanup:

    return dwError;

error:
    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
LocalFindUserByNameEx(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    DWORD   dwUserInfoLevel,
    PWSTR*  ppwszUserDN,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    PWSTR pwszUserDN = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    BAIL_ON_INVALID_HANDLE(hProvider);

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCrackDomainQualifiedName(
                    pszLoginId,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LocalServicesDomain(pLoginInfo->pszFullDomainName))
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!strcasecmp(pLoginInfo->pszName, "root"))
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalDirFindUserByName(
                    hProvider,
                    pLoginInfo->pszFullDomainName,
                    pLoginInfo->pszName,
                    dwUserInfoLevel,
                    ppwszUserDN ? &pwszUserDN : NULL,
                    &pUserInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    if (ppwszUserDN)
    {
        *ppwszUserDN = pwszUserDN;
    }
    *ppUserInfo = pUserInfo;

cleanup:

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    if (ppwszUserDN)
    {
        *ppwszUserDN = pwszUserDN;
    }
    *ppUserInfo = NULL;

    LW_SAFE_FREE_MEMORY(pwszUserDN);

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
LocalFindUserById(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirFindUserById(
                    hProvider,
                    uid,
                    dwUserInfoLevel,
                    NULL,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    switch(dwUserInfoLevel)
    {
        /* Add any info levels that contain the NT/LM hash here */
        case 2:
        {
            PLSA_USER_INFO_2 pUserInfo2 = (PLSA_USER_INFO_2)pUserInfo;

            if (pUserInfo2->pNTHash)
            {
                memset(pUserInfo2->pNTHash, 0, pUserInfo2->dwNTHashLen);
            }

            if (pUserInfo2->pLMHash)
            {
                memset(pUserInfo2->pLMHash, 0, pUserInfo2->dwLMHashLen);
            }
        }
        break;
    }

    *ppUserInfo = pUserInfo;
    pUserInfo = NULL;

cleanup:

    return dwError;

error:

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
LocalGetGroupsForUser(
    IN HANDLE hProvider,
    IN OPTIONAL PCSTR pszUserName,
    IN OPTIONAL uid_t uid,
    IN LSA_FIND_FLAGS dwFindFlags,
    IN DWORD dwGroupInfoLevel,
    IN PDWORD pdwNumGroupsFound,
    IN PVOID** pppGroupInfoList
    )
{
    DWORD             dwError = 0;
    PWSTR             pwszUserDN = NULL;
    DWORD             dwUserInfoLevel = 0;
    PLSA_USER_INFO_0  pUserInfo = NULL;
    PVOID*            ppGroupInfoList = NULL;
    DWORD             dwNumGroupsFound = 0;

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszUserName)
    {
        dwError = LocalFindUserByNameEx(
                        hProvider,
                        pszUserName,
                        dwUserInfoLevel,
                        &pwszUserDN,
                        (PVOID*)&pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LocalDirFindUserById(
                        hProvider,
                        uid,
                        dwUserInfoLevel,
                        &pwszUserDN,
                        (PVOID*)&pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalDirGetGroupsForUser(
                    hProvider,
                    pwszUserDN,
                    dwGroupInfoLevel,
                    &dwNumGroupsFound,
                    &ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    *pppGroupInfoList = ppGroupInfoList;
    *pdwNumGroupsFound = dwNumGroupsFound;

cleanup:

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    LW_SAFE_FREE_MEMORY(pwszUserDN);

    return dwError;

error:

    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = dwNumGroupsFound;

    if (ppGroupInfoList)
    {
        LsaFreeGroupInfoList(
                dwGroupInfoLevel,
                ppGroupInfoList,
                dwNumGroupsFound);
    }

    goto cleanup;
}

DWORD
LocalBeginEnumUsers(
    HANDLE         hProvider,
    DWORD          dwInfoLevel,
    LSA_FIND_FLAGS dwFindFlags,
    PHANDLE        phResume
    )
{
    DWORD  dwError = 0;
    HANDLE hResume = (HANDLE)NULL;

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirBeginEnumUsers(
                        hProvider,
                        dwInfoLevel,
                        &hResume);
    BAIL_ON_LSA_ERROR(dwError);

    *phResume = hResume;

cleanup:

    return dwError;

error:

    *phResume = (HANDLE)NULL;

    if (hResume)
    {
        LocalFreeEnumState(hResume);
    }

    goto cleanup;
}

DWORD
LocalEnumUsers(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxNumRecords,
    PDWORD   pdwUsersFound,
    PVOID**  pppUserInfoList
    )
{
    DWORD dwError = 0;

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError =  LocalDirEnumUsers(
                    hProvider,
                    hResume,
                    dwMaxNumRecords,
                    pdwUsersFound,
                    pppUserInfoList
                    );
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

VOID
LocalEndEnumUsers(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    LocalFreeUserState(
            hProvider,
            (PLOCAL_PROVIDER_ENUM_STATE)hResume);
}

DWORD
LocalFindGroupByName(
    IN HANDLE         hProvider,
    IN PCSTR          pszGroupName,
    IN LSA_FIND_FLAGS dwFindFlags,
    IN DWORD          dwInfoLevel,
    OUT PVOID*        ppGroupInfo
    )
{
    return LocalFindGroupByNameEx(
                hProvider,
                pszGroupName,
                dwFindFlags,
                dwInfoLevel,
                NULL,
                ppGroupInfo);
}

DWORD
LocalFindGroupByNameEx(
    IN  HANDLE         hProvider,
    IN  PCSTR          pszGroupName,
    IN  LSA_FIND_FLAGS dwFindFlags,
    IN  DWORD          dwInfoLevel,
    OUT PWSTR*         ppwszGroupDN,
    OUT PVOID*         ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    PWSTR pwszGroupDN = NULL;
    PCSTR pszDomainName = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    BAIL_ON_INVALID_HANDLE(hProvider);

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCrackDomainQualifiedName(
                    pszGroupName,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!strcasecmp(pLoginInfo->pszName, "root"))
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LocalServicesDomain(pLoginInfo->pszDomainNetBiosName))
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pLoginInfo->pszFullDomainName)
    {
        pszDomainName = pLoginInfo->pszFullDomainName;
    }
    else
    {
        pszDomainName = gLPGlobals.pszLocalDomain;
    }

    dwError = LocalDirFindGroupByName(
                    hProvider,
                    pszDomainName,
                    pLoginInfo->pszName,
                    dwInfoLevel,
                    (ppwszGroupDN ? &pwszGroupDN : NULL),
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (ppwszGroupDN)
    {
        *ppwszGroupDN = pwszGroupDN;
    }
    *ppGroupInfo = pGroupInfo;

cleanup:

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

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
LocalFindGroupById(
    IN HANDLE hProvider,
    IN gid_t gid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirFindGroupById(
                    hProvider,
                    gid,
                    dwInfoLevel,
                    NULL,
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    *ppGroupInfo = pGroupInfo;

cleanup:

    return dwError;

error:

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
LocalBeginEnumGroups(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    BOOLEAN bCheckOnline,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD  dwError = 0;
    HANDLE hResume = NULL;

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirBeginEnumGroups(
                        hProvider,
                        dwInfoLevel,
                        &hResume);
    BAIL_ON_LSA_ERROR(dwError);

    *phResume = hResume;

cleanup:

    return dwError;

error:

    *phResume = (HANDLE)NULL;

    if (hResume)
    {
        LocalFreeEnumState(hResume);
    }

    goto cleanup;
}

DWORD
LocalEnumGroups(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxGroups,
    PDWORD   pdwGroupsFound,
    PVOID**  pppGroupInfoList
    )
{
    DWORD dwError = 0;

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirEnumGroups(
                    hProvider,
                    hResume,
                    dwMaxGroups,
                    pdwGroupsFound,
                    pppGroupInfoList
                    );
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

VOID
LocalEndEnumGroups(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    LocalFreeGroupState(hProvider, (PLOCAL_PROVIDER_ENUM_STATE)hResume);
}

DWORD
LocalChangePassword(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword
    )
{
    DWORD dwError = 0;
    DWORD dwInfoLevel = 0;
    PWSTR pwszUserDN  = NULL;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    PWSTR pwszOldPassword = NULL;
    PWSTR pwszNewPassword = NULL;

    BAIL_ON_INVALID_HANDLE(hProvider);

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalFindUserByNameEx(
                    hProvider,
                    pszLoginId,
                    dwInfoLevel,
                    &pwszUserDN,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCheckForPasswordChangeAccess(
                    hProvider,
                    pUserInfo->uid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                        (pszPassword ? pszPassword : ""),
                        &pwszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                        (pszOldPassword ? pszOldPassword : ""),
                        &pwszOldPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirChangePassword(
                    hProvider,
                    pwszUserDN,
                    pwszOldPassword,
                    pwszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }

    LW_SAFE_FREE_MEMORY(pwszNewPassword);
    LW_SAFE_FREE_MEMORY(pwszOldPassword);
    LW_SAFE_FREE_MEMORY(pwszUserDN);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalSetPassword(
    HANDLE hProvider,
    PCSTR pszLoginId,
    PCSTR pszPassword
    )
{
    DWORD dwError = 0;
    DWORD dwInfoLevel = 0;
    PWSTR pwszUserDN = NULL;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    PWSTR pwszNewPassword = NULL;

    BAIL_ON_INVALID_HANDLE(hProvider);

    dwError = LocalFindUserByNameEx(
                    hProvider,
                    pszLoginId,
                    dwInfoLevel,
                    &pwszUserDN,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                        (pszPassword ? pszPassword : ""),
                        &pwszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirSetPassword(
                    hProvider,
                    pwszUserDN,
                    pwszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (pUserInfo)
    {
        LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }

    LW_SAFE_FREE_MEMORY(pwszNewPassword);
    LW_SAFE_FREE_MEMORY(pwszUserDN);

    return dwError;

error:
    goto cleanup;
}

DWORD
LocalAddUser(
    HANDLE hProvider,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(hProvider);
    BAIL_ON_INVALID_POINTER(pUserInfo);

    dwError = LocalCheckForAddAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirAddUser(
                    hProvider,
                    dwUserInfoLevel,
                    pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    DWORD   dwError = 0;

    dwError = LocalCheckForModifyAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirModifyUser(
                    hProvider,
                    pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
LocalDeleteUser(
    HANDLE hProvider,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    DWORD dwInfoLevel = 0;
    PWSTR pwszUserDN  = NULL;
    PLSA_USER_INFO_0 pUserInfo = NULL;

    BAIL_ON_INVALID_HANDLE(hProvider);

    dwError = LocalCheckForDeleteAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirFindUserById(
                    hProvider,
                    uid,
                    dwInfoLevel,
                    &pwszUserDN,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirDeleteUser(
                    hProvider,
                    pwszUserDN);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }

    LW_SAFE_FREE_MEMORY(pwszUserDN);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalAddGroup(
    HANDLE hProvider,
    DWORD  dwGroupInfoLevel,
    PVOID  pGroupInfo
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_HANDLE(hProvider);
    BAIL_ON_INVALID_POINTER(pGroupInfo);

    dwError = LocalCheckForAddAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirAddGroup(
                    hProvider,
                    dwGroupInfoLevel,
                    pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalModifyGroup(
    HANDLE hProvider,
    PLSA_GROUP_MOD_INFO pGroupModInfo
    )
{
    DWORD dwError = 0;

    dwError = LocalCheckForModifyAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirModifyGroup(
                    hProvider,
                    pGroupModInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
LocalDeleteGroup(
    HANDLE hProvider,
    gid_t  gid
    )
{
    DWORD dwError = 0;
    DWORD dwInfoLevel = 0;
    PWSTR pwszGroupDN  = NULL;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;

    BAIL_ON_INVALID_HANDLE(hProvider);

    dwError = LocalCheckForDeleteAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirFindGroupById(
                    hProvider,
                    gid,
                    dwInfoLevel,
                    &pwszGroupDN,
                    (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirDeleteGroup(
                    hProvider,
                    pwszGroupDN);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwInfoLevel, pGroupInfo);
    }

    LW_SAFE_FREE_MEMORY(pwszGroupDN);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalOpenSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    BOOLEAN bCreateHomedir = FALSE;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PWSTR pwszUserDN = NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalCrackDomainQualifiedName(
                    pszLoginId,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalFindUserByNameEx(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    &pwszUserDN,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    // Allow directory creation only if this is
    //
    if ((pContext->uid != 0) &&
        (pContext->uid != ((PLSA_USER_INFO_0)(pUserInfo))->uid)) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalCfgMustCreateHomedir(&bCreateHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    if (bCreateHomedir)
    {
        dwError = LocalCreateHomeDirectory((PLSA_USER_INFO_0)pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LocalUpdateUserLoginTime(
                    hProvider,
                    pwszUserDN);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    LW_SAFE_FREE_MEMORY(pwszUserDN);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalCloseSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoLevel = 0;
    PVOID pUserInfo = NULL;
    PWSTR pwszUserDN = NULL;

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalFindUserByNameEx(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    &pwszUserDN,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalUpdateUserLogoffTime(
                    hProvider,
                    pwszUserDN);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    LW_SAFE_FREE_MEMORY(pwszUserDN);

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalGetNamesBySidList(
    HANDLE          hProvider,
    size_t          sCount,
    PSTR*           ppszSidList,
    PSTR**          pppszDomainNames,
    PSTR**          pppszSamAccounts,
    ADAccountType** ppTypes
    )
{
    DWORD dwError = 0;
    PSTR* ppszDomainNames = NULL;
    PSTR* ppszSamAccounts = NULL;
    ADAccountType* pTypes = NULL;

    BAIL_ON_INVALID_HANDLE(hProvider);

    dwError = LocalCheckForQueryAccess(hProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirGetNamesBySidList(
                        hProvider,
                        sCount,
                        ppszSidList,
                        &ppszDomainNames,
                        &ppszSamAccounts,
                        &pTypes);
    BAIL_ON_LSA_ERROR(dwError);

    *pppszDomainNames = ppszDomainNames;
    *pppszSamAccounts = ppszSamAccounts;
    *ppTypes = pTypes;

cleanup:

    return dwError;

error:

    *pppszDomainNames = NULL;
    *pppszSamAccounts = NULL;
    *ppTypes = NULL;

    LwFreeStringArray(ppszDomainNames, sCount);
    LwFreeStringArray(ppszSamAccounts, sCount);
    LW_SAFE_FREE_MEMORY(pTypes);

    goto cleanup;
}

DWORD
LocalFindNSSArtefactByKey(
    HANDLE hProvider,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
    *ppNSSArtefactInfo = NULL;

    return LW_ERROR_NOT_HANDLED;
}


DWORD
LocalBeginEnumNSSArtefacts(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;

    dwError = LW_ERROR_NOT_HANDLED;

    *phResume = (HANDLE)NULL;

    return dwError;
}

DWORD
LocalEnumNSSArtefacts(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxNSSArtefacts,
    PDWORD   pdwNSSArtefactsFound,
    PVOID**  pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;

    dwError = LW_ERROR_NOT_HANDLED;

    *pdwNSSArtefactsFound = 0;
    *pppNSSArtefactInfoList = NULL;

    return dwError;
}

VOID
LocalEndEnumNSSArtefacts(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    return;
}

DWORD
LocalGetStatus(
    HANDLE hProvider,
    PLSA_AUTH_PROVIDER_STATUS* ppProviderStatus
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus = NULL;

    dwError = LwAllocateMemory(
                   sizeof(LSA_AUTH_PROVIDER_STATUS),
                   (PVOID*)&pProviderStatus);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    gpszLocalProviderName,
                    &pProviderStatus->pszId);
    BAIL_ON_LSA_ERROR(dwError);

    pProviderStatus->mode = LSA_PROVIDER_MODE_LOCAL_SYSTEM;
    pProviderStatus->status = LSA_AUTH_PROVIDER_STATUS_ONLINE;

    *ppProviderStatus = pProviderStatus;

cleanup:

    return dwError;

error:

    *ppProviderStatus = NULL;

    if (pProviderStatus)
    {
        LocalFreeStatus(pProviderStatus);
    }

    goto cleanup;
}

DWORD
LocalRefreshConfiguration(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    PSTR pszConfigFilePath = NULL;
    LOCAL_CONFIG config = {0};
    BOOLEAN bInLock = FALSE;

    dwError = LocalCfgGetFilePath(&pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszConfigFilePath))
    {
        dwError = LocalCfgParseFile(
                        pszConfigFilePath,
                        &config);
        BAIL_ON_LSA_ERROR(dwError);

        LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.mutex);

        dwError = LocalCfgTransferContents(
                        &config,
                        &gLPGlobals.cfg);
        BAIL_ON_LSA_ERROR(dwError);
    }

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    LocalEventLogConfigReload();

cleanup:

    LW_SAFE_FREE_STRING(pszConfigFilePath);

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    return dwError;

error:

    LocalCfgFreeContents(&config);

    goto cleanup;
}

VOID
LocalFreeStatus(
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus
    )
{
    LW_SAFE_FREE_STRING(pProviderStatus->pszId);
    LW_SAFE_FREE_STRING(pProviderStatus->pszDomain);
    LW_SAFE_FREE_STRING(pProviderStatus->pszForest);
    LW_SAFE_FREE_STRING(pProviderStatus->pszSite);
    LW_SAFE_FREE_STRING(pProviderStatus->pszCell);

    LwFreeMemory(pProviderStatus);
}

DWORD
LocalIoControl(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN gid_t   peerGID,
    IN DWORD   dwIoControlCode,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;

    switch (dwIoControlCode)
    {
    case LSA_LOCAL_IO_GETGROUPMEMBERSHIP:
        dwError = LocalGetGroupMembership(
                        hProvider,
                        peerUID,
                        peerGID,
                        dwInputBufferSize,
                        pInputBuffer,
                        pdwOutputBufferSize,
                        ppOutputBuffer);
        break;

    default:
        dwError = LW_ERROR_NOT_HANDLED;
        break;
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    *pdwOutputBufferSize = 0;
    *ppOutputBuffer      = NULL;

    goto cleanup;
}

DWORD
LocalGetGroupMembership(
    HANDLE hProvider,
    uid_t  peerUID,
    gid_t  peerGID,
    DWORD  dwInputBufferSize,
    PVOID  pInputBuffer,
    PDWORD pdwOutputBufferSize,
    PVOID *ppOutputBuffer
    )
{
    DWORD dwError = 0;
    LWMsgContext *context = NULL;
    LWMsgDataContext *pDataContext = NULL;
    PLSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REQ pRequest = NULL;
    LSA_LOCAL_IPC_GET_GROUP_MEMBERSHIP_REP Reply;
    PWSTR pwszDN = NULL;
    DWORD dwGroupsCount = 0;
    PVOID* ppGroupInfoList = NULL;
    size_t repBufferSize = 0;
    PVOID pRepBuffer = NULL;

    memset(&Reply, 0, sizeof(Reply));

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                              pDataContext,
                              LsaLocalIPCGetGroupMembershipReqSpec(),
                              pInputBuffer,
                              dwInputBufferSize,
                              (PVOID*)&pRequest));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pRequest->pszDN,
                    &pwszDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LocalDirGetGroupsForUser(
                    hProvider,
                    pwszDN,
                    pRequest->dwGroupInfoLevel,
                    &dwGroupsCount,
                    &ppGroupInfoList);
    if (dwError == 0) {
        Reply.dwNumGroups      = dwGroupsCount;
        Reply.dwGroupInfoLevel = pRequest->dwGroupInfoLevel;
        Reply.Groups.ppInfo0   = (PLSA_GROUP_INFO_0*)ppGroupInfoList;

    } else if (dwError == LW_ERROR_NO_SUCH_USER) {
        Reply.dwNumGroups      = 0;
        Reply.dwGroupInfoLevel = pRequest->dwGroupInfoLevel;
        Reply.Groups.ppInfo0   = NULL;

    } else {
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataContext,
                              LsaLocalIPCGetGroupMembershipRepSpec(),
                              &Reply,
                              &pRepBuffer,
                              &repBufferSize));
    BAIL_ON_LSA_ERROR(dwError);

    *pdwOutputBufferSize = (DWORD) repBufferSize;
    *ppOutputBuffer      = pRepBuffer;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszDN);

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(
                    pRequest->dwGroupInfoLevel,
                    ppGroupInfoList,
                    dwGroupsCount);
    }

    if (pRequest) {
        lwmsg_data_free_graph(
            pDataContext,
            LsaLocalIPCGetGroupMembershipReqSpec(),
            pRequest);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if (context) {
        lwmsg_context_delete(context);
    }

    return dwError;

error:
    *pdwOutputBufferSize = 0;
    *ppOutputBuffer      = NULL;
    goto cleanup;
}


DWORD
LSA_SHUTDOWN_PROVIDER(local)(
    PSTR pszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE pFnTable
    )
{
    LW_SAFE_FREE_STRING(gLPGlobals.pszConfigFilePath);
    LW_SAFE_FREE_STRING(gLPGlobals.pszLocalDomain);
    LW_SAFE_FREE_STRING(gLPGlobals.pszNetBIOSName);

    return 0;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
