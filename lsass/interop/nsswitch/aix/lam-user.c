/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "includes.h"
#include "lam-user.h"
#include "lam-group.h"

#include <ctype.h>

DWORD
LsaNssFindUserByAixName(
    HANDLE hLsaConnection,
    PCSTR  pszName,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    )
{
    uint64_t qwUid = 0;
    PCSTR pszPos = pszName;
    char chDigit = 0;

    if (pszName[0] == '_' && strlen(pszName) == S_NAMELEN - 1)
    {
        // This is AIX mangled. The name is really an encoded uid
        pszPos++;
        if (*pszPos >= 'a')
        {
            // This is in base 32
            while(*pszPos)
            {
                if (*pszPos >= 'a')
                {
                    chDigit = *pszPos - 'a' + 10;
                    if (chDigit > 32)
                    {
                        goto not_mangled;
                    }
                }
                else
                {
                    chDigit = *pszPos - '0';
                    if (chDigit > 10)
                    {
                        goto not_mangled;
                    }
                }
                qwUid *= 32;
                qwUid += chDigit;
                pszPos++;
            }
            if (qwUid < 10737418240ull + 10000000)
            {
                goto not_mangled;
            }
            qwUid -= 10737418240ull;
            if (qwUid > 0xFFFFFFFF)
            {
                goto not_mangled;
            }
        }
        else
        {
            // This is base 10
            while(*pszPos)
            {
                chDigit = *pszPos - '0';
                if (chDigit > 10)
                {
                    goto not_mangled;
                }

                qwUid *= 10;
                qwUid += chDigit;
                pszPos++;
            }
            if (qwUid > 10000000)
            {
                goto not_mangled;
            }
        }
        return LsaFindUserById(
                    hLsaConnection,
                    (uid_t)qwUid,
                    dwUserInfoLevel,
                    ppUserInfo);
    }

not_mangled:

    return LsaFindUserByName(
                hLsaConnection,
                pszName,
                dwUserInfoLevel,
                ppUserInfo);
}

void
LsaNssFreeLastUser(
        VOID
        )
{
    LW_SAFE_FREE_MEMORY(gNssState.pLastUser);
}

DWORD LsaNssAllocateUserFromInfo0(
        PLSA_USER_INFO_0 pInfo,
        struct passwd** ppResult
        )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    // Do not free directly. Free pStartMem instead on error
    struct passwd* pResult = NULL;
    void *pStartMem = NULL;
    // Do not free
    PBYTE pMem = NULL;
    size_t sRequiredSize = 0;
    size_t sLen = 0;

    sRequiredSize += sizeof(struct passwd);
    sRequiredSize += strlen(pInfo->pszName) + 1;
    if ( LW_IS_NULL_OR_EMPTY_STR(pInfo->pszPasswd) )
    {
        sRequiredSize += strlen(LSA_NSS_NOPASSWORD) + 1;
    }
    else
    {
        sRequiredSize += strlen(pInfo->pszPasswd) + 1;
    }
    sRequiredSize += strlen(pInfo->pszGecos) + 1;
    sRequiredSize += strlen(pInfo->pszHomedir) + 1;
    sRequiredSize += strlen(pInfo->pszShell) + 1;

    dwError = LwAllocateMemory(
            sRequiredSize,
            &pStartMem);
    BAIL_ON_LSA_ERROR(dwError);

    pMem = pStartMem;
    pResult = (struct passwd *)pMem;
    pMem += sizeof(struct passwd);

    sLen = strlen(pInfo->pszName);
    pResult->pw_name = (char *)pMem;
    memcpy(pResult->pw_name, pInfo->pszName, sLen + 1);
    pMem += sLen + 1;

    pResult->pw_passwd = (char *)pMem;
    if ( LW_IS_NULL_OR_EMPTY_STR(pInfo->pszPasswd) )
    {
        sLen = strlen(LSA_NSS_NOPASSWORD);
        memcpy(pResult->pw_passwd, LSA_NSS_NOPASSWORD, sLen + 1);
    }
    else
    {
        sLen = strlen(pInfo->pszPasswd);
        memcpy(pResult->pw_passwd, pInfo->pszPasswd, sLen + 1);
    }
    pMem += sLen + 1;

    pResult->pw_uid = pInfo->uid;
    pResult->pw_gid = pInfo->gid;

    sLen = strlen(pInfo->pszGecos);
    pResult->pw_gecos = (char *)pMem;
    memcpy(pResult->pw_gecos, pInfo->pszGecos, sLen + 1);
    pMem += sLen + 1;

    sLen = strlen(pInfo->pszHomedir);
    pResult->pw_dir = (char *)pMem;
    memcpy(pResult->pw_dir, pInfo->pszHomedir, sLen + 1);
    pMem += sLen + 1;

    sLen = strlen(pInfo->pszShell);
    pResult->pw_shell = (char *)pMem;
    memcpy(pResult->pw_shell, pInfo->pszShell, sLen + 1);
    pMem += sLen + 1;
    assert(pMem == pStartMem + sRequiredSize);

    *ppResult = pResult;

cleanup:

    return dwError;

error:

    *ppResult = NULL;
    LW_SAFE_FREE_MEMORY(pStartMem);

    goto cleanup;
}

struct passwd *LsaNssGetPwUid(uid_t uid)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_USER_INFO_0 pInfo = NULL;
    const DWORD dwInfoLevel = 0;
    struct passwd *pResult = NULL;

    LSA_LOG_PAM_DEBUG("Lsass queried by getpwuid for [%ld]", (long)uid);

    dwError = LsaNssCommonEnsureConnected(&lsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindUserById(
                lsaConnection.hLsaConnection,
                uid,
                dwInfoLevel,
                (PVOID*)&pInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNssAllocateUserFromInfo0(
                pInfo,
                &pResult);
    BAIL_ON_LSA_ERROR(dwError);

    LsaNssFreeLastUser();
    gNssState.pLastUser = pResult;

cleanup:

    if (pInfo != NULL)
    {
        LsaFreeUserInfo(
                dwInfoLevel,
                (PVOID)pInfo);
    }

    if (dwError != LW_ERROR_SUCCESS)
    {
        LsaNssMapErrorCode(dwError, &errno);
    }
    return pResult;

error:
    LsaNssCommonCloseConnection(&lsaConnection);

    goto cleanup;
}

struct passwd *LsaNssGetPwNam(PCSTR pszName)
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_USER_INFO_0 pInfo = NULL;
    const DWORD dwInfoLevel = 0;
    struct passwd *pResult = NULL;

    LSA_LOG_PAM_DEBUG("Lsass queried by getpwnam for [%s]", pszName);

    dwError = LsaNssCommonEnsureConnected(&lsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNssFindUserByAixName(
                lsaConnection.hLsaConnection,
                pszName,
                dwInfoLevel,
                (PVOID*)&pInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNssAllocateUserFromInfo0(
                pInfo,
                &pResult);
    BAIL_ON_LSA_ERROR(dwError);

    LsaNssFreeLastUser();
    gNssState.pLastUser = pResult;

cleanup:

    if (pInfo != NULL)
    {
        LsaFreeUserInfo(
                dwInfoLevel,
                pInfo);
    }

    if (dwError != LW_ERROR_SUCCESS)
    {
        LsaNssMapErrorCode(dwError, &errno);
    }
    return pResult;

error:
    LsaNssCommonCloseConnection(&lsaConnection);

    goto cleanup;
}

DWORD
LsaNssListUsers(
        HANDLE hLsaConnection,
        attrval_t* pResult
        )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    const DWORD dwInfoLevel = 0;
    const DWORD dwEnumLimit = 100000;
    DWORD dwIndex = 0;
    DWORD dwUsersFound = 0;
    HANDLE hResume = (HANDLE)NULL;
    size_t sRequiredMem = 0;
    PLSA_USER_INFO_0* ppUserList = NULL;
    PSTR pszListStart = NULL;
    // Do not free
    PSTR pszPos = NULL;

    LSA_LOG_PAM_DEBUG("Enumerating users");

    dwError = LsaBeginEnumUsers(
                hLsaConnection,
                dwInfoLevel,
                dwEnumLimit,
                LSA_FIND_FLAGS_NSS,
                &hResume);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaEnumUsers(
                hLsaConnection,
                hResume,
                &dwUsersFound,
                (PVOID**)&ppUserList);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwUsersFound == dwEnumLimit)
    {
        pResult->attr_flag = ENOMEM;
        goto error;
    }

    sRequiredMem = 1;
    for (dwIndex = 0; dwIndex < dwUsersFound; dwIndex++)
    {
        sRequiredMem += strlen(ppUserList[dwIndex]->pszName) + 1;
    }

    dwError = LwAllocateMemory(
                sRequiredMem,
                (PVOID*)&pszListStart);
    BAIL_ON_LSA_ERROR(dwError);
    pszPos = pszListStart;

    for (dwIndex = 0; dwIndex < dwUsersFound; dwIndex++)
    {
        strcpy(pszPos, ppUserList[dwIndex]->pszName);
        pszPos += strlen(pszPos) + 1;
    }
    *pszPos++ = 0;

    assert(pszPos == pszListStart + sRequiredMem);

    pResult->attr_un.au_char = pszListStart;
    pResult->attr_flag = 0;

cleanup:

    if (hResume != (HANDLE)NULL)
    {
        LsaEndEnumUsers(
                hLsaConnection,
                hResume);
    }
    if (ppUserList != NULL)
    {
        LsaFreeUserInfoList(
                dwInfoLevel,
                (PVOID*)ppUserList,
                dwUsersFound);
    }

    return dwError;

error:

    pResult->attr_un.au_char = NULL;
    LW_SAFE_FREE_MEMORY(pszListStart);
    goto cleanup;
}

typedef struct _LW_AUDIT_CLASS_LIST
{
    PSTR    *ppszAuditClasses;
    DWORD   dwCount;
    DWORD   dwAlloc;
    DWORD   dwLength;
} LW_AUDIT_CLASS_LIST, *PLW_AUDIT_CLASS_LIST;

#define AUDIT_CLASS_ALLOC_INCR  8

static
VOID
LwNssFreeAuditClassList(
        PLW_AUDIT_CLASS_LIST pAuditClassList
        )
{
    if (pAuditClassList->ppszAuditClasses)
    {
        DWORD dwAuditClass;

        for (dwAuditClass = 0;
                dwAuditClass < pAuditClassList->dwCount;
                ++dwAuditClass)
        {
            LwFreeString(pAuditClassList->ppszAuditClasses[dwAuditClass]);
        }

        LwFreeMemory(pAuditClassList);
    }
}

static DWORD
LsaNssAddAuditClass(
        PLW_AUDIT_CLASS_LIST pAuditClassList,
        PCSTR psAuditClass, /* Not NUL-terminated. */
        DWORD dwLength
        )
{
    PSTR pszAuditClassCopy = NULL;
    DWORD dwIndex = 0;
    DWORD dwError = LW_ERROR_SUCCESS;

    for (dwIndex = 0; dwIndex < pAuditClassList->dwCount; ++dwIndex)
    {
        if (!strncmp(
                pAuditClassList->ppszAuditClasses[dwIndex],
                psAuditClass,
                dwLength))
        {
            /* The audit class is already in the list. */
            goto cleanup;
        }
    }

    dwError = LwAllocateMemory(dwLength + 1, (PVOID *) &pszAuditClassCopy);
    BAIL_ON_LSA_ERROR(dwError);

    strncpy(pszAuditClassCopy, psAuditClass, dwLength);
    pszAuditClassCopy[dwLength] = '\0';

    if (pAuditClassList->dwCount == pAuditClassList->dwAlloc)
    {
        PSTR *ppszNewAuditClasses;

        dwError = LwReallocMemory(
                    pAuditClassList->ppszAuditClasses,
                    (PVOID *) &ppszNewAuditClasses,
                    (pAuditClassList->dwAlloc + AUDIT_CLASS_ALLOC_INCR) *
                        sizeof(*ppszNewAuditClasses));
        BAIL_ON_LSA_ERROR(dwError);

        pAuditClassList->ppszAuditClasses = ppszNewAuditClasses;
        pAuditClassList->dwAlloc += AUDIT_CLASS_ALLOC_INCR;
    }

    pAuditClassList->ppszAuditClasses[pAuditClassList->dwCount++] =
        pszAuditClassCopy;
    pAuditClassList->dwLength += dwLength + 1;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_STRING(pszAuditClassCopy);
    goto cleanup;
}

static DWORD
LsaNssAddAuditClasses(
        PLW_AUDIT_CLASS_LIST pAuditClassList,
        PCSTR pszAuditClasses)
{
    PCSTR psAuditClass;
    DWORD dwLength;
    DWORD dwError = LW_ERROR_SUCCESS;

    while (*pszAuditClasses && *pszAuditClasses != '\n')
    {
        while (*pszAuditClasses &&
                (*pszAuditClasses == ',' || isspace(*pszAuditClasses)))
        {
            ++pszAuditClasses;
        }

        if (*pszAuditClasses == '\0')
        {
            break;
        }

        psAuditClass = pszAuditClasses;

        while (*pszAuditClasses &&
                *pszAuditClasses != ',' && !isspace(*pszAuditClasses))
        {
            ++pszAuditClasses;
        }

        dwLength = pszAuditClasses - psAuditClass;
        dwError = LsaNssAddAuditClass(
                    pAuditClassList,
                    psAuditClass,
                    dwLength);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

VOID
LsaNssGetUserAttr(
        HANDLE hLsaConnection,
        PLSA_USER_INFO_2 pInfo,
        PSTR pszAttribute,
        attrval_t* pResult
        )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    const DWORD dwGroupInfoLevel = 0;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    FILE *fp = NULL;
    PSTR pszDefaultAuditClasses = NULL;
    PSTR pszGroups = NULL;
    PLW_AUDIT_CLASS_LIST pAuditClassList = NULL;

    if (!strcmp(pszAttribute, S_ID))
    {
        pResult->attr_un.au_long = pInfo->uid;
    }
    else if (!strcmp(pszAttribute, S_PGID))
    {
        pResult->attr_un.au_long = pInfo->gid;
    }
    else if (!strcmp(pszAttribute, S_PWD))
    {
        dwError = LwAllocateString(
                    pInfo->pszPasswd,
                    &pResult->attr_un.au_char);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!strcmp(pszAttribute, S_HOME))
    {
        dwError = LwAllocateString(
                    pInfo->pszHomedir,
                    &pResult->attr_un.au_char);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!strcmp(pszAttribute, S_SHELL))
    {
        dwError = LwAllocateString(
                    pInfo->pszShell,
                    &pResult->attr_un.au_char);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!strcmp(pszAttribute, S_REGISTRY))
    {
        dwError = LwAllocateString(
                    gNssState.pszRegistryName,
                    &pResult->attr_un.au_char);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!strcmp(pszAttribute, S_GECOS))
    {
        dwError = LwAllocateString(
                    pInfo->pszGecos,
                    &pResult->attr_un.au_char);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!strcmp(pszAttribute, S_PGRP))
    {
        dwError = LsaFindGroupById(
                    hLsaConnection,
                    pInfo->gid,
                    LSA_FIND_FLAGS_NSS,
                    dwGroupInfoLevel,
                    (PVOID*)&pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwAllocateString(
                    pGroupInfo->pszName,
                    &pResult->attr_un.au_char);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!strcmp(pszAttribute, S_GROUPS))
    {
        dwError = LsaNssGetGroupList(
                    hLsaConnection,
                    pInfo,
                    &pResult->attr_un.au_char);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!strcmp(pszAttribute, S_GROUPSIDS))
    {
        dwError = LsaNssGetGidList(
                    hLsaConnection,
                    pInfo,
                    &pResult->attr_un.au_char);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!strcmp(pszAttribute, S_AUDITCLASSES))
    {
        if (geteuid() != 0)
        {
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
        }

        pResult->attr_un.au_char = NULL;
        fp = fopen("/etc/likewise/auditclasses", "r");
        if (fp != NULL)
        {
            char buf[1024];
            DWORD dwLength = strlen(pInfo->pszName);
            LW_BOOL bSawUser = LW_FALSE;
            PSTR pszCopy = NULL;
            DWORD dwAuditClass = 0;

            dwError = LwAllocateMemory(
                        sizeof(*pAuditClassList),
                        (PVOID *) &pAuditClassList);
            BAIL_ON_LSA_ERROR(dwError);

            while (fgets(buf, sizeof(buf), fp) != NULL)
            {
                char *line = buf;
                char *comment = NULL;
                char *colon = NULL;

                while (isspace(*line))
                {
                    ++line;
                }

                if ((comment = strchr(line, '#')) != NULL)
                {
                    *comment = '\0';
                }

                if (*line == '\0')
                {
                    // blank or comment-only line.
                    continue;
                }

                if (line[dwLength] == ':' &&
                        !strncmp(line, pInfo->pszName, dwLength))
                {
                    dwError = LsaNssAddAuditClasses(
                                pAuditClassList,
                                line + dwLength + 1);
                    BAIL_ON_LSA_ERROR(dwError);
                    bSawUser = LW_TRUE;
                }
                else if ((colon = strchr(line, ':')) != NULL)
                {
                    if (*line == '@')
                    {
                        // Audit class for a group; see if the user is in it.
                        PCSTR pszGroup;
                        PCSTR psAuditGroup = line + 1; // not NUL-terminated.
                        LW_BOOL bMatch = LW_FALSE;

                        if (pszGroups == NULL)
                        {
                            dwError = LsaNssGetGroupList(
                                        hLsaConnection,
                                        pInfo,
                                        &pszGroups);
                            BAIL_ON_LSA_ERROR(dwError);
                        }

                        pszGroup = pszGroups;
                        while (*pszGroup != '\0')
                        {
                            bMatch = LW_TRUE;
                            psAuditGroup = line + 1;

                            while (*pszGroup != '\0')
                            {
                                if (bMatch && *pszGroup != *psAuditGroup++)
                                {
                                    bMatch = LW_FALSE;
                                    // no "break" here - keep reading pszGroup.
                                }

                                ++pszGroup;
                            }

                            if (bMatch)
                            {
                                break;
                            }

                            ++pszGroup;
                        }

                        if (bMatch)
                        {
                            dwError = LsaNssAddAuditClasses(
                                        pAuditClassList,
                                        colon + 1);
                            BAIL_ON_LSA_ERROR(dwError);
                        }
                    }
                    /* Else this is a line for another user. */
                }
                else
                {
                    /* This is the default audit classes line. */
                    if (pszDefaultAuditClasses == NULL)
                    {
                        dwError = LwAllocateString(line, &pszDefaultAuditClasses);
                        BAIL_ON_LSA_ERROR(dwError);
                    }
                }
            }

            if (!bSawUser && pszDefaultAuditClasses != NULL)
            {
                dwError = LsaNssAddAuditClasses(
                            pAuditClassList,
                            pszDefaultAuditClasses);
                BAIL_ON_LSA_ERROR(dwError);
            }

            if (pAuditClassList->dwLength)
            {
                dwLength = pAuditClassList->dwLength + 1;
            }
            else
            {
                dwLength = 2;
            }

            dwError = LwAllocateMemory(
                        dwLength,
                        (PVOID *) &pResult->attr_un.au_char);
            BAIL_ON_LSA_ERROR(dwError);

            pszCopy = pResult->attr_un.au_char;

            for (dwAuditClass = 0;
                    dwAuditClass < pAuditClassList->dwCount;
                    ++dwAuditClass)
            {
                PCSTR pszSource = pAuditClassList->ppszAuditClasses[dwAuditClass];

                while (*pszSource)
                {
                    *pszCopy++ = *pszSource++;
                }

                *pszCopy++ = '\0';
            }

            if (pAuditClassList->dwCount == 0)
            {
                *pszCopy++ = '\0';
            }

            *pszCopy++ = '\0';
        }
    }
    else if (!strcmp(pszAttribute, S_LOCKED))
    {
        pResult->attr_un.au_int = pInfo->bAccountLocked;
    }
    else if (!strcmp(pszAttribute, "UPN"))
    {
        dwError = LwAllocateString(
                    pInfo->pszUPN,
                    &pResult->attr_un.au_char);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!strcmp(pszAttribute, "SID"))
    {
        dwError = LwAllocateString(
                    pInfo->pszSid,
                    &pResult->attr_un.au_char);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = EINVAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pGroupInfo != NULL)
    {
        LsaFreeGroupInfo(
		dwGroupInfoLevel,
                (PVOID)pGroupInfo);
    }
    if (dwError != LW_ERROR_SUCCESS)
    {
        LsaNssMapErrorCode(dwError, &pResult->attr_flag);
    }
    else
    {
        pResult->attr_flag = 0;
    }

    if (fp != NULL)
    {
        fclose(fp);
    }

    LW_SAFE_FREE_STRING(pszDefaultAuditClasses);
    LW_SAFE_FREE_STRING(pszGroups);

    if (pAuditClassList)
    {
        LwNssFreeAuditClassList(pAuditClassList);
    }

    return;

error:

    goto cleanup;
}

DWORD
LsaNssGetUserAttrs(
        HANDLE hLsaConnection,
        PSTR pszKey,
        PSTR* ppszAttributes,
        attrval_t* pResults,
        int iAttrCount
        )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    const DWORD dwInfoLevel = 2;
    PLSA_USER_INFO_2 pInfo = NULL;
    int iIndex = 0;

    dwError = LsaNssFindUserByAixName(
                hLsaConnection,
                pszKey,
                dwInfoLevel,
                (PVOID*)&pInfo);
    BAIL_ON_LSA_ERROR(dwError);

    for (iIndex = 0; iIndex < iAttrCount; iIndex++)
    {
        // This function stores the error in pResults[iIndex].attr_flag
        LsaNssGetUserAttr(
            hLsaConnection,
            pInfo,
            ppszAttributes[iIndex],
            &pResults[iIndex]);
    }

cleanup:

    if (pInfo != NULL)
    {
        LsaFreeUserInfo(
                dwInfoLevel,
                (PVOID)pInfo);
    }
    return dwError;

error:

    goto cleanup;
}
