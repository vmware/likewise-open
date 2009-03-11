#include "includes.h"

DWORD
DirectoryInitializeProvider(
    PCSTR pszConfigFilePath,
    PSTR* ppszProviderName,
    PDIRECTORY_PROVIDER_FUNCTION_TABLE* ppFnTable
    )
{
    DWORD dwError = 0;
    DIRECTORY_PROVIDER_FUNCTION_TABLE providerAPITable =
        {
                .pfnDirectoryOpen   = &SamDbOpen,
                .pfnDirectoryBind   = &SamDbBind,
                .pfnDirectoryAdd    = &SamDbAddObject,
                .pfnDirectoryModify = &SamDbModifyObject,
                .pfnDirectoryDelete = &SamDbDeleteObject,
                .pfnDirectorySearch = &SamDbSearchObject,
                .pfnDirectoryClose  = &SamDbClose
        };

    gSamGlobals.pszProviderName = "Likewise SAM Local Database";
    gSamGlobals.providerFunctionTable = providerAPITable;

    dwError = SamDbBuildDbInstanceLock(&gSamGlobals.pDbInstanceLock);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppszProviderName = gSamGlobals.pszProviderName;
    *ppFnTable = &gSamGlobals.providerFunctionTable;

error:

    return dwError;
}

DWORD
DirectoryShutdownProvider(
    PSTR pszProviderName,
    PDIRECTORY_PROVIDER_FUNCTION_TABLE pFnTable
    )
{
    DWORD dwError = 0;

    if (gSamGlobals.pDbInstanceLock)
    {
        SamDbReleaseDbInstanceLock(gSamGlobals.pDbInstanceLock);
        gSamGlobals.pDbInstanceLock = NULL;
    }

    return dwError;
}


