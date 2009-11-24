/*
 * Copyright Likewise Software
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
 *
 * Module Name:
 *
 *        api2.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Server API (version 2)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 */

#include "api.h"

typedef struct _LSA_SRV_ENUM_HANDLE
{
    enum
    {
        LSA_SRV_ENUM_OBJECTS,
        LSA_SRV_ENUM_MEMBERS
    } Type;
    LSA_FIND_FLAGS FindFlags;
    LSA_OBJECT_TYPE ObjectType;
    PSTR pszDomainName;
    PSTR pszSid;
    PLSA_AUTH_PROVIDER pProvider;
    HANDLE hProvider;
    HANDLE hEnum;
    BOOLEAN bMergeResults;
    BOOLEAN bReleaseLock;
} LSA_SRV_ENUM_HANDLE, *PLSA_SRV_ENUM_HANDLE;

static
VOID
LsaSrvFreeSecurityObject(
    PLSA_SECURITY_OBJECT pObject
    )
{
    if (pObject)
    {
        LW_SAFE_FREE_STRING(pObject->pszDN);
        LW_SAFE_FREE_STRING(pObject->pszObjectSid);
        LW_SAFE_FREE_STRING(pObject->pszNetbiosDomainName);
        LW_SAFE_FREE_STRING(pObject->pszSamAccountName);

        switch(pObject->type)
        {
        case AccountType_Group:
            LW_SAFE_FREE_STRING(pObject->groupInfo.pszAliasName);
            LW_SAFE_FREE_STRING(pObject->groupInfo.pszPasswd);
            break;
        case AccountType_User:
            LW_SAFE_FREE_STRING(pObject->userInfo.pszUPN);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszAliasName);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszPasswd);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszGecos);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszShell);
            LW_SAFE_FREE_STRING(pObject->userInfo.pszHomedir);
            break;
        }
    }
}

static
void
LsaSrvFreeSecurityObjectList(
    DWORD dwCount,
    PLSA_SECURITY_OBJECT* ppObjectList
    )
{
    DWORD dwIndex = 0;

    if (ppObjectList)
    {
        for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            LsaSrvFreeSecurityObject(ppObjectList[dwIndex]);
        }

        LwFreeMemory(ppObjectList);
    }
}

static
void
LsaSrvFreeSidList(
    DWORD dwCount,
    PSTR* ppszSidList
    )
{
    DWORD dwIndex = 0;

    if (ppszSidList)
    {
        for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
        {
            LW_SAFE_FREE_STRING(ppszSidList[dwIndex]);
        }

        LwFreeMemory(ppszSidList);
    }
}

static
DWORD
LsaSrvConcatenateSidLists(
    IN DWORD dwSidCount,
    IN OUT PSTR** pppszSidList,
    IN DWORD dwAppendCount,
    IN PSTR* ppszAppend
    )
{
    DWORD dwError = 0;
    PSTR* ppszNewList = NULL;

    dwError = LwReallocMemory(*pppszSidList, OUT_PPVOID(&ppszNewList),
                              sizeof(*ppszNewList) * (dwSidCount + dwAppendCount));
    BAIL_ON_LSA_ERROR(dwError);

    memcpy(ppszNewList + dwSidCount, ppszAppend, sizeof(*ppszNewList) * dwAppendCount);

    *pppszSidList = ppszNewList;

error:

    return dwError;
}

static
VOID
LsaSrvConstructPartialQuery(
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    IN PLSA_SECURITY_OBJECT* ppCombinedObjects,
    OUT PDWORD pdwPartialCount,
    OUT LSA_QUERY_LIST PartialQueryList
    )
{
    DWORD dwPartialIndex = 0;
    DWORD dwIndex = 0;

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        if (ppCombinedObjects[dwIndex] == NULL)
        {
            switch (QueryType)
            {
            case LSA_QUERY_TYPE_BY_UID:
            case LSA_QUERY_TYPE_BY_GID:
                PartialQueryList.pdwIds[dwPartialIndex++] = QueryList.pdwIds[dwIndex];
                break;
            default:
                PartialQueryList.ppszStrings[dwPartialIndex++] = QueryList.ppszStrings[dwIndex];
                break;
            }
        }
    }

    *pdwPartialCount = dwPartialIndex;

    return;
}

static
VOID
LsaSrvMergePartialQueryResult(
    IN DWORD dwCount,
    IN PLSA_SECURITY_OBJECT* ppPartialObjects,
    OUT PLSA_SECURITY_OBJECT* ppCombinedObjects
    )
{
    DWORD dwIndex = 0;
    DWORD dwPartialIndex = 0;

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        if (ppCombinedObjects[dwIndex] == NULL)
        {
            ppCombinedObjects[dwIndex] = ppPartialObjects[dwPartialIndex++];
        }
    }
}

DWORD
LsaSrvFindObjects(
    IN HANDLE hServer,
    IN PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    HANDLE hProvider = NULL;
    PLSA_SECURITY_OBJECT* ppCombinedObjects = NULL;
    PLSA_SECURITY_OBJECT* ppPartialObjects = NULL;
    LSA_QUERY_LIST PartialQueryList;
    DWORD dwPartialCount = 0;
    BOOLEAN bFoundProvider = FALSE;

    memset(&PartialQueryList, 0, sizeof(PartialQueryList));

    dwError = LwAllocateMemory(
        sizeof(*ppCombinedObjects) * dwCount,
        OUT_PPVOID(&ppCombinedObjects));
    BAIL_ON_LSA_ERROR(dwError);

    switch (QueryType)
    {
    case LSA_QUERY_TYPE_BY_UID:
    case LSA_QUERY_TYPE_BY_GID:
        dwError = LwAllocateMemory(
            sizeof(*PartialQueryList.pdwIds) * dwCount,
            OUT_PPVOID(&PartialQueryList.pdwIds));
        BAIL_ON_LSA_ERROR(dwError);
        break;
    default:
        dwError = LwAllocateMemory(
            sizeof(*PartialQueryList.ppszStrings) * dwCount,
            OUT_PPVOID(&PartialQueryList.ppszStrings));
        BAIL_ON_LSA_ERROR(dwError);
        break;
    }

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        if (pszTargetProvider)
        {
            if (!strcmp(pszTargetProvider, pProvider->pszName))
            {
                bFoundProvider = TRUE;
            }
            else
            {
                continue;
            }
        }

        if (pProvider->pFnTable2 == NULL)
        {
            continue;
        }

        LsaSrvConstructPartialQuery(
            QueryType,
            dwCount,
            QueryList,
            ppCombinedObjects,
            &dwPartialCount,
            PartialQueryList);

        /* Stop iterating if all keys now have results */
        if (dwPartialCount == 0)
        {
            break;
        }

        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable2->pfnFindObjects(
            hProvider,
            FindFlags,
            ObjectType,
            QueryType,
            dwPartialCount,
            PartialQueryList,
            &ppPartialObjects);

        if (dwError == LW_ERROR_NOT_HANDLED)
        {
            dwError = LW_ERROR_SUCCESS;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);

            LsaSrvMergePartialQueryResult(
                dwCount,
                ppPartialObjects,
                ppCombinedObjects);
        }

        LW_SAFE_FREE_MEMORY(ppPartialObjects);
        LsaSrvCloseProvider(pProvider, hProvider);
        hProvider = NULL;
    }

    if (pszTargetProvider && !bFoundProvider)
    {
        /* FIXME: add better error code for this */
        dwError = LW_ERROR_NO_HANDLER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppObjects = ppCombinedObjects;

cleanup:

    /* All objects inside the partial result list were moved into
       the combined list, so we do a shallow free */
    LW_SAFE_FREE_MEMORY(ppPartialObjects);
    /* Note that this is safe regardless of the type of the query */
    LW_SAFE_FREE_MEMORY(PartialQueryList.ppszStrings);

    if (hProvider != NULL)
    {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    return dwError;

error:

    *pppObjects = NULL;

    if (ppCombinedObjects)
    {
        LsaSrvFreeSecurityObjectList(dwCount, ppCombinedObjects);
    }

    goto cleanup;
}

DWORD
LsaSrvOpenEnumObjects(
    IN HANDLE hServer,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    PLSA_SRV_ENUM_HANDLE pEnum = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;

    dwError = LwAllocateMemory(sizeof(*pEnum), OUT_PPVOID(&pEnum));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pszDomainName, &pEnum->pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    pEnum->Type = LSA_SRV_ENUM_OBJECTS;
    pEnum->FindFlags = FindFlags;
    pEnum->ObjectType = ObjectType;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(pEnum->bReleaseLock);

    if (pszTargetProvider)
    {
        for (pProvider = gpAuthProviderList;
             pProvider;
             pProvider = pProvider->pNext)
        {
            if (!strcmp(pszTargetProvider, pProvider->pszName))
            {
                pEnum->pProvider = pProvider;
                break;
            }
        }

        if (!pEnum->pProvider)
        {
            dwError = LW_ERROR_NO_HANDLER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        pEnum->pProvider = gpAuthProviderList;
        pEnum->bMergeResults = TRUE;
    }

    *phEnum = pEnum;

cleanup:

    return dwError;

error:

    if (pEnum)
    {
        LsaSrvCloseEnum(hServer, pEnum);
    }

    goto cleanup;
}

DWORD
LsaSrvEnumObjects(
    IN HANDLE hServer,
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_SRV_ENUM_HANDLE pEnum = hEnum;
    PLSA_SECURITY_OBJECT* ppCombinedObjects = NULL;
    PLSA_SECURITY_OBJECT* ppPartialObjects = NULL;
    DWORD dwCombinedObjectCount = 0;
    DWORD dwPartialObjectCount = 0;

    if (pEnum->Type != LSA_SRV_ENUM_OBJECTS)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
        sizeof(*ppCombinedObjects) * dwMaxObjectsCount,
        OUT_PPVOID(&ppCombinedObjects));
    BAIL_ON_LSA_ERROR(dwError);

    while (dwCombinedObjectCount < dwMaxObjectsCount && pEnum->pProvider != NULL)
    {
        if (pEnum->pProvider->pFnTable2 == NULL)
        {
            pEnum->pProvider = pEnum->bMergeResults ? pEnum->pProvider->pNext : NULL;
            continue;
        }

        if (!pEnum->hProvider)
        {
            dwError = LsaSrvOpenProvider(hServer, pEnum->pProvider, &pEnum->hProvider);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (!pEnum->hEnum)
        {
            dwError = pEnum->pProvider->pFnTable2->pfnOpenEnumObjects(
                pEnum->hProvider,
                &pEnum->hEnum,
                pEnum->FindFlags,
                pEnum->ObjectType,
                pEnum->pszDomainName);
            if (dwError == LW_ERROR_NOT_HANDLED)
            {
                dwError = LW_ERROR_SUCCESS;
                pEnum->pProvider->pFnTable2->pfnCloseHandle(pEnum->hProvider);
                pEnum->pProvider = pEnum->bMergeResults ? pEnum->pProvider->pNext : NULL;
                continue;
            }
            else
            {
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        dwError = pEnum->pProvider->pFnTable2->pfnEnumObjects(
            pEnum->hEnum,
            dwMaxObjectsCount - dwCombinedObjectCount,
            &dwPartialObjectCount,
            &ppPartialObjects);
        if (dwError == ERROR_NO_MORE_ITEMS)
        {
            dwError = LW_ERROR_SUCCESS;
            pEnum->pProvider->pFnTable2->pfnCloseEnum(pEnum->hEnum);
            pEnum->hEnum = NULL;
            pEnum->pProvider->pFnTable2->pfnCloseHandle(pEnum->hProvider);
            pEnum->hProvider = NULL;
            pEnum->pProvider = pEnum->bMergeResults ? pEnum->pProvider->pNext : NULL;
            continue;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }

        memcpy(ppCombinedObjects + dwCombinedObjectCount,
               ppPartialObjects,
               sizeof(*ppPartialObjects) * dwPartialObjectCount);
        dwCombinedObjectCount += dwPartialObjectCount;

        LW_SAFE_FREE_MEMORY(ppPartialObjects);
    }

    if (dwCombinedObjectCount == 0)
    {
        dwError = ERROR_NO_MORE_ITEMS;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        *pppObjects = ppCombinedObjects;
        *pdwObjectsCount = dwCombinedObjectCount;
    }

cleanup:

    LW_SAFE_FREE_MEMORY(ppPartialObjects);

    return dwError;

error:

    *pdwObjectsCount = 0;
    *pppObjects = NULL;

    if (ppCombinedObjects)
    {
        LsaSrvFreeSecurityObjectList(dwCombinedObjectCount, ppCombinedObjects);
    }

    goto cleanup;
}

DWORD
LsaSrvOpenEnumMembers(
    IN HANDLE hServer,
    IN PCSTR pszTargetProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid
    )
{
   DWORD dwError = 0;
   PLSA_SRV_ENUM_HANDLE pEnum = NULL;
   PLSA_AUTH_PROVIDER pProvider = NULL;

   dwError = LwAllocateMemory(sizeof(*pEnum), OUT_PPVOID(&pEnum));
   BAIL_ON_LSA_ERROR(dwError);

   dwError = LwAllocateString(pszSid, &pEnum->pszSid);
   BAIL_ON_LSA_ERROR(dwError);

   pEnum->Type = LSA_SRV_ENUM_MEMBERS;
   pEnum->FindFlags = FindFlags;

   ENTER_AUTH_PROVIDER_LIST_READER_LOCK(pEnum->bReleaseLock);

    if (pszTargetProvider)
    {
        for (pProvider = gpAuthProviderList;
             pProvider;
             pProvider = pProvider->pNext)
        {
            if (!strcmp(pszTargetProvider, pProvider->pszName))
            {
                pEnum->pProvider = pProvider;
                break;
            }
        }

        if (!pEnum->pProvider)
        {
            dwError = LW_ERROR_NO_HANDLER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        pEnum->pProvider = gpAuthProviderList;
        pEnum->bMergeResults = TRUE;
    }

    *phEnum = pEnum;

cleanup:

    return dwError;

error:

    if (pEnum)
    {
        LsaSrvCloseEnum(hServer, pEnum);
    }

    goto cleanup;
}

DWORD
LsaSrvEnumMembers(
    IN HANDLE hServer,
    IN HANDLE hEnum,
    IN DWORD dwMaxSidCount,
    OUT PDWORD pdwSidCount,
    OUT PSTR** pppszSids
    )
{
    DWORD dwError = 0;
    PLSA_SRV_ENUM_HANDLE pEnum = hEnum;
    PSTR* ppszCombinedSids = NULL;
    PSTR* ppszPartialSids = NULL;
    DWORD dwCombinedSidCount = 0;
    DWORD dwPartialSidCount = 0;

    if (pEnum->Type != LSA_SRV_ENUM_MEMBERS)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
        sizeof(*ppszCombinedSids) * dwMaxSidCount,
        OUT_PPVOID(&ppszCombinedSids));
    BAIL_ON_LSA_ERROR(dwError);

    while (dwCombinedSidCount < dwMaxSidCount && pEnum->pProvider != NULL)
    {
        if (pEnum->pProvider->pFnTable2 == NULL)
        {
            pEnum->pProvider = pEnum->bMergeResults ? pEnum->pProvider->pNext : NULL;
            continue;
        }

        if (!pEnum->hProvider)
        {
            dwError = LsaSrvOpenProvider(hServer, pEnum->pProvider, &pEnum->hProvider);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (!pEnum->hEnum)
        {
            dwError = pEnum->pProvider->pFnTable2->pfnOpenEnumGroupMembers(
                pEnum->hProvider,
                &pEnum->hEnum,
                pEnum->FindFlags,
                pEnum->pszSid);
            if (dwError == LW_ERROR_NOT_HANDLED)
            {
                dwError = LW_ERROR_SUCCESS;
                pEnum->pProvider->pFnTable2->pfnCloseHandle(pEnum->hProvider);
                pEnum->pProvider = pEnum->bMergeResults ? pEnum->pProvider->pNext : NULL;
                continue;
            }
            else
            {
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        dwError = pEnum->pProvider->pFnTable2->pfnEnumGroupMembers(
            pEnum->hEnum,
            dwMaxSidCount - dwCombinedSidCount,
            &dwPartialSidCount,
            &ppszPartialSids);
        if (dwError == ERROR_NO_MORE_ITEMS)
        {
            dwError = LW_ERROR_SUCCESS;
            pEnum->pProvider->pFnTable2->pfnCloseEnum(pEnum->hEnum);
            pEnum->hEnum = NULL;
            pEnum->pProvider->pFnTable2->pfnCloseHandle(pEnum->hProvider);
            pEnum->hProvider = NULL;
            pEnum->pProvider = pEnum->bMergeResults ? pEnum->pProvider->pNext : NULL;
            continue;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);
        }

        memcpy(ppszCombinedSids + dwCombinedSidCount,
               ppszPartialSids,
               sizeof(*ppszPartialSids) * dwPartialSidCount);
        dwCombinedSidCount += dwPartialSidCount;

        LW_SAFE_FREE_MEMORY(ppszPartialSids);
    }

    if (dwCombinedSidCount == 0)
    {
        dwError = ERROR_NO_MORE_ITEMS;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        *pppszSids = ppszCombinedSids;
        *pdwSidCount = dwCombinedSidCount;
    }

cleanup:

    LW_SAFE_FREE_MEMORY(ppszPartialSids);

    return dwError;

error:

    *pdwSidCount = 0;
    *pppszSids = NULL;

    if (ppszCombinedSids)
    {
        LsaSrvFreeSidList(dwCombinedSidCount, ppszCombinedSids);
    }

    goto cleanup;
}

DWORD
LsaSrvQueryMemberOf(
    IN HANDLE hServer,
    IN OPTIONAL PCSTR pszTargetProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN PSTR* ppszSids,
    OUT PDWORD pdwObjectsCount,
    OUT PSTR** pppszGroupSids
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bInLock = FALSE;
    HANDLE hProvider = NULL;
    PSTR* ppszCombinedSids = NULL;
    PSTR* ppszPartialSids = NULL;
    DWORD dwCombinedCount = 0;
    DWORD dwPartialCount = 0;
    BOOLEAN bFoundProvider = FALSE;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        if (pszTargetProvider)
        {
            if (!strcmp(pszTargetProvider, pProvider->pszName))
            {
                bFoundProvider = TRUE;
            }
            else
            {
                continue;
            }
        }

        if (pProvider->pFnTable2 == NULL)
        {
            continue;
        }

        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable2->pfnQueryMemberOf(
            hProvider,
            FindFlags,
            ppszSids,
            &dwPartialCount,
            &ppszPartialSids);
        if (dwError == LW_ERROR_NOT_HANDLED)
        {
            dwError = LW_ERROR_SUCCESS;
        }
        else
        {
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaSrvConcatenateSidLists(
                dwCombinedCount,
                &ppszCombinedSids,
                dwPartialCount,
                ppszPartialSids);
            BAIL_ON_LSA_ERROR(dwError);

            dwCombinedCount += dwPartialCount;
        }

        LW_SAFE_FREE_MEMORY(ppszPartialSids);
        LsaSrvCloseProvider(pProvider, hProvider);
        hProvider = NULL;
    }

    if (pszTargetProvider && !bFoundProvider)
    {
        dwError = LW_ERROR_NO_HANDLER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppszGroupSids = ppszCombinedSids;
    *pdwObjectsCount = dwCombinedCount;

cleanup:

    LW_SAFE_FREE_MEMORY(ppszPartialSids);

    if (hProvider != NULL)
    {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    return dwError;

error:

    *pppszGroupSids = NULL;
    *pdwObjectsCount = 0;

    if (ppszCombinedSids)
    {
        LsaSrvFreeSidList(dwCombinedCount, ppszCombinedSids);
    }

    goto cleanup;
}

VOID
LsaSrvCloseEnum(
    IN HANDLE hServer,
    IN OUT HANDLE hEnum
    )
{
    PLSA_SRV_ENUM_HANDLE pEnum = hEnum;

    if (pEnum)
    {
        if (pEnum->hEnum)
        {
            pEnum->pProvider->pFnTable2->pfnCloseEnum(pEnum->hEnum);
        }

        if (pEnum->hProvider)
        {
            pEnum->pProvider->pFnTable2->pfnCloseHandle(pEnum->hProvider);
        }

        LW_SAFE_FREE_STRING(pEnum->pszDomainName);
        LW_SAFE_FREE_STRING(pEnum->pszSid);
        LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(pEnum->bReleaseLock);
        LwFreeMemory(pEnum);
    }
}
