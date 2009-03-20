#include "includes.h"

#define SAMDB_BUILTIN_TAG    "BUILTIN"
#define SAMDB_BUILTIN_SID    "S-1-5-32"

static
DWORD
SamDbAddDefaultEntries(
    HANDLE hDirectory
    );

static
DWORD
SamDbAddBuiltinDomain(
    HANDLE hDirectory
    );

static
DWORD
SamDbAddMachineDomain(
    HANDLE hDirectory
    );

static
DWORD
SamDbAddLocalDomain(
    HANDLE hDirectory,
    PCSTR  pszDomainName,
    PCSTR  pszNetBIOSName,
    PCSTR  pszMachineSID
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
    DWORD     dwError = 0;

    dwError = SamDbAddBuiltinDomain(hDirectory);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddMachineDomain(hDirectory);
    BAIL_ON_SAMDB_ERROR(dwError);

error:

    return dwError;
}

static
DWORD
SamDbAddBuiltinDomain(
    HANDLE hDirectory
    )
{
    return SamDbAddLocalDomain(
                    hDirectory,
                    SAMDB_BUILTIN_TAG,
                    SAMDB_BUILTIN_TAG,
                    SAMDB_BUILTIN_SID);
}

static
DWORD
SamDbAddMachineDomain(
    HANDLE hDirectory
    )
{
    DWORD  dwError = 0;
    PSTR   pszHostname = NULL;
    PSTR   pszMachineSID = NULL;
    CHAR   szNetBIOSName[16];
    ULONG  ulSubAuth[3];
    PBYTE  pSubAuth = NULL;
    uuid_t GUID;

    uuid_generate(GUID);

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_SAMDB_ERROR(dwError);

    LsaStrToUpper(pszHostname);

    memset(szNetBIOSName, 0, sizeof(szNetBIOSName));
    strncpy(szNetBIOSName, pszHostname, 15);

    pSubAuth = (PBYTE)GUID;
    ulSubAuth[0] = *((PULONG)pSubAuth);
    pSubAuth += sizeof(ULONG);
    ulSubAuth[1] = *((PULONG)pSubAuth);
    pSubAuth += sizeof(ULONG);
    ulSubAuth[2] = *((PULONG)pSubAuth);

    dwError = LsaAllocateStringPrintf(
                    &pszMachineSID,
                    "S-1-5-21-%lu-%lu-%lu",
                    ulSubAuth[0],
                    ulSubAuth[1],
                    ulSubAuth[2]);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddLocalDomain(
                    hDirectory,
                    pszHostname,
                    szNetBIOSName,
                    pszMachineSID);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    if (pszHostname)
    {
        DirectoryFreeMemory(pszHostname);
    }
    if (pszMachineSID)
    {
        LsaFreeMemory(pszMachineSID);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbAddLocalDomain(
    HANDLE hDirectory,
    PCSTR  pszDomainName,
    PCSTR  pszNetBIOSName,
    PCSTR  pszMachineSID
    )
{
    DWORD     dwError = 0;
    PWSTR     pwszObjectName = NULL;
    PWSTR     pwszMachineSID = NULL;
    PWSTR     pwszNetBIOSName = NULL;
    ATTRIBUTE_VALUE avMachineSID = {0};
    ATTRIBUTE_VALUE avNetBIOSName = {0};
    DIRECTORY_MOD mods[3];
    ULONG     iMod = 0;

    memset(mods, 0, sizeof(mods));

    dwError = LsaMbsToWc16s(
                    pszDomainName,
                    &pwszObjectName);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszMachineSID,
                    &pwszMachineSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszNetBIOSName,
                    &pwszNetBIOSName);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    DIRECTORY_ATTR_TAG_DOMAIN_SID,
                    &mods[0].pwszAttributeName);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[0].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[0].ulNumValues = 1;
    avMachineSID.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avMachineSID.pwszStringValue = pwszMachineSID;
    mods[0].pAttributeValues = &avMachineSID;

    dwError = LsaMbsToWc16s(
                    DIRECTORY_ATTR_TAG_DOMAIN_NETBIOS_NAME,
                    &mods[1].pwszAttributeName);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[1].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[1].ulNumValues = 1;
    avNetBIOSName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avNetBIOSName.pwszStringValue = pwszNetBIOSName;
    mods[1].pAttributeValues = &avNetBIOSName;

    dwError = SamDbAddDomain(
                    hDirectory,
                    pwszObjectName,
                    mods);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    if (pwszObjectName)
    {
        DirectoryFreeMemory(pwszObjectName);
    }
    if (pwszMachineSID)
    {
        DirectoryFreeMemory(pwszMachineSID);
    }
    if (pwszNetBIOSName)
    {
        DirectoryFreeMemory(pwszNetBIOSName);
    }
    for (iMod = 0; iMod < sizeof(mods)/sizeof(mods[0]); iMod++)
    {
        if (mods[iMod].pwszAttributeName)
        {
            DirectoryFreeMemory(mods[iMod].pwszAttributeName);
        }
    }

    return dwError;

error:

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
