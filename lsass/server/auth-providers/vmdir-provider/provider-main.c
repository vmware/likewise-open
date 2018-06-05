/*
 * Copyright (C) VMware. All rights reserved.
 */

#include "includes.h"

static
DWORD
VmDirGetJoinState(
    VOID
    );

static
VOID
VmDirSetJoinState(
    VMDIR_JOIN_STATE joinState
    );

static
BOOLEAN
isValidProviderContext(
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext
    );

static
VOID
InitVmDirCacheFunctionTable(
    PVMCACHE_PROVIDER_FUNCTION_TABLE pVmDirCacheProviderTable
    );

/**
 * Entry point to initialize the authentication provider
 *
 * @param[out] ppszProviderName Name tag of this authentication provider
 * @param[out] ppFunctionTable  Function table containing various entry points
 *                              in this authentication provider
 *
 * @return ERROR_SUCCESS on successful execution
 */
DWORD
LsaInitializeProvider(
    PCSTR*                        ppszProviderName, /* OUT */
    PLSA_PROVIDER_FUNCTION_TABLE* ppFunctionTable   /* OUT */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVMDIR_BIND_INFO pBindInfo = NULL;
    PCSTR pszDbPath = NULL;
    PLSA_VMDIR_PROVIDER_STATE pState = NULL;

    LOG_FUNC_ENTER;

    if (!ppszProviderName || !ppFunctionTable)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    pthread_rwlock_init(&gVmDirAuthProviderGlobals.mutex_rw, NULL);
    gVmDirAuthProviderGlobals.pMutex_rw = &gVmDirAuthProviderGlobals.mutex_rw;

    pthread_mutex_init(&gVmDirAuthProviderGlobals.mutex, NULL);
    gVmDirAuthProviderGlobals.pMutex = &gVmDirAuthProviderGlobals.mutex;
    
    dwError = VmDirGetBindProtocol(&gVmDirAuthProviderGlobals.bindProtocol);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirGetCacheEntryExpiry(&gVmDirAuthProviderGlobals.dwCacheEntryExpiry);
    BAIL_ON_VMDIR_ERROR(dwError);

    gVmDirAuthProviderGlobals.joinState = VMDIR_JOIN_STATE_UNSET;
    dwError = VmDirGetBindInfo(&pBindInfo);
    if (dwError == 0)
    {
        gVmDirAuthProviderGlobals.joinState = VMDIR_JOIN_STATE_JOINED;
    }
    else
    {
        gVmDirAuthProviderGlobals.joinState = VMDIR_JOIN_STATE_NOT_JOINED;
        dwError = 0;
    }

    if (gVmDirAuthProviderGlobals.bindProtocol == VMDIR_BIND_PROTOCOL_KERBEROS)
    {
        dwError = VmDirStartMachineAccountRefresh(
                          &gVmDirAuthProviderGlobals.pRefreshContext);
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    InitVmDirCacheFunctionTable(gpCacheProvider);

    dwError = VMCacheOpen(
                  pszDbPath,
                  pState,
                  &gVmDirAuthProviderGlobals.hDb);
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppszProviderName = gpszVmDirProviderName;
    *ppFunctionTable = &gVmDirProviderAPITable;

cleanup:

    if (pBindInfo)
    {
        VmDirReleaseBindInfo(pBindInfo);
    }

    LOG_FUNC_EXIT;

    return dwError;

error:

    if (ppszProviderName)
    {
        *ppszProviderName = NULL;
    }

    if (ppFunctionTable)
    {
        *ppFunctionTable = NULL;
    }

    goto cleanup;
}

DWORD
VmDirFindObjects(
    HANDLE                 hProvider,  /* IN              */
    LSA_FIND_FLAGS         findFlags,  /* IN              */
    LSA_OBJECT_TYPE        objectType, /* IN     OPTIONAL */
    LSA_QUERY_TYPE         queryType,  /* IN              */
    DWORD                  dwCount,    /* IN              */
    LSA_QUERY_LIST         queryList,  /* IN              */
    PLSA_SECURITY_OBJECT** pppObjects  /*    OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD i = 0;

    LOG_FUNC_ENTER;

    if (!hProvider || !pppObjects || !dwCount)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT)hProvider;
    if (!isValidProviderContext(pContext))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                    sizeof(PLSA_SECURITY_OBJECT) * dwCount,
                    (PVOID*)&ppObjects);
    BAIL_ON_VMDIR_ERROR(dwError);

    for (; i < dwCount ; i++)
    {
        DWORD dwError2 = 0;
        PCSTR pszKey = NULL;

        switch (queryType)
        {
            case LSA_QUERY_TYPE_BY_NT4:
            case LSA_QUERY_TYPE_BY_UPN:
            case LSA_QUERY_TYPE_BY_ALIAS:
            case LSA_QUERY_TYPE_BY_NAME:

                pszKey = queryList.ppszStrings[i];

                if (objectType == LSA_OBJECT_TYPE_USER)
                {
                    dwError2 = VmDirFindUserByName(
                                    &pContext->dirContext,
                                    pszKey,
                                    &ppObjects[i]);
                }
                else if (objectType == LSA_OBJECT_TYPE_GROUP)
                {
                    dwError2 = VmDirFindGroupByName(
                                    &pContext->dirContext,
                                    pszKey,
                                    &ppObjects[i]);
                }
                else if (objectType == LSA_OBJECT_TYPE_UNDEFINED)
                {
                    dwError2 = VmDirFindUserByName(
                                    &pContext->dirContext,
                                    pszKey,
                                    &ppObjects[i]);
                    if (dwError2)
                    {
                        dwError2 = VmDirFindGroupByName(
                                        &pContext->dirContext,
                                        pszKey,
                                        &ppObjects[i]);
                    }
                }

                break;

            case LSA_QUERY_TYPE_BY_UNIX_ID:

                if (objectType == LSA_OBJECT_TYPE_USER)
                {
                    uid_t uid = queryList.pdwIds[i];

                    dwError2 = VmDirFindUserById(
                                    &pContext->dirContext,
                                    uid,
                                    &ppObjects[i]);
                }
                else if (objectType == LSA_OBJECT_TYPE_GROUP)
                {
                    gid_t gid = queryList.pdwIds[i];

                    dwError2 = VmDirFindGroupById(
                                    &pContext->dirContext,
                                    gid,
                                    &ppObjects[i]);
                }
                else if (objectType == LSA_OBJECT_TYPE_UNDEFINED)
                {
                    uid_t uid = queryList.pdwIds[i];

                    dwError2 = VmDirFindUserById(
                                    &pContext->dirContext,
                                    uid,
                                    &ppObjects[i]);
                    if (dwError2)
                    {
                        gid_t gid = queryList.pdwIds[i];

                        dwError2 = VmDirFindGroupById(
                                        &pContext->dirContext,
                                        gid,
                                        &ppObjects[i]);
                    }
                }

                break;

            case LSA_QUERY_TYPE_BY_SID:

                pszKey = queryList.ppszStrings[i];

                dwError2 = VmDirFindObjectBySID(
                                &pContext->dirContext,
                                pszKey,
                                &ppObjects[i]);
                if (dwError2 == LW_ERROR_SUCCESS)
                {
                    switch (objectType)
                    {
                        case LSA_OBJECT_TYPE_UNDEFINED:
                            break;

                        case LSA_OBJECT_TYPE_USER:
                            if (ppObjects[i]->type != LSA_OBJECT_TYPE_USER)
                            {
                                LsaUtilFreeSecurityObject(ppObjects[i]);
                                ppObjects[i] = NULL;
                            }
                            break;

                        case LSA_OBJECT_TYPE_GROUP:
                            if (ppObjects[i]->type != LSA_OBJECT_TYPE_GROUP)
                            {
                                LsaUtilFreeSecurityObject(ppObjects[i]);
                                ppObjects[i] = NULL;
                            }
                            break;

                        default:
                            LsaUtilFreeSecurityObject(ppObjects[i]);
                            ppObjects[i] = NULL;
                            break;
                    }
                }

                break;

            default:

                break;
        }
    }

    *pppObjects = ppObjects;

cleanup:

    LOG_FUNC_EXIT;

    return dwError;

error:

    if (pppObjects)
    {
        *pppObjects = NULL;
    }

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwCount, ppObjects);
    }

    goto cleanup;
}

DWORD
VmDirOpenEnumObjects(
    HANDLE          hProvider,    /* IN              */
    PHANDLE         phEnum,       /*    OUT          */
    LSA_FIND_FLAGS  findFlags,    /* IN              */
    LSA_OBJECT_TYPE objectType,   /* IN              */
    PCSTR           pszDomainName /* IN     OPTIONAL */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;
    PVMDIR_ENUM_HANDLE pEnumHandle = NULL;

    LOG_FUNC_ENTER;

    if (!hProvider || !phEnum)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    if (VmDirGetJoinState() != VMDIR_JOIN_STATE_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT)hProvider;
    if (!isValidProviderContext(pContext))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    switch (objectType)
    {
        case LSA_OBJECT_TYPE_USER:

            dwError = VmDirCreateUserEnumHandle(
                            &pContext->dirContext,
                            &pEnumHandle);

            break;

        case LSA_OBJECT_TYPE_GROUP:

            dwError = VmDirCreateGroupEnumHandle(
                            &pContext->dirContext,
                            &pEnumHandle);

            break;

        default:

            dwError = LW_ERROR_NOT_HANDLED;

            break;
    }
    BAIL_ON_VMDIR_ERROR(dwError);

    *phEnum = pEnumHandle;

cleanup:

    LOG_FUNC_EXIT;
    
    return dwError;

error:

    if (phEnum)
    {
        *phEnum = NULL;
    }

    if (pEnumHandle)
    {
        VmDirCloseEnum(pEnumHandle);
    }

    goto cleanup;
}

DWORD
VmDirEnumObjects(
    HANDLE                 hEnum,             /* IN              */
    DWORD                  dwMaxObjectsCount, /* IN              */
    PDWORD                 pdwObjectsCount,   /*    OUT          */
    PLSA_SECURITY_OBJECT** pppObjects         /*    OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVMDIR_ENUM_HANDLE    pEnumHandle = (PVMDIR_ENUM_HANDLE)hEnum;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD                 dwCount = 0;

    if (!pEnumHandle || (pEnumHandle->type != VMDIR_ENUM_HANDLE_TYPE_OBJECTS) ||
        !pppObjects || !pdwObjectsCount || !dwMaxObjectsCount)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    LOG_FUNC_ENTER;

    if (VmDirGetJoinState() != VMDIR_JOIN_STATE_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = VmDirRepositoryEnumObjects(
                    pEnumHandle,
                    dwMaxObjectsCount,
                    &ppObjects,
                    &dwCount);
    BAIL_ON_VMDIR_ERROR(dwError);

    *pppObjects      = ppObjects;
    *pdwObjectsCount = dwCount;
    
cleanup:

    LOG_FUNC_EXIT;

    return dwError;

error:

    if (pppObjects)
    {
        *pppObjects = NULL;
    }
    if (pdwObjectsCount)
    {
        *pdwObjectsCount = 0;
    }
    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwCount, ppObjects);
    }

    goto cleanup;
}

DWORD
VmDirOpenEnumMembers(
    HANDLE         hProvider, /* IN              */
    PHANDLE        phEnum,    /*    OUT          */
    LSA_FIND_FLAGS findFlags, /* IN              */
    PCSTR          pszSid     /* IN              */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;
    PVMDIR_ENUM_HANDLE pEnumHandle = NULL;

    LOG_FUNC_ENTER;

    if (!hProvider || !phEnum || IsNullOrEmptyString(pszSid))
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT)hProvider;
    if (!isValidProviderContext(pContext))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = VmDirInitEnumMembersHandle(
                    &pContext->dirContext,
                    pszSid,
                    &pEnumHandle);
    BAIL_ON_VMDIR_ERROR(dwError);

    *phEnum = pEnumHandle;

cleanup:

    LOG_FUNC_EXIT;
    
    return dwError;

error:

    if (phEnum)
    {
        *phEnum = NULL;
    }

    if (pEnumHandle)
    {
        VmDirCloseEnum(pEnumHandle);
    }

    goto cleanup;
}

DWORD
VmDirEnumMembers(
    HANDLE hEnum,               /* IN              */
    DWORD  dwMaxMemberSidCount, /* IN              */
    PDWORD pdwMemberSidCount,   /*    OUT          */
    PSTR** pppszMemberSids      /*    OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVMDIR_ENUM_HANDLE pEnumHandle = (PVMDIR_ENUM_HANDLE)hEnum;
    DWORD dwCount = 0;
    PSTR* ppszMemberSids = NULL;

    LOG_FUNC_ENTER;

    if (    !pEnumHandle ||
            (pEnumHandle->type != VMDIR_ENUM_HANDLE_TYPE_MEMBERS) ||
            !dwMaxMemberSidCount ||
            !pdwMemberSidCount ||
            !pppszMemberSids )
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = VmDirRepositoryEnumMembers(
                    pEnumHandle,
                    dwMaxMemberSidCount,
                    &ppszMemberSids,
                    &dwCount);
    BAIL_ON_VMDIR_ERROR(dwError);

    *pppszMemberSids = ppszMemberSids;
    *pdwMemberSidCount = dwCount;

cleanup:

    LOG_FUNC_EXIT;
    
    return dwError;

error:

    if (pppszMemberSids)
    {
        *pppszMemberSids = NULL;
    }
    if (pdwMemberSidCount)
    {
        *pdwMemberSidCount = 0;
    }
    if (ppszMemberSids)
    {
        LwFreeStringArray(ppszMemberSids, dwCount);
    }

    goto cleanup;
}

VOID
VmDirCloseEnum(
    HANDLE hEnum                /* IN OUT          */
    )
{
    PVMDIR_ENUM_HANDLE pEnumHandle = (PVMDIR_ENUM_HANDLE)hEnum;

    LOG_FUNC_ENTER;

    if (pEnumHandle)
    {
        if (pEnumHandle->ppszDNArray)
        {
            LwFreeStringArray(pEnumHandle->ppszDNArray, pEnumHandle->dwDNCount);
        }

        if (pEnumHandle->pSearchResult)
        {
            VmDirLdapFreeMessage(pEnumHandle->pSearchResult);
            pEnumHandle->pSearchResult = NULL;
        }

        LwFreeMemory(pEnumHandle);
    }

    LOG_FUNC_EXIT;
}

DWORD
VmDirQueryMemberOf(
    HANDLE         hProvider,        /* IN              */
    LSA_FIND_FLAGS findFlags,        /* IN              */
    DWORD          dwSidCount,       /* IN              */
    PSTR*          ppszSids,         /* IN              */
    PDWORD         pdwGroupSidCount, /*    OUT          */
    PSTR**         pppszGroupSids    /*    OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;
    DWORD iSid = 0;
    PLW_HASH_TABLE   pGroupSidTable = NULL;
    LW_HASH_ITERATOR iter = {0};
    LW_HASH_ENTRY*   pHashEntry = NULL;
    DWORD dwGroupSidCount = 0;
    PSTR* ppszGroupSids = NULL;

    LOG_FUNC_ENTER;

    if (!hProvider || !ppszSids || !dwSidCount || !pdwGroupSidCount || !pppszGroupSids)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT)hProvider;
    if (!isValidProviderContext(pContext))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = LwHashCreate(
                    13,
                    LwHashCaselessStringCompare,
                    LwHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pGroupSidTable);
    BAIL_ON_VMDIR_ERROR(dwError);

    for (; iSid < dwSidCount; iSid++)
    {
        PSTR pszSid = ppszSids[iSid];

        dwError = VmDirFindMemberships(
                        &pContext->dirContext,
                        pszSid,
                        pGroupSidTable);
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwGroupSidCount = LwHashGetKeyCount(pGroupSidTable);

    if (dwGroupSidCount > 0)
    {
        dwError = LwAllocateMemory(
                        sizeof(PSTR) * dwGroupSidCount,
                        (PVOID*)&ppszGroupSids);
        BAIL_ON_VMDIR_ERROR(dwError);

        dwError = LwHashGetIterator(pGroupSidTable, &iter);
        BAIL_ON_VMDIR_ERROR(dwError);

        iSid = 0;
        while ((pHashEntry = LwHashNext(&iter)))
        {
            ppszGroupSids[iSid++] = (PSTR)pHashEntry->pValue;
            pHashEntry->pValue = NULL;
        }
    }

    *pdwGroupSidCount = dwGroupSidCount;
    *pppszGroupSids = ppszGroupSids;

cleanup:

    if (pGroupSidTable)
    {
        if (LwHashGetIterator(pGroupSidTable, &iter) == 0)
        {
            while ((pHashEntry = LwHashNext(&iter)))
            {
                LW_SAFE_FREE_MEMORY(pHashEntry->pValue);
            }
        }

        LwHashSafeFree(&pGroupSidTable);
    }

    LOG_FUNC_EXIT;
    
    return dwError;

error:

    if (pdwGroupSidCount)
    {
        *pdwGroupSidCount = 0;
    }
    if (pppszGroupSids)
    {
        *pppszGroupSids = NULL;
    }
    if (ppszGroupSids)
    {
        LwFreeStringArray(ppszGroupSids, dwGroupSidCount);
    }

    if (dwError == LW_ERROR_LDAP_SERVER_DOWN ||
        dwError == LW_ERROR_LDAP_INVALID_CREDENTIALS)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }

    goto cleanup;
}

DWORD
VmDirGetSmartCardUserObject(
    HANDLE                hProvider,          /* IN              */
    PLSA_SECURITY_OBJECT* ppObject,           /*    OUT          */
    PSTR*                 ppszSmartCardReader /* IN              */
    )
{
    LOG_FUNC_ENTER;

    if (ppObject)
    {
        *ppObject = NULL;
    }

    if (ppszSmartCardReader)
    {
        *ppszSmartCardReader = NULL;
    }
    
    LOG_FUNC_EXIT;

    return LW_ERROR_NOT_HANDLED;
}

DWORD
VmDirGetMachineAccountInfoA(
    PCSTR                        dnsDomainName, /* IN     OPTIONAL */
    PLSA_MACHINE_ACCOUNT_INFO_A* ppAccountInfo  /* IN              */
    )
{
    LOG_FUNC_ENTER;

    if (ppAccountInfo)
    {
        *ppAccountInfo = NULL;
    }
    
    LOG_FUNC_EXIT;

    return LW_ERROR_NOT_HANDLED;
}

DWORD
VmDirGetMachineAccountInfoW(
    PCSTR                        dnsDomainName, /* IN     OPTIONAL */
    PLSA_MACHINE_ACCOUNT_INFO_W* ppAccountInfo  /*    OUT          */
    )
{
    LOG_FUNC_ENTER;

    if (ppAccountInfo)
    {
        *ppAccountInfo = NULL;
    }

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}

DWORD
VmDirGetMachinePasswordInfoA(
    PCSTR                         dnsDomainName, /* IN     OPTIONAL */
    PLSA_MACHINE_PASSWORD_INFO_A* ppPasswordInfo /*    OUT          */
    )
{
    LOG_FUNC_ENTER;

    if (ppPasswordInfo)
    {
        *ppPasswordInfo = NULL;
    }
    
    LOG_FUNC_EXIT;

    return LW_ERROR_NOT_HANDLED;
}

DWORD
VmDirGetMachinePasswordInfoW(
    PCSTR                         dnsDomainName, /* IN     OPTIONAL */
    PLSA_MACHINE_PASSWORD_INFO_W* ppPasswordInfo /*    OUT          */
    )
{
    LOG_FUNC_ENTER;

    if (ppPasswordInfo)
    {
        *ppPasswordInfo = NULL;
    }

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}

DWORD
VmDirShutdownProvider(
    VOID
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    LOG_FUNC_ENTER;

    VMCacheSafeClose(&gVmDirAuthProviderGlobals.hDb);

    if (gVmDirAuthProviderGlobals.pRefreshContext)
    {
        VmDirStopMachineAccountRefresh(gVmDirAuthProviderGlobals.pRefreshContext);
        gVmDirAuthProviderGlobals.pRefreshContext = NULL;
    }

    if (gVmDirAuthProviderGlobals.pMutex_rw)
    {
        pthread_rwlock_destroy(&gVmDirAuthProviderGlobals.mutex_rw);
        gVmDirAuthProviderGlobals.pMutex_rw = NULL;
    }

    if (gVmDirAuthProviderGlobals.pMutex)
    {
        pthread_mutex_destroy(&gVmDirAuthProviderGlobals.mutex);
        gVmDirAuthProviderGlobals.pMutex = NULL;
    }

    if (gVmDirAuthProviderGlobals.pBindInfo)
    {
        VmDirReleaseBindInfo(gVmDirAuthProviderGlobals.pBindInfo);

        gVmDirAuthProviderGlobals.pBindInfo = NULL;
    }

    LOG_FUNC_EXIT;

    return dwError;
}

DWORD
VmDirOpenHandle(
    HANDLE  hServer,     /* IN              */
    PCSTR   pszInstance, /* IN              */
    PHANDLE phProvider   /*    OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;

    LOG_FUNC_ENTER;

    if (!hServer || !phProvider)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                    sizeof(VMDIR_AUTH_PROVIDER_CONTEXT),
                    (PVOID*)&pContext);
    BAIL_ON_VMDIR_ERROR(dwError);

    pthread_mutex_init(&pContext->mutex, NULL);
    pContext->pMutex = &pContext->mutex;

    LsaSrvGetClientId(
            hServer,
            &pContext->peer_uid,
            &pContext->peer_gid,
            &pContext->peer_pid);

    if (VmDirGetJoinState() == VMDIR_JOIN_STATE_JOINED)
    {
        dwError = LW_ERROR_SUCCESS;
    }

    *phProvider = pContext;

cleanup:

    LOG_FUNC_EXIT;

    return dwError;

error:

    if (phProvider)
    {
        *phProvider = NULL;
    }

    if (pContext)
    {
        VmDirCloseHandle(pContext);
    }

    goto cleanup;
}

VOID
VmDirCloseHandle(
    HANDLE hProvider /* IN OUT          */
    )
{
    PVMDIR_AUTH_PROVIDER_CONTEXT pProviderContext =
                                    (PVMDIR_AUTH_PROVIDER_CONTEXT)hProvider;

    LOG_FUNC_ENTER;

    if (pProviderContext)
    {
        if (pProviderContext->dirContext.pBindInfo)
        {
            VmDirReleaseBindInfo(pProviderContext->dirContext.pBindInfo);
        }
        if (pProviderContext->dirContext.pLd)
        {
            VmDirLdapClose(pProviderContext->dirContext.pLd);
        }
        if (pProviderContext->pMutex)
        {
            pthread_mutex_destroy(&pProviderContext->mutex);

            pProviderContext->pMutex = NULL;
        }

        LwFreeMemory(pProviderContext);
    }
    
    LOG_FUNC_EXIT;
}

DWORD
VmDirServicesDomain(
    PCSTR    pszDomain,       /* IN              */
    BOOLEAN* pbServicesDomain /* IN OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    LOG_FUNC_ENTER;

    if (!pszDomain || !pbServicesDomain)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    *pbServicesDomain = VmDirMatchesDomain(pszDomain);

cleanup:

    LOG_FUNC_EXIT;
    
    return dwError;

error:

    if (pbServicesDomain)
    {
        *pbServicesDomain = FALSE;
    }

    goto cleanup;
}

DWORD
VmDirAuthenticateUserPam(
    HANDLE                    hProvider,    /* IN              */
    PLSA_AUTH_USER_PAM_PARAMS pParams,      /* IN              */
    PLSA_AUTH_USER_PAM_INFO*  ppPamAuthInfo /*    OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PLSA_AUTH_USER_PAM_INFO pPamAuthInfo = NULL;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;

    LOG_FUNC_ENTER;

    if (!hProvider || !pParams || !pParams->pszPassword || !ppPamAuthInfo)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT)hProvider;
    if (!isValidProviderContext(pContext))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = VmDirFindUserByName(
                                &pContext->dirContext,
                                pParams->pszLoginName,
                                &pObject);
    BAIL_ON_VMDIR_ERROR(dwError);

    if (pObject->userInfo.bAccountDisabled)
    {
        dwError = LW_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    if (pObject->userInfo.bAccountLocked)
    {
        dwError = LW_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    if (pObject->userInfo.bAccountExpired)
    {
        dwError = LW_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    if (pObject->userInfo.bPasswordExpired)
    {
        dwError = LW_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }
    
    switch (gVmDirAuthProviderGlobals.bindProtocol)
    {
        case VMDIR_BIND_PROTOCOL_KERBEROS:
            
                dwError = VmDirInitializeUserLoginCredentials(
                                pObject->userInfo.pszUPN,
                                pParams->pszPassword,
                                pObject->userInfo.uid,
                                pObject->userInfo.gid,
                                NULL);
            
                break;
            
        case VMDIR_BIND_PROTOCOL_SRP:
            
                dwError = VmDirRepositoryVerifyPassword(
                                &pContext->dirContext,
                                pObject->userInfo.pszUPN,
                                pParams->pszPassword);
            
                break;
            
        default:
            
                dwError = ERROR_INVALID_STATE;
            
                break;
            
    }
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(LSA_AUTH_USER_PAM_INFO),
                    (PVOID*)&pPamAuthInfo);
    BAIL_ON_VMDIR_ERROR(dwError);

    // pPamAuthInfo->pszMessage = NULL;
    pPamAuthInfo->bOnlineLogon = TRUE;

    *ppPamAuthInfo = pPamAuthInfo;

cleanup:

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    LOG_FUNC_EXIT;
    
    return dwError;

error:

    if (ppPamAuthInfo)
    {
        *ppPamAuthInfo = NULL;
    }

    if (pPamAuthInfo)
    {
        LsaFreeAuthUserPamInfo(pPamAuthInfo);
    }

    goto cleanup;
}

DWORD
VmDirAuthenticateUserEx(
    HANDLE                hProvider,   /* IN              */
    PLSA_AUTH_USER_PARAMS pUserParams, /* IN              */
    PLSA_AUTH_USER_INFO*  ppUserInfo   /*    OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    LOG_FUNC_ENTER;

    if (!hProvider || !pUserParams || !ppUserInfo)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    *ppUserInfo = NULL;

    dwError = LW_ERROR_NOT_HANDLED;

error:

    LOG_FUNC_EXIT;
    
    return dwError;
}

DWORD
VmDirValidateUser(
    HANDLE hProvider,  /* IN              */
    PCSTR  pszLoginId, /* IN              */
    PCSTR  pszPassword /* IN              */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;

    LOG_FUNC_ENTER;

    if (!hProvider || !pszLoginId)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT) hProvider;
    if (!isValidProviderContext(pContext))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    LSA_LOG_INFO("VmDirValidateUser: Validating user (%s)",
                    LSA_SAFE_LOG_STRING(pszLoginId));

    dwError = VmDirFindUserByName(&pContext->dirContext, pszLoginId, &pObject);
    BAIL_ON_VMDIR_ERROR(dwError);

    if (pObject->userInfo.bPasswordExpired)
    {
        dwError = ERROR_PASSWORD_EXPIRED;
    }
    else if (pObject->userInfo.bAccountDisabled)
    {
        dwError = ERROR_ACCOUNT_DISABLED;
    }
    else if (pObject->userInfo.bAccountExpired)
    {
        dwError = ERROR_ACCOUNT_EXPIRED;
    }
    else if (pObject->userInfo.bAccountLocked)
    {
        dwError = ERROR_ACCOUNT_DISABLED;
    }
    BAIL_ON_VMDIR_ERROR(dwError);

cleanup:

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    LOG_FUNC_EXIT;
    
    return dwError;

error:

    goto cleanup;
}

DWORD
VmDirCheckUserInList(
    HANDLE hProvider,  /* IN              */
    PCSTR  pszLoginId, /* IN              */
    PCSTR  pszListName /* IN              */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;

    LOG_FUNC_ENTER;

    if (!hProvider || !pszLoginId || !pszListName)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT) hProvider;
    if (!isValidProviderContext(pContext))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    LSA_LOG_INFO(
                "VmDirCheckUserInList: Checking user (%s) in list (%s)",
                LSA_SAFE_LOG_STRING(pszLoginId),
                LSA_SAFE_LOG_STRING(pszListName));

cleanup:

    LOG_FUNC_EXIT;
    
    return LW_ERROR_SUCCESS;

error:

    goto cleanup;
}

DWORD
VmDirChangePassword(
    HANDLE hProvider,     /* IN              */
    PCSTR  pszLoginId,    /* IN              */
    PCSTR  pszPassword,   /* IN              */
    PCSTR  pszOldPassword /* IN              */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;
    PLSA_SECURITY_OBJECT pUser = NULL;
    PCSTR pszOldPassword_local = (pszOldPassword ? pszOldPassword : "");
    PCSTR pszNewPassword_local = (pszPassword ? pszPassword : "");

    LOG_FUNC_ENTER;

    if (!hProvider || !pszLoginId || !*pszLoginId)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT)hProvider;
    if (!isValidProviderContext(pContext))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = VmDirFindUserByName(&pContext->dirContext, pszLoginId, &pUser);
    BAIL_ON_VMDIR_ERROR(dwError);

    if (!pUser->userInfo.bUserCanChangePassword)
    {
        dwError = LW_ERROR_PASSWORD_RESTRICTION;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = VmDirRepositoryChangePassword(
                    &pContext->dirContext,
                    pUser->userInfo.pszUPN,
                    pszNewPassword_local,
                    pszOldPassword_local);
    BAIL_ON_VMDIR_ERROR(dwError);

cleanup:

    if (pUser)
    {
        LsaUtilFreeSecurityObject(pUser);
    }

    LOG_FUNC_EXIT;
    
    return dwError;

error:

    goto cleanup;
}

DWORD
VmDirSetPassword (
    HANDLE hProvider,  /* IN              */
    PCSTR  pszLoginId, /* IN              */
    PCSTR  pszPassword /* IN              */
    )
{
    LOG_FUNC_ENTER;

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}


DWORD
VmDirAddUser (
    HANDLE             hProvider, /* IN              */
    PLSA_USER_ADD_INFO pUserInfo  /* IN              */
    )
{
    LOG_FUNC_ENTER;

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}

DWORD
VmDirModifyUser (
    HANDLE               hProvider,   /* IN              */
    PLSA_USER_MOD_INFO_2 pUserModInfo /* IN              */
    )
{
    LOG_FUNC_ENTER;

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}

DWORD
VmDirAddGroup (
    HANDLE              hProvider, /* IN              */
    PLSA_GROUP_ADD_INFO pGroupInfo /* IN              */
    )
{
    LOG_FUNC_ENTER;

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}

DWORD
VmDirModifyGroup (
    HANDLE                hProvider,    /* IN              */
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo /* IN              */
    )
{
    LOG_FUNC_ENTER;

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}

DWORD
VmDirDeleteObject (
    HANDLE hProvider, /* IN              */
    PCSTR  pszSid     /* IN              */
    )
{
    LOG_FUNC_ENTER;

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}

DWORD
VmDirOpenSession(
    HANDLE hProvider, /* IN              */
    PCSTR  pszLoginId /* IN              */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
    BOOLEAN bExists = FALSE;
    BOOLEAN bHomedirCreated = FALSE;

    LOG_FUNC_ENTER;

    if (!hProvider || !pszLoginId || !*pszLoginId)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT)hProvider;
    if (!isValidProviderContext(pContext))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = VmDirFindUserByName(&pContext->dirContext, pszLoginId, &pObject);
    BAIL_ON_VMDIR_ERROR(dwError);

    if (pObject->type != LSA_OBJECT_TYPE_USER)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    if (!pObject->userInfo.pszHomedir || !*pObject->userInfo.pszHomedir)
    {
        dwError = ERROR_INVALID_STATE;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = LwCheckFileTypeExists(
                    pObject->userInfo.pszHomedir,
                    LWFILE_DIRECTORY,
                    &bExists);
    BAIL_ON_VMDIR_ERROR(dwError);

    if (!bExists)
    {
        DWORD  umask = 022;
        mode_t mode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        dwError = LwCreateDirectory(
                        pObject->userInfo.pszHomedir,
                        mode & (~umask));
        BAIL_ON_VMDIR_ERROR(dwError);

        bHomedirCreated = TRUE;

        // TODO : Provision home directory
    }

    dwError = LwChangeOwner(
                    pObject->userInfo.pszHomedir,
                    pObject->userInfo.uid,
                    pObject->userInfo.gid);
    BAIL_ON_VMDIR_ERROR(dwError);

cleanup:

    if (pObject)
    {
        LsaUtilFreeSecurityObject(pObject);
    }

    LOG_FUNC_EXIT;
    
    return dwError;

error:

    if (bHomedirCreated)
    {
        // TODO : Remove home directory if we created it
       ;
    }

    goto cleanup;
}

DWORD
VmDirCloseSession (
    HANDLE hProvider, /* IN              */
    PCSTR  pszLoginId /* IN              */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    LOG_FUNC_ENTER;

    LOG_FUNC_EXIT;

    return dwError;
}

DWORD
VmDirFindNSSArtefactByKey(
    HANDLE                  hProvider,        /* IN              */
    PCSTR                   pszKeyName,       /* IN              */
    PCSTR                   pszMapName,       /* IN              */
    DWORD                   dwInfoLevel,      /* IN              */
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,          /* IN              */
    PVOID*                  ppNSSArtefactInfo /*    OUT          */
    )
{
    LOG_FUNC_ENTER;

    if (ppNSSArtefactInfo)
    {
        *ppNSSArtefactInfo = NULL;
    }
    
    LOG_FUNC_EXIT;

    return LW_ERROR_NOT_HANDLED;
}

DWORD
VmDirBeginEnumNSSArtefacts(
    HANDLE                  hProvider,   /* IN              */
    DWORD                   dwInfoLevel, /* IN              */
    PCSTR                   pszMapName,  /* IN              */
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,     /* IN              */
    PHANDLE                 phResume     /*    OUT          */
    )
{
    LOG_FUNC_ENTER;

    if (phResume)
    {
        *phResume = (HANDLE)NULL;
    }

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}

DWORD
VmDirEnumNSSArtefacts (
    HANDLE  hProvider,             /* IN              */
    HANDLE  hResume,               /* IN              */
    DWORD   dwMaxNumGroups,        /* IN              */
    PDWORD  pdwNSSArtefactsFound,  /*    OUT          */
    PVOID** pppNSSArtefactInfoList /*    OUT          */
    )
{
    LOG_FUNC_ENTER;

    if (pdwNSSArtefactsFound)
    {
        *pdwNSSArtefactsFound = 0;
    }

    if (pppNSSArtefactInfoList)
    {
        *pppNSSArtefactInfoList = NULL;
    }

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}

VOID
VmDirEndEnumNSSArtefacts(
    HANDLE hProvider, /* IN              */
    HANDLE hResume    /* IN              */
    )
{
    LOG_FUNC_ENTER;
    
    LOG_FUNC_EXIT;
}

DWORD
VmDirGetStatus(
    HANDLE                     hProvider,           /* IN              */
    PLSA_AUTH_PROVIDER_STATUS* ppAuthProviderStatus /*    OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus = NULL;
    PVMDIR_BIND_INFO pBindInfo = NULL;
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext = NULL;
    PSTR pszDomainSid = NULL;

    LOG_FUNC_ENTER;

    pContext = (PVMDIR_AUTH_PROVIDER_CONTEXT)hProvider;
    if (!isValidProviderContext(pContext))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    if (!ppAuthProviderStatus)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                   sizeof(LSA_AUTH_PROVIDER_STATUS),
                   (PVOID*)&pProviderStatus);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateString(gpszVmDirProviderName, &pProviderStatus->pszId);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirGetBindInfo(&pBindInfo);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirFindDomainSID(&pContext->dirContext, &pszDomainSid);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateString(
                  pBindInfo->pszDomainFqdn,
                  &pProviderStatus->pszDomain);
    BAIL_ON_VMDIR_ERROR(dwError);

    pProviderStatus->mode = LSA_PROVIDER_MODE_UNPROVISIONED;
    pProviderStatus->status = LSA_AUTH_PROVIDER_STATUS_ONLINE;
    pProviderStatus->pszDomainSid = pszDomainSid;
    pszDomainSid = NULL;

    *ppAuthProviderStatus = pProviderStatus;

cleanup:
    LOG_FUNC_EXIT;

    return dwError;

error:

    if (ppAuthProviderStatus)
    {
        *ppAuthProviderStatus = NULL;
    }

    if (pProviderStatus)
    {
        VmDirFreeStatus(pProviderStatus);
    }
    LW_SAFE_FREE_STRING(pszDomainSid);

    goto cleanup;
}

VOID
VmDirFreeStatus(
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus /* IN OUT          */
    )
{
    LOG_FUNC_ENTER;
    
    if (pProviderStatus)
    {
        LW_SAFE_FREE_STRING(pProviderStatus->pszId);
        LW_SAFE_FREE_STRING(pProviderStatus->pszDomain);
        LW_SAFE_FREE_STRING(pProviderStatus->pszDomainSid);
        LW_SAFE_FREE_STRING(pProviderStatus->pszForest);
        LW_SAFE_FREE_STRING(pProviderStatus->pszSite);
        LW_SAFE_FREE_STRING(pProviderStatus->pszCell);

        LwFreeMemory(pProviderStatus);
    }

    LOG_FUNC_EXIT;
}

DWORD
VmDirRefreshConfiguration(
    VOID
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;

    LOG_FUNC_ENTER;

    dwError = VMDIR_ACQUIRE_RWLOCK_EXCLUSIVE(
                        &gVmDirAuthProviderGlobals.mutex_rw,
                        bInLock);
    BAIL_ON_VMDIR_ERROR(dwError);

    if (gVmDirAuthProviderGlobals.pBindInfo)
    {
        VmDirReleaseBindInfo(gVmDirAuthProviderGlobals.pBindInfo);

        gVmDirAuthProviderGlobals.pBindInfo = NULL;
    }

cleanup:

    VMDIR_RELEASE_RWLOCK(&gVmDirAuthProviderGlobals.mutex_rw, bInLock);

    LOG_FUNC_EXIT;

    return dwError;

error:

    goto cleanup;
}

static
DWORD
VmDirGetJoinState(
    VOID)
{
    VMDIR_JOIN_STATE joinState = VMDIR_JOIN_STATE_UNSET;

    pthread_mutex_lock(&gVmDirAuthProviderGlobals.mutex);
    joinState = gVmDirAuthProviderGlobals.joinState;
    pthread_mutex_unlock(&gVmDirAuthProviderGlobals.mutex);

    return joinState;
}

static
VOID
VmDirSetJoinState(
    VMDIR_JOIN_STATE joinState)
{
    pthread_mutex_lock(&gVmDirAuthProviderGlobals.mutex);
    gVmDirAuthProviderGlobals.joinState = joinState;
    pthread_mutex_unlock(&gVmDirAuthProviderGlobals.mutex);
}

static
DWORD
VmDirSignalProvider(
    HANDLE hProvider,
    uid_t  peerUID,
    gid_t  peerGID,
    DWORD  dwFlags
    )
{
    DWORD dwError = 0;
    PVMDIR_BIND_INFO pBindInfo = NULL;

    if (peerUID != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    if (!(dwFlags & LSA_VMDIR_SIGNAL_RELOAD_CONFIG))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = VmDirRefreshConfiguration();
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirGetBindInfo(&pBindInfo);
    if (dwError == 0)
    {
        VmDirSetJoinState(VMDIR_JOIN_STATE_JOINED);
    }
    else
    {
        VmDirSetJoinState(VMDIR_JOIN_STATE_NOT_JOINED);
    }
    VmDirSignalMachineAccountRefresh(gVmDirAuthProviderGlobals.pRefreshContext);

cleanup:

    if (pBindInfo)
    {
        VmDirReleaseBindInfo(pBindInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
VmDirProviderIoControl (
    HANDLE hProvider,           /* IN              */
    uid_t  peerUID,             /* IN              */
    gid_t  peerGID,             /* IN              */
    DWORD  dwIoControlCode,     /* IN              */
    DWORD  dwInputBufferSize,   /* IN              */
    PVOID  pInputBuffer,        /* IN              */
    DWORD* pdwOutputBufferSize, /*    OUT          */
    PVOID* ppOutputBuffer       /*    OUT          */
    )
{
    DWORD dwError = 0;

    LOG_FUNC_ENTER;

    if (!pdwOutputBufferSize || !ppOutputBuffer)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    switch (dwIoControlCode)
    {
        case LSA_VMDIR_IO_SIGNAL:
            dwError = VmDirSignalProvider(
                          hProvider,
                          peerUID,
                          peerGID,
                          *(PDWORD)pInputBuffer);
            *pdwOutputBufferSize = 0;
            *ppOutputBuffer = NULL;
            break;
        default:
            dwError = LW_ERROR_NOT_HANDLED;
            break;
    }
    BAIL_ON_VMDIR_ERROR(dwError);

cleanup:

    LOG_FUNC_EXIT;

    return dwError;

error:

    if (pdwOutputBufferSize)
    {
        *pdwOutputBufferSize = 0;
    }
    if (ppOutputBuffer)
    {
        *ppOutputBuffer = NULL;
    }

    goto cleanup;
}

static
BOOLEAN
isValidProviderContext(
    PVMDIR_AUTH_PROVIDER_CONTEXT pContext
    )
{
    BOOLEAN isValid = FALSE;

    if (pContext &&
        VmDirGetJoinState() == VMDIR_JOIN_STATE_JOINED)
    {
        isValid = TRUE;
    }

    return isValid;
}

static
VOID
InitVmDirCacheFunctionTable(
    PVMCACHE_PROVIDER_FUNCTION_TABLE pVmDirCacheProviderTable)
{
    InitializeMemCacheProvider(pVmDirCacheProviderTable);
}
