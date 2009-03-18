#include "includes.h"

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
