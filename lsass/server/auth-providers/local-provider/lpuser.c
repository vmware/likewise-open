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
 *        lsassdb.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Directory User Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
LocalDirFindUserByName(
    HANDLE  hProvider,
    PCSTR   pszNetBIOSName,
    PCSTR   pszUserName,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = 0;

    switch(dwUserInfoLevel)
    {
        case 0:

            dwError = LocalDirFindUserByName_0(
                            hProvider,
                            pszUserName,
                            ppUserInfo
                            );
            break;

        case 1:

            dwError = LocalDirFindUserByName_1(
                            hProvider,
                            pszUserName,
                            ppUserInfo
                            );
            break;

        case 2:

            dwError = LocalDirFindUserByName_2(
                            hProvider,
                            pszUserName,
                            ppUserInfo
                            );
            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;

            break;
    }

    return dwError;
}

DWORD
LocalDirFindUserByName_0(
    HANDLE hProvider,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
LocalDirFindUserByName_1(
    HANDLE hProvider,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
LocalDirFindUserByName_2(
    HANDLE hProvider,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirFindUserById(
    HANDLE hProvider,
    uid_t  uid,
    DWORD  dwInfoLevel,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    switch(dwInfoLevel)
    {
        case 0:

            dwError = LocalDirFindUserById_0(hProvider, uid, ppUserInfo);

            break;

        case 1:

            dwError = LocalDirFindUserById_1(hProvider, uid, ppUserInfo);

            break;

        case 2:

            dwError = LocalDirFindUserById_2(hProvider, uid, ppUserInfo);

            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;

            break;
    }

    return dwError;
}

DWORD
LocalDirFindUserById_0(
    HANDLE hProvider,
    uid_t  uid,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PLOCAL_PROVIDER_CONTEXT pContext = (PLOCAL_PROVIDER_CONTEXT)hProvider;
    wchar16_t wszAttrNameUID[] = LOCAL_DIR_ATTR_UID;
    wchar16_t wszAttrNameGID[] = LOCAL_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = LOCAL_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNamePassword[] = LOCAL_DIR_ATTR_PASSWORD;
    wchar16_t wszAttrNameGecos[] = LOCAL_DIR_ATTR_GECOS;
    wchar16_t wszAttrNameShell[] = LOCAL_DIR_ATTR_SHELL;
    wchar16_t wszAttrNameHomedir[] = LOCAL_DIR_ATTR_HOME_DIR;
    wchar16_t wszAttrNameObjectSID[] = LOCAL_DIR_ATTR_OBJECT_SID;
    PWSTR wszAttrs[] =
    {
        &wszAttrNameUID[0],
        &wszAttrNameGID[0],
        &wszAttrNameSamAccountName[0],
        &wszAttrNamePassword[0],
        &wszAttrNameGecos[0],
        &wszAttrNameShell[0],
        &wszAttrNameHomedir[0],
        &wszAttrNameObjectSID[0],
        NULL
    };
    DWORD dwNumAttrs = (sizeof(wszAttrs)/sizeof(wszAttrs[0])) - 1;
    PDIRECTORY_ENTRY pEntries = NULL;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwNumEntries = 0;
    PCSTR pszFilterTemplate = LOCAL_DB_DIR_ATTR_UID " = %u" \
                              " AND " LOCAL_DB_DIR_ATTR_OBJECT_CLASS " = %d";
    PSTR pszFilter = NULL;
    PWSTR pwszFilter = NULL;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    DWORD iAttr = 0;

    // Should we include the domain also?
    dwError = LsaAllocateStringPrintf(
                    &pszFilter,
                    pszFilterTemplate,
                    uid,
                    LOCAL_OBJECT_CLASS_USER);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszFilter,
                    &pwszFilter);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = DirectorySearch(
                    pContext->hDirectory,
                    NULL,
                    0,
                    pwszFilter,
                    wszAttrs,
                    FALSE,
                    &pEntries,
                    &dwNumEntries);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNumEntries == 0)
    {
        dwError = LSA_ERROR_NO_SUCH_USER;
    }
    else if (dwNumEntries != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pEntry = &pEntries[0];
    if (pEntry->ulNumAttributes != dwNumAttrs)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(LSA_USER_INFO_0),
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iAttr < pEntry->ulNumAttributes; iAttr++)
    {
        PDIRECTORY_ATTRIBUTE pAttr = &pEntry->pAttributes[iAttr];

        if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameUID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pUserInfo->uid = pAttr->pValues[0].ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameGID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_INTEGER))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pUserInfo->gid = pAttr->pValues[0].ulValue;
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameSamAccountName[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].pwszStringValue,
                            &pUserInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNamePassword[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].pwszStringValue,
                                &pUserInfo->pszPasswd);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameGecos[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].pwszStringValue,
                                &pUserInfo->pszGecos);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameShell[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].pwszStringValue,
                                &pUserInfo->pszShell);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameHomedir[0]))
        {
            if (pAttr->ulNumValues > 0)
            {
                if ((pAttr->ulNumValues != 1) ||
                    (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
                {
                    dwError = LSA_ERROR_DATA_ERROR;
                    BAIL_ON_LSA_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttr->pValues[0].pwszStringValue,
                                &pUserInfo->pszHomedir);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
        else if (!wc16scasecmp(pAttr->pwszName, &wszAttrNameObjectSID[0]))
        {
            if ((pAttr->ulNumValues != 1) ||
                (pAttr->pValues[0].Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING))
            {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaWc16sToMbs(
                            pAttr->pValues[0].pwszStringValue,
                            &pUserInfo->pszSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *ppUserInfo = pUserInfo;

cleanup:

    LSA_SAFE_FREE_STRING(pszFilter);
    LSA_SAFE_FREE_MEMORY(pwszFilter);

    if (pEntries)
    {
        DirectoryFreeEntries(pEntries, dwNumEntries);
    }

    return dwError;

error:

    *ppUserInfo = pUserInfo;

    if (pUserInfo)
    {
        LsaFreeUserInfo(0, pUserInfo);
    }

    goto cleanup;
}

DWORD
LocalDirFindUserById_1(
    HANDLE hProvider,
    uid_t  uid,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirFindUserById_2(
    HANDLE hProvider,
    uid_t  uid,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirEnumUsers(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    DWORD   dwStartingRecordId,
    DWORD   nMaxUsers,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;

    switch (dwInfoLevel)
    {
        case 0:
        {
            dwError = LocalDirEnumUsers_0(
                                hProvider,
                                dwStartingRecordId,
                                nMaxUsers,
                                pdwNumUsersFound,
                                pppUserInfoList
                                );
            break;
        }
        case 1:
        {
            dwError = LocalDirEnumUsers_1(
                                hProvider,
                                dwStartingRecordId,
                                nMaxUsers,
                                pdwNumUsersFound,
                                pppUserInfoList
                                );
            break;
        }
        case 2:
        {
            dwError = LocalDirEnumUsers_2(
                                hProvider,
                                dwStartingRecordId,
                                nMaxUsers,
                                pdwNumUsersFound,
                                pppUserInfoList
                                );
            break;
        }
    }

    return dwError;
}

DWORD
LocalDirEnumUsers_0(
    HANDLE  hProvider,
    DWORD   dwOffset,
    DWORD   dwLimit,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirEnumUsers_1(
    HANDLE hProvider,
    DWORD  dwOffset,
    DWORD  dwLimit,
    PDWORD pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
LocalDirEnumUsers_2(
    HANDLE  hProvider,
    DWORD   dwOffset,
    DWORD   dwLimit,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirGetGroupsForUser(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;

    switch(dwInfoLevel)
    {
        case 0:
        {
            dwError = LocalDirGetGroupsForUser_0(
                                hProvider,
                                uid,
                                pdwGroupsFound,
                                pppGroupInfoList
                                );
            break;
        }
        case 1:
        {
            dwError = LocalDirGetGroupsForUser_1(
                                hProvider,
                                uid,
                                pdwGroupsFound,
                                pppGroupInfoList
                                );
            break;
        }

    }

    return dwError;
}

DWORD
LocalDirGetGroupsForUser_0_Unsafe(
    HANDLE  hProvider,
    uid_t   uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirGetGroupsForUser_1_Unsafe(
    HANDLE  hProvider,
    uid_t   uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirChangePassword(
    HANDLE hProvider,
    uid_t  uid,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirAddUser(
    HANDLE hProvider,
    DWORD  dwInfoLevel,
    PVOID  pUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirModifyUser(
    HANDLE             hProvider,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    DWORD dwError = 0;
#if 0
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;

    // TODO: Implement this in a database transaction

    dwError = LocalFindUserById(
                        hProvider,
                        pUserModInfo->uid,
                        dwUserInfoLevel,
                        &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pUserModInfo->actions.bEnableUser) {
        dwError = LocalEnableUser(
                       hProvider,
                       pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bDisableUser) {
        dwError = LocalDisableUser(
                        hProvider,
                        pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bUnlockUser) {
        dwError = LocalUnlockUser(
                        hProvider,
                        pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetChangePasswordOnNextLogon) {
        dwError = LocalSetChangePasswordOnNextLogon(
                        hProvider,
                        pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LocalSetPasswordExpires(
                        hProvider,
                        pUserModInfo->uid,
                        TRUE);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetAccountExpiryDate) {
        dwError = LocalSetAccountExpiryDate(
                        hProvider,
                        pUserModInfo->uid,
                        pUserModInfo->pszExpiryDate);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bRemoveFromGroups) {
        dwError = LocalRemoveFromGroups(
                        hProvider,
                        pUserModInfo->uid,
                        pUserModInfo->pszRemoveFromGroups);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bAddToGroups) {
        dwError = LocalAddToGroups(
                        hProvider,
                        pUserModInfo->uid,
                        pUserModInfo->pszAddToGroups);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetPasswordMustExpire) {
        dwError = LocalSetPasswordExpires(
                        hProvider,
                        pUserModInfo->uid,
                        TRUE);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetPasswordNeverExpires) {
        dwError = LocalSetPasswordExpires(
                        hProvider,
                        pUserModInfo->uid,
                        FALSE);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pUserInfo) {
       LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return dwError;

error:

    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LocalCreateHomeDirectory(
    PLSA_USER_INFO_0 pUserInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    mode_t  umask = 022;
    mode_t  perms = (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BOOLEAN bRemoveDir = FALSE;

    if (IsNullOrEmptyString(pUserInfo->pszHomedir))
    {
       LSA_LOG_ERROR("The user's [Uid:%ld] home directory is not defined",
                     (long)pUserInfo->uid);
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

       dwError = LocalProvisionHomeDir(
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
LocalProvisionHomeDir(
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

    if (bExists)
    {
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
LocalCheckAccountFlags(
    PLSA_USER_INFO_2 pUserInfo2
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    BAIL_ON_INVALID_POINTER(pUserInfo2);

    if (pUserInfo2->bAccountDisabled)
    {
        dwError = LSA_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bAccountLocked)
    {
        dwError = LSA_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bAccountExpired)
    {
        dwError = LSA_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bPasswordExpired)
    {
        dwError = LSA_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

error:

    return dwError;
}


