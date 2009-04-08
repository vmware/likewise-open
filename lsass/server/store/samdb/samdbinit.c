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
    PCSTR  pszDomainName,
    PCSTR  pszNetBIOSName,
    PSID   *ppMachineSid
    );

static
DWORD
SamDbAddContainer(
    HANDLE             hDirectory,
    PCSTR              pszDomainDN,
    PCSTR              pszGroupName,
    PCSTR              pszGroupSID,
    SAMDB_OBJECT_CLASS objectClass
    );

static
DWORD
SamDbAddLocalDomain(
    HANDLE hDirectory,
    PCSTR  pszDomainDN,
    PCSTR  pszDomainName,
    PCSTR  pszNetBIOSName,
    PCSTR  pszMachineSID
    );

static
DWORD
SamDbAddBuiltinAccounts(
    HANDLE             hDirectory,
    PCSTR              pszDomainDN
    );

static
DWORD
SamDbAddLocalAccounts(
    HANDLE    hDirectory,
    PCSTR     pszDomainDN,
    PSID      pMachineSid
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
                .pfnDirectoryGetUserCount   = &SamDbGetUserCount,
                .pfnDirectoryGetGroupCount  = &SamDbGetGroupCount,
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
    PSID   pMachineSid = NULL;

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
                    pszHostname,
                    &szNetBIOSName[0],
                    &pMachineSid);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddBuiltin(
                    hDirectory,
                    pszDomainDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddBuiltinAccounts(
                    hDirectory,
                    pszDomainDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAddLocalAccounts(
                    hDirectory,
                    pszDomainDN,
                    pMachineSid);
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
    return SamDbAddContainer(
                    hDirectory,
                    pszDomainDN,
                    SAMDB_BUILTIN_TAG,
                    SAMDB_BUILTIN_SID,
                    SAMDB_OBJECT_CLASS_BUILTIN_DOMAIN);
}

static
DWORD
SamDbAddMachineDomain(
    HANDLE hDirectory,
    PCSTR  pszDomainDN,
    PCSTR  pszDomainName,
    PCSTR  pszNetBIOSName,
    PSID   *ppMachineSid
    )
{
    const ULONG ulSubAuthCount = 4;
    DWORD  dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG  ulSidLength = 0;
    PSTR   pszMachineSID = NULL;
    ULONG  ulSubAuth[3];
    PBYTE  pSubAuth = NULL;
    uuid_t GUID;
    PSID   pMachineSid = NULL;
    SID_IDENTIFIER_AUTHORITY AuthId = { SECURITY_NT_AUTHORITY };

    uuid_generate(GUID);

    pSubAuth = (PBYTE)GUID;
    ulSubAuth[0] = *((PULONG)pSubAuth);
    pSubAuth += sizeof(ULONG);
    ulSubAuth[1] = *((PULONG)pSubAuth);
    pSubAuth += sizeof(ULONG);
    ulSubAuth[2] = *((PULONG)pSubAuth);

    ulSidLength = RtlLengthRequiredSid(ulSubAuthCount);

    dwError = LsaAllocateMemory(ulSidLength,
                                (void**)&pMachineSid);
    BAIL_ON_SAMDB_ERROR(dwError);

    status = RtlInitializeSid(pMachineSid,
                              &AuthId,
                              ulSubAuthCount);
    if (status != 0) {
        dwError = LSA_ERROR_SAM_INIT_ERROR;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pMachineSid->SubAuthority[0] = 21;
    pMachineSid->SubAuthority[1] = ulSubAuth[0];
    pMachineSid->SubAuthority[2] = ulSubAuth[1];
    pMachineSid->SubAuthority[3] = ulSubAuth[2];

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
                    pszDomainName,
                    pszNetBIOSName,
                    pszMachineSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppMachineSid = pMachineSid;

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
    PCSTR  pszDomainName,
    PCSTR  pszNetBIOSName,
    PCSTR  pszMachineSID
    )
{
    DWORD     dwError = 0;
    wchar16_t wszAttrNameObjectClass[] = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    wchar16_t wszAttrNameObjectSID[]   = SAM_DB_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameNetBIOSName[] = SAM_DB_DIR_ATTR_NETBIOS_NAME;
    wchar16_t wszAttrNameDomain[]      = SAM_DB_DIR_ATTR_DOMAIN;
    wchar16_t wszAttrNameCommonName[]  = SAM_DB_DIR_ATTR_COMMON_NAME;
    wchar16_t wszAttrNameSamAccountName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    PWSTR     pwszObjectDN    = NULL;
    PWSTR     pwszMachineSID  = NULL;
    PWSTR     pwszDomainName  = NULL;
    PWSTR     pwszNetBIOSName = NULL;
    ATTRIBUTE_VALUE avObjectClass = {0};
    ATTRIBUTE_VALUE avMachineSID  = {0};
    ATTRIBUTE_VALUE avDomainName  = {0};
    ATTRIBUTE_VALUE avNetBIOSName = {0};
    DIRECTORY_MOD mods[7];
    ULONG     iMod = 0;

    memset(mods, 0, sizeof(mods));

    dwError = LsaMbsToWc16s(
                    pszDomainDN,
                    &pwszObjectDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[iMod].pwszAttrName = &wszAttrNameObjectClass[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avObjectClass.Type = DIRECTORY_ATTR_TYPE_INTEGER;
    avObjectClass.data.ulValue = SAMDB_OBJECT_CLASS_DOMAIN;
    mods[iMod].pAttrValues = &avObjectClass;

    dwError = LsaMbsToWc16s(
                    pszMachineSID,
                    &pwszMachineSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[++iMod].pwszAttrName = &wszAttrNameObjectSID[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avMachineSID.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avMachineSID.data.pwszStringValue = pwszMachineSID;
    mods[iMod].pAttrValues = &avMachineSID;

    dwError = LsaMbsToWc16s(
                    pszDomainName,
                    &pwszDomainName);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[++iMod].pwszAttrName = &wszAttrNameDomain[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avDomainName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avDomainName.data.pwszStringValue = pwszDomainName;
    mods[iMod].pAttrValues = &avDomainName;

    mods[++iMod].pwszAttrName = &wszAttrNameCommonName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    mods[iMod].pAttrValues = &avDomainName;

    mods[++iMod].pwszAttrName = &wszAttrNameSamAccountName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    mods[iMod].pAttrValues = &avDomainName;

    dwError = LsaMbsToWc16s(
                    pszNetBIOSName,
                    &pwszNetBIOSName);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[++iMod].pwszAttrName = &wszAttrNameNetBIOSName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avNetBIOSName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avNetBIOSName.data.pwszStringValue = pwszNetBIOSName;
    mods[iMod].pAttrValues = &avNetBIOSName;

    mods[++iMod].pwszAttrName = NULL;
    mods[iMod].ulNumValues = 0;
    mods[iMod].pAttrValues = NULL;

    dwError = SamDbAddObject(
                    hDirectory,
                    pwszObjectDN,
                    mods);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    DIRECTORY_FREE_MEMORY(pwszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszMachineSID);
    DIRECTORY_FREE_MEMORY(pwszDomainName);
    DIRECTORY_FREE_MEMORY(pwszNetBIOSName);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SamDbAddContainer(
    HANDLE             hDirectory,
    PCSTR              pszDomainDN,
    PCSTR              pszName,
    PCSTR              pszSID,
    SAMDB_OBJECT_CLASS objectClass
    )
{
    DWORD dwError = 0;
    wchar16_t wszAttrNameObjectClass[] = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    wchar16_t wszAttrNameObjectSID[] = SAM_DB_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameContainerName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameCommonName[] = SAM_DB_DIR_ATTR_COMMON_NAME;
    PSTR      pszObjectDN = NULL;
    PWSTR     pwszObjectDN = NULL;
    PWSTR     pwszSID = NULL;
    PWSTR     pwszContainerName = NULL;
    ATTRIBUTE_VALUE avContainerName = {0};
    ATTRIBUTE_VALUE avSID = {0};
    ATTRIBUTE_VALUE avObjectClass = {0};
    DIRECTORY_MOD mods[5];
    ULONG     iMod = 0;

    memset(mods, 0, sizeof(mods));

    dwError = LsaAllocateStringPrintf(
                    &pszObjectDN,
                    "CN=%s,%s",
                    pszName,
                    pszDomainDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszObjectDN,
                    &pwszObjectDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszName,
                    &pwszContainerName);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszSID,
                    &pwszSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    mods[iMod].pwszAttrName = &wszAttrNameObjectSID[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avSID.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avSID.data.pwszStringValue = pwszSID;
    mods[iMod].pAttrValues = &avSID;

    mods[++iMod].pwszAttrName = &wszAttrNameObjectClass[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avObjectClass.Type = DIRECTORY_ATTR_TYPE_INTEGER;
    avObjectClass.data.ulValue = objectClass;
    mods[iMod].pAttrValues = &avObjectClass;

    mods[++iMod].pwszAttrName = &wszAttrNameContainerName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avContainerName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avContainerName.data.pwszStringValue = pwszContainerName;
    mods[iMod].pAttrValues = &avContainerName;

    mods[++iMod].pwszAttrName = &wszAttrNameCommonName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    mods[iMod].pAttrValues = &avContainerName;

    dwError = SamDbAddObject(
                    hDirectory,
                    pwszObjectDN,
                    mods);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    DIRECTORY_FREE_STRING(pszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszSID);
    DIRECTORY_FREE_MEMORY(pwszContainerName);

    return dwError;

error:

    goto cleanup;
}


static
DWORD
SamDbAddBuiltinAccounts(
    HANDLE             hDirectory,
    PCSTR              pszDomainDN
    )
{
    struct builtin_account {
        PCSTR               pszName;
        PCSTR               pszSID;
        PCSTR               pszDescription;
        SAMDB_OBJECT_CLASS  objectClass;
    } BuiltinAccounts[] = {
        {
            .pszName        = "Administrators",
            .pszSID         = "S-1-5-32-544",
            .pszDescription = "Administrators have complete and unrestricted "
                              "access to the computer/domain",
            .objectClass    = SAMDB_OBJECT_CLASS_GROUP
        },
        {
            .pszName        = "Guests",
            .pszSID         = "S-1-5-32-544",
            .pszDescription = "Guests have the same access as members of the "
                              "Users group by default, except for the Guest "
                              "account which is further restricted",
            .objectClass    = SAMDB_OBJECT_CLASS_GROUP
        }
    };

    DWORD dwError = 0;
    wchar16_t wszAttrNameObjectClass[] = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    wchar16_t wszAttrNameObjectSID[] = SAM_DB_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameSamAccountName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameCommonName[] = SAM_DB_DIR_ATTR_COMMON_NAME;
    wchar16_t wszAttrNameDescription[] = SAM_DB_DIR_ATTR_DESCRIPTION;
    PCSTR     pszName = NULL;
    PCSTR     pszSID = NULL;
    PCSTR     pszDescription = NULL;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;
    PSTR      pszObjectDN = NULL;
    PWSTR     pwszObjectDN = NULL;
    PWSTR     pwszSamAccountName = NULL;
    PWSTR     pwszSID = NULL;
    PWSTR     pwszDescription = NULL;
    ATTRIBUTE_VALUE avGroupName = {0};
    ATTRIBUTE_VALUE avSID = {0};
    ATTRIBUTE_VALUE avObjectClass = {0};
    ATTRIBUTE_VALUE avDescription = {0};
    DIRECTORY_MOD mods[6];
    ULONG     iMod = 0;
    DWORD     i = 0;

    for (i = 0; i < sizeof(BuiltinAccounts)/sizeof(BuiltinAccounts[0]); i++) {

        pszName        = BuiltinAccounts[i].pszName;
        pszSID         = BuiltinAccounts[i].pszSID;
        pszDescription = BuiltinAccounts[i].pszDescription;
        objectClass    = BuiltinAccounts[i].objectClass;

        iMod = 0;
        memset(mods, 0, sizeof(mods));

        dwError = LsaAllocateStringPrintf(
                        &pszObjectDN,
                        "CN=%s,CN=Builtin,%s",
                        pszName,
                        pszDomainDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        pszObjectDN,
                        &pwszObjectDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        pszName,
                        &pwszSamAccountName);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        pszSID,
                        &pwszSID);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        pszDescription,
                        &pwszDescription);
        BAIL_ON_SAMDB_ERROR(dwError);

        mods[iMod].pwszAttrName = &wszAttrNameObjectSID[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avSID.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avSID.data.pwszStringValue = pwszSID;
        mods[iMod].pAttrValues = &avSID;

        mods[++iMod].pwszAttrName = &wszAttrNameObjectClass[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avObjectClass.Type = DIRECTORY_ATTR_TYPE_INTEGER;
        avObjectClass.data.ulValue = objectClass;
        mods[iMod].pAttrValues = &avObjectClass;

        mods[++iMod].pwszAttrName = &wszAttrNameSamAccountName[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avGroupName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avGroupName.data.pwszStringValue = pwszSamAccountName;
        mods[iMod].pAttrValues = &avGroupName;

        mods[++iMod].pwszAttrName = &wszAttrNameCommonName[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        mods[iMod].pAttrValues = &avGroupName;

        avDescription.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avDescription.data.pwszStringValue = pwszDescription;
        mods[++iMod].pwszAttrName = &wszAttrNameDescription[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        mods[iMod].pAttrValues = &avDescription;

        mods[++iMod].pwszAttrName = NULL;
        mods[iMod].pAttrValues = NULL;

        dwError = SamDbAddObject(
                        hDirectory,
                        pwszObjectDN,
                        mods);
        BAIL_ON_SAMDB_ERROR(dwError);

        DIRECTORY_FREE_STRING_AND_RESET(pszObjectDN);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszObjectDN);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszSamAccountName);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszSID);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszDescription);
    }

cleanup:

    DIRECTORY_FREE_STRING(pszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszSamAccountName);
    DIRECTORY_FREE_MEMORY(pwszSID);
    DIRECTORY_FREE_MEMORY(pwszDescription);

    return dwError;

error:

    goto cleanup;
}


static
DWORD
SamDbAddLocalAccounts(
    HANDLE    hDirectory,
    PCSTR     pszDomainDN,
    PSID      pMachineSid
    )
{
    struct local_account {
        PCSTR               pszName;
        DWORD               dwRid;
        PCSTR               pszDescription;
        SAMDB_OBJECT_CLASS  objectClass;
    } LocalAccounts[] = {
        {
            .pszName        = "Administrator",
            .dwRid          = DOMAIN_USER_RID_ADMIN,
            .pszDescription = "Built-in account for administering the "
                              "computer/domain",
            .objectClass    = SAMDB_OBJECT_CLASS_USER
        },
        {
            .pszName        = "Guest",
            .dwRid          = DOMAIN_USER_RID_GUEST,
            .pszDescription = "Built-in account for guest access to the "
                              "computer/domain",
            .objectClass    = SAMDB_OBJECT_CLASS_USER
        }
    };

    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t wszAttrNameObjectClass[] = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    wchar16_t wszAttrNameObjectSID[] = SAM_DB_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameSamAccountName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameCommonName[] = SAM_DB_DIR_ATTR_COMMON_NAME;
    wchar16_t wszAttrNameDescription[] = SAM_DB_DIR_ATTR_DESCRIPTION;
    PCSTR     pszName = NULL;
    DWORD     dwRid = 0;
    PCSTR     pszDescription = NULL;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;
    PSTR      pszObjectDN = NULL;
    PSID      pUserSid = NULL;
    ULONG     ulUserSidLength = 0;
    PWSTR     pwszSamAccountName = NULL;
    PWSTR     pwszObjectDN = NULL;
    PWSTR     pwszSID = NULL;
    PWSTR     pwszDescription = NULL;
    ATTRIBUTE_VALUE avGroupName = {0};
    ATTRIBUTE_VALUE avSID = {0};
    ATTRIBUTE_VALUE avObjectClass = {0};
    ATTRIBUTE_VALUE avDescription = {0};
    DIRECTORY_MOD mods[6];
    ULONG     iMod = 0;
    DWORD     i = 0;

    for (i = 0; i < sizeof(LocalAccounts)/sizeof(LocalAccounts[0]); i++) {

        pszName        = LocalAccounts[i].pszName;
        dwRid          = LocalAccounts[i].dwRid;
        pszDescription = LocalAccounts[i].pszDescription;
        objectClass    = LocalAccounts[i].objectClass;

        iMod    = 0;
        memset(mods, 0, sizeof(mods));

        ulUserSidLength = RtlLengthRequiredSid(
                                  pMachineSid->SubAuthorityCount + 1);
        dwError = LsaAllocateMemory(ulUserSidLength, (void**)&pUserSid);
        BAIL_ON_SAMDB_ERROR(dwError);

        status = RtlCopySid(ulUserSidLength, pUserSid, pMachineSid);
        if (status != 0) {
            dwError = LSA_ERROR_SAM_INIT_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        status = RtlAppendRidSid(ulUserSidLength, pUserSid, dwRid);
        if (status != 0) {
            dwError = LSA_ERROR_SAM_INIT_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        dwError = LsaAllocateStringPrintf(
                        &pszObjectDN,
                        "CN=%s,CN=Users,%s",
                        pszName,
                        pszDomainDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        pszObjectDN,
                        &pwszObjectDN);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        pszName,
                        &pwszSamAccountName);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        pszDescription,
                        &pwszDescription);
        BAIL_ON_SAMDB_ERROR(dwError);

        status = RtlAllocateWC16StringFromSid(
                        &pwszSID,
                        pUserSid);
        if (status != 0) {
            dwError = LSA_ERROR_SAM_INIT_ERROR;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        mods[iMod].pwszAttrName = &wszAttrNameObjectSID[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avSID.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avSID.data.pwszStringValue = pwszSID;
        mods[iMod].pAttrValues = &avSID;

        mods[++iMod].pwszAttrName = &wszAttrNameObjectClass[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avObjectClass.Type = DIRECTORY_ATTR_TYPE_INTEGER;
        avObjectClass.data.ulValue = objectClass;
        mods[iMod].pAttrValues = &avObjectClass;

        mods[++iMod].pwszAttrName = &wszAttrNameSamAccountName[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avGroupName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avGroupName.data.pwszStringValue = pwszSamAccountName;
        mods[iMod].pAttrValues = &avGroupName;

        mods[++iMod].pwszAttrName = &wszAttrNameCommonName[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        mods[iMod].pAttrValues = &avGroupName;

        mods[++iMod].pwszAttrName = &wszAttrNameDescription[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avDescription.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avDescription.data.pwszStringValue = pwszDescription;
        mods[iMod].pAttrValues = &avDescription;

        mods[++iMod].pwszAttrName = NULL;
        mods[iMod].pAttrValues = NULL;

        dwError = SamDbAddObject(
                        hDirectory,
                        pwszObjectDN,
                        mods);
        BAIL_ON_SAMDB_ERROR(dwError);

        if (pszObjectDN) {
            DIRECTORY_FREE_STRING(pszObjectDN);
            pszObjectDN = NULL;
        }

        if (pwszObjectDN) {
            DIRECTORY_FREE_MEMORY(pwszObjectDN);
            pwszObjectDN = NULL;
        }

        if (pwszSamAccountName) {
            DIRECTORY_FREE_MEMORY(pwszSamAccountName);
            pwszSamAccountName = NULL;
        }

        if (pwszDescription) {
            DIRECTORY_FREE_MEMORY(pwszDescription);
            pwszDescription = NULL;
        }

        if (pwszSID) {
            RTL_FREE(&pwszSID);
            pwszSID = NULL;
        }

        if (pUserSid) {
            DIRECTORY_FREE_MEMORY(pUserSid);
            pUserSid = NULL;
        }
    }

cleanup:
    if (pszObjectDN) {
        DIRECTORY_FREE_STRING(pszObjectDN);
    }

    if (pwszObjectDN) {
        DIRECTORY_FREE_MEMORY(pwszObjectDN);
    }

    if (pwszSamAccountName) {
        DIRECTORY_FREE_MEMORY(pwszSamAccountName);
    }

    if (pwszSID) {
        RTL_FREE(&pwszSID);
    }

    if (pUserSid) {
        LSA_SAFE_FREE_MEMORY(pUserSid);
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
