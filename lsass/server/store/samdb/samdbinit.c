#include "includes.h"

#define SAMDB_BUILTIN_TAG    "BUILTIN"
#define SAMDB_BUILTIN_TAG_L L"BUILTIN"
#define SAMDB_BUILTIN_SID    "S-1-22"
#define SAMDB_BUILTIN_SID_L L"S-1-22"

static
DWORD
SamDbAddDefaultEntries(
    HANDLE hDirectory
    );

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

    dwError = SamDbBuildAttributeLookup(&gSamGlobals.pAttrLookup);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbInit();
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppszProviderName = gSamGlobals.pszProviderName;
    *ppFnTable = &gSamGlobals.providerFunctionTable;

cleanup:

    return dwError;

error:

    *ppszProviderName = NULL;
    *ppFnTable = NULL;

    goto cleanup;
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

DWORD
SamDbInit(
    VOID
    )
{
    DWORD dwError = 0;
    HANDLE hDirectory = (HANDLE)NULL;
    PSAM_DIRECTORY_CONTEXT pDirectory = NULL;
    BOOLEAN bExists = FALSE;

    dwError = LsaCheckFileExists(
                    SAM_DB,
                    &bExists);
    BAIL_ON_SAMDB_ERROR(dwError);

    // TODO: Implement an upgrade scenario
    if (bExists)
    {
       goto cleanup;
    }

    dwError = LsaCheckDirectoryExists(
                    SAM_DB_DIR,
                    &bExists);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (!bExists)
    {
        mode_t mode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        /* Allow go+rx to the base folder */
        dwError = LsaCreateDirectory(SAM_DB_DIR, mode);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    /* restrict access to u+rwx to the db folder */
    dwError = LsaChangeOwnerAndPermissions(
                    SAM_DB_DIR,
                    0,
                    0,
                    S_IRWXU);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbOpen(&hDirectory);
    BAIL_ON_SAMDB_ERROR(dwError);

    pDirectory = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    dwError = SamDbInitDomainTable(pDirectory->pDbContext);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbInitGroupTable(pDirectory->pDbContext);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbInitUserTable(pDirectory->pDbContext);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddDefaultEntries(pDirectory);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaChangeOwnerAndPermissions(
                    SAM_DB,
                    0,
                    0,
                    S_IRWXU);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    if (hDirectory)
    {
        SamDbClose(hDirectory);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbAddDefaultEntries(
    HANDLE hDirectory
    )
{
    DWORD dwError = 0;

    return dwError;
}
