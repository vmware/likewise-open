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
 *        vmcache.c
 *
 * Abstract:
 *
 *        This is the public interface for the Vmdir Provider Local Cache
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#include "includes.h"

DWORD
VMCacheOpen(
    IN PCSTR pszDbPath,
    IN PLSA_VMDIR_PROVIDER_STATE pState,
    OUT PLSA_DB_HANDLE phDb
    )
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnOpenHandle)(
                        pszDbPath,
                        pState,
                        phDb
                        );
    return dwError;
}

void
VMCacheSafeClose(
    PLSA_DB_HANDLE phDb
    )
{
    (*gpCacheProvider->pfnSafeClose)(
                        phDb
                        );
    return;
}

DWORD
VMCacheFindUserByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnFindUserByName)(
                        hDb,
                        pUserNameInfo,
                        ppObject
                        );
    return dwError;
}

DWORD
VMCacheFindUserById(
    LSA_DB_HANDLE hDb,
    uid_t uid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnFindUserById)(
                    hDb,
                    uid,
                    ppObject
                    );
    return dwError;
}

DWORD
VMCacheFindGroupByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnFindGroupByName)(
                    hDb,
                    pGroupNameInfo,
                    ppObject
                    );
    return dwError;
}

DWORD
VMCacheFindGroupById(
    LSA_DB_HANDLE hDb,
    gid_t gid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnFindGroupById)(
                    hDb,
                    gid,
                    ppObject
                    );
    return dwError;
}

DWORD
VMCacheEmptyCache(
    IN LSA_DB_HANDLE hDb
    )
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnEmptyCache)(
                    hDb
                    );
    return dwError;
}

DWORD
VMCacheStoreObjectEntry(
    LSA_DB_HANDLE hDb,
    PLSA_SECURITY_OBJECT pObject
    )
{
    return VMCacheStoreObjectEntries(
            hDb,
            1,
            &pObject
            );
}

DWORD
VMCacheStoreObjectEntries(
    LSA_DB_HANDLE hDb,
    size_t  sObjectCount,
    PLSA_SECURITY_OBJECT* ppObjects
    )
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnStoreObjectEntries)(
                        hDb,
                        sObjectCount,
                        ppObjects
                        );
    return dwError;
}

DWORD
VMCacheStoreGroupMembership(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszParentSid,
    IN size_t sMemberCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMembers
    )
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnStoreGroupMembership)(
                        hDb,
                        pszParentSid,
                        sMemberCount,
                        ppMembers
                        );
    return dwError;
}

DWORD
VMCacheStoreGroupsForUser(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszChildSid,
    IN size_t sMemberCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMembers,
    IN BOOLEAN bIsPacAuthoritative
    )
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnStoreGroupsForUser)(
                    hDb,
                    pszChildSid,
                    sMemberCount,
                    ppMembers,
                    bIsPacAuthoritative
                    );
    return dwError;
}

static
DWORD
VMCacheGetMemberships(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bIsGroupMembers,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    )
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnGetMemberships)(
                    hDb,
                    pszSid,
                    bIsGroupMembers,
                    bFilterNotInPacNorLdap,
                    psCount,
                    pppResults
                    );

    return dwError;
}

DWORD
VMCacheGetGroupsForUser(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    )
{
    DWORD dwError = 0;

    dwError = VMCacheGetMemberships(
                        hDb,
                        pszSid,
                        FALSE,
                        bFilterNotInPacNorLdap,
                        psCount,
                        pppResults
                        );
    return dwError;
}

DWORD
VMCacheEnumUsersCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumUsers,
    IN PCSTR                   pszResume,
    OUT DWORD*                 pdwNumUsersFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnEnumUsersCache)(
                        hDb,
                        dwMaxNumUsers,
                        pszResume,
                        pdwNumUsersFound,
                        pppObjects
                        );

    return dwError;
}

DWORD
VMCacheEnumGroupsCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumGroups,
    IN PCSTR                   pszResume,
    OUT DWORD*                 pdwNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnEnumGroupsCache)(
                        hDb,
                        dwMaxNumGroups,
                        pszResume,
                        pdwNumGroupsFound,
                        pppObjects
                        );

    return dwError;
}

DWORD
VMCacheFindObjectByDN(
    LSA_DB_HANDLE hDb,
    PCSTR pszDN,
    PLSA_SECURITY_OBJECT *ppObject)
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnFindObjectByDN)(
                        hDb,
                        pszDN,
                        ppObject
                        );
    return dwError;
}

DWORD
VMCacheFindObjectsByDNList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszDnList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnFindObjectsByDNList)(
                        hDb,
                        sCount,
                        ppszDnList,
                        pppResults
                        );
    return dwError;
}

DWORD
VMCacheFindObjectBySid(
    LSA_DB_HANDLE hDb,
    PCSTR pszSid,
    PLSA_SECURITY_OBJECT *ppObject)
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnFindObjectBySid)(
                            hDb,
                            pszSid,
                            ppObject
                            );

    return dwError;
}

// Leaves NULLs in pppResults for the objects which can't be found in the
// version.
DWORD
VMCacheFindObjectsBySidList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = 0;

    dwError = (*gpCacheProvider->pfnFindObjectsBySidList)(
                        hDb,
                        sCount,
                        ppszSidList,
                        pppResults
                        );

    return dwError;

}

void
VMCacheSafeFreeObject(
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    if (*ppObject)
    {
        LsaUtilFreeSecurityObject(*ppObject);
        *ppObject = NULL;
    }
}

DWORD
VMCacheDuplicateObject(
    OUT PLSA_SECURITY_OBJECT* ppDest,
    IN PLSA_SECURITY_OBJECT pSrc
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pDest = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*pDest),
                    (PVOID*)&pDest);
    BAIL_ON_LSA_ERROR(dwError);

    pDest->version = pSrc->version;

    dwError = LwAllocateString(
                    pSrc->pszObjectSid,
                    &pDest->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    pDest->enabled = pSrc->enabled;

    dwError = LwAllocateString(
                    pSrc->pszSamAccountName,
                    &pDest->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pSrc->pszDN,
                    &pDest->pszDN);
    BAIL_ON_LSA_ERROR(dwError);

    pDest->type = pSrc->type;

    if (pDest->type == LSA_OBJECT_TYPE_USER)
    {
        pDest->userInfo.uid = pSrc->userInfo.uid;
        pDest->userInfo.gid = pSrc->userInfo.gid;

        dwError = LwStrDupOrNull(
                        pSrc->userInfo.pszPrimaryGroupSid,
                        &pDest->userInfo.pszPrimaryGroupSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwStrDupOrNull(
                        pSrc->userInfo.pszUPN,
                        &pDest->userInfo.pszUPN);
        BAIL_ON_LSA_ERROR(dwError);
        dwError = LwStrDupOrNull(
                        pSrc->userInfo.pszAliasName,
                        &pDest->userInfo.pszAliasName);
        BAIL_ON_LSA_ERROR(dwError);
        dwError = LwStrDupOrNull(
                        pSrc->userInfo.pszUnixName,
                        &pDest->userInfo.pszUnixName);
        BAIL_ON_LSA_ERROR(dwError);
        dwError = LwStrDupOrNull(
                        pSrc->userInfo.pszPasswd,
                        &pDest->userInfo.pszPasswd);
        BAIL_ON_LSA_ERROR(dwError);
        dwError = LwStrDupOrNull(
                        pSrc->userInfo.pszGecos,
                        &pDest->userInfo.pszGecos);
        BAIL_ON_LSA_ERROR(dwError);
        dwError = LwStrDupOrNull(
                        pSrc->userInfo.pszShell,
                        &pDest->userInfo.pszShell);
        BAIL_ON_LSA_ERROR(dwError);
        dwError = LwStrDupOrNull(
                        pSrc->userInfo.pszHomedir,
                        &pDest->userInfo.pszHomedir);
        BAIL_ON_LSA_ERROR(dwError);

        pDest->userInfo.qwPwdLastSet = pSrc->userInfo.qwPwdLastSet;
        pDest->userInfo.qwPwdExpires = pSrc->userInfo.qwPwdExpires;
        pDest->userInfo.qwAccountExpires = pSrc->userInfo.qwAccountExpires;

        pDest->userInfo.bIsGeneratedUPN = pSrc->userInfo.bIsGeneratedUPN;
        pDest->userInfo.bIsAccountInfoKnown = pSrc->userInfo.bIsAccountInfoKnown;
        pDest->userInfo.bPasswordExpired = pSrc->userInfo.bPasswordExpired;
        pDest->userInfo.bPasswordNeverExpires = pSrc->userInfo.bPasswordNeverExpires;
        pDest->userInfo.bPromptPasswordChange = pSrc->userInfo.bPromptPasswordChange;
        pDest->userInfo.bUserCanChangePassword = pSrc->userInfo.bUserCanChangePassword;
        pDest->userInfo.bAccountDisabled = pSrc->userInfo.bAccountDisabled;
        pDest->userInfo.bAccountExpired = pSrc->userInfo.bAccountExpired;
        pDest->userInfo.bAccountLocked = pSrc->userInfo.bAccountLocked;
    }
    else if (pDest->type == LSA_OBJECT_TYPE_GROUP)
    {
        pDest->groupInfo.gid = pSrc->groupInfo.gid;

        dwError = LwStrDupOrNull(
                        pSrc->groupInfo.pszAliasName,
                        &pDest->groupInfo.pszAliasName);
        BAIL_ON_LSA_ERROR(dwError);
        dwError = LwStrDupOrNull(
                        pSrc->groupInfo.pszUnixName,
                        &pDest->groupInfo.pszUnixName);
        BAIL_ON_LSA_ERROR(dwError);
        dwError = LwStrDupOrNull(
                        pSrc->groupInfo.pszPasswd,
                        &pDest->groupInfo.pszPasswd);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppDest = pDest;

cleanup:
    return dwError;

error:
    VMCacheSafeFreeObject(&pDest);
    *ppDest = NULL;
    goto cleanup;
}

DWORD
VMCacheDuplicateMembership(
    PLSA_GROUP_MEMBERSHIP* ppDest,
    PLSA_GROUP_MEMBERSHIP pSrc
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_MEMBERSHIP pDest = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*pDest),
                    (PVOID*)&pDest);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = VMCacheDuplicateMembershipContents(
                    pDest,
                    pSrc);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDest = pDest;

cleanup:
    return dwError;

error:
    VMCacheSafeFreeGroupMembership(&pDest);
    *ppDest = NULL;
    goto cleanup;
}

DWORD
VMCacheDuplicateMembershipContents(
    PLSA_GROUP_MEMBERSHIP pDest,
    PLSA_GROUP_MEMBERSHIP pSrc
    )
{
    DWORD dwError = 0;

    dwError = LwStrDupOrNull(
                    pSrc->pszParentSid,
                    &pDest->pszParentSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pSrc->pszChildSid,
                    &pDest->pszChildSid);
    BAIL_ON_LSA_ERROR(dwError);

    pDest->version = pSrc->version;
    pDest->bIsInPac = pSrc->bIsInPac;
    pDest->bIsInPacOnly = pSrc->bIsInPacOnly;
    pDest->bIsInLdap = pSrc->bIsInLdap;
    pDest->bIsDomainPrimaryGroup = pSrc->bIsDomainPrimaryGroup;

cleanup:
    return dwError;

error:
    goto cleanup;
}

void
VMCacheSafeFreeGroupMembership(
        PLSA_GROUP_MEMBERSHIP* ppMembership)
{
    VMCacheFreeGroupMembershipContents(*ppMembership);
    LW_SAFE_FREE_MEMORY(*ppMembership);
}

void
VMCacheFreeGroupMembershipContents(
        PLSA_GROUP_MEMBERSHIP pMembership)
{
    if (pMembership != NULL)
    {
        LW_SAFE_FREE_STRING(pMembership->pszParentSid);
        LW_SAFE_FREE_STRING(pMembership->pszChildSid);
    }
}

void
VMCacheSafeFreeGroupMembershipList(
        size_t sCount,
        PLSA_GROUP_MEMBERSHIP** pppMembershipList)
{
    if (*pppMembershipList != NULL)
    {
        size_t iMember;
        for (iMember = 0; iMember < sCount; iMember++)
        {
            VMCacheSafeFreeGroupMembership(&(*pppMembershipList)[iMember]);
        }
        LW_SAFE_FREE_MEMORY(*pppMembershipList);
    }
}

void
VMCacheSafeFreeObjectList(
        size_t sCount,
        PLSA_SECURITY_OBJECT** pppObjectList)
{
    if (*pppObjectList != NULL)
    {
        size_t sIndex;
        for (sIndex = 0; sIndex < sCount; sIndex++)
        {
            VMCacheSafeFreeObject(&(*pppObjectList)[sIndex]);
        }
        LW_SAFE_FREE_MEMORY(*pppObjectList);
    }
}
