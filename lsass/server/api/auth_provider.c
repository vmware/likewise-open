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
 *        auth_provider.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Authentication Provider Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "api.h"

static
DWORD
LsaSrvAuthProviderReadRegistry(
    PLSA_STACK *ppProviderStack
    );


VOID
LsaSrvFreeAuthProvider(
    PLSA_AUTH_PROVIDER pProvider
    )
{
    if (pProvider)
    {
        if (pProvider->pFnTable && pProvider->pFnTable->pfnShutdownProvider)
        {
           pProvider->pFnTable->pfnShutdownProvider();
        }

        if (pProvider->pLibHandle)
        {
           dlclose(pProvider->pLibHandle);
        }

        LW_SAFE_FREE_STRING(pProvider->pszId);
        LW_SAFE_FREE_STRING(pProvider->pszProviderLibpath);

        LwFreeMemory(pProvider);
    }
}


VOID
LsaSrvFreeAuthProviderList(
    PLSA_AUTH_PROVIDER pProviderList
    )
{
    PLSA_AUTH_PROVIDER pProvider = NULL;

    while (pProviderList) {
        pProvider = pProviderList;
        pProviderList = pProviderList->pNext;
        LsaSrvFreeAuthProvider(pProvider);
        pProvider = NULL;
    }
}


DWORD
LsaSrvValidateProvider(
    PLSA_AUTH_PROVIDER pProvider
    )
{
    if (!pProvider ||
        !pProvider->pFnTable ||
        !pProvider->pFnTable->pfnShutdownProvider ||
        !pProvider->pFnTable->pfnOpenHandle ||
        !pProvider->pFnTable->pfnCloseHandle ||
        !pProvider->pFnTable->pfnServicesDomain ||
        !pProvider->pFnTable->pfnAuthenticateUser ||
        !pProvider->pFnTable->pfnValidateUser ||
        !pProvider->pFnTable->pfnCheckUserInList ||
        !pProvider->pFnTable->pfnLookupUserByName ||
        !pProvider->pFnTable->pfnLookupUserById ||
        !pProvider->pFnTable->pfnBeginEnumUsers ||
        !pProvider->pFnTable->pfnEnumUsers ||
        !pProvider->pFnTable->pfnEndEnumUsers ||
        !pProvider->pFnTable->pfnLookupGroupByName ||
        !pProvider->pFnTable->pfnLookupGroupById ||
        !pProvider->pFnTable->pfnGetGroupsForUser ||
        !pProvider->pFnTable->pfnBeginEnumGroups ||
        !pProvider->pFnTable->pfnEnumGroups ||
        !pProvider->pFnTable->pfnEndEnumGroups ||
        !pProvider->pFnTable->pfnChangePassword ||
        !pProvider->pFnTable->pfnAddUser ||
        !pProvider->pFnTable->pfnModifyUser ||
        !pProvider->pFnTable->pfnDeleteUser ||
        !pProvider->pFnTable->pfnAddGroup ||
        !pProvider->pFnTable->pfnDeleteGroup ||
        !pProvider->pFnTable->pfnOpenSession ||
        !pProvider->pFnTable->pfnCloseSession ||
        !pProvider->pFnTable->pfnGetNamesBySidList ||
        !pProvider->pFnTable->pfnLookupNSSArtefactByKey ||
        !pProvider->pFnTable->pfnBeginEnumNSSArtefacts ||
        !pProvider->pFnTable->pfnEnumNSSArtefacts ||
        !pProvider->pFnTable->pfnEndEnumNSSArtefacts ||
        !pProvider->pFnTable->pfnGetStatus ||
        !pProvider->pFnTable->pfnFreeStatus ||
        !pProvider->pFnTable->pfnRefreshConfiguration ||
        !pProvider->pFnTable->pfnProviderIoControl
        )
    {
        return LW_ERROR_INVALID_AUTH_PROVIDER;
    }

    return 0;
}

DWORD
LsaSrvInitAuthProvider(
    IN PLSA_AUTH_PROVIDER pProvider,
    IN OPTIONAL PLSA_STATIC_PROVIDER pStaticProviders
    )
{
    DWORD dwError = 0;
    PFNINITIALIZEPROVIDER pfnInitProvider = NULL;
    PCSTR pszError = NULL;
    PSTR pszProviderLibpath = NULL;
    int i = 0;

    if (pStaticProviders)
    {
        /* First look for a static provider entry with the given name */
        for (i = 0; pStaticProviders[i].pszId; i++)
        {
            if (!strcmp(pStaticProviders[i].pszId, pProvider->pszId))
            {
                pfnInitProvider = pStaticProviders[i].pInitialize;
                LSA_LOG_DEBUG("Provider %s loaded from static list", pProvider->pszId);
                break;
            }
        }
    }

    if (!pfnInitProvider)
    {
        /* Try to load the provider dynamically */
        if (LW_IS_NULL_OR_EMPTY_STR(pProvider->pszProviderLibpath))
        {
            dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        pszProviderLibpath = pProvider->pszProviderLibpath;

        dlerror();
        pProvider->pLibHandle = dlopen(pszProviderLibpath, RTLD_NOW | RTLD_GLOBAL);
        if (!pProvider->pLibHandle)
        {
            LSA_LOG_ERROR("Failed to open auth provider at path '%s'", pszProviderLibpath);

            pszError = dlerror();
            if (!LW_IS_NULL_OR_EMPTY_STR(pszError))
            {
                LSA_LOG_ERROR("%s", pszError);
            }

            dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dlerror();
        pfnInitProvider = (PFNINITIALIZEPROVIDER)dlsym(
            pProvider->pLibHandle,
            LSA_SYMBOL_NAME_INITIALIZE_PROVIDER);
        if (!pfnInitProvider)
        {
            LSA_LOG_ERROR("Ignoring invalid auth provider at path '%s'", pszProviderLibpath);

            pszError = dlerror();
            if (!LW_IS_NULL_OR_EMPTY_STR(pszError))
            {
                LSA_LOG_ERROR("%s", pszError);
            }

            dwError = LW_ERROR_INVALID_AUTH_PROVIDER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = pfnInitProvider(
                    &pProvider->pszName,
                    &pProvider->pFnTable);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvValidateProvider(pProvider);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvInitAuthProviders(
    IN OPTIONAL PLSA_STATIC_PROVIDER pStaticProviders
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProviderList = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    PLSA_STACK pProviderStack = NULL;
    BOOLEAN bInLock = FALSE;

    dwError = LsaSrvAuthProviderReadRegistry(&pProviderStack);
    BAIL_ON_LSA_ERROR(dwError);

    pProviderStack = LsaStackReverse(pProviderStack);

    pProvider = (PLSA_AUTH_PROVIDER) LsaStackPop(&pProviderStack);

    while(pProvider)
    {
        dwError = LsaSrvInitAuthProvider(pProvider, pStaticProviders);

        if (dwError)
        {
            LSA_LOG_ERROR("Failed to load provider '%s' from '%s' - error %u (%s)",
                LSA_SAFE_LOG_STRING(pProvider->pszId),
                LSA_SAFE_LOG_STRING(pProvider->pszProviderLibpath),
                dwError,
                LwWin32ErrorToName(dwError));

            LsaSrvFreeAuthProvider(pProvider);
            pProvider = NULL;
            dwError = 0;
        }
        else
        {
            pProvider->pNext = pProviderList;
            pProviderList = pProvider;
        }
        pProvider = (PLSA_AUTH_PROVIDER) LsaStackPop(&pProviderStack);
    }

    ENTER_AUTH_PROVIDER_LIST_WRITER_LOCK(bInLock);

    LsaSrvFreeAuthProviderList(gpAuthProviderList);

    gpAuthProviderList = pProviderList;
    pProviderList = NULL;

    LEAVE_AUTH_PROVIDER_LIST_WRITER_LOCK(bInLock);

cleanup:

    if (pProviderStack)
    {
        LsaStackForeach(
            pProviderStack,
            &LsaCfgFreeAuthProviderInStack,
            NULL
            );
        LsaStackFree(pProviderStack);
    }

    return dwError;

error:

    if (pProviderList) {
        LsaSrvFreeAuthProviderList(pProviderList);
    }

    goto cleanup;
}

DWORD
LsaCfgFreeAuthProviderInStack(
    PVOID pItem,
    PVOID pUserData
    )
{
    DWORD dwError = 0;

    if (pItem) {
        LsaSrvFreeAuthProvider((PLSA_AUTH_PROVIDER)pItem);
    }

    return dwError;
}

static
DWORD
LsaSrvPushProvider(
    PCSTR pszId,
    PCSTR pszPath,
    PLSA_STACK *ppProviderStack
    )
{
    DWORD dwError = 0;

    PLSA_AUTH_PROVIDER pProvider = NULL;

    dwError = LwAllocateMemory(
                sizeof(LSA_AUTH_PROVIDER),
                (PVOID*)&pProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pszPath, &(pProvider->pszProviderLibpath));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(pszId, &(pProvider->pszId));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaStackPush(pProvider, ppProviderStack);
    BAIL_ON_LSA_ERROR(dwError);
    pProvider = NULL;


cleanup:

    return dwError;

error:

    if (pProvider)
    {
        LsaSrvFreeAuthProvider(pProvider);
        pProvider = NULL;
    }
    goto cleanup;

}
static
DWORD
LsaSrvAuthProviderRead(
    PCSTR   pszProviderName,
    PCSTR   pszProviderKey,
    PLSA_STACK *ppProviderStack
    )
{
    DWORD dwError = 0;

    PLSA_CONFIG_REG pReg = NULL;

    PSTR pszId = NULL;
    PSTR pszPath = NULL;

    dwError = LsaOpenConfig(
                pszProviderKey,
                pszProviderKey,
                &pReg);
    BAIL_ON_LSA_ERROR(dwError);

    if (pReg == NULL)
    {
        goto error;
    }

    dwError = LsaReadConfigString(
                pReg,
                "Id",
                FALSE,
                &pszId);
    BAIL_ON_LSA_ERROR(dwError);

    if (LW_IS_NULL_OR_EMPTY_STR(pszId))
    {
        goto error;
    }

    dwError = LsaReadConfigString(
                pReg,
                "Path",
                FALSE,
                &pszPath);
    BAIL_ON_LSA_ERROR(dwError);

    if (LW_IS_NULL_OR_EMPTY_STR(pszPath))
    {
        goto error;
    }

    dwError = LsaSrvPushProvider(
                pszId,
                pszPath,
                ppProviderStack);
    BAIL_ON_LSA_ERROR(dwError);


cleanup:

    LW_SAFE_FREE_STRING(pszId);
    LW_SAFE_FREE_STRING(pszPath);

    LsaCloseConfig(pReg);
    pReg = NULL;

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvAuthProviderReadRegistry(
    PLSA_STACK *ppProviderStack
    )
{
    DWORD dwError = 0;

    PLSA_CONFIG_REG pReg = NULL;
    PSTR pszProviders = NULL;
    PSTR pszProviderKey = NULL;

    PSTR pszProvider = NULL;
    PSTR pszTokenState = NULL;

    BAIL_ON_INVALID_POINTER(ppProviderStack);

    dwError = LsaOpenConfig(
                "Services\\lsass\\Parameters\\Providers",
                "Policy\\Services\\lsass\\Parameters\\Providers",
                &pReg);
    BAIL_ON_LSA_ERROR(dwError);

    if (pReg == NULL)
    {
        goto error;
    }

    dwError = LsaReadConfigString(
                pReg,
                "Load",
                FALSE,
                &pszProviders);
    BAIL_ON_LSA_ERROR(dwError);

    LsaCloseConfig(pReg);
    pReg = NULL;

    if (pszProviders == NULL )
    {
        goto error;
    }

    pszProvider = strtok_r(pszProviders, ",", &pszTokenState);
    while ( pszProvider != NULL )
    {
        dwError = LwAllocateStringPrintf(
                    &pszProviderKey,
                    "Services\\lsass\\Parameters\\Providers\\%s",
                    pszProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaSrvAuthProviderRead(
                    pszProvider,
                    pszProviderKey,
                    ppProviderStack);
        BAIL_ON_LSA_ERROR(dwError);

        pszProvider = strtok_r(NULL, ",", &pszTokenState);
    }

cleanup:

    LW_SAFE_FREE_STRING(pszProviders);
    LW_SAFE_FREE_STRING(pszProviderKey);

    LsaCloseConfig(pReg);
    pReg = NULL;

    return dwError;

error:

    if (dwError == 0)
    {
        /* We should only get here if there is some problem with the
         * registry -- can't access it, the key isn't there, ...
         * -- so we will try a default set of providers.
         */
        LSA_LOG_ERROR("Problem accessing provider configuration in registry. Trying compiled defaults [Local, ActiveDirectory].");

        LsaSrvPushProvider(
                "lsa-local-provider",
                LSA_PROVIDER_LOCAL_PATH,
                ppProviderStack);

        LsaSrvPushProvider(
                "lsa-activedirectory-provider",
                LSA_PROVIDER_AD_PATH,
                ppProviderStack);
    }

    goto cleanup;
}

VOID
LsaSrvFreeAuthProviders(
    VOID
    )
{
    BOOLEAN bInLock = FALSE;

    ENTER_AUTH_PROVIDER_LIST_WRITER_LOCK(bInLock);

    LsaSrvFreeAuthProviderList(gpAuthProviderList);
    gpAuthProviderList = NULL;

    LEAVE_AUTH_PROVIDER_LIST_WRITER_LOCK(bInLock);
}

DWORD
LsaGetNumberOfProviders_inlock(
    VOID
    )
{
    DWORD dwCount = 0;
    PLSA_AUTH_PROVIDER pProvider = NULL;

    for (pProvider = gpAuthProviderList; pProvider; pProvider = pProvider->pNext)
    {
        dwCount++;
    }

    return dwCount;
}

