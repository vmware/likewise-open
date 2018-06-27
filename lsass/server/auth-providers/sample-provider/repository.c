/*
 * Copyright (C) VMware. All rights reserved.
 */

#include "includes.h"

static
PSAMPLE_USER_INFO*
SampleRepositoryFindUserByName_inlock(
	PCSTR pszAcctName
	);

static
DWORD
SampleMarshalUserInfo(
    PSAMPLE_USER_INFO     pUser,
    PCSTR                pszDomain,
    PLSA_SECURITY_OBJECT pObject
    );

static
DWORD
SampleMarshalGroupInfo(
    PSAMPLE_GROUP_INFO    pGroup,
    PCSTR                pszDomain,
    PLSA_SECURITY_OBJECT pObject
    );

static
DWORD
SampleFindGroupSID_inlock(
    gid_t gid,
    PSTR* ppszSID
    );

static
DWORD
SampleFindUserCount_inlock(
    PLONG64 pllCount
    );

static
DWORD
SampleFindGroupCount_inlock(
    PLONG64 pllCount
    );

DWORD
SampleCrackLoginId(
    PCSTR pszLoginId,
    PSTR* ppszDomain,
    PSTR* ppszAccount
    )
{
    DWORD dwError    = 0;
    PCSTR pszCursor  = pszLoginId;
    PSTR  pszDomain  = NULL;
    PSTR  pszAccount = NULL;

    while (*pszCursor && (*pszCursor != '\\') && (*pszCursor != '@'))
    {
        pszCursor++;
    }

    if (*pszCursor == '\\')
    {
        // NT4 Style => Domain\\Account

        size_t len = pszCursor - pszLoginId;

        dwError = LwAllocateMemory(len+1, (PVOID*)&pszDomain);
        BAIL_ON_SAMPLE_ERROR(dwError);

        memcpy(pszDomain, pszLoginId, len);

        if (++pszCursor && *pszCursor)
        {
            dwError = LwAllocateString(pszCursor, &pszAccount);
            BAIL_ON_SAMPLE_ERROR(dwError);
        }
    }
    else if (*pszCursor == '@')
    {
        // UPN => Account@Domain

        size_t len = pszCursor - pszLoginId;

        dwError = LwAllocateMemory(len+1, (PVOID*)&pszAccount);
        BAIL_ON_SAMPLE_ERROR(dwError);

        memcpy(pszAccount, pszLoginId, len);

        if (++pszCursor && *pszCursor)
        {
            dwError = LwAllocateString(pszCursor, &pszDomain);
            BAIL_ON_SAMPLE_ERROR(dwError);
        }
    }
    else
    {
        // Account

        dwError = LwAllocateString(pszLoginId, &pszAccount);
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    *ppszDomain = pszDomain;
    *ppszAccount = pszAccount;

cleanup:

    return dwError;

error:

    *ppszDomain = NULL;
    *ppszAccount = NULL;

    LW_SAFE_FREE_MEMORY(pszDomain);
    LW_SAFE_FREE_MEMORY(pszAccount);

    goto cleanup;
}

BOOLEAN
SampleMatchesDomain(
    PCSTR pszDomain
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bIsMatch = FALSE;
    BOOLEAN bInLock = FALSE;

    dwError = SampleRWLockAcquire(
                    &gSampleAuthProviderGlobals.mutex_rw,
                    FALSE,
                    &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    if (!strcasecmp(pszDomain, gSampleAuthProviderGlobals.pszDomain))
    {
        bIsMatch = TRUE;
    }

cleanup:

    if (bInLock)
    {
        SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
    }

    return bIsMatch;

error:

    // bIsMatch = FALSE;

    goto cleanup;
}

DWORD
SampleGetDomain(
    PSTR* ppszDomain
    )
{
    DWORD   dwError   = LW_ERROR_SUCCESS;
    BOOLEAN bInLock   = FALSE;
    PSTR    pszDomain = NULL;

    dwError = SampleRWLockAcquire(
                        &gSampleAuthProviderGlobals.mutex_rw,
                        FALSE,
                        &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = LwAllocateString(
                        gSampleAuthProviderGlobals.pszDomain,
                        &pszDomain);
    BAIL_ON_SAMPLE_ERROR(dwError);

    *ppszDomain = pszDomain;

cleanup:

    if (bInLock)
    {
        SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
    }

    return dwError;

error:

    *ppszDomain = NULL;

    goto cleanup;
}

DWORD
SampleFindUserByName(
    PCSTR                 pszLoginId,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PSTR  pszDomain  = NULL;
    PSTR  pszAccount = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PSAMPLE_USER_INFO* ppUser = NULL;

    dwError = SampleCrackLoginId(pszLoginId, &pszDomain, &pszAccount);
    BAIL_ON_SAMPLE_ERROR(dwError);

    if (pszDomain && !SampleMatchesDomain(pszDomain))
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    if (!pszDomain)
    {
        dwError = SampleGetDomain(&pszDomain);
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = SampleRWLockAcquire(
                    &gSampleAuthProviderGlobals.mutex_rw,
                    FALSE,
                    &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    ppUser = SampleRepositoryFindUserByName_inlock(pszAccount);

    if (!ppUser || !*ppUser)
    {
    	dwError = LW_ERROR_NO_SUCH_USER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

	dwError = LwAllocateMemory(sizeof(*pObject), (PVOID*)&pObject);
	BAIL_ON_SAMPLE_ERROR(dwError);

	dwError = SampleMarshalUserInfo(*ppUser, pszDomain, pObject);
	BAIL_ON_SAMPLE_ERROR(dwError);

    *ppObject = pObject;

cleanup:

    if (bInLock)
    {
        SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
    }

    LW_SAFE_FREE_MEMORY(pszDomain);
    LW_SAFE_FREE_MEMORY(pszAccount);

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    goto cleanup;
}

DWORD
SampleFindUserById(
    uid_t                 uid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PSTR  pszDomain  = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PSAMPLE_USER_INFO* ppUser = NULL;

    dwError = SampleGetDomain(&pszDomain);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = SampleRWLockAcquire(
                    &gSampleAuthProviderGlobals.mutex_rw,
                    FALSE,
                    &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    for (ppUser = gSampleAuthProviderGlobals.ppUsers; ppUser && *ppUser; ppUser++)
    {
        if ((*ppUser)->uid == uid)
        {
            break;
        }
    }

    if (!ppUser || !*ppUser)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

	dwError = LwAllocateMemory(sizeof(*pObject), (PVOID*)&pObject);
	BAIL_ON_SAMPLE_ERROR(dwError);

	dwError = SampleMarshalUserInfo(*ppUser, pszDomain, pObject);
	BAIL_ON_SAMPLE_ERROR(dwError);

    *ppObject = pObject;

cleanup:

    if (bInLock)
    {
        SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
    }

    LW_SAFE_FREE_MEMORY(pszDomain);

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    goto cleanup;
}

DWORD
SampleFindGroupByName(
    PCSTR                 pszGroupName,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PSTR  pszDomain  = NULL;
    PSTR  pszAccount = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PSAMPLE_GROUP_INFO* ppGroup = NULL;

    dwError = SampleCrackLoginId(pszGroupName, &pszDomain, &pszAccount);
    BAIL_ON_SAMPLE_ERROR(dwError);

    if (pszDomain && !SampleMatchesDomain(pszDomain))
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    if (!pszDomain)
    {
        dwError = SampleGetDomain(&pszDomain);
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = SampleRWLockAcquire(
                    &gSampleAuthProviderGlobals.mutex_rw,
                    FALSE,
                    &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    for (ppGroup = gSampleAuthProviderGlobals.ppGroups; ppGroup && *ppGroup; ppGroup++)
    {
        if (!strcmp((*ppGroup)->pszName, pszAccount))
        {
            break;
        }
    }

    if (!ppGroup || !*ppGroup)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

	dwError = LwAllocateMemory(sizeof(*pObject), (PVOID*)&pObject);
	BAIL_ON_SAMPLE_ERROR(dwError);

	dwError = SampleMarshalGroupInfo(*ppGroup, pszDomain, pObject);
	BAIL_ON_SAMPLE_ERROR(dwError);

    *ppObject = pObject;

cleanup:

    if (bInLock)
    {
        SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
    }

    LW_SAFE_FREE_MEMORY(pszDomain);
    LW_SAFE_FREE_MEMORY(pszAccount);

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    goto cleanup;
}

DWORD
SampleFindGroupById(
    gid_t                 gid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PSTR  pszDomain  = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PSAMPLE_GROUP_INFO* ppGroup = NULL;

    dwError = SampleGetDomain(&pszDomain);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = SampleRWLockAcquire(
                    &gSampleAuthProviderGlobals.mutex_rw,
                    FALSE,
                    &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    for (ppGroup = gSampleAuthProviderGlobals.ppGroups; ppGroup && *ppGroup; ppGroup++)
    {
        if ((*ppGroup)->gid == gid)
        {
            break;
        }
    }

    if (!ppGroup || !*ppGroup)
    {
    	dwError = LW_ERROR_NO_SUCH_GROUP;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

	dwError = LwAllocateMemory(sizeof(*pObject), (PVOID*)&pObject);
	BAIL_ON_SAMPLE_ERROR(dwError);

	dwError = SampleMarshalGroupInfo(*ppGroup, pszDomain, pObject);
	BAIL_ON_SAMPLE_ERROR(dwError);

    *ppObject = pObject;

cleanup:

    if (bInLock)
    {
        SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
    }

    LW_SAFE_FREE_MEMORY(pszDomain);

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    goto cleanup;
}

DWORD
SampleFindObjectBySID(
    PCSTR                 pszSID,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PSTR  pszDomain  = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PSAMPLE_USER_INFO*  ppUser = NULL;

    dwError = SampleGetDomain(&pszDomain);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = SampleRWLockAcquire(
                    &gSampleAuthProviderGlobals.mutex_rw,
                    FALSE,
                    &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    for (ppUser = gSampleAuthProviderGlobals.ppUsers; ppUser && *ppUser; ppUser++)
    {
        if (!strcasecmp((*ppUser)->pszSID, pszSID))
        {
            break;
        }
    }

    if (ppUser && *ppUser)
    {
        dwError = LwAllocateMemory(sizeof(*pObject), (PVOID*)&pObject);
        BAIL_ON_SAMPLE_ERROR(dwError);

        dwError = SampleMarshalUserInfo(*ppUser, pszDomain, pObject);
        BAIL_ON_SAMPLE_ERROR(dwError);
    }
    else
    {
        PSAMPLE_GROUP_INFO* ppGroup = NULL;

        for (ppGroup = gSampleAuthProviderGlobals.ppGroups; ppGroup && *ppGroup; ppGroup++)
        {
            if (!strcasecmp((*ppGroup)->pszSID, pszSID))
            {
                break;
            }
        }

        if (ppGroup && *ppGroup)
        {
            dwError = LwAllocateMemory(sizeof(*pObject), (PVOID*)&pObject);
            BAIL_ON_SAMPLE_ERROR(dwError);

            dwError = SampleMarshalGroupInfo(*ppGroup, pszDomain, pObject);
            BAIL_ON_SAMPLE_ERROR(dwError);
        }
        else
        {
            dwError = LW_ERROR_NO_SUCH_OBJECT;
            BAIL_ON_SAMPLE_ERROR(dwError);
        }
    }

    *ppObject = pObject;

cleanup:

    if (bInLock)
    {
        SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
    }

    LW_SAFE_FREE_MEMORY(pszDomain);

    return dwError;

error:

    *ppObject = NULL;

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    goto cleanup;
}

DWORD
SampleInitEnumHandle(
    PSAMPLE_ENUM_HANDLE pEnumHandle
    )
{
    DWORD   dwError = LW_ERROR_SUCCESS;

    switch (pEnumHandle->type)
    {
        case LSA_OBJECT_TYPE_USER:

            dwError = SampleFindUserCount_inlock(&pEnumHandle->llTotalCount);

            break;

        case LSA_OBJECT_TYPE_GROUP:

            dwError = SampleFindGroupCount_inlock(&pEnumHandle->llTotalCount);

            break;

        default:

            dwError = ERROR_INVALID_STATE;

            break;
    }
    BAIL_ON_SAMPLE_ERROR(dwError);

    pEnumHandle->llNextIndex  = 0;

error:

    return dwError;
}

DWORD
SampleRepositoryEnumObjects(
    PSAMPLE_ENUM_HANDLE     pEnumHandle,
    PLSA_SECURITY_OBJECT** pppObjects,
    DWORD                  dwCount
    )
{
    DWORD   dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszDomain = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    LONG64 llTotalCount = 0;

    dwError = SampleGetDomain(&pszDomain);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(PLSA_SECURITY_OBJECT) * dwCount,
                    (PVOID*)&ppObjects);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = SampleRWLockAcquire(
                    &gSampleAuthProviderGlobals.mutex_rw,
                    FALSE,
                    &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    switch (pEnumHandle->type)
    {
        case LSA_OBJECT_TYPE_USER:

            dwError = SampleFindUserCount_inlock(&llTotalCount);
            BAIL_ON_SAMPLE_ERROR(dwError);

            if ((pEnumHandle->llNextIndex + dwCount) > llTotalCount)
            {
                dwError = ERROR_INVALID_DATA;
                BAIL_ON_SAMPLE_ERROR(dwError);
            }
            else
            {
                PSAMPLE_USER_INFO* ppUser = NULL;
                DWORD  iObject = 0;

                for (    ppUser = &gSampleAuthProviderGlobals.ppUsers[pEnumHandle->llNextIndex];
                        ppUser && *ppUser && (iObject < dwCount);
                        ppUser++ )
                {
                    dwError = LwAllocateMemory(
                                    sizeof(LSA_SECURITY_OBJECT),
                                    (PVOID*)&ppObjects[iObject]);
                    BAIL_ON_SAMPLE_ERROR(dwError);

                    dwError = SampleMarshalUserInfo(
                                    *ppUser,
                                    pszDomain,
                                    ppObjects[iObject++]);
                    BAIL_ON_SAMPLE_ERROR(dwError);
                }
            }

            break;

        case LSA_OBJECT_TYPE_GROUP:

            dwError = SampleFindGroupCount_inlock(&llTotalCount);
            BAIL_ON_SAMPLE_ERROR(dwError);

            if ((pEnumHandle->llNextIndex + dwCount) > llTotalCount)
            {
                dwError = ERROR_INVALID_DATA;
                BAIL_ON_SAMPLE_ERROR(dwError);
            }
            else
            {
                PSAMPLE_GROUP_INFO* ppGroup = NULL;
                DWORD  iObject = 0;

                for (    ppGroup = &gSampleAuthProviderGlobals.ppGroups[pEnumHandle->llNextIndex];
                        ppGroup && *ppGroup && (iObject < dwCount);
                        ppGroup++ )
                {
                    dwError = LwAllocateMemory(
                                    sizeof(LSA_SECURITY_OBJECT),
                                    (PVOID*)&ppObjects[iObject]);
                    BAIL_ON_SAMPLE_ERROR(dwError);

                    dwError = SampleMarshalGroupInfo(
                                    *ppGroup,
                                    pszDomain,
                                    ppObjects[iObject++]);
                    BAIL_ON_SAMPLE_ERROR(dwError);
                }
            }

            break;

        default:

            dwError = ERROR_INVALID_STATE;
            BAIL_ON_SAMPLE_ERROR(dwError);

            break;
    }

    *pppObjects = ppObjects;

cleanup:

    if (bInLock)
    {
        SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
    }

    LW_SAFE_FREE_MEMORY(pszDomain);

    return dwError;

error:

    *pppObjects = NULL;

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwCount, ppObjects);
    }

    goto cleanup;
}

DWORD
SampleInitEnumMembersHandle(
    PLSA_SECURITY_OBJECT pObject,
    PSAMPLE_ENUM_HANDLE   pEnumHandle
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PSAMPLE_GROUP_INFO* ppGroup = NULL;
    PSTR* ppszMember = NULL;

    dwError = SampleRWLockAcquire(
                    &gSampleAuthProviderGlobals.mutex_rw,
                    FALSE,
                    &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    for (ppGroup = gSampleAuthProviderGlobals.ppGroups; ppGroup && *ppGroup; ppGroup++)
    {
        if ((*ppGroup)->gid == pObject->groupInfo.gid)
        {
            break;
        }
    }

    if (!ppGroup || !*ppGroup)
    {
        dwError = LW_ERROR_NO_SUCH_OBJECT;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    pEnumHandle->bEnumMembers = TRUE;

    pEnumHandle->llNextIndex = 0;
    pEnumHandle->llTotalCount = 0;

    for (ppszMember = (*ppGroup)->ppszMembers; ppszMember && *ppszMember; ppszMember++)
    {
        pEnumHandle->llTotalCount++;
    }

cleanup:

    if (bInLock)
    {
        SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
SampleRepositoryEnumMembers(
    PSAMPLE_ENUM_HANDLE pEnumHandle,
    PSTR**             pppszMemberSids,
    DWORD              dwCount
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSAMPLE_GROUP_INFO* ppGroup = NULL;
    LONG64  llTotalCount = 0;
    PSTR*   ppszMember = NULL;
    PSTR*   ppszMemberSids = NULL;
    DWORD   iMember = 0;

    dwError = LwAllocateMemory(sizeof(PSTR*) * dwCount, (PVOID*)&ppszMemberSids);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = SampleRWLockAcquire(
                    &gSampleAuthProviderGlobals.mutex_rw,
                    FALSE,
                    &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    for (ppGroup = gSampleAuthProviderGlobals.ppGroups; ppGroup && *ppGroup; ppGroup++)
    {
        if (!strcasecmp((*ppGroup)->pszSID, pEnumHandle->pszSID))
        {
            break;
        }
    }

    if (!ppGroup || !*ppGroup)
    {
        dwError = ERROR_INVALID_STATE;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    for (ppszMember = (*ppGroup)->ppszMembers; ppszMember && *ppszMember; ppszMember++)
    {
        llTotalCount++;
    }

    if ((pEnumHandle->llNextIndex + dwCount) > llTotalCount)
    {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    for (    ppszMember = &(*ppGroup)->ppszMembers[pEnumHandle->llNextIndex];
            ppszMember && *ppszMember && (iMember < dwCount);
            ppszMember++ )
    {
        dwError = LwAllocateString(*ppszMember, &ppszMemberSids[iMember++]);
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    *pppszMemberSids = ppszMemberSids;

cleanup:

    if (bInLock)
    {
        SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
    }

    return dwError;

error:

    *pppszMemberSids = NULL;

    if (ppszMemberSids)
    {
        LwFreeStringArray(ppszMemberSids, dwCount);
    }

    goto cleanup;
}

DWORD
SampleFindMemberships(
    PCSTR          pszSid,
    PLW_HASH_TABLE pGroupSidTable
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PSAMPLE_GROUP_INFO* ppGroup = NULL;
    BOOLEAN bInLock = FALSE;
    PSTR pszGroupSID = NULL;

    dwError = SampleFindObjectBySID(pszSid, &pObject);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = SampleRWLockAcquire(
                    &gSampleAuthProviderGlobals.mutex_rw,
                    FALSE,
                    &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    for (    ppGroup = gSampleAuthProviderGlobals.ppGroups;
            ppGroup && *ppGroup;
            ppGroup++ )
    {
        PSTR* ppszMember = NULL;

        for (    ppszMember = (*ppGroup)->ppszMembers;
                ppszMember && *ppszMember;
                ppszMember++ )
        {
            if (!strcmp(pObject->pszSamAccountName, *ppszMember))
            {
                PSTR pszExistingSID = NULL;

                LW_SAFE_FREE_MEMORY(pszGroupSID);

                dwError = LwAllocateString((*ppGroup)->pszSID, &pszGroupSID);
                BAIL_ON_SAMPLE_ERROR(dwError);

                dwError = LwHashGetValue(
                                pGroupSidTable,
                                pszGroupSID,
                                (PVOID*)&pszExistingSID);

                if (dwError == ERROR_NOT_FOUND)
                {
                    dwError = LwHashSetValue(
                                pGroupSidTable,
                                pszGroupSID,
                                pszGroupSID);
                    BAIL_ON_SAMPLE_ERROR(dwError);

                    pszGroupSID = NULL;
                }

                break;
            }
        }
    }

cleanup:

    if (bInLock)
    {
        SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
    }

    LW_SAFE_FREE_MEMORY(pszGroupSID);

    return dwError;

error:

    goto cleanup;
}

DWORD
SampleRepositoryVerifyPassword(
	PCSTR pszUsername,
	PCSTR pszPassword
	)
{
	DWORD dwError = 0;
	BOOLEAN bInLock = FALSE;
	PSTR  pszPassword_local = NULL;

    dwError = SampleRWLockAcquire(
                    &gSampleAuthProviderGlobals.mutex_rw,
                    FALSE,
                    &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = LwHashGetValue(
    					gSampleAuthProviderGlobals.pPasswordTable,
    					pszUsername,
    					(PVOID*)&pszPassword_local);
    if (dwError == ERROR_NOT_FOUND)
    {
    	dwError = ERROR_NO_SUCH_USER;
    }
    BAIL_ON_SAMPLE_ERROR(dwError);

    if (0 != strcmp(pszPassword_local, pszPassword))
    {
    	dwError = LW_ERROR_PASSWORD_MISMATCH;
    }
    BAIL_ON_SAMPLE_ERROR(dwError);

cleanup:

	if (bInLock)
	{
		SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
	}

	return dwError;

error:

	goto cleanup;
}

DWORD
SampleRepositoryChangePassword(
    PCSTR pszAcctName,
	PCSTR pszNewPassword,
	PCSTR pszOldPassword
	)
{
	DWORD dwError = 0;
	BOOLEAN bInLock = FALSE;
	PSTR  pszPassword_local = NULL;
	PSTR  pszNewPassword_local = NULL;
	PSAMPLE_USER_INFO* ppUser = NULL;

    dwError = SampleRWLockAcquire(
                    &gSampleAuthProviderGlobals.mutex_rw,
                    TRUE, /* exclusive */
                    &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    // Use the name from this structure to set as the key in the hash table
    // Because this will stay in global scope in this sample provider
    ppUser = SampleRepositoryFindUserByName_inlock(pszAcctName);
    if (!ppUser || !*ppUser)
    {
    	dwError = ERROR_NO_SUCH_USER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = LwHashGetValue(
    					gSampleAuthProviderGlobals.pPasswordTable,
    					pszAcctName,
    					(PVOID*)&pszPassword_local);
    if (dwError == ERROR_NOT_FOUND)
    {
    	dwError = ERROR_NO_SUCH_USER;
    }
    BAIL_ON_SAMPLE_ERROR(dwError);

    if (0 != strcmp(pszPassword_local, pszOldPassword))
    {
    	dwError = LW_ERROR_PASSWORD_MISMATCH;
    }
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = LwAllocateString(
    				pszNewPassword ? pszNewPassword : "",
    				&pszNewPassword_local);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = LwHashSetValue(
    				gSampleAuthProviderGlobals.pPasswordTable,
    		    	(PVOID)(*ppUser)->pszName,
    		    	pszNewPassword_local);
    BAIL_ON_SAMPLE_ERROR(dwError);

    pszNewPassword_local = NULL;

    LwFreeMemory(pszPassword_local);

cleanup:

	if (bInLock)
	{
		SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
	}

	LW_SAFE_FREE_STRING(pszNewPassword_local);

	return dwError;

error:

	goto cleanup;
}

static
PSAMPLE_USER_INFO*
SampleRepositoryFindUserByName_inlock(
	PCSTR pszAcctName
	)
{
	PSAMPLE_USER_INFO* ppUser = NULL;

	for (	ppUser = gSampleAuthProviderGlobals.ppUsers;
			ppUser && *ppUser;
			ppUser++ )
	{
		if (!strcmp((*ppUser)->pszName, pszAcctName))
		{
			break;
		}
	}

	return ppUser;
}

static
DWORD
SampleMarshalUserInfo(
    PSAMPLE_USER_INFO     pUser,
    PCSTR                pszDomain,
    PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = 0;

    pObject->type = LSA_OBJECT_TYPE_USER;

    pObject->version.qwDbId = -1;
    pObject->version.fWeight = 0;
    pObject->version.dwObjectSize = 0;
    pObject->version.tLastUpdated = 0;

    dwError = LwAllocateString(
                    pUser->pszName,
                    &pObject->pszSamAccountName);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
    				&pObject->userInfo.pszUnixName,
                    "%s\\%s",
                    pszDomain,
                    pUser->pszName);
    BAIL_ON_SAMPLE_ERROR(dwError);

    pObject->userInfo.pszPasswd = NULL; // Never give out the password

    if (pUser->pszUPN)
    {
        dwError = LwAllocateString(
                        pUser->pszUPN,
                        &pObject->userInfo.pszUPN);
        BAIL_ON_SAMPLE_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateStringPrintf(
                        &pObject->userInfo.pszUPN,
                        "%s@%s",
                        pUser->pszName,
                        pszDomain);
        BAIL_ON_SAMPLE_ERROR(dwError);

        pObject->userInfo.bIsGeneratedUPN = TRUE;
    }

    dwError = LwAllocateString(
                        pUser->pszShell,
                        &pObject->userInfo.pszShell);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = LwAllocateString(
                        pUser->pszHomedir,
                        &pObject->userInfo.pszHomedir);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = SampleFindGroupSID_inlock(
                        pUser->gid,
                        &pObject->userInfo.pszPrimaryGroupSid);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = LwAllocateString(
                        pUser->pszSID,
                        &pObject->pszObjectSid);
    BAIL_ON_SAMPLE_ERROR(dwError);

    if (pUser->pszGecos)
    {
        dwError = LwAllocateString(
                        pUser->pszGecos,
                        &pObject->userInfo.pszGecos);
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    pObject->userInfo.uid = pUser->uid;
    pObject->userInfo.gid = pUser->gid;
    pObject->userInfo.bAccountDisabled = pUser->bAccountDisabled;
    pObject->userInfo.bAccountExpired  = pUser->bAccountExpired;
    pObject->userInfo.bAccountLocked   = pUser->bAccountLocked;
    pObject->userInfo.bPasswordExpired = pUser->bPasswordExpired;
    pObject->userInfo.bUserCanChangePassword = pUser->bUserCanChangePassword;

    pObject->enabled = TRUE;

error:

    return dwError;
}

static
DWORD
SampleMarshalGroupInfo(
    PSAMPLE_GROUP_INFO    pGroup,
    PCSTR                pszDomain,
    PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    pObject->type = LSA_OBJECT_TYPE_GROUP;

    pObject->version.qwDbId = -1;
    pObject->version.fWeight = 0;
    pObject->version.dwObjectSize = 0;
    pObject->version.tLastUpdated = 0;

    dwError = LwAllocateString(
                    pGroup->pszName,
                    &pObject->pszSamAccountName);
    BAIL_ON_SAMPLE_ERROR(dwError);

    pObject->groupInfo.gid = pGroup->gid;

    dwError = LwAllocateString(
                    pGroup->pszName,
                    &pObject->groupInfo.pszUnixName);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = LwAllocateString(
                        pGroup->pszSID,
                        &pObject->pszObjectSid);
    BAIL_ON_SAMPLE_ERROR(dwError);

    pObject->enabled = TRUE;

error:

    return dwError;
}

static
DWORD
SampleFindGroupSID_inlock(
    gid_t gid,
    PSTR* ppszSID
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSTR  pszSID = NULL;
    PSAMPLE_GROUP_INFO* ppGroup = NULL;

    for (    ppGroup = gSampleAuthProviderGlobals.ppGroups;
            ppGroup && *ppGroup && ((*ppGroup)->gid != gid);
            ppGroup++);

    if (!ppGroup || !*ppGroup)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    if (!(*ppGroup)->pszSID)
    {
        dwError = ERROR_INVALID_STATE;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = LwAllocateString((*ppGroup)->pszSID, &pszSID);
    BAIL_ON_SAMPLE_ERROR(dwError);

    *ppszSID = pszSID;

cleanup:

    return dwError;

error:

    *ppszSID = NULL;

    goto cleanup;
}

static
DWORD
SampleFindUserCount_inlock(
    PLONG64 pllCount
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSAMPLE_USER_INFO* ppUser = NULL;
    LONG64 llCount = 0;

    dwError = SampleRWLockAcquire(
                    &gSampleAuthProviderGlobals.mutex_rw,
                    FALSE,
                    &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    for (    ppUser = gSampleAuthProviderGlobals.ppUsers;
            ppUser && *ppUser;
            ppUser++ )
    {
        llCount++;
    }

    *pllCount = llCount;

cleanup:

    if (bInLock)
    {
        SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
    }

    return dwError;

error:

    *pllCount = 0;

    goto cleanup;
}

static
DWORD
SampleFindGroupCount_inlock(
    PLONG64 pllCount
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSAMPLE_GROUP_INFO* ppGroup = NULL;
    LONG64 llCount = 0;

    dwError = SampleRWLockAcquire(
                    &gSampleAuthProviderGlobals.mutex_rw,
                    FALSE,
                    &bInLock);
    BAIL_ON_SAMPLE_ERROR(dwError);

    for (    ppGroup = gSampleAuthProviderGlobals.ppGroups;
            ppGroup && *ppGroup;
            ppGroup++ )
    {
        llCount++;
    }

    *pllCount = llCount;

cleanup:

    if (bInLock)
    {
        SampleRWLockRelease(&gSampleAuthProviderGlobals.mutex_rw, &bInLock);
    }

    return dwError;

error:

    *pllCount = 0;

    goto cleanup;
}
