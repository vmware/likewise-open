#include "includes.h"

DWORD
DirectoryGetProvider(
    PDIRECTORY_PROVIDER* ppProvider
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PDIRECTORY_PROVIDER_INFO pProviderInfo = NULL;

    DIRECTORY_LOCK_MUTEX(bInLock, &gDirGlobals.mutex);

    if (!gDirGlobals.pProvider)
    {
        dwError = DirectoryGetProviderInfo(&pProviderInfo);
        BAIL_ON_DIRECTORY_ERROR(dwError);

        dwError = DirectoryLoadProvider(
                        pProviderInfo,
                        &gDirGlobals.pProvider);
        BAIL_ON_DIRECTORY_ERROR(dwError);

        gDirGlobals.pProvider->pProviderInfo = pProviderInfo;

        pProviderInfo = NULL;
    }

    InterlockedIncrement(&gDirGlobals.pProvider->refCount);

    *ppProvider = gDirGlobals.pProvider;

cleanup:

    DIRECTORY_UNLOCK_MUTEX(bInLock, &gDirGlobals.mutex);

    if (pProviderInfo)
    {
        DirectoryFreeProviderInfo(pProviderInfo);
    }

    return dwError;

error:

    *ppProvider = NULL;

    goto cleanup;
}

DWORD
DirectoryGetProviderInfo(
    PDIRECTORY_PROVIDER_INFO* ppProviderInfo
    )
{
    DWORD dwError = 0;
    PDIRECTORY_PROVIDER_INFO pProviderInfo = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(DIRECTORY_PROVIDER_INFO),
                    (PVOID*)&pProviderInfo);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    pProviderInfo->dirType = LOCAL_SAM;

    dwError = DirectoryAllocateString(
                    SAM_DB_PROVIDER_PATH,
                    &pProviderInfo->pszProviderPath);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    *ppProviderInfo = pProviderInfo;

cleanup:

    return dwError;

error:

    *ppProviderInfo = NULL;

    if (pProviderInfo)
    {
        DirectoryFreeProviderInfo(pProviderInfo);
    }

    goto cleanup;
}

DWORD
DirectoryLoadProvider(
    PDIRECTORY_PROVIDER_INFO pProviderInfo,
    PDIRECTORY_PROVIDER* ppProvider
    )
{
    DWORD dwError = 0;
    PFNINITIALIZEDIRPROVIDER pfnInitProvider = NULL;
    PDIRECTORY_PROVIDER pProvider = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(DIRECTORY_PROVIDER),
                    (PVOID*)&pProvider);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    pProvider->refCount = 1;

    dlerror();

    pProvider->pLibHandle = dlopen(pProvider->pProviderInfo->pszProviderPath,
                                    RTLD_NOW | RTLD_GLOBAL);
    if (pProvider->pLibHandle == NULL)
    {
        PSTR pszError = NULL;

        DIRECTORY_LOG_ERROR("Failed to open directory provider at path [%s]",
                       pProvider->pProviderInfo->pszProviderPath);

        pszError = dlerror();
        if (!IsNullOrEmptyString(pszError))
        {
          DIRECTORY_LOG_ERROR("%s", pszError);
        }

        dwError = LSA_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dlerror();
    pfnInitProvider = (PFNINITIALIZEDIRPROVIDER)dlsym(
                                        pProvider->pLibHandle,
                                        DIRECTORY_SYMBOL_NAME_INITIALIZE_PROVIDER);
    if (pfnInitProvider == NULL)
    {
        PSTR pszError = NULL;

        DIRECTORY_LOG_ERROR("Invalid directory provider at path [%s]",
                       pProvider->pProviderInfo->pszProviderPath);

        pszError = dlerror();
        if (!IsNullOrEmptyString(pszError))
        {
          DIRECTORY_LOG_ERROR("%s", pszError);
        }

        dwError = LSA_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dlerror();
    pProvider->pfnShutdown = (PFNSHUTDOWNDIRPROVIDER)dlsym(
                                        pProvider->pLibHandle,
                                        DIRECTORY_SYMBOL_NAME_SHUTDOWN_PROVIDER);
    if (pProvider->pfnShutdown == NULL)
    {
        PSTR pszError = NULL;

        DIRECTORY_LOG_ERROR("Invalid directory provider at path [%s]",
                       pProvider->pProviderInfo->pszProviderPath);

        pszError = dlerror();
        if (!IsNullOrEmptyString(pszError))
        {
            DIRECTORY_LOG_ERROR("%s", pszError);
        }

        dwError = LSA_ERROR_INVALID_AUTH_PROVIDER;
        BAIL_ON_DIRECTORY_ERROR(dwError);
    }

    dwError = pfnInitProvider(
                    NULL,
                    &pProvider->pszProviderName,
                    &pProvider->pProviderFnTbl);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    dwError = DirectoryValidateProvider(pProvider);
    BAIL_ON_DIRECTORY_ERROR(dwError);

    *ppProvider = pProvider;

cleanup:

    return dwError;

error:

    *ppProvider = NULL;

    if (pProvider)
    {
        DirectoryReleaseProvider(pProvider);
    }

    goto cleanup;
}

DWORD
DirectoryValidateProvider(
    PDIRECTORY_PROVIDER pProvider
    )
{
    DWORD dwError = 0;

    if (!pProvider ||
        !pProvider->pfnShutdown ||
        !pProvider->pProviderFnTbl ||
        !pProvider->pProviderFnTbl->pfnDirectoryAdd ||
        !pProvider->pProviderFnTbl->pfnDirectoryBind ||
        !pProvider->pProviderFnTbl->pfnDirectoryClose ||
        !pProvider->pProviderFnTbl->pfnDirectoryDelete ||
        !pProvider->pProviderFnTbl->pfnDirectoryModify ||
        !pProvider->pProviderFnTbl->pfnDirectoryOpen ||
        !pProvider->pProviderFnTbl->pfnDirectorySearch)
    {
        dwError = LSA_ERROR_INVALID_AUTH_PROVIDER;
    }

    return dwError;
}

VOID
DirectoryReleaseProvider(
    PDIRECTORY_PROVIDER pProvider
    )
{
    if (InterlockedDecrement(&pProvider->refCount) == 0)
    {
        DirectoryFreeProvider(pProvider);
    }
}

VOID
DirectoryFreeProvider(
    PDIRECTORY_PROVIDER pProvider
    )
{
    if (pProvider->pLibHandle)
    {
        if (pProvider->pfnShutdown)
        {
            DWORD dwError = 0;

            dwError = pProvider->pfnShutdown(
                            pProvider->pszProviderName,
                            pProvider->pProviderFnTbl);
            if (dwError)
            {
                DIRECTORY_LOG_ERROR("Failed to shutdown provider [Name:%s][code: %d]",
                                    (pProvider->pszProviderName ? pProvider->pszProviderName : ""),
                                    dwError);
            }
        }

        dlclose(pProvider->pLibHandle);
    }

    if (pProvider->pProviderInfo)
    {
        DirectoryFreeProviderInfo(pProvider->pProviderInfo);
    }

    DirectoryFreeMemory(pProvider);
}

VOID
DirectoryFreeProviderInfo(
    PDIRECTORY_PROVIDER_INFO pProviderInfo
    )
{
    if (pProviderInfo->pszProviderPath)
    {
        DirectoryFreeMemory(pProviderInfo->pszProviderPath);
    }

    DirectoryFreeMemory(pProviderInfo);
}
