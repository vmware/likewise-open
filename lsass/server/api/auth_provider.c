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

VOID
LsaSrvFreeAuthProvider(
    PLSA_AUTH_PROVIDER pProvider
    )
{
    if (pProvider) {

        LSA_SAFE_FREE_STRING(pProvider->pszProviderLibpath);

        if (pProvider->pFnShutdown) {

           pProvider->pFnShutdown(
                       pProvider->pszName,
                       pProvider->pFnTable
                       );

        }

        if (pProvider->pLibHandle) {
           dlclose(pProvider->pLibHandle);
        }

        LSA_SAFE_FREE_STRING(pProvider->pszId);
        LSA_SAFE_FREE_STRING(pProvider->pszProviderLibpath);

        LsaFreeMemory(pProvider);
    }
}

VOID
LsaSrvFreeAuthProviderStack(
    PLSA_STACK pProviderStack
    )
{
    while (pProviderStack) {
        PLSA_AUTH_PROVIDER pProvider =
            (PLSA_AUTH_PROVIDER)LsaStackPop(&pProviderStack);
        LsaSrvFreeAuthProvider(pProvider);
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
        !pProvider->pFnTable->pfnBeginEnumNSSArtefacts ||
        !pProvider->pFnTable->pfnEnumNSSArtefacts ||
        !pProvider->pFnTable->pfnEndEnumNSSArtefacts ||
        !pProvider->pFnTable->pfnGetStatus ||
        !pProvider->pFnTable->pfnFreeStatus
        )
    {
        return LSA_ERROR_INVALID_AUTH_PROVIDER;
    }

    return 0;
}

DWORD
LsaSrvInitAuthProvider(
    PCSTR pszConfigFilePath,
    PLSA_AUTH_PROVIDER pProvider
    )
{
    DWORD dwError = 0;
    PFNINITIALIZEPROVIDER pfnInitProvider = NULL;
    PCSTR  pszError = NULL;
    PSTR pszProviderLibpath = NULL;

    if (IsNullOrEmptyString(pProvider->pszProviderLibpath)) {
        dwError = ENOENT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pszProviderLibpath = pProvider->pszProviderLibpath;

    dlerror();

    pProvider->pLibHandle = dlopen(pszProviderLibpath, RTLD_NOW | RTLD_GLOBAL);
    if (pProvider->pLibHandle == NULL) {
       LSA_LOG_ERROR("Failed to open auth provider at path [%s]", pszProviderLibpath);

       pszError = dlerror();
       if (!IsNullOrEmptyString(pszError)) {
          LSA_LOG_ERROR("%s", pszError);
       }

       dwError = LSA_ERROR_INVALID_AUTH_PROVIDER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dlerror();
    pfnInitProvider = (PFNINITIALIZEPROVIDER)dlsym(
                                        pProvider->pLibHandle,
                                        LSA_SYMBOL_NAME_INITIALIZE_PROVIDER);
    if (pfnInitProvider == NULL) {
       LSA_LOG_ERROR("Ignoring invalid auth provider at path [%s]", pszProviderLibpath);

       pszError = dlerror();
       if (!IsNullOrEmptyString(pszError)) {
          LSA_LOG_ERROR("%s", pszError);
       }

       dwError = LSA_ERROR_INVALID_AUTH_PROVIDER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dlerror();
    pProvider->pFnShutdown = (PFNSHUTDOWNPROVIDER)dlsym(
                                        pProvider->pLibHandle,
                                        LSA_SYMBOL_NAME_SHUTDOWN_PROVIDER);
    if (pProvider->pFnShutdown == NULL) {
       LSA_LOG_ERROR("Ignoring invalid auth provider at path [%s]", pszProviderLibpath);

       pszError = dlerror();
       if (!IsNullOrEmptyString(pszError)) {
          LSA_LOG_ERROR("%s", pszError);
       }

       dwError = LSA_ERROR_INVALID_AUTH_PROVIDER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = pfnInitProvider(
                    pszConfigFilePath,
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
    PCSTR pszConfigFilePath
    )
{
    DWORD dwError = 0;
    PLSA_AUTH_PROVIDER pProviderList = NULL;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    PLSA_STACK pProviderStack = NULL;
    BOOLEAN bInLock = FALSE;

    dwError = LsaParseConfigFile(
                    pszConfigFilePath,
                    LSA_CFG_OPTION_STRIP_ALL,
                    &LsaSrvAuthProviderConfigStartSection,
                    NULL,
                    &LsaSrvAuthProviderConfigNameValuePair,
                    NULL,
                    (PVOID)&pProviderStack
                    );
    BAIL_ON_LSA_ERROR(dwError);

    pProvider = (PLSA_AUTH_PROVIDER) LsaStackPop(&pProviderStack);

    while(pProvider)
    {
        dwError = LsaSrvInitAuthProvider(pszConfigFilePath, pProvider);

        if (dwError)
        {
            LSA_LOG_ERROR("Failed to load provider [%s] at [%s] [error code:%d]",
                (pProvider->pszName ? pProvider->pszName : "<null>"),
                (pProvider->pszProviderLibpath ? pProvider->pszProviderLibpath : "<null>"),
                dwError);

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


DWORD
LsaSrvAuthProviderConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PLSA_STACK* ppProviderStack = (PLSA_STACK*)pData;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bSkipSection = FALSE;
    PCSTR   pszLibName = NULL;

    BAIL_ON_INVALID_POINTER(ppProviderStack);

    if (IsNullOrEmptyString(pszSectionName) ||
        strncasecmp(pszSectionName, LSA_CFG_TAG_AUTH_PROVIDER, sizeof(LSA_CFG_TAG_AUTH_PROVIDER)-1))
    {
        bSkipSection = TRUE;
        goto done;
    }

    pszLibName = pszSectionName + sizeof(LSA_CFG_TAG_AUTH_PROVIDER) - 1;
    if (IsNullOrEmptyString(pszLibName)) {
        LSA_LOG_WARNING("No Auth Provider Plugin name was specified");
        bSkipSection = TRUE;
        goto done;
    }

    dwError = LsaAllocateMemory(
                sizeof(LSA_AUTH_PROVIDER),
                (PVOID*)&pProvider);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(
                pszLibName,
                &pProvider->pszId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaStackPush(
                pProvider,
                ppProviderStack
                );
    BAIL_ON_LSA_ERROR(dwError);

done:

    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

cleanup:

    return dwError;

error:

    *pbContinue = FALSE;
    *pbSkipSection = TRUE;

    if (pProvider)
    {
        LsaSrvFreeAuthProvider(pProvider);
    }

    goto cleanup;
}

DWORD
LsaSrvAuthProviderConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PLSA_STACK* ppProviderStack = (PLSA_STACK*)pData;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    PSTR pszProviderLibpath = NULL;

    BAIL_ON_INVALID_POINTER(ppProviderStack);

    pProvider = (PLSA_AUTH_PROVIDER) LsaStackPeek(*ppProviderStack);

    if (!pProvider)
    {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (strcasecmp(pszName, "path") == 0)
    {
        if (!IsNullOrEmptyString(pszValue)) {
            dwError = LsaAllocateString(
                        pszValue,
                        &pszProviderLibpath);
            BAIL_ON_LSA_ERROR(dwError);
        }

        //don't allow redefinition a value within a section.
        if (pProvider->pszProviderLibpath != NULL)
        {
            LSA_LOG_WARNING("path redefined in configuration file");
            LSA_SAFE_FREE_STRING(pProvider->pszProviderLibpath);
        }

        pProvider->pszProviderLibpath = pszProviderLibpath;
        pszProviderLibpath = NULL;
    }

    *pbContinue = TRUE;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszProviderLibpath);

    *pbContinue = FALSE;

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

