/*
 * Copyright (C) VMware. All rights reserved.
 */

#include "includes.h"

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
    PSAMPLE_USER_INFO* ppUser = NULL;
    PSTR pszPassword = NULL;

    LOG_FUNC_ENTER;

    if (!ppszProviderName || !ppFunctionTable)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    pthread_rwlock_init(&gSampleAuthProviderGlobals.mutex_rw, NULL);
    gSampleAuthProviderGlobals.pMutex_rw = &gSampleAuthProviderGlobals.mutex_rw;

    dwError = LwHashCreate(
					13,
					LwHashStringCompare,
					LwHashStringHash,
					NULL,
					NULL,
					&gSampleAuthProviderGlobals.pPasswordTable);
    BAIL_ON_SAMPLE_ERROR(dwError);

    // Initial value of the password is same as user account name
    for (	ppUser = gSampleAuthProviderGlobals.ppUsers;
    		ppUser && *ppUser;
    		ppUser++ )
    {
    	dwError = LwAllocateString((*ppUser)->pszName, &pszPassword);
    	BAIL_ON_SAMPLE_ERROR(dwError);

    	dwError = LwHashSetValue(
    					gSampleAuthProviderGlobals.pPasswordTable,
    					(*ppUser)->pszName,
    					pszPassword);
    	BAIL_ON_SAMPLE_ERROR(dwError);

    	pszPassword = NULL;
    }

    *ppszProviderName = gpszSampleProviderName;
    *ppFunctionTable = &gSampleProviderAPITable;

cleanup:

	LW_SAFE_FREE_STRING(pszPassword);

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
SampleFindObjects(
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
    PSAMPLE_AUTH_PROVIDER_CONTEXT pContext = NULL;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD i = 0;

    LOG_FUNC_ENTER;
    
    if (!hProvider || !pppObjects || !dwCount)
    {
    	dwError = ERROR_INVALID_PARAMETER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    pContext = (PSAMPLE_AUTH_PROVIDER_CONTEXT)hProvider;

    dwError = LwAllocateMemory(
    				sizeof(PLSA_SECURITY_OBJECT) * dwCount,
    				(PVOID*)&ppObjects);
    BAIL_ON_SAMPLE_ERROR(dwError);

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
					dwError2 = SampleFindUserByName(pszKey, &ppObjects[i]);
				}
				else if (objectType == LSA_OBJECT_TYPE_GROUP)
				{
					dwError2 = SampleFindGroupByName(pszKey, &ppObjects[i]);
				}
				else if (objectType == LSA_OBJECT_TYPE_UNDEFINED)
				{
					dwError2 = SampleFindUserByName(pszKey, &ppObjects[i]);
					if (dwError2)
					{
						dwError2 = SampleFindGroupByName(pszKey, &ppObjects[i]);
					}
				}

				break;

			case LSA_QUERY_TYPE_BY_UNIX_ID:

				if (objectType == LSA_OBJECT_TYPE_USER)
				{
					uid_t uid = queryList.pdwIds[i];

					dwError2 = SampleFindUserById(uid, &ppObjects[i]);
				}
				else if (objectType == LSA_OBJECT_TYPE_GROUP)
				{
					gid_t gid = queryList.pdwIds[i];

					dwError2 = SampleFindGroupById(gid, &ppObjects[i]);
				}
				else if (objectType == LSA_OBJECT_TYPE_UNDEFINED)
				{
					uid_t uid = queryList.pdwIds[i];

					dwError2 = SampleFindUserById(uid, &ppObjects[i]);
					if (dwError2)
					{
						gid_t gid = queryList.pdwIds[i];

						dwError2 = SampleFindGroupById(gid, &ppObjects[i]);
					}
				}

				break;

			case LSA_QUERY_TYPE_BY_SID:

				pszKey = queryList.ppszStrings[i];

				dwError2 = SampleFindObjectBySID(pszKey, &ppObjects[i]);
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
SampleOpenEnumObjects(
    HANDLE          hProvider,    /* IN              */
    PHANDLE         phEnum,       /*    OUT          */
    LSA_FIND_FLAGS  findFlags,    /* IN              */
    LSA_OBJECT_TYPE objectType,   /* IN              */
    PCSTR           pszDomainName /* IN     OPTIONAL */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSAMPLE_ENUM_HANDLE pEnumHandle = NULL;

    LOG_FUNC_ENTER;

    if (!phEnum)
    {
    	dwError = ERROR_INVALID_PARAMETER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sizeof(*pEnumHandle), (PVOID*)&pEnumHandle);
    BAIL_ON_SAMPLE_ERROR(dwError);

    switch (objectType)
    {
		case LSA_OBJECT_TYPE_USER:

			pEnumHandle->type = LSA_OBJECT_TYPE_USER;

			break;

		case LSA_OBJECT_TYPE_GROUP:

			pEnumHandle->type = LSA_OBJECT_TYPE_GROUP;

			break;

		default:

			dwError = LW_ERROR_NOT_HANDLED;
			BAIL_ON_SAMPLE_ERROR(dwError);

			break;
    }

    dwError = SampleInitEnumHandle(pEnumHandle);
    BAIL_ON_SAMPLE_ERROR(dwError);

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
		LwFreeMemory(pEnumHandle);
	}

	goto cleanup;
}

DWORD
SampleEnumObjects(
    HANDLE                 hEnum,             /* IN              */
    DWORD                  dwMaxObjectsCount, /* IN              */
    PDWORD                 pdwObjectsCount,   /*    OUT          */
    PLSA_SECURITY_OBJECT** pppObjects         /*    OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSAMPLE_ENUM_HANDLE    pEnumHandle = (PSAMPLE_ENUM_HANDLE)hEnum;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD                 dwCount = 0;

    if (!pEnumHandle || pEnumHandle->bEnumMembers || !pppObjects || !pdwObjectsCount || !dwMaxObjectsCount)
    {
    	dwError = ERROR_INVALID_PARAMETER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    LOG_FUNC_ENTER;

    if (pEnumHandle->llNextIndex >= pEnumHandle->llTotalCount)
    {
    	dwError = ERROR_NO_MORE_ITEMS;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwCount = pEnumHandle->llTotalCount - pEnumHandle->llNextIndex;
    if (dwCount > dwMaxObjectsCount)
    {
    	dwCount = dwMaxObjectsCount;
    }

    dwError = SampleRepositoryEnumObjects(pEnumHandle, &ppObjects, dwCount);
    BAIL_ON_SAMPLE_ERROR(dwError);

    pEnumHandle->llNextIndex += dwCount;

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
SampleOpenEnumMembers(
    HANDLE         hProvider, /* IN              */
    PHANDLE        phEnum,    /*    OUT          */
    LSA_FIND_FLAGS findFlags, /* IN              */
    PCSTR          pszSid     /* IN              */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSAMPLE_ENUM_HANDLE pEnumHandle = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;

    LOG_FUNC_ENTER;

    if (!hProvider || !phEnum || !pszSid || !*pszSid)
    {
    	dwError = ERROR_INVALID_PARAMETER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = SampleFindObjectBySID(pszSid, &pObject);
    BAIL_ON_SAMPLE_ERROR(dwError);

    if (pObject->type != LSA_OBJECT_TYPE_GROUP)
    {
    	dwError = LW_ERROR_NO_SUCH_OBJECT;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sizeof(*pEnumHandle), (PVOID*)&pEnumHandle);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = LwAllocateString(pszSid, &pEnumHandle->pszSID);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = SampleInitEnumMembersHandle(pObject, pEnumHandle);
    BAIL_ON_SAMPLE_ERROR(dwError);

    *phEnum = pEnumHandle;

cleanup:

	if (pObject)
	{
		LsaUtilFreeSecurityObject(pObject);
	}

    LOG_FUNC_EXIT;
    
    return dwError;

error:

	if (phEnum)
	{
		*phEnum = NULL;
	}

	if (pEnumHandle)
	{
		SampleCloseEnum(pEnumHandle);
	}

	goto cleanup;
}

DWORD
SampleEnumMembers(
    HANDLE hEnum,               /* IN              */
    DWORD  dwMaxMemberSidCount, /* IN              */
    PDWORD pdwMemberSidCount,   /*    OUT          */
    PSTR** pppszMemberSids      /*    OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSAMPLE_ENUM_HANDLE pEnumHandle = (PSAMPLE_ENUM_HANDLE)hEnum;
    DWORD dwCount = 0;
    PSTR* ppszMemberSids = NULL;

    LOG_FUNC_ENTER;

    if (	!pEnumHandle ||
    		!pEnumHandle->bEnumMembers ||
    		!pEnumHandle->pszSID ||
    		!*pEnumHandle->pszSID ||
    		!dwMaxMemberSidCount ||
    		!pdwMemberSidCount ||
    		!pppszMemberSids )
    {
    	dwError = ERROR_INVALID_PARAMETER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    if (pEnumHandle->llNextIndex >= pEnumHandle->llTotalCount)
    {
    	dwError = ERROR_NO_MORE_ITEMS;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwCount = pEnumHandle->llTotalCount - pEnumHandle->llNextIndex;
    if (dwCount > dwMaxMemberSidCount)
    {
    	dwCount = dwMaxMemberSidCount;
    }

    dwError = SampleRepositoryEnumMembers(pEnumHandle, &ppszMemberSids, dwCount);
    BAIL_ON_SAMPLE_ERROR(dwError);

    pEnumHandle->llNextIndex += dwCount;

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
SampleCloseEnum(
    HANDLE hEnum                /* IN OUT          */
    )
{
	PSAMPLE_ENUM_HANDLE pEnumHandle = (PSAMPLE_ENUM_HANDLE)hEnum;

    LOG_FUNC_ENTER;

    if (pEnumHandle)
    {
    	LW_SAFE_FREE_MEMORY(pEnumHandle->pszSID);

    	LwFreeMemory(pEnumHandle);
    }

    LOG_FUNC_EXIT;
}

DWORD
SampleQueryMemberOf(
    HANDLE         hProvider,        /* IN              */
    LSA_FIND_FLAGS findFlags,        /* IN              */
    DWORD          dwSidCount,       /* IN              */
    PSTR*          ppszSids,         /* IN              */
    PDWORD         pdwGroupSidCount, /*    OUT          */
    PSTR**         pppszGroupSids    /*    OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD iSid = 0;
    PLW_HASH_TABLE   pGroupSidTable = NULL;
    LW_HASH_ITERATOR iter = {0};
    LW_HASH_ENTRY*   pHashEntry = NULL;
    DWORD dwGroupSidCount = 0;
    PSTR* ppszGroupSids = NULL;

    LOG_FUNC_ENTER;

    if (!ppszSids || !dwSidCount || !pdwGroupSidCount || !pppszGroupSids)
    {
    	dwError = ERROR_INVALID_PARAMETER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = LwHashCreate(
                    13,
                    LwHashCaselessStringCompare,
                    LwHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pGroupSidTable);
    BAIL_ON_SAMPLE_ERROR(dwError);

    for (; iSid < dwSidCount; iSid++)
    {
    	PSTR pszSid = ppszSids[iSid];

    	dwError = SampleFindMemberships(pszSid, pGroupSidTable);
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwGroupSidCount = LwHashGetKeyCount(pGroupSidTable);

    if (dwGroupSidCount > 0)
    {
    	dwError = LwAllocateMemory(
    					sizeof(PSTR) * dwGroupSidCount,
    					(PVOID*)&ppszGroupSids);
    	BAIL_ON_SAMPLE_ERROR(dwError);

    	dwError = LwHashGetIterator(pGroupSidTable, &iter);
    	BAIL_ON_SAMPLE_ERROR(dwError);

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

	goto cleanup;
}

DWORD
SampleGetSmartCardUserObject(
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
SampleGetMachineAccountInfoA(
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
SampleGetMachineAccountInfoW(
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
SampleGetMachinePasswordInfoA(
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
SampleGetMachinePasswordInfoW(
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
SampleShutdownProvider(
    VOID
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    LOG_FUNC_ENTER;
    
    if (gSampleAuthProviderGlobals.pMutex_rw)
    {
    	pthread_rwlock_destroy(&gSampleAuthProviderGlobals.mutex_rw);
    	gSampleAuthProviderGlobals.pMutex_rw = NULL;
    }

    if (gSampleAuthProviderGlobals.pPasswordTable)
    {
    	LW_HASH_ENTRY* pHashEntry = NULL;
    	LW_HASH_ITERATOR iter = {0};

    	if (!LwHashGetIterator(
    				gSampleAuthProviderGlobals.pPasswordTable,
    				&iter))
		{
			while ((pHashEntry = LwHashNext(&iter)))
			{
				LW_SAFE_FREE_MEMORY(pHashEntry->pValue);
			}
		}

		LwHashSafeFree(&gSampleAuthProviderGlobals.pPasswordTable);
		gSampleAuthProviderGlobals.pPasswordTable = NULL;
    }

    LOG_FUNC_EXIT;

    return dwError;
}

DWORD
SampleOpenHandle(
    HANDLE  hServer,     /* IN              */
    PCSTR   pszInstance, /* IN              */
    PHANDLE phProvider   /*    OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSAMPLE_AUTH_PROVIDER_CONTEXT pContext = NULL;

    LOG_FUNC_ENTER;
    
    if (!hServer || !phProvider)
    {
    	dwError = ERROR_INVALID_PARAMETER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
    				sizeof(SAMPLE_AUTH_PROVIDER_CONTEXT),
    				(PVOID*)&pContext);
    BAIL_ON_SAMPLE_ERROR(dwError);

    pthread_mutex_init(&pContext->mutex, NULL);
    pContext->pMutex = &pContext->mutex;

    LsaSrvGetClientId(
    		hServer,
    		&pContext->peer_uid,
    		&pContext->peer_gid,
    		&pContext->peer_pid);

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
		SampleCloseHandle(pContext);
	}

	goto cleanup;
}

VOID
SampleCloseHandle(
    HANDLE hProvider /* IN OUT          */
    )
{
	PSAMPLE_AUTH_PROVIDER_CONTEXT pProviderContext =
									(PSAMPLE_AUTH_PROVIDER_CONTEXT)hProvider;

    LOG_FUNC_ENTER;

    if (pProviderContext)
    {
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
SampleServicesDomain(
    PCSTR    pszDomain,       /* IN              */
    BOOLEAN* pbServicesDomain /* IN OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    LOG_FUNC_ENTER;

    if (!pszDomain || !pbServicesDomain)
    {
    	dwError = ERROR_INVALID_PARAMETER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    *pbServicesDomain = SampleMatchesDomain(pszDomain);

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
SampleAuthenticateUserPam(
    HANDLE                    hProvider,    /* IN              */
    PLSA_AUTH_USER_PAM_PARAMS pParams,      /* IN              */
    PLSA_AUTH_USER_PAM_INFO*  ppPamAuthInfo /*    OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PLSA_AUTH_USER_PAM_INFO pPamAuthInfo = NULL;
    PSAMPLE_AUTH_PROVIDER_CONTEXT pContext = NULL;

    LOG_FUNC_ENTER;

    if (!hProvider || !pParams || !pParams->pszPassword || !ppPamAuthInfo)
    {
    	dwError = ERROR_INVALID_PARAMETER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    pContext = (PSAMPLE_AUTH_PROVIDER_CONTEXT)hProvider;

    dwError = SampleFindUserByName(pParams->pszLoginName, &pObject);
    BAIL_ON_SAMPLE_ERROR(dwError);

    if (pObject->userInfo.bAccountDisabled)
    {
        dwError = LW_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    if (pObject->userInfo.bAccountLocked)
    {
        dwError = LW_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    if (pObject->userInfo.bAccountExpired)
    {
        dwError = LW_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    if (pObject->userInfo.bPasswordExpired)
    {
        dwError = LW_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = SampleRepositoryVerifyPassword(
    				pObject->pszSamAccountName,
    				pParams->pszPassword);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = LwAllocateMemory(
    				sizeof(LSA_AUTH_USER_PAM_INFO),
    				(PVOID*)&pPamAuthInfo);
    BAIL_ON_SAMPLE_ERROR(dwError);

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
SampleAuthenticateUserEx(
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
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    *ppUserInfo = NULL;

    dwError = LW_ERROR_NOT_HANDLED;

error:

    LOG_FUNC_EXIT;
    
    return dwError;
}

DWORD
SampleValidateUser(
    HANDLE hProvider,  /* IN              */
    PCSTR  pszLoginId, /* IN              */
    PCSTR  pszPassword /* IN              */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSAMPLE_AUTH_PROVIDER_CONTEXT pContext = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;

    LOG_FUNC_ENTER;

    if (!hProvider || !pszLoginId)
    {
    	dwError = ERROR_INVALID_PARAMETER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    pContext = (PSAMPLE_AUTH_PROVIDER_CONTEXT) hProvider;

    LSA_LOG_INFO("SampleValidateUser: Validating user (%s)",
    				LSA_SAFE_LOG_STRING(pszLoginId));

    dwError = SampleFindUserByName(pszLoginId, &pObject);
    BAIL_ON_SAMPLE_ERROR(dwError);

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
	BAIL_ON_SAMPLE_ERROR(dwError);

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
SampleCheckUserInList(
    HANDLE hProvider,  /* IN              */
    PCSTR  pszLoginId, /* IN              */
    PCSTR  pszListName /* IN              */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PSAMPLE_AUTH_PROVIDER_CONTEXT pContext = NULL;

    LOG_FUNC_ENTER;

    if (!hProvider || !pszLoginId || !pszListName)
    {
    	dwError = ERROR_INVALID_PARAMETER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    pContext = (PSAMPLE_AUTH_PROVIDER_CONTEXT) hProvider;

    LSA_LOG_INFO(
    			"SampleCheckUserInList: Checking user (%s) in list (%s)",
    			LSA_SAFE_LOG_STRING(pszLoginId),
    			LSA_SAFE_LOG_STRING(pszListName));

cleanup:

	LOG_FUNC_EXIT;
    
    return LW_ERROR_SUCCESS;

error:

	goto cleanup;
}

DWORD
SampleChangePassword(
    HANDLE hProvider,     /* IN              */
    PCSTR  pszLoginId,    /* IN              */
    PCSTR  pszPassword,   /* IN              */
    PCSTR  pszOldPassword /* IN              */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_SECURITY_OBJECT pUser = NULL;
    PCSTR pszOldPassword_local = (pszOldPassword ? pszOldPassword : "");
    PCSTR pszNewPassword_local = (pszPassword ? pszPassword : "");

    LOG_FUNC_ENTER;

    if (!hProvider || !pszLoginId || !*pszLoginId)
    {
    	dwError = ERROR_INVALID_PARAMETER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = SampleFindUserByName(pszLoginId, &pUser);
    BAIL_ON_SAMPLE_ERROR(dwError);

    if (!pUser->userInfo.bUserCanChangePassword)
    {
    	dwError = LW_ERROR_PASSWORD_RESTRICTION;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = SampleRepositoryChangePassword(
    				pUser->pszSamAccountName,
    				pszNewPassword_local,
    				pszOldPassword_local);
    BAIL_ON_SAMPLE_ERROR(dwError);

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
SampleSetPassword (
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
SampleAddUser (
    HANDLE             hProvider, /* IN              */
    PLSA_USER_ADD_INFO pUserInfo  /* IN              */
    )
{
    LOG_FUNC_ENTER;

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}

DWORD
SampleModifyUser (
    HANDLE               hProvider,   /* IN              */
    PLSA_USER_MOD_INFO_2 pUserModInfo /* IN              */
    )
{
    LOG_FUNC_ENTER;

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}

DWORD
SampleAddGroup (
    HANDLE              hProvider, /* IN              */
    PLSA_GROUP_ADD_INFO pGroupInfo /* IN              */
    )
{
    LOG_FUNC_ENTER;

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}

DWORD
SampleModifyGroup (
    HANDLE                hProvider,    /* IN              */
    PLSA_GROUP_MOD_INFO_2 pGroupModInfo /* IN              */
    )
{
    LOG_FUNC_ENTER;

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}

DWORD
SampleDeleteObject (
    HANDLE hProvider, /* IN              */
    PCSTR  pszSid     /* IN              */
    )
{
    LOG_FUNC_ENTER;

    LOG_FUNC_EXIT;
    
    return LW_ERROR_NOT_HANDLED;
}

DWORD
SampleOpenSession(
    HANDLE hProvider, /* IN              */
    PCSTR  pszLoginId /* IN              */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_SECURITY_OBJECT pObject = NULL;
    BOOLEAN bExists = FALSE;
    BOOLEAN bHomedirCreated = FALSE;

    LOG_FUNC_ENTER;

    if (!hProvider || !pszLoginId || !*pszLoginId)
    {
    	dwError = ERROR_INVALID_PARAMETER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = SampleFindUserByName(pszLoginId, &pObject);
    BAIL_ON_SAMPLE_ERROR(dwError);

    if (pObject->type != LSA_OBJECT_TYPE_USER)
    {
    	dwError = LW_ERROR_NO_SUCH_USER;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    if (!pObject->userInfo.pszHomedir || !*pObject->userInfo.pszHomedir)
    {
    	dwError = ERROR_INVALID_STATE;
    	BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = LwCheckFileTypeExists(
    				pObject->userInfo.pszHomedir,
    				LWFILE_DIRECTORY,
    				&bExists);
    BAIL_ON_SAMPLE_ERROR(dwError);

    if (!bExists)
    {
    	DWORD  umask = 022;
    	mode_t mode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

    	dwError = LwCreateDirectory(
    					pObject->userInfo.pszHomedir,
    					mode & (~umask));
    	BAIL_ON_SAMPLE_ERROR(dwError);

    	bHomedirCreated = TRUE;

    	// TODO : Provision home directory
    }

	dwError = LwChangeOwner(
					pObject->userInfo.pszHomedir,
					pObject->userInfo.uid,
					pObject->userInfo.gid);
	BAIL_ON_SAMPLE_ERROR(dwError);

cleanup:

	if (pObject)
	{
		LsaUtilFreeSecurityObject(pObject);
	}

    LOG_FUNC_EXIT;
    
    return dwError;

error:

	// TODO : Remove home directory if we created it

	goto cleanup;
}

DWORD
SampleCloseSession (
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
SampleFindNSSArtefactByKey(
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
SampleBeginEnumNSSArtefacts(
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
SampleEnumNSSArtefacts (
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
SampleEndEnumNSSArtefacts(
    HANDLE hProvider, /* IN              */
    HANDLE hResume    /* IN              */
    )
{
    LOG_FUNC_ENTER;
    
    LOG_FUNC_EXIT;
}

DWORD
SampleGetStatus(
    HANDLE                     hProvider,           /* IN              */
    PLSA_AUTH_PROVIDER_STATUS* ppAuthProviderStatus /*    OUT          */
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus = NULL;

    LOG_FUNC_ENTER;
    
    if (!ppAuthProviderStatus)
    {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMPLE_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                   sizeof(LSA_AUTH_PROVIDER_STATUS),
                   (PVOID*)&pProviderStatus);
    BAIL_ON_SAMPLE_ERROR(dwError);

    dwError = LwAllocateString(gpszSampleProviderName, &pProviderStatus->pszId);
    BAIL_ON_SAMPLE_ERROR(dwError);

    pProviderStatus->mode = LSA_PROVIDER_MODE_UNPROVISIONED;
    pProviderStatus->status = LSA_AUTH_PROVIDER_STATUS_ONLINE;

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
        SampleFreeStatus(pProviderStatus);
    }

    goto cleanup;
}

VOID
SampleFreeStatus(
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
SampleRefreshConfiguration(
    VOID
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    LOG_FUNC_ENTER;

    LOG_FUNC_EXIT;
    
    return dwError;
}

DWORD
SampleProviderIoControl (
    HANDLE hProvider,           /* IN              */
    uid_t  peerUid,             /* IN              */
    gid_t  peerGID,             /* IN              */
    DWORD  dwIoControlCode,     /* IN              */
    DWORD  dwInputBufferSize,   /* IN              */
    PVOID  pInputBuffer,        /* IN              */
    DWORD* pdwOutputBufferSize, /*    OUT          */
    PVOID* ppOutputBuffer       /*    OUT          */
    )
{
    LOG_FUNC_ENTER;

    if (pdwOutputBufferSize)
    {
        pdwOutputBufferSize = 0;
    }
    if (ppOutputBuffer)
    {
        *ppOutputBuffer = NULL;
    }
    
    LOG_FUNC_EXIT;

    return LW_ERROR_NOT_HANDLED;
}
