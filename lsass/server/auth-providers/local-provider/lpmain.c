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
 */

#include "includes.h"

DWORD
LsaInitializeProvider(
    PCSTR pszConfigFilePath,
    PSTR* ppszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE* ppFunctionTable)
{
    DWORD dwError = 0;
    LOCAL_CONFIG config = {0};
    BOOLEAN bEventLogEnabled = FALSE;

    dwError = LsaDnsGetHostInfo(&gLPGlobals.pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    if (!IsNullOrEmptyString(pszConfigFilePath)) {

        dwError = LocalCfgParseFile(
                        pszConfigFilePath,
                        &config);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LocalCfgTransferContents(
                        &config,
                        &gLPGlobals.cfg);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaAllocateString(
                        pszConfigFilePath,
                        &gLPGlobals.pszConfigFilePath);
        BAIL_ON_LSA_ERROR(dwError);

    }

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
LsaLPOpenHandle(
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
        LsaLPCloseHandle((HANDLE)pContext);
    }

    goto cleanup;
}

void
LsaLPCloseHandle(
    HANDLE hProvider
    )
{
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    if (pContext)
    {
        LsaFreeMemory(pContext);
    }
}

BOOLEAN
LsaLPServicesDomain(
    PCSTR pszDomain
    )
{
    BOOLEAN bResult = FALSE;

    if (IsNullOrEmptyString(pszDomain) ||
        !strcasecmp(pszDomain, "localhost") ||
        !strcasecmp(pszDomain, gLPGlobals.pszHostname))
    {
        bResult = TRUE;
    }

    return bResult;
}

#if 0
static
DWORD
CheckAccountFlags(
    PLSA_USER_INFO_2 pUserInfo2
    )
{
    DWORD dwError = LSA_ERROR_INTERNAL;

    BAIL_ON_INVALID_POINTER(pUserInfo2);

    if (pUserInfo2->bAccountDisabled) {
        dwError = LSA_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bAccountLocked) {
        dwError = LSA_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bAccountExpired) {
        dwError = LSA_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bPasswordExpired) {
        dwError = LSA_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LSA_ERROR_SUCCESS;

cleanup:
    return dwError;

error:
    goto cleanup;
}
#endif

DWORD
LsaLPAuthenticateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
#if 0
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

    dwError = LsaLPFindUserByName(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    /* Check for disable, expired, etc..  accounts */

    dwError = CheckAccountFlags(pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

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

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    LSA_SAFE_FREE_MEMORY(pHash);

    return dwError;

error:

    goto cleanup;
#else
    return dwError;
#endif
}


DWORD
LsaLPAuthenticateUserEx(
    HANDLE hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO *ppUserInfo
    )
{
    DWORD dwError = LSA_ERROR_INTERNAL;
#if 0
    PLSA_USER_INFO_2 pUserInfo2 = NULL;
    PSTR pszAccountName = NULL;
    PCSTR pszDomain = NULL;
    DWORD dwLen = 0;
    BYTE NTResponse[24] = { 0 };
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PBYTE pChal = NULL;
    PBYTE pNTresp = NULL;
    DWORD dwUserInfoLevel = 2;

    BAIL_ON_INVALID_POINTER(pUserParams->pszAccountName);

    /* Assume the local domain (localhost) if we don't have one */

    if (pUserParams->pszDomain)
        pszDomain = pUserParams->pszDomain;
    else
        pszDomain = "LOCALHOST";

    /* Allow the next provider to continue if we don't handle this domain */

    if (!LsaLPServicesDomain(pszDomain)) {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* calculate length includeing '\' and terminating NULL */

    dwLen = strlen(pszDomain) + strlen(pUserParams->pszAccountName) + 2;
    dwError = LsaAllocateMemory(dwLen, (PVOID*)&pszAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    snprintf(pszAccountName, dwLen,
             "%s\\%s",
             pszDomain,
             pUserParams->pszAccountName);

    /* Find the user */

    dwError = LsaLPFindUserByName(hProvider,
                                              pszAccountName,
                                              dwUserInfoLevel,
                                              (PVOID*)&pUserInfo2);
    BAIL_ON_LSA_ERROR(dwError);

    /* Check Account Status */

    dwError = CheckAccountFlags(pUserInfo2);
    BAIL_ON_LSA_ERROR(dwError);

    /* generate the responses and compare */

    pChal = LsaDataBlobBuffer(pUserParams->pass.chap.pChallenge);
    BAIL_ON_INVALID_POINTER(pChal);

    ntError = NTLMv1EncryptChallenge(pChal,
                                     NULL,     /* ignore LM hash */
                                     pUserInfo2->info1.pNTHash,
                                     NULL,
                                     NTResponse);
    if (ntError != STATUS_SUCCESS) {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pNTresp = LsaDataBlobBuffer(pUserParams->pass.chap.pNT_resp);
    BAIL_ON_INVALID_POINTER(pNTresp);

    if (memcmp(pNTresp, NTResponse, 24) != 0)
    {
        dwError = LSA_ERROR_PASSWORD_MISMATCH;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pUserInfo2) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo2);
    }

    LSA_SAFE_FREE_MEMORY(pszAccountName);

    return dwError;

error:

    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LsaLPValidateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
#if 0
    DWORD dwUserInfoLevel = 2;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPFindUserByName(
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

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
#else

    return dwError;

#endif
}

DWORD
LsaLPCheckUserInList(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszListName
    )
{
    return LSA_ERROR_NOT_HANDLED;
}

DWORD
LsaLPFindUserByName(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = 0;
#if 0
    HANDLE hDb = (HANDLE)NULL;
    PVOID pUserInfo = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LsaLPServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!strcasecmp(pLoginInfo->pszName, "root"))
    {
	dwError = LSA_ERROR_NO_SUCH_USER;
	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbFindUserByName(
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
        LsaLPDbClose(hDb);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    goto cleanup;
#else

    return dwError;
#endif
}

DWORD
LsaLPFindUserById(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = 0;
#if 0
    HANDLE hDb = (HANDLE)NULL;
    PVOID pUserInfo = NULL;

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    if (uid == 0)
    {
	dwError = LSA_ERROR_NO_SUCH_USER;
	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbFindUserById(
                    hDb,
                    uid,
                    dwUserInfoLevel,
                    &pUserInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    *ppUserInfo = pUserInfo;

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    return dwError;

error:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LsaLPGetGroupsForUser(
    IN HANDLE hProvider,
    IN uid_t uid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    IN PDWORD pdwGroupsFound,
    IN PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
#if 0
    HANDLE hDb = (HANDLE)NULL;

    if (uid == 0)
    {
	dwError = LSA_ERROR_NO_SUCH_USER;
	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbGetGroupsForUser(
                    hDb,
                    uid,
                    dwGroupInfoLevel,
                    pdwGroupsFound,
                    pppGroupInfoList
                    );
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    return dwError;

error:
    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LsaLPBeginEnumUsers(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    dwError = LsaLPCreateUserState(
                        hProvider,
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
LsaLPEnumUsers(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxNumRecords,
    PDWORD   pdwUsersFound,
    PVOID**  pppUserInfoList
    )
{
    DWORD dwError = 0;
#if 0
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    pEnumState = (PLOCAL_PROVIDER_ENUM_STATE)hResume;

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbEnumUsers(
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
        LsaLPDbClose(hDb);
    }

    return dwError;

error:

    *pdwUsersFound = 0;
    *pppUserInfoList = NULL;

    goto cleanup;
#else
    *pdwUsersFound = 0;
    *pppUserInfoList = NULL;
    return dwError;
#endif
}

VOID
LsaLPEndEnumUsers(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    LsaLPFreeUserState(
            hProvider,
            (PLOCAL_PROVIDER_ENUM_STATE)hResume);
}

DWORD
LsaLPFindGroupByName(
    IN HANDLE hProvider,
    IN PCSTR pszGroupName,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
#if 0
    HANDLE hDb = (HANDLE)NULL;
    PVOID pGroupInfo = NULL;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszGroupName,
                    NULL,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LsaLPServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!strcasecmp(pLoginInfo->pszName, "root"))
    {
	dwError = LSA_ERROR_NO_SUCH_GROUP;
	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbFindGroupByName(
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
        LsaLPDbClose(hDb);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LsaLPFindGroupById(
    IN HANDLE hProvider,
    IN gid_t gid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
#if 0
    HANDLE hDb = (HANDLE)NULL;
    PVOID pGroupInfo = NULL;

    if (gid == 0)
    {
	dwError = LSA_ERROR_NO_SUCH_GROUP;
	BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbFindGroupById(
                    hDb,
                    gid,
                    dwGroupInfoLevel,
                    &pGroupInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    *ppGroupInfo = pGroupInfo;

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    return dwError;

error:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LsaLPBeginEnumGroups(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    BOOLEAN bCheckOnline,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = NULL;

    dwError = LsaLPCreateGroupState(
                        hProvider,
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
LsaLPEnumGroups(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxGroups,
    PDWORD   pdwGroupsFound,
    PVOID**  pppGroupInfoList
    )
{
    DWORD dwError = 0;
#if 0
    PLOCAL_PROVIDER_ENUM_STATE pEnumState = (PLOCAL_PROVIDER_ENUM_STATE)hResume;
    HANDLE hDb = (HANDLE)NULL;

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbEnumGroups(
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
        LsaLPDbClose(hDb);
    }

    return dwError;

error:

    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;

    goto cleanup;
#else
    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;
    return dwError;
#endif
}

VOID
LsaLPEndEnumGroups(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    LsaLPFreeGroupState(
            hProvider,
            (PLOCAL_PROVIDER_ENUM_STATE)hResume);
}

DWORD
LsaLPChangePassword(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword
    )
{
    DWORD dwError = 0;
#if 0
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

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbFindUserByName(
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

        dwError = LsaLPAuthenticateUser(
                        hProvider,
                        pszLoginId,
                        pszOldPassword);
        BAIL_ON_LSA_ERROR(dwError);

    }

    dwError = LsaLPDbChangePassword(
                    hDb,
                    ((PLSA_USER_INFO_2)pUserInfo)->uid,
                    pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    return dwError;

error:

    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LsaLPAddUser(
    HANDLE hProvider,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    )
{
    DWORD dwError = 0;
#if 0
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

    if (!LsaLPServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pContext->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbAddUser(
                    hDb,
                    dwUserInfoLevel,
                    pUserInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaLPEventlogEnabled()){
        LocalEventLogUserAdd(pLoginInfo->pszName, ((PLSA_USER_INFO_0)pUserInfo)->uid);
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LsaLPModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    DWORD dwError = 0;
#if 0
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    if (pContext->uid) {
       dwError = EACCES;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbModifyUser(
                        hDb,
                        pUserModInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hDb != (HANDLE)NULL) {
       LsaLPDbClose(hDb);
    }

    return dwError;

error:

    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LsaLPDeleteUser(
    HANDLE hProvider,
    uid_t  uid
    )
{
    DWORD dwError = 0;
#if 0
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    if (pContext->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbDeleteUser(
                    hDb,
                    uid);
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaLPEventlogEnabled())
    {
        LocalEventLogUserDelete(uid);
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    return dwError;

error:

    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LsaLPAddGroup(
    HANDLE hProvider,
    DWORD  dwGroupInfoLevel,
    PVOID  pGroupInfo
    )
{
    DWORD dwError = 0;
#if 0
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

    if (!LsaLPServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pContext->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbAddGroup(
                    hDb,
                    pLoginInfo->pszDomainNetBiosName,
                    dwGroupInfoLevel,
                    pGroupInfo
                    );
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaLPEventlogEnabled()){
        LocalEventLogGroupAdd(pLoginInfo->pszName, ((PLSA_GROUP_INFO_0)pGroupInfo)->gid);
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LsaLPDeleteGroup(
    HANDLE hProvider,
    gid_t  gid
    )
{
    DWORD dwError = 0;
#if 0
    HANDLE hDb = (HANDLE)NULL;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;

    if (pContext->uid) {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaLPDbDeleteGroup(
                    hDb,
                    gid);
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaLPEventlogEnabled()){
        LocalEventLogGroupDelete(gid);
    }

cleanup:

    if (hDb != (HANDLE)NULL) {
        LsaLPDbClose(hDb);
    }

    return dwError;

error:

    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LsaLPOpenSession(
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

    dwError = LsaLPFindUserByName(
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

    dwError = LsaLPCreateHomeDirectory(
                    (PLSA_USER_INFO_0)pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaLPCreateHomeDirectory(
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

    if (!bExists)
    {
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

       dwError = LsaLPProvisionHomeDir(
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
LsaLPProvisionHomeDir(
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
LsaLPCloseSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoLevel = 0;
    PVOID pUserInfo = NULL;

    dwError = LsaLPFindUserByName(
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
    LSA_SAFE_FREE_STRING(gLPGlobals.pszConfigFilePath);

    LSA_SAFE_FREE_STRING(gLPGlobals.pszHostname);

    return 0;
}

DWORD
LsaLPGetNamesBySidList(
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
LsaLPFindNSSArtefactByKey(
    HANDLE hProvider,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
    *ppNSSArtefactInfo = NULL;

    return LSA_ERROR_NOT_HANDLED;
}


DWORD
LsaLPBeginEnumNSSArtefacts(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;

    dwError = LSA_ERROR_NOT_HANDLED;

    *phResume = (HANDLE)NULL;

    return dwError;
}

DWORD
LsaLPEnumNSSArtefacts(
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
LsaLPEndEnumNSSArtefacts(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    return;
}

DWORD
LsaLPGetStatus(
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
        LsaLPFreeStatus(pProviderStatus);
    }

    goto cleanup;
}

DWORD
LsaLPRefreshConfiguration(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    PSTR pszConfigFilePath = NULL;
    LOCAL_CONFIG config = {0};
    BOOLEAN bInLock = FALSE;

    dwError = LocalCfgGetFilePath(&pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);

    if (!IsNullOrEmptyString(pszConfigFilePath))
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

    LocalEventLogConfigReload();

cleanup:

    LSA_SAFE_FREE_STRING(pszConfigFilePath);

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    return dwError;

error:

    LocalCfgFreeContents(&config);

    goto cleanup;
}

VOID
LsaLPFreeStatus(
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

DWORD
LsaLPProviderIoControl(
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
    return LSA_ERROR_NOT_HANDLED;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
