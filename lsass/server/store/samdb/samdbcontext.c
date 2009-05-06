#include "includes.h"

DWORD
SamDbBuildDirectoryContext(
    PSAM_DB_INSTANCE_LOCK               pDbInstanceLock,
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassAttrMaps,
    DWORD                               dwNumObjectClassAttrMaps,
    PSAM_DB_ATTR_LOOKUP                 pAttrLookup,
    PSAM_DIRECTORY_CONTEXT*             ppDirContext
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PCSTR pszDbPath = SAM_DB;

    dwError = DirectoryAllocateMemory(
                    sizeof(SAM_DIRECTORY_CONTEXT),
                    (PVOID*)&pDirContext);
    BAIL_ON_SAMDB_ERROR(dwError);

    pthread_rwlock_init(&pDirContext->rwLock, NULL);
    pDirContext->pRwLock = &pDirContext->rwLock;

    dwError = DirectoryAllocateMemory(
                    sizeof(SAM_DB_CONTEXT),
                    (PVOID*)&pDirContext->pDbContext);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbAcquireDbInstanceLock(
                    pDbInstanceLock,
                    &pDirContext->pDbContext->pDbLock);
    BAIL_ON_SAMDB_ERROR(dwError);

    pDirContext->pObjectClassAttrMaps = pObjectClassAttrMaps;
    pDirContext->dwNumObjectClassAttrMaps = dwNumObjectClassAttrMaps;
    pDirContext->pAttrLookup = pAttrLookup;

    dwError = sqlite3_open(
                    pszDbPath,
                    &pDirContext->pDbContext->pDbHandle);
    BAIL_ON_SAMDB_ERROR(dwError);

    *ppDirContext = pDirContext;

cleanup:

    return dwError;

error:

    *ppDirContext = NULL;

    if (pDirContext)
    {
        SamDbFreeDirectoryContext(pDirContext);
    }

    goto cleanup;
}

VOID
SamDbFreeDirectoryContext(
    PSAM_DIRECTORY_CONTEXT pDirContext
    )
{
    if (pDirContext->pRwLock)
    {
        pthread_rwlock_destroy(&pDirContext->rwLock);
    }

    if (pDirContext->pwszCredential)
    {
        DirectoryFreeMemory(pDirContext->pwszCredential);
    }

    if (pDirContext->pwszDistinguishedName)
    {
        DirectoryFreeMemory(pDirContext->pwszDistinguishedName);
    }

    if (pDirContext->pDbContext)
    {
        if (pDirContext->pDbContext->pDbLock)
        {
            SamDbReleaseDbInstanceLock(pDirContext->pDbContext->pDbLock);
        }

        if (pDirContext->pDbContext->pDelObjectStmt)
        {
            sqlite3_finalize(pDirContext->pDbContext->pDelObjectStmt);
            pDirContext->pDbContext->pDelObjectStmt = NULL;
        }

        if (pDirContext->pDbContext->pDbHandle)
        {
            sqlite3_close(pDirContext->pDbContext->pDbHandle);
        }

        DirectoryFreeMemory(pDirContext->pDbContext);
    }

    DirectoryFreeMemory(pDirContext);
}
