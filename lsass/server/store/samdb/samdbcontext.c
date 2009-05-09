#include "includes.h"

static
DWORD
SamDbAcquireDbContext(
    PSAM_DB_CONTEXT* ppDbContext
    );

static
VOID
SamDbReleaseDbContext(
    PSAM_DB_CONTEXT pDbContext
    );

DWORD
SamDbBuildDirectoryContext(
    PSAMDB_OBJECTCLASS_TO_ATTR_MAP_INFO pObjectClassAttrMaps,
    DWORD                               dwNumObjectClassAttrMaps,
    PSAM_DB_ATTR_LOOKUP                 pAttrLookup,
    PSAM_DIRECTORY_CONTEXT*             ppDirContext
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(SAM_DIRECTORY_CONTEXT),
                    (PVOID*)&pDirContext);
    BAIL_ON_SAMDB_ERROR(dwError);

    pDirContext->pObjectClassAttrMaps = pObjectClassAttrMaps;
    pDirContext->dwNumObjectClassAttrMaps = dwNumObjectClassAttrMaps;
    pDirContext->pAttrLookup = pAttrLookup;

    dwError = SamDbAcquireDbContext(&pDirContext->pDbContext);
    BAIL_ON_LSA_ERROR(dwError);

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

static
DWORD
SamDbAcquireDbContext(
    PSAM_DB_CONTEXT* ppDbContext
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PCSTR pszDbPath = SAM_DB;
    PSAM_DB_CONTEXT pDbContext = NULL;

    SAMDB_LOCK_MUTEX(bInLock, &gSamGlobals.mutex);

    if (gSamGlobals.dwNumDbContexts)
    {
        pDbContext = gSamGlobals.pDbContextList;

        gSamGlobals.pDbContextList = gSamGlobals.pDbContextList->pNext;
        gSamGlobals.dwNumDbContexts--;

        pDbContext->pNext = NULL;
    }

    SAMDB_UNLOCK_MUTEX(bInLock, &gSamGlobals.mutex);

    if (!pDbContext)
    {
        dwError = DirectoryAllocateMemory(
                        sizeof(SAM_DB_CONTEXT),
                        (PVOID*)&pDbContext);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = sqlite3_open(
                        pszDbPath,
                        &pDbContext->pDbHandle);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *ppDbContext = pDbContext;

cleanup:

    SAMDB_UNLOCK_MUTEX(bInLock, &gSamGlobals.mutex);

    return dwError;

error:

    *ppDbContext = pDbContext;

    if (pDbContext)
    {
        SamDbFreeDbContext(pDbContext);
    }

    goto cleanup;
}

VOID
SamDbFreeDirectoryContext(
    PSAM_DIRECTORY_CONTEXT pDirContext
    )
{
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
        SamDbReleaseDbContext(pDirContext->pDbContext);
    }

    DirectoryFreeMemory(pDirContext);
}

static
VOID
SamDbReleaseDbContext(
    PSAM_DB_CONTEXT pDbContext
    )
{
    BOOLEAN bInLock = FALSE;

    SAMDB_LOCK_MUTEX(bInLock, &gSamGlobals.mutex);

    if (gSamGlobals.dwNumDbContexts < gSamGlobals.dwNumMaxDbContexts)
    {
        pDbContext->pNext = gSamGlobals.pDbContextList;
        gSamGlobals.pDbContextList = pDbContext;

        gSamGlobals.dwNumDbContexts++;
    }
    else
    {
        SamDbFreeDbContext(pDbContext);
    }

    SAMDB_UNLOCK_MUTEX(bInLock, &gSamGlobals.mutex);
}

VOID
SamDbFreeDbContext(
    PSAM_DB_CONTEXT pDbContext
    )
{
    if (pDbContext->pDelObjectStmt)
    {
        sqlite3_finalize(pDbContext->pDelObjectStmt);
    }

    if (pDbContext->pQueryObjectCountStmt)
    {
        sqlite3_finalize(pDbContext->pQueryObjectCountStmt);
    }

    if (pDbContext->pQueryObjectRecordInfoStmt)
    {
        sqlite3_finalize(pDbContext->pQueryObjectRecordInfoStmt);
    }

    if (pDbContext->pDbHandle)
    {
        sqlite3_close(pDbContext->pDbHandle);
    }

    DirectoryFreeMemory(pDbContext);
}
