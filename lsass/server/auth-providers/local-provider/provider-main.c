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
 */
#include "localprovider.h"

DWORD
LsaInitializeProvider(
    PCSTR pszConfigFilePath,
    PSTR* ppszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE* ppFunctionTable)
{
    DWORD dwError = 0;
    LOCAL_CONFIG config = {0};

    pthread_rwlock_init(&gProviderLocalGlobalDataLock, NULL);

    dwError = LsaDnsGetHostInfo(&gProviderLocal_Hostname);
    BAIL_ON_LSA_ERROR(dwError);

    if (!IsNullOrEmptyString(pszConfigFilePath)) {

        dwError = LsaProviderLocal_ParseConfigFile(
                        pszConfigFilePath,
                        &config);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaProviderLocal_TransferConfigContents(
                        &config,
                        &gLocalConfig);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaProviderLocal_SetConfigFilePath(pszConfigFilePath);
        BAIL_ON_LSA_ERROR(dwError);

    }

    LsaProviderLocal_DbInitGlobals();

    dwError = LsaProviderLocal_DbCreate();
    BAIL_ON_LSA_ERROR(dwError);
    
    if (LsaProviderLocal_EventlogEnabled())
    {
        LsaLocalProviderLogServiceStartEvent();
    }

    *ppszProviderName = (PSTR)gpszLocalProviderName;
    *ppFunctionTable = &gLocalProviderAPITable;

cleanup:

    return dwError;

error:

    LsaProviderLocal_FreeConfigContents(&config);

    *ppszProviderName = NULL;
    *ppFunctionTable = NULL;

    goto cleanup;
}

DWORD
LsaProviderLocal_OpenHandle(
    uid_t uid,
    gid_t gid,
    PHANDLE phProvider
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(LOCAL_PROVIDER_CONTEXT),
                    (PVOID*)&pContext
                    );
    BAIL_ON_LSA_ERROR(dwError);

    pContext->uid = uid;
    pContext->gid = gid;

    *phProvider = (HANDLE)pContext;

cleanup:

    return dwError;

error:

    *(phProvider) = (HANDLE)NULL;

    if (pContext) {
        LsaProviderLocal_CloseHandle((HANDLE)pContext);
    }

    goto cleanup;
}

void
LsaProviderLocal_CloseHandle(
    HANDLE hProvider
    )
{
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    if (pContext) {
        LsaProviderLocal_FreeStateList(pContext->pGroupEnumStateList);

        LsaProviderLocal_FreeStateList(pContext->pUserEnumStateList);

        LsaFreeMemory(pContext);
    }
}

BOOLEAN
LsaProviderLocal_ServicesDomain(
    PCSTR pszDomain
    )
{
    BOOLEAN bResult = FALSE;

    if (IsNullOrEmptyString(pszDomain) ||
        !strcasecmp(pszDomain, "localhost") ||
        !strcasecmp(pszDomain, gProviderLocal_Hostname))
    {
        bResult = TRUE;
    }

    return bResult;
}

DWORD
LsaProviderLocal_AuthenticateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    PBYTE pHash = NULL;
    DWORD dwHashLen = 0;
    DWORD dwUserInfoLevel = 2;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    BOOLEAN bAuthenticated = FALSE;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_FindUserByName(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pUserInfo->bAccountDisabled) {
        dwError = LSA_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bAccountLocked) {
        dwError = LSA_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bAccountExpired) {
        dwError = LSA_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bPasswordExpired) {
        dwError = LSA_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvComputeNTHash(pszPassword, &pHash, &dwHashLen);
    BAIL_ON_LSA_ERROR(dwError);

    // Either both hashes are null, or they must match
    bAuthenticated =
        (((pUserInfo->dwNTHashLen) &&
         (pUserInfo->dwNTHashLen == dwHashLen) &&
         (pHash) &&
         (pUserInfo->pNTHash) &&
         !memcmp(pUserInfo->pNTHash, pHash, dwHashLen)) ||
         (!pUserInfo->dwNTHashLen && !dwHashLen));

    if (!bAuthenticated) {
        dwError = LSA_ERROR_PASSWORD_MISMATCH;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (pLoginInfo) {
        LsaFreeNameInfo(pLoginInfo);
    }

    LSA_SAFE_FREE_MEMORY(pHash);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaProviderLocal_ValidateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoLevel = 2;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_FindUserByName(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    /* Map any failures to find the user to 'user not found'. This way if the
     * unknown_ok option is specified for the pam module, this error will be
     * ignored instead of blocking all logins.
     */
    if (dwError != LSA_ERROR_SUCCESS)
    {
        LSA_LOG_DEBUG(
                "Failed to find user '%s' while validating login "
                "[error code:%d]",
                pszLoginId,
                dwError);
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bPasswordExpired) {
        dwError = LSA_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (pLoginInfo) {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaProviderLocal_CheckUserInList(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszListName
    )
{
    return LSA_ERROR_NOT_HANDLED;
}

DWORD
LsaProviderLocal_FindUserByName(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PVOID pUserInfo = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LsaProviderLocal_ServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!strcasecmp(pLoginInfo->pszName, "root"))
    {
    	dwError = LSA_ERROR_NO_SUCH_USER;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_DbFindUserByName(
                    hDb,
                    pLoginInfo->pszDomainNetBiosName,
                    pLoginInfo->pszName,
                    dwUserInfoLevel,
                    &pUserInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    *ppUserInfo = pUserInfo;

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaProviderLocal_DbClose(hDb);
    }

    if (pLoginInfo) {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
LsaProviderLocal_FindUserById(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PVOID pUserInfo = NULL;

    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    if (uid == 0)
    {
    	dwError = LSA_ERROR_NO_SUCH_USER;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbFindUserById(
                    hDb,
                    uid,
                    dwUserInfoLevel,
                    &pUserInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    *ppUserInfo = pUserInfo;

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaProviderLocal_DbClose(hDb);
    }

    return dwError;

error:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
LsaProviderLocal_GetGroupsForUser(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;

    if (uid == 0)
    {
    	dwError = LSA_ERROR_NO_SUCH_USER;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_DbGetGroupsForUser(
                    hDb,
                    uid,
                    dwGroupInfoLevel,
                    pdwGroupsFound,
                    pppGroupInfoList
                    );
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaProviderLocal_DbClose(hDb);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
LsaProviderLocal_BeginEnumUsers(
    HANDLE  hProvider,
    PCSTR   pszGUID,
    DWORD   dwInfoLevel,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    dwError = LsaProviderLocal_AddUserState(
                        hProvider,
                        pszGUID,
                        dwInfoLevel,
                        &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    *phResume = (HANDLE)pEnumState;

cleanup:

    return dwError;

error:

    *phResume = (HANDLE)NULL;

    goto cleanup;
}

DWORD
LsaProviderLocal_EnumUsers(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxNumRecords,
    PDWORD   pdwUsersFound,
    PVOID**  pppUserInfoList
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    pEnumState = (PLOCAL_PROVIDER_ENUM_STATE)hResume;

    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_DbEnumUsers(
                    hDb,
                    pEnumState->dwInfoLevel,
                    pEnumState->dwNextStartingId,
                    dwMaxNumRecords,
                    pdwUsersFound,
                    pppUserInfoList
                    );
    BAIL_ON_LSA_ERROR(dwError);

    if (*pdwUsersFound) {
        pEnumState->dwNextStartingId += *pdwUsersFound;
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaProviderLocal_DbClose(hDb);
    }

    return dwError;

error:

    *pdwUsersFound = 0;
    *pppUserInfoList = NULL;

    goto cleanup;
}

VOID
LsaProviderLocal_EndEnumUsers(
    HANDLE hProvider,
    PCSTR  pszGUID
    )
{
    LsaProviderLocal_FreeUserState(hProvider, pszGUID);
}

DWORD
LsaProviderLocal_FindGroupByName(
    HANDLE  hProvider,
    PCSTR   pszGroupName,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PVOID pGroupInfo = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszGroupName,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LsaProviderLocal_ServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!strcasecmp(pLoginInfo->pszName, "root"))
    {
    	dwError = LSA_ERROR_NO_SUCH_GROUP;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_DbFindGroupByName(
                    hDb,
                    pLoginInfo->pszDomainNetBiosName,
                    pLoginInfo->pszName,
                    dwGroupInfoLevel,
                    &pGroupInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    *ppGroupInfo = pGroupInfo;

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaProviderLocal_DbClose(hDb);
    }

    if (pLoginInfo) {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
LsaProviderLocal_FindGroupById(
    HANDLE  hProvider,
    gid_t   gid,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PVOID pGroupInfo = NULL;

    if (gid == 0)
    {
    	dwError = LSA_ERROR_NO_SUCH_GROUP;
    	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_DbFindGroupById(
                    hDb,
                    gid,
                    dwGroupInfoLevel,
                    &pGroupInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    *ppGroupInfo = pGroupInfo;

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaProviderLocal_DbClose(hDb);
    }

    return dwError;

error:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

DWORD
LsaProviderLocal_BeginEnumGroups(
    HANDLE  hProvider,
    PCSTR   pszGUID,
    DWORD   dwInfoLevel,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    dwError = LsaProviderLocal_AddGroupState(
                        hProvider,
                        pszGUID,
                        dwInfoLevel,
                        &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    *phResume = (HANDLE)pEnumState;

cleanup:

    return dwError;

error:

    *phResume = (HANDLE)NULL;

    goto cleanup;
}

DWORD
LsaProviderLocal_EnumGroups(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxGroups,
    PDWORD   pdwGroupsFound,
    PVOID**  pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = (PLOCAL_PROVIDER_ENUM_STATE)hResume;
    HANDLE hDb = (HANDLE)NULL;

    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_DbEnumGroups(
                    hDb,
                    pEnumState->dwInfoLevel,
                    pEnumState->dwNextStartingId,
                    dwMaxGroups,
                    pdwGroupsFound,
                    pppGroupInfoList
                    );
    BAIL_ON_LSA_ERROR(dwError);

    if (*pdwGroupsFound) {
        pEnumState->dwNextStartingId += *pdwGroupsFound;
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaProviderLocal_DbClose(hDb);
    }

    return dwError;

error:

    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;

    goto cleanup;
}

VOID
LsaProviderLocal_EndEnumGroups(
    HANDLE hProvider,
    PCSTR  pszGUID
    )
{
    LsaProviderLocal_FreeGroupState(hProvider, pszGUID);
}

DWORD
LsaProviderLocal_ChangePassword(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoLevel = 2;
    HANDLE hDb = (HANDLE)NULL;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_DbFindUserByName(
                    hDb,
                    pLoginInfo->pszDomainNetBiosName,
                    pLoginInfo->pszName,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if ((pContext->uid != 0) &&
        (pContext->uid != ((PLSA_USER_INFO_2)pUserInfo)->uid)) {
       // Only the user or root can change the password
       dwError = EACCES;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bAccountDisabled) {
        dwError = LSA_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bAccountExpired) {
        dwError = LSA_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo->bAccountLocked) {
        dwError = LSA_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // If the account initiating the password change has super user privileges,
    // don't bother checking the old password
    if (pContext->uid) {

        if (!pUserInfo->bUserCanChangePassword) {
            dwError = LSA_ERROR_USER_CANNOT_CHANGE_PASSWD;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaProviderLocal_AuthenticateUser(
                        hProvider,
                        pszLoginId,
                        pszOldPassword);
        BAIL_ON_LSA_ERROR(dwError);

    }

    dwError = LsaProviderLocal_DbChangePassword(
                    hDb,
                    ((PLSA_USER_INFO_2)pUserInfo)->uid,
                    pszPassword);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (LsaProviderLocal_EventlogEnabled())
    {        
        LsaSrvLogUserPWChangeSuccessEvent(
                pszLoginId,
                "Local");
    }

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (pLoginInfo) {
        LsaFreeNameInfo(pLoginInfo);
    }

    if (hDb != (HANDLE)NULL) {
        LsaProviderLocal_DbClose(hDb);
    }

    return dwError;

error:

    if (LsaProviderLocal_EventlogEnabled())
    {        
        LsaSrvLogUserPWChangeFailureEvent(
                pszLoginId,
                "Local",
                dwError);
    }

    goto cleanup;
}

DWORD
LsaProviderLocal_AddUser(
    HANDLE hProvider,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    if (pContext->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch(dwUserInfoLevel)
    {
        case 0:
            dwError = LsaCrackDomainQualifiedName(
                            ((PLSA_USER_INFO_0)(pUserInfo))->pszName,
                            NULL,
                            &pLoginInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        case 1:
            dwError = LsaCrackDomainQualifiedName(
                            ((PLSA_USER_INFO_1)(pUserInfo))->pszName,
                            NULL,
                            &pLoginInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        case 2:
            dwError = LsaCrackDomainQualifiedName(
                            ((PLSA_USER_INFO_2)(pUserInfo))->pszName,
                            NULL,
                            &pLoginInfo);
            BAIL_ON_LSA_ERROR(dwError);
            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LsaProviderLocal_ServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pContext->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_DbAddUser(
                    hDb,
                    dwUserInfoLevel,
                    pUserInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);
    
    if (LsaProviderLocal_EventlogEnabled()){        
        LsaLocalProviderLogUserAddEvent(pLoginInfo->pszName);
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaProviderLocal_DbClose(hDb);
    }

    if (pLoginInfo) {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaProviderLocal_ModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    if (pContext->uid) {
       dwError = EACCES;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_DbModifyUser(
                        hDb,
                        pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hDb != (HANDLE)NULL) {
       LsaProviderLocal_DbClose(hDb);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaProviderLocal_DeleteUser(
    HANDLE hProvider,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    if (pContext->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_DbDeleteUser(
                    hDb,
                    uid);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (LsaProviderLocal_EventlogEnabled()){        
        LsaLocalProviderLogUserDeleteEvent(uid);
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaProviderLocal_DbClose(hDb);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaProviderLocal_AddGroup(
    HANDLE hProvider,
    DWORD  dwGroupInfoLevel,
    PVOID  pGroupInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    switch(dwGroupInfoLevel)
    {
        case 1:
            dwError = LsaCrackDomainQualifiedName(
                            ((PLSA_GROUP_INFO_1)(pGroupInfo))->pszName,
                            NULL,
                            &pLoginInfo);
            BAIL_ON_LSA_ERROR(dwError);

            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LsaProviderLocal_ServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pContext->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_DbAddGroup(
                    hDb,
                    pLoginInfo->pszDomainNetBiosName,
                    dwGroupInfoLevel,
                    pGroupInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaProviderLocal_DbClose(hDb);
    }

    if (pLoginInfo) {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaProviderLocal_DeleteGroup(
    HANDLE hProvider,
    gid_t  gid
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    if (pContext->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_DbDeleteGroup(
                    hDb,
                    gid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaProviderLocal_DbClose(hDb);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaProviderLocal_OpenSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_FindUserByName(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    // Allow directory creation only if this is
    //
    if ((pContext->uid != 0) &&
        (pContext->uid != ((PLSA_USER_INFO_0)(pUserInfo))->uid)) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_CreateHomeDirectory(
                    (PLSA_USER_INFO_0)pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (pLoginInfo) {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaProviderLocal_CreateHomeDirectory(
    PLSA_USER_INFO_0 pUserInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    mode_t  umask = 022;
    mode_t  perms = (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BOOLEAN bRemoveDir = FALSE;

    if (IsNullOrEmptyString(pUserInfo->pszHomedir)) {
       LSA_LOG_ERROR("The user's [Uid:%ld] home directory is not defined", (long)pUserInfo->uid);
       dwError = LSA_ERROR_FAILED_CREATE_HOMEDIR;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaCheckDirectoryExists(
                    pUserInfo->pszHomedir,
                    &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists) {
       dwError = LsaCreateDirectory(
                    pUserInfo->pszHomedir,
                    perms & (~umask));
       BAIL_ON_LSA_ERROR(dwError);

       bRemoveDir = TRUE;

       dwError = LsaChangeOwner(
                    pUserInfo->pszHomedir,
                    pUserInfo->uid,
                    pUserInfo->gid);
       BAIL_ON_LSA_ERROR(dwError);

       bRemoveDir = FALSE;

       dwError = LsaProviderLocal_ProvisionHomeDir(
                       pUserInfo->uid,
                       pUserInfo->gid,
                       pUserInfo->pszHomedir);
       BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    if (bRemoveDir) {
       LsaRemoveDirectory(pUserInfo->pszHomedir);
    }

    goto cleanup;
}

DWORD
LsaProviderLocal_ProvisionHomeDir(
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszHomedirPath
    )
{
    DWORD   dwError = 0;
    BOOLEAN bExists = FALSE;

    dwError = LsaCheckDirectoryExists(
                    "/etc/skel",
                    &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExists) {
        dwError = LsaCopyDirectory(
                    "/etc/skel",
                    ownerUid,
                    ownerGid,
                    pszHomedirPath);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}


DWORD
LsaProviderLocal_CloseSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoLevel = 0;
    PVOID pUserInfo = NULL;

    dwError = LsaProviderLocal_FindUserByName(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaShutdownProvider(
    PSTR pszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE pFnTable
    )
{
    // TODO: Should we grab the global lock?
    LSA_SAFE_FREE_STRING(gpszConfigFilePath);

    LSA_SAFE_FREE_STRING(gProviderLocal_Hostname);

    return 0;
}

DWORD
LsaProviderLocal_GetNamesBySidList(
    HANDLE          hProvider,
    size_t          sCount,
    PSTR*           ppszSidList,
    PSTR**          pppszDomainNames,
    PSTR**          pppszSamAccounts,
    ADAccountType** ppTypes)
{
    return LSA_ERROR_NOT_HANDLED;
}


DWORD
LsaProviderLocal_BeginEnumNSSArtefacts(
    HANDLE  hProvider,
    PCSTR   pszGUID,
    DWORD   dwInfoLevel,
    DWORD   dwMapType,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;

    dwError = LSA_ERROR_NOT_HANDLED;

    *phResume = (HANDLE)NULL;

    return dwError;
}

DWORD
LsaProviderLocal_EnumNSSArtefacts(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxNSSArtefacts,
    PDWORD   pdwNSSArtefactsFound,
    PVOID**  pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;

    dwError = LSA_ERROR_NOT_HANDLED;

    *pdwNSSArtefactsFound = 0;
    *pppNSSArtefactInfoList = NULL;

    return dwError;
}

VOID
LsaProviderLocal_EndEnumNSSArtefacts(
    HANDLE hProvider,
    PCSTR  pszGUID
    )
{
    return;
}

DWORD
LsaProviderLocal_GetStatus(
    HANDLE hProvider,
    PLSA_AUTH_PROVIDER_STATUS* ppProviderStatus
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus = NULL;

    dwError = LsaAllocateMemory(
                   sizeof(LSA_AUTH_PROVIDER_STATUS),
                   (PVOID*)&pProviderStatus);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(
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
        LsaProviderLocal_FreeStatus(pProviderStatus);
    }

    goto cleanup;
}

DWORD
LsaProviderLocal_RefreshConfiguration(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    PSTR pszConfigFilePath = NULL;
    LOCAL_CONFIG config = {0};
    BOOLEAN bInLock = FALSE;

    dwError = LsaProviderLocal_GetConfigFilePath(&pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);

    if (!IsNullOrEmptyString(pszConfigFilePath)) {
        dwError = LsaProviderLocal_ParseConfigFile(
                        pszConfigFilePath,
                        &config);
        BAIL_ON_LSA_ERROR(dwError);

        ENTER_LOCAL_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

        dwError = LsaProviderLocal_TransferConfigContents(
                        &config,
                        &gLocalConfig);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LSA_SAFE_FREE_STRING(pszConfigFilePath);

    LEAVE_LOCAL_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    LsaProviderLocal_FreeConfigContents(&config);

    goto cleanup;
}

VOID
LsaProviderLocal_FreeStatus(
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus
    )
{
    LSA_SAFE_FREE_STRING(pProviderStatus->pszId);
    LSA_SAFE_FREE_STRING(pProviderStatus->pszDomain);
    LSA_SAFE_FREE_STRING(pProviderStatus->pszForest);
    LSA_SAFE_FREE_STRING(pProviderStatus->pszSite);
    LSA_SAFE_FREE_STRING(pProviderStatus->pszCell);

    LsaFreeMemory(pProviderStatus);
}

VOID
LsaLocalProviderLogServiceStartEvent(
    VOID
    )
{
    DWORD dwError = 0;    
    HANDLE hDb = (HANDLE)NULL;    
    int nUserCount = 0;
    int nGroupCount = 0;    
    
    PSTR pszDescription = NULL;
    
    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);    
    
    dwError = LsaProviderLocal_DbGetUserCount(
                    hDb,
                    &nUserCount);        
    BAIL_ON_LSA_ERROR(dwError);   
    
    dwError = LsaProviderLocal_DbGetGroupCount(
                    hDb,
                    &nGroupCount);        
    BAIL_ON_LSA_ERROR(dwError);    

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Local provider starts successfully. Details: Current number of local accounts in DB are: [%d].",
                 nUserCount + nGroupCount);
    BAIL_ON_LSA_ERROR(dwError);       
    
    LsaSrvLogServiceSuccessEvent( 
            SERVICESTART_EVENT_CATEGORY,
            pszDescription,
            "<null>");
    
cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaProviderLocal_DbClose(hDb);
    }
    
    LSA_SAFE_FREE_STRING(pszDescription);
    
    return;
    
error:

    goto cleanup;    
}       

VOID
LsaLocalProviderLogUserAddEvent(
    PCSTR pszUsername
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    
    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "A local user account was created for user '%s'.",                 
                 LSA_SAFE_LOG_STRING(pszUsername));
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaSrvLogServiceSuccessEvent(
            GENERAL_EVENT_CATEGORY,
            pszDescription,
            "<null>");
cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);
    
    return;
    
error:
    goto cleanup;
}

VOID
LsaLocalProviderLogUserDeleteEvent(
    uid_t uid
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    
    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "A local user account was deleted for user with uid '%d'.",                 
                 uid);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaSrvLogServiceSuccessEvent(
            GENERAL_EVENT_CATEGORY,
            pszDescription,
            "<null>");
cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);
    
    return;
    
error:
    goto cleanup;
}

