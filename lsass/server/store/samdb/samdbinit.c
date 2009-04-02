#include "includes.h"

#define SAMDB_BUILTIN_TAG    "BUILTIN"
#define SAMDB_BUILTIN_GID    32
#define SAMDB_BUILTIN_SID    "S-1-5-32"

static
DWORD
SamDbCreateTables(
    PSAM_DB_CONTEXT pDbContext
    );

static
DWORD
SamDbAddDefaultEntries(
    HANDLE hDirectory
    );

static
DWORD
SamDbAddBuiltin(
    HANDLE hDirectory,
    PCSTR  pszDomainDN
    );

static
DWORD
SamDbAddMachineDomain(
    HANDLE hDirectory,
    PCSTR  pszDomainDN,
    PCSTR  pszNetBIOSName
    );

static
DWORD
SamDbAddLocalGroup(
    HANDLE hDirectory,
    PCSTR  pszDomainDN,
    PCSTR  pszGroupName,
    PCSTR  pszGroupSID,
    DWORD  dwGID
    );

static
DWORD
SamDbAddLocalDomain(
    HANDLE hDirectory,
    PCSTR  pszDomainDN,
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
                .pfnDirectoryOpen           = &SamDbOpen,
                .pfnDirectoryBind           = &SamDbBind,
                .pfnDirectoryAdd            = &SamDbAddObject,
                .pfnDirectoryModify         = &SamDbModifyObject,
                .pfnDirectorySetPassword    = &SamDbSetPassword,
                .pfnDirectoryChangePassword = &SamDbChangePassword,
                .pfnDirectoryVerifyPassword = &SamDbVerifyPassword,
                .pfnDirectoryDelete         = &SamDbDeleteObject,
                .pfnDirectorySearch         = &SamDbSearchObject,
                .pfnDirectoryClose          = &SamDbClose
        };

    gSamGlobals.pszProviderName = "Likewise SAM Local Database";
    gSamGlobals.providerFunctionTable = providerAPITable;

    dwError = SamDbAttributeLookupInitContents(
                &gSamGlobals.attrLookup,
                gSamGlobals.pAttrMaps,
                gSamGlobals.dwNumMaps);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbBuildDbInstanceLock(&gSamGlobals.pDbInstanceLock);
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

    SamDbAttributeLookupFreeContents(&gSamGlobals.attrLookup);

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

    dwError = SamDbCreateTables(pDirectory->pDbContext);
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
SamDbCreateTables(
    PSAM_DB_CONTEXT pDbContext
    )
{
    DWORD dwError = 0;
    PSTR pszError = NULL;

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    SAM_DB_QUERY_CREATE_TABLES,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    return dwError;

error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

static
DWORD
SamDbAddDefaultEntries(
    HANDLE hDirectory
    )
{
    DWORD  dwError = 0;
    PSTR   pszHostname = NULL;
    PSTR   pszDomainDN = NULL;
    CHAR   szNetBIOSName[16];

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_SAMDB_ERROR(dwError);

    LsaStrToUpper(pszHostname);

    memset(szNetBIOSName, 0, sizeof(szNetBIOSName));
    strncpy(szNetBIOSName, pszHostname, 15);

    dwError = SamDbInitConfig(hDirectory);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                    &pszDomainDN,
                    "DC=%s",
                    pszHostname);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddMachineDomain(
                    hDirectory,
                    pszDomainDN,
                    &szNetBIOSName[0]);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddBuiltin(
                    hDirectory,
                    pszDomainDN);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    DIRECTORY_FREE_STRING(pszHostname);
    DIRECTORY_FREE_STRING(pszDomainDN);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbAddBuiltin(
    HANDLE hDirectory,
    PCSTR  pszDomainDN
    )
{
    return SamDbAddLocalGroup(
                    hDirectory,
                    pszDomainDN,
                    SAMDB_BUILTIN_TAG,
                    SAMDB_BUILTIN_SID,
                    SAMDB_BUILTIN_GID);
}

static
DWORD
SamDbAddMachineDomain(
    HANDLE hDirectory,
    PCSTR  pszDomainDN,
    PCSTR  pszNetBIOSName
    )
{
    DWORD  dwError = 0;
    PSTR   pszMachineSID = NULL;
    ULONG  ulSubAuth[3];
    PBYTE  pSubAuth = NULL;
    uuid_t GUID;

    uuid_generate(GUID);

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
                    pszDomainDN,
                    pszNetBIOSName,
                    pszMachineSID);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

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
    PCSTR  pszDomainDN,
    PCSTR  pszNetBIOSName,
    PCSTR  pszMachineSID
    )
{
    DWORD     dwError = 0;
    PWSTR     pwszObjectDN = NULL;
    wchar16_t wszAttrNameObjectSID[] = SAM_DB_DIR_ATTR_OBJECT_SID;
    PWSTR     pwszMachineSID = NULL;
    PWSTR     pwszNetBIOSName = NULL;
    wchar16_t wszAttrNameNetBIOSName[] = SAM_DB_DIR_ATTR_NETBIOS_NAME;
    ATTRIBUTE_VALUE avMachineSID = {0};
    ATTRIBUTE_VALUE avNetBIOSName = {0};
    DIRECTORY_MOD mods[3];
    ULONG     iMod = 0;

    memset(mods, 0, sizeof(mods));

    dwError = LsaMbsToWc16s(
                    pszDomainDN,
                    &pwszObjectDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszMachineSID,
                    &pwszMachineSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[iMod].pwszAttrName = &wszAttrNameObjectSID[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avMachineSID.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avMachineSID.pwszStringValue = pwszMachineSID;
    mods[iMod].pAttrValues = &avMachineSID;

    dwError = LsaMbsToWc16s(
                    pszNetBIOSName,
                    &pwszNetBIOSName);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[++iMod].pwszAttrName = &wszAttrNameNetBIOSName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avNetBIOSName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avNetBIOSName.pwszStringValue = pwszNetBIOSName;
    mods[iMod].pAttrValues = &avNetBIOSName;

    dwError = SamDbAddObject(
                    hDirectory,
                    pwszObjectDN,
                    mods);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    DIRECTORY_FREE_MEMORY(pwszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszMachineSID);
    DIRECTORY_FREE_MEMORY(pwszNetBIOSName);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbAddLocalGroup(
    HANDLE hDirectory,
    PCSTR  pszDomainDN,
    PCSTR  pszGroupName,
    PCSTR  pszGroupSID,
    DWORD  dwGID
    )
{
    DWORD dwError = 0;
    wchar16_t wszAttrNameObjectSID[] = SAM_DB_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameGroupName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameGID[]       = SAM_DB_DIR_ATTR_GID;
    PSTR      pszObjectDN = NULL;
    PWSTR     pwszObjectDN = NULL;
    PWSTR     pwszGroupSID = NULL;
    PWSTR     pwszGroupName = NULL;
    ATTRIBUTE_VALUE avGroupName = {0};
    ATTRIBUTE_VALUE avGroupSID = {0};
    ATTRIBUTE_VALUE avGID = {0};
    DIRECTORY_MOD mods[4];
    ULONG     iMod = 0;

    memset(mods, 0, sizeof(mods));

    dwError = LsaAllocateStringPrintf(
                    &pszObjectDN,
                    "CN=%s,%s",
                    pszGroupName,
                    pszDomainDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszObjectDN,
                    &pwszObjectDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszGroupName,
                    &pwszGroupName);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszGroupSID,
                    &pwszGroupSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[iMod].pwszAttrName = &wszAttrNameObjectSID[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avGroupSID.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avGroupSID.pwszStringValue = pwszGroupSID;
    mods[iMod].pAttrValues = &avGroupSID;

    mods[++iMod].pwszAttrName = &wszAttrNameGroupName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avGroupName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avGroupName.pwszStringValue = pwszGroupName;
    mods[iMod].pAttrValues = &avGroupName;

    mods[++iMod].pwszAttrName = &wszAttrNameGID[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avGID.Type = DIRECTORY_ATTR_TYPE_INTEGER;
    avGID.ulValue = dwGID;
    mods[iMod].pAttrValues = &avGID;

    dwError = SamDbAddObject(
                    hDirectory,
                    pwszObjectDN,
                    mods);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    DIRECTORY_FREE_STRING(pszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszGroupSID);
    DIRECTORY_FREE_MEMORY(pwszGroupName);

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
