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
    PCSTR              pszNetBIOSName,
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
    PCSTR  pszMachineSID,
    LONG64 llMaxPwdAge,
    LONG64 llPwdChangeTime
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
                .pfnDirectoryOpen            = &SamDbOpen,
                .pfnDirectoryBind            = &SamDbBind,
                .pfnDirectoryAdd             = &SamDbAddObject,
                .pfnDirectoryModify          = &SamDbModifyObject,
                .pfnDirectorySetPassword     = &SamDbSetPassword,
                .pfnDirectoryChangePassword  = &SamDbChangePassword,
                .pfnDirectoryVerifyPassword  = &SamDbVerifyPassword,
                .pfnDirectoryGetGroupMembers = &SamDbGetGroupMembers,
                .pfnDirectoryGetMemberships  = &SamDbGetUserMemberships,
                .pfnDirectoryAddToGroup      = &SamDbAddToGroup,
                .pfnDirectoryRemoveFromGroup = &SamDbRemoveFromGroup,
                .pfnDirectoryDelete          = &SamDbDeleteObject,
                .pfnDirectorySearch          = &SamDbSearchObject,
                .pfnDirectoryGetUserCount    = &SamDbGetUserCount,
                .pfnDirectoryGetGroupCount   = &SamDbGetGroupCount,
                .pfnDirectoryClose           = &SamDbClose
        };

    gSamGlobals.pszProviderName = "Likewise SAM Local Database";
    gSamGlobals.providerFunctionTable = providerAPITable;

    dwError = SamDbAttributeLookupInitContents(
                &gSamGlobals.attrLookup,
                gSamGlobals.pAttrMaps,
                gSamGlobals.dwNumMaps);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbInit();
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppszProviderName = gSamGlobals.pszProviderName;
    *ppFnTable = &gSamGlobals.providerFunctionTable;

    gSamGlobals.dwNumMaxDbContexts = SAM_DB_CONTEXT_POOL_MAX_ENTRIES;

    pthread_rwlock_init(&gSamGlobals.rwLock, NULL);
    gSamGlobals.pRwLock = &gSamGlobals.rwLock;

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

    pthread_mutex_lock(&gSamGlobals.mutex);

    SamDbAttributeLookupFreeContents(&gSamGlobals.attrLookup);

    while (gSamGlobals.pDbContextList)
    {
        PSAM_DB_CONTEXT pDbContext = gSamGlobals.pDbContextList;

        gSamGlobals.pDbContextList = gSamGlobals.pDbContextList->pNext;

        SamDbFreeDbContext(pDbContext);
    }

    // Set this so that any further pending contexts will get freed upon release
    gSamGlobals.dwNumDbContexts = gSamGlobals.dwNumMaxDbContexts;

    pthread_mutex_unlock(&gSamGlobals.mutex);

    if (gSamGlobals.pRwLock)
    {
        pthread_rwlock_destroy(&gSamGlobals.rwLock);
        gSamGlobals.pRwLock = NULL;
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
    PCSTR  pszDbDirPath = SAM_DB_DIR;
    PCSTR  pszDbPath = SAM_DB;
    BOOLEAN bExists = FALSE;

    dwError = LsaCheckFileExists(
                    pszDbPath,
                    &bExists);
    BAIL_ON_SAMDB_ERROR(dwError);

    // TODO: Implement an upgrade scenario
    if (bExists)
    {
       goto cleanup;
    }

    dwError = LsaCheckDirectoryExists(
                    pszDbDirPath,
                    &bExists);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (!bExists)
    {
        mode_t mode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        /* Allow go+rx to the base folder */
        dwError = LsaCreateDirectory(pszDbDirPath, mode);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    /* restrict access to u+rwx to the db folder */
    dwError = LsaChangeOwnerAndPermissions(
                    pszDbDirPath,
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
                    pszDbPath,
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
    PCSTR pszQuery = SAM_DB_QUERY_CREATE_TABLES;

    SAMDB_LOG_DEBUG("Query used to create tables [%s]", pszQuery);

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    pszQuery,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    return dwError;

error:

    SAMDB_LOG_DEBUG("Sqlite3 Error (code: %d): %s",
                dwError,
                LSA_SAFE_LOG_STRING(pszError));

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
    LONG64 llMaxPwdAge = SAMDB_MAX_PWD_AGE_DEFAULT * 10000000LL;
    LONG64 llPwdPromptTime = SAMDB_PASSWD_PROMPT_TIME_DEFAULT * 10000000LL;

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
                    pszMachineSID,
                    llMaxPwdAge,
                    llPwdPromptTime);
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
    PCSTR  pszMachineSID,
    LONG64 llMaxPwdAge,
    LONG64 llPwdChangeTime
    )
{
    DWORD     dwError = 0;
    wchar16_t wszAttrNameObjectClass[]    = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    wchar16_t wszAttrNameObjectSID[]      = SAM_DB_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameNetBIOSName[]    = SAM_DB_DIR_ATTR_NETBIOS_NAME;
    wchar16_t wszAttrNameDomain[]         = SAM_DB_DIR_ATTR_DOMAIN;
    wchar16_t wszAttrNameCommonName[]     = SAM_DB_DIR_ATTR_COMMON_NAME;
    wchar16_t wszAttrNameSamAccountName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameMaxPwdAge[]      = SAM_DB_DIR_ATTR_MAX_PWD_AGE;
    wchar16_t wszAttrNamePwdChangeTime[]  = SAM_DB_DIR_ATTR_PWD_PROMPT_TIME;
    PWSTR     pwszObjectDN    = NULL;
    PWSTR     pwszMachineSID  = NULL;
    PWSTR     pwszDomainName  = NULL;
    PWSTR     pwszNetBIOSName = NULL;
    ATTRIBUTE_VALUE avObjectClass = {0};
    ATTRIBUTE_VALUE avMachineSID  = {0};
    ATTRIBUTE_VALUE avDomainName  = {0};
    ATTRIBUTE_VALUE avNetBIOSName = {0};
    ATTRIBUTE_VALUE avMaxPwdAge   = {0};
    ATTRIBUTE_VALUE avPwdChangeTime = {0};
    DIRECTORY_MOD mods[9];
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

    mods[++iMod].pwszAttrName = &wszAttrNameMaxPwdAge[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avMaxPwdAge.Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER;
    avMaxPwdAge.data.llValue = llMaxPwdAge;
    mods[iMod].pAttrValues = &avMaxPwdAge;

    mods[++iMod].pwszAttrName = &wszAttrNamePwdChangeTime[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avPwdChangeTime.Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER;
    avPwdChangeTime.data.llValue = llPwdChangeTime;
    mods[iMod].pAttrValues = &avPwdChangeTime;

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
    PCSTR              pszNetBIOSName,
    PCSTR              pszName,
    PCSTR              pszSID,
    SAMDB_OBJECT_CLASS objectClass
    )
{
    DWORD dwError = 0;
    wchar16_t wszAttrNameObjectClass[] = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    wchar16_t wszAttrNameObjectSID[] = SAM_DB_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameContainerName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameDomainName[] = SAM_DB_DIR_ATTR_DOMAIN;
    wchar16_t wszAttrNameNetBIOSName[] = SAM_DB_DIR_ATTR_NETBIOS_NAME;
    wchar16_t wszAttrNameCommonName[] = SAM_DB_DIR_ATTR_COMMON_NAME;
    PSTR      pszObjectDN = NULL;
    PWSTR     pwszObjectDN = NULL;
    PWSTR     pwszSID = NULL;
    PWSTR     pwszDomainName = NULL;
    PWSTR     pwszNetBIOSName = NULL;
    ATTRIBUTE_VALUE avContainerName = {0};
    ATTRIBUTE_VALUE avSID = {0};
    ATTRIBUTE_VALUE avObjectClass = {0};
    ATTRIBUTE_VALUE avDomainName = {0};
    ATTRIBUTE_VALUE avNetBIOSName = {0};
    DIRECTORY_MOD mods[7];
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
                    &pwszDomainName);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszNetBIOSName,
                    &pwszNetBIOSName);
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

    mods[++iMod].pwszAttrName = &wszAttrNameDomainName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avDomainName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avDomainName.data.pwszStringValue = pwszDomainName;
    mods[iMod].pAttrValues = &avDomainName;

    mods[++iMod].pwszAttrName = &wszAttrNameNetBIOSName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avNetBIOSName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avNetBIOSName.data.pwszStringValue = pwszNetBIOSName;
    mods[iMod].pAttrValues = &avNetBIOSName;

    mods[++iMod].pwszAttrName = &wszAttrNameContainerName[0];
    mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
    mods[iMod].ulNumValues = 1;
    avContainerName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
    avContainerName.data.pwszStringValue = pwszDomainName;
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
    DIRECTORY_FREE_MEMORY(pwszDomainName);
    DIRECTORY_FREE_MEMORY(pwszNetBIOSName);

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
        DWORD               dwGID;
        PCSTR               pszDescription;
        PCSTR               pszDomainName;
        PCSTR               pszNetBIOSDomain;
        SAMDB_ACB           flags;
        SAMDB_OBJECT_CLASS  objectClass;
    } BuiltinAccounts[] = {
        {
            .pszName        = "Administrators",
            .pszSID         = "S-1-5-32-544",
            .dwGID          = 544,
            .pszDescription = "Administrators have complete and unrestricted "
                              "access to the computer/domain",
            .pszDomainName  = "BUILTIN",
            .pszNetBIOSDomain = "BUILTIN",
            .flags          = 0,
            .objectClass    = SAMDB_OBJECT_CLASS_GROUP
        },
        {
            .pszName        = "Users",
            .pszSID         = "S-1-5-32-545",
            .dwGID          = 545,
            .pszDescription = "Users are prevented from making accidental "
                              "or intentional system-wide changes. Thus, "
                              "users can run certified applications, but not "
                              "most legacy applications",
            .pszDomainName  = "BUILTIN",
            .pszNetBIOSDomain = "BUILTIN",
            .flags          = 0,
            .objectClass    = SAMDB_OBJECT_CLASS_GROUP
        },
        {
            .pszName        = "Guests",
            .pszSID         = "S-1-5-32-546",
            .dwGID          = 546,
            .pszDescription = "Guests have the same access as members of the "
                              "Users group by default, except for the Guest "
                              "account which is further restricted",
            .pszDomainName  = "BUILTIN",
            .pszNetBIOSDomain = "BUILTIN",
            .flags          = 0,
            .objectClass    = SAMDB_OBJECT_CLASS_GROUP
        },
    };

    DWORD dwError = 0;
    wchar16_t wszAttrNameObjectClass[]    = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    wchar16_t wszAttrNameObjectSID[]      = SAM_DB_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameGID[]            = SAM_DB_DIR_ATTR_GID;
    wchar16_t wszAttrNameSamAccountName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameCommonName[]     = SAM_DB_DIR_ATTR_COMMON_NAME;
    wchar16_t wszAttrNameDomainName[]     = SAM_DB_DIR_ATTR_DOMAIN;
    wchar16_t wszAttrNameNetBIOSDomain[]  = SAM_DB_DIR_ATTR_NETBIOS_NAME;
    wchar16_t wszAttrNameDescription[]    = SAM_DB_DIR_ATTR_DESCRIPTION;
    wchar16_t wszAttrAccountFlags[]       = SAM_DB_DIR_ATTR_ACCOUNT_FLAGS;
    PCSTR     pszName = NULL;
    PCSTR     pszSID = NULL;
    PCSTR     pszDescription = NULL;
    PCSTR     pszDomainName = NULL;
    PCSTR     pszNetBIOSDomain = NULL;
    SAMDB_ACB AccountFlags;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;
    PSTR      pszObjectDN = NULL;
    PWSTR     pwszObjectDN = NULL;
    PWSTR     pwszSamAccountName = NULL;
    PWSTR     pwszSID = NULL;
    PWSTR     pwszDescription = NULL;
    PWSTR     pwszDomainName = NULL;
    PWSTR     pwszNetBIOSDomain = NULL;
    DWORD     dwGID = 0;
    ATTRIBUTE_VALUE avGroupName = {0};
    ATTRIBUTE_VALUE avSID = {0};
    ATTRIBUTE_VALUE avGID = {0};
    ATTRIBUTE_VALUE avObjectClass = {0};
    ATTRIBUTE_VALUE avDomainName = {0};
    ATTRIBUTE_VALUE avNetBIOSDomain = {0};
    ATTRIBUTE_VALUE avDescription = {0};
    ATTRIBUTE_VALUE avAccountFlags = {0};
    DIRECTORY_MOD mods[10];
    ULONG     iMod = 0;
    DWORD     i = 0;

    for (i = 0; i < sizeof(BuiltinAccounts)/sizeof(BuiltinAccounts[0]); i++)
    {
        pszName          = BuiltinAccounts[i].pszName;
        pszSID           = BuiltinAccounts[i].pszSID;
        dwGID            = BuiltinAccounts[i].dwGID;
        pszDescription   = BuiltinAccounts[i].pszDescription;
        pszDomainName    = BuiltinAccounts[i].pszDomainName;
        pszNetBIOSDomain = BuiltinAccounts[i].pszNetBIOSDomain;
        AccountFlags     = BuiltinAccounts[i].flags;
        objectClass      = BuiltinAccounts[i].objectClass;

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
                        pszDomainName,
                        &pwszDomainName);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        pszNetBIOSDomain,
                        &pwszNetBIOSDomain);
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

        mods[++iMod].pwszAttrName = &wszAttrNameGID[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avGID.Type = DIRECTORY_ATTR_TYPE_INTEGER;
        avGID.data.ulValue = dwGID;
        mods[iMod].pAttrValues = &avGID;

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

        avDomainName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avDomainName.data.pwszStringValue = pwszDomainName;
        mods[++iMod].pwszAttrName = &wszAttrNameDomainName[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        mods[iMod].pAttrValues = &avDomainName;

        avNetBIOSDomain.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avNetBIOSDomain.data.pwszStringValue = pwszNetBIOSDomain;
        mods[++iMod].pwszAttrName = &wszAttrNameNetBIOSDomain[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        mods[iMod].pAttrValues = &avNetBIOSDomain;

        avDescription.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avDescription.data.pwszStringValue = pwszDescription;
        mods[++iMod].pwszAttrName = &wszAttrNameDescription[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        mods[iMod].pAttrValues = &avDescription;

        if (AccountFlags) {
            avAccountFlags.Type = DIRECTORY_ATTR_TYPE_INTEGER;
            avAccountFlags.data.ulValue = AccountFlags;
            mods[++iMod].pwszAttrName = &wszAttrAccountFlags[0];
            mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
            mods[iMod].ulNumValues = 1;
            mods[iMod].pAttrValues = &avAccountFlags;
        }

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
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszDomainName);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszNetBIOSDomain);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszDescription);
    }

cleanup:

    DIRECTORY_FREE_STRING(pszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszSamAccountName);
    DIRECTORY_FREE_MEMORY(pwszSID);
    DIRECTORY_FREE_MEMORY(pwszDomainName);
    DIRECTORY_FREE_MEMORY(pwszNetBIOSDomain);
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
        DWORD               dwUid;
        DWORD               dwGid;
        DWORD               dwRid;
        PCSTR               pszDescription;
        PCSTR               pszShell;
        PCSTR               pszHomedir;
        PCSTR               pszDomain;
        PCSTR               pszNetBIOSDomain;
        SAMDB_ACB           flags;
        SAMDB_OBJECT_CLASS  objectClass;
    } LocalAccounts[] = {
        {
            .pszName          = "Administrator",
            .dwUid            = DOMAIN_USER_RID_ADMIN,
            .dwGid            = DOMAIN_ALIAS_RID_ADMINS,
            .dwRid            = DOMAIN_USER_RID_ADMIN,
            .pszDescription   = "Built-in account for administering the "
                                "computer/domain",
            .pszShell         = SAM_DB_DEFAULT_ADMINISTRATOR_SHELL,
            .pszHomedir       = SAM_DB_DEFAULT_ADMINISTRATOR_HOMEDIR,
            .pszDomain        = "BUILTIN",
            .pszNetBIOSDomain = "BUILTIN",
            .flags            = SAMDB_ACB_NORMAL,
            .objectClass      = SAMDB_OBJECT_CLASS_USER
        },
        {
            .pszName          = "Guest",
            .dwUid            = DOMAIN_USER_RID_GUEST,
            .dwGid            = DOMAIN_ALIAS_RID_GUESTS,
            .dwRid            = DOMAIN_USER_RID_GUEST,
            .pszDescription   = "Built-in account for guest access to the "
                                "computer/domain",
            .pszShell         = SAM_DB_DEFAULT_GUEST_SHELL,
            .pszHomedir       = SAM_DB_DEFAULT_GUEST_HOMEDIR,
            .pszDomain        = "BUILTIN",
            .pszNetBIOSDomain = "BUILTIN",
            .flags            = SAMDB_ACB_NORMAL | SAMDB_ACB_DISABLED,
            .objectClass      = SAMDB_OBJECT_CLASS_USER
        }
    };

    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    wchar16_t wszAttrNameObjectClass[]    = SAM_DB_DIR_ATTR_OBJECT_CLASS;
    wchar16_t wszAttrNameObjectSID[]      = SAM_DB_DIR_ATTR_OBJECT_SID;
    wchar16_t wszAttrNameUID[]            = SAM_DB_DIR_ATTR_UID;
    wchar16_t wszAttrNameSamAccountName[] = SAM_DB_DIR_ATTR_SAM_ACCOUNT_NAME;
    wchar16_t wszAttrNameCommonName[]     = SAM_DB_DIR_ATTR_COMMON_NAME;
    wchar16_t wszAttrNameDescription[]    = SAM_DB_DIR_ATTR_DESCRIPTION;
    wchar16_t wszAttrNameShell[]          = SAM_DB_DIR_ATTR_SHELL;
    wchar16_t wszAttrNameHomedir[]        = SAM_DB_DIR_ATTR_HOME_DIR;
    wchar16_t wszAttrAccountFlags[]       = SAM_DB_DIR_ATTR_ACCOUNT_FLAGS;
    wchar16_t wszAttrNamePrimaryGroup[]   = SAM_DB_DIR_ATTR_PRIMARY_GROUP;
    wchar16_t wszAttrNameDomain[]         = SAM_DB_DIR_ATTR_DOMAIN;
    wchar16_t wszAttrNameNetBIOSDomain[]  = SAM_DB_DIR_ATTR_NETBIOS_NAME;
    PCSTR     pszName = NULL;
    DWORD     dwUID = 0;
    DWORD     dwGID = 0;
    DWORD     dwRid = 0;
    PCSTR     pszDescription = NULL;
    PCSTR     pszShell = NULL;
    PCSTR     pszHomedir = NULL;
    PCSTR     pszDomain = NULL;
    PCSTR     pszNetBIOSDomain = NULL;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_UNKNOWN;
    SAMDB_ACB AccountFlags = 0;
    PSTR      pszObjectDN = NULL;
    PSID      pUserSid = NULL;
    ULONG     ulUserSidLength = 0;
    PWSTR     pwszSamAccountName = NULL;
    PWSTR     pwszObjectDN = NULL;
    PWSTR     pwszSID = NULL;
    PWSTR     pwszDescription = NULL;
    PWSTR     pwszShell = NULL;
    PWSTR     pwszHomedir = NULL;
    PWSTR     pwszDomain = NULL;
    PWSTR     pwszNetBIOSDomain = NULL;
    ATTRIBUTE_VALUE avUserName = {0};
    ATTRIBUTE_VALUE avSID = {0};
    ATTRIBUTE_VALUE avUID = {0};
    ATTRIBUTE_VALUE avGID = {0};
    ATTRIBUTE_VALUE avObjectClass = {0};
    ATTRIBUTE_VALUE avDescription = {0};
    ATTRIBUTE_VALUE avAccountFlags = {0};
    ATTRIBUTE_VALUE avShell = {0};
    ATTRIBUTE_VALUE avHomedir = {0};
    ATTRIBUTE_VALUE avDomain = {0};
    ATTRIBUTE_VALUE avNetBIOSDomain = {0};
    DIRECTORY_MOD mods[13];
    ULONG     iMod = 0;
    DWORD     i = 0;

    for (i = 0; i < sizeof(LocalAccounts)/sizeof(LocalAccounts[0]); i++) {

        pszName          = LocalAccounts[i].pszName;
        dwUID            = LocalAccounts[i].dwUid;
        dwGID            = LocalAccounts[i].dwGid;
        dwRid            = LocalAccounts[i].dwRid;
        pszDescription   = LocalAccounts[i].pszDescription;
        AccountFlags     = LocalAccounts[i].flags;
        objectClass      = LocalAccounts[i].objectClass;
        pszShell         = LocalAccounts[i].pszShell;
        pszHomedir       = LocalAccounts[i].pszHomedir;
        pszDomain        = LocalAccounts[i].pszDomain;
        pszNetBIOSDomain = LocalAccounts[i].pszNetBIOSDomain;

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

        dwError = LsaMbsToWc16s(
                        pszShell,
                        &pwszShell);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        pszHomedir,
                        &pwszHomedir);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        pszDomain,
                        &pwszDomain);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        pszNetBIOSDomain,
                        &pwszNetBIOSDomain);
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

        mods[++iMod].pwszAttrName = &wszAttrNameUID[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avUID.Type = DIRECTORY_ATTR_TYPE_INTEGER;
        avUID.data.ulValue = dwUID;
        mods[iMod].pAttrValues = &avUID;

        mods[++iMod].pwszAttrName = &wszAttrNamePrimaryGroup[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avGID.Type = DIRECTORY_ATTR_TYPE_INTEGER;
        avGID.data.ulValue = dwGID;
        mods[iMod].pAttrValues = &avGID;

        mods[++iMod].pwszAttrName = &wszAttrNameObjectClass[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avObjectClass.Type = DIRECTORY_ATTR_TYPE_INTEGER;
        avObjectClass.data.ulValue = objectClass;
        mods[iMod].pAttrValues = &avObjectClass;

        mods[++iMod].pwszAttrName = &wszAttrNameSamAccountName[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avUserName.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avUserName.data.pwszStringValue = pwszSamAccountName;
        mods[iMod].pAttrValues = &avUserName;

        mods[++iMod].pwszAttrName = &wszAttrNameCommonName[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        mods[iMod].pAttrValues = &avUserName;

        mods[++iMod].pwszAttrName = &wszAttrAccountFlags[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avAccountFlags.Type = DIRECTORY_ATTR_TYPE_INTEGER;
        avAccountFlags.data.ulValue = AccountFlags;
        mods[iMod].pAttrValues = &avAccountFlags;

        mods[++iMod].pwszAttrName = &wszAttrNameDescription[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avDescription.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avDescription.data.pwszStringValue = pwszDescription;
        mods[iMod].pAttrValues = &avDescription;

        mods[++iMod].pwszAttrName = &wszAttrNameShell[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avShell.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avShell.data.pwszStringValue = pwszShell;
        mods[iMod].pAttrValues = &avShell;

        mods[++iMod].pwszAttrName = &wszAttrNameHomedir[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avHomedir.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avHomedir.data.pwszStringValue = pwszHomedir;
        mods[iMod].pAttrValues = &avHomedir;

        mods[++iMod].pwszAttrName = &wszAttrNameDomain[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avDomain.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avDomain.data.pwszStringValue = pwszDomain;
        mods[iMod].pAttrValues = &avDomain;

        mods[++iMod].pwszAttrName = &wszAttrNameNetBIOSDomain[0];
        mods[iMod].ulOperationFlags = DIR_MOD_FLAGS_ADD;
        mods[iMod].ulNumValues = 1;
        avNetBIOSDomain.Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING;
        avNetBIOSDomain.data.pwszStringValue = pwszNetBIOSDomain;
        mods[iMod].pAttrValues = &avNetBIOSDomain;

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
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszDescription);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszShell);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszHomedir);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszDomain);
        DIRECTORY_FREE_MEMORY_AND_RESET(pwszNetBIOSDomain);

        if (pwszSID) {
            RTL_FREE(&pwszSID);
            pwszSID = NULL;
        }

        DIRECTORY_FREE_MEMORY_AND_RESET(pUserSid);
    }

cleanup:

    DIRECTORY_FREE_STRING(pszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszObjectDN);
    DIRECTORY_FREE_MEMORY(pwszSamAccountName);
    DIRECTORY_FREE_MEMORY(pwszDescription);
    DIRECTORY_FREE_MEMORY(pwszShell);
    DIRECTORY_FREE_MEMORY(pwszHomedir);
    DIRECTORY_FREE_MEMORY(pwszDomain);
    DIRECTORY_FREE_MEMORY(pwszNetBIOSDomain);

    if (pwszSID) {
        RTL_FREE(&pwszSID);
    }

    LSA_SAFE_FREE_MEMORY(pUserSid);

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
