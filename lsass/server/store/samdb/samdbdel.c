#include "includes.h"

DWORD
SamDbDeleteObject(
    HANDLE hDirectory,
    PWSTR  pwszObjectDN
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = NULL;
    PSTR pszObjectDN = NULL;
    PSAM_DB_DN pDN = NULL;
    BOOLEAN bInLock = FALSE;
    DWORD dwNumDependents = 0;

    pDirectoryContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    if (!hDirectory || !pwszObjectDN || !*pwszObjectDN)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbParseDN(
                    pwszObjectDN,
                    &pDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaWc16sToMbs(
                    pwszObjectDN,
                    &pszObjectDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSamGlobals.rwLock);

    dwError = SamDbGetNumberOfDependents_inlock(
                    pDirectoryContext,
                    pszObjectDN,
                    &dwNumDependents);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumDependents)
    {
        dwError = LSA_ERROR_OBJECT_IN_USE;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    if (!pDirectoryContext->pDbContext->pDelObjectStmt)
    {
        PCSTR pszDelQueryTemplate =
            "DELETE FROM " SAM_DB_OBJECTS_TABLE \
            " WHERE " SAM_DB_COL_DISTINGUISHED_NAME " = ?1;";

        dwError = sqlite3_prepare_v2(
                    pDirectoryContext->pDbContext->pDbHandle,
                    pszDelQueryTemplate,
                    -1,
                    &pDirectoryContext->pDbContext->pDelObjectStmt,
                    NULL);
        BAIL_ON_SAMDB_SQLITE_ERROR_DB(
                        dwError,
                        pDirectoryContext->pDbContext->pDbHandle);
    }

    dwError = sqlite3_bind_text(
                    pDirectoryContext->pDbContext->pDelObjectStmt,
                    1,
                    pszObjectDN,
                    -1,
                    SQLITE_TRANSIENT);
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(
                    dwError,
                    pDirectoryContext->pDbContext->pDelObjectStmt);

    dwError = sqlite3_step(pDirectoryContext->pDbContext->pDelObjectStmt);
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_SAMDB_SQLITE_ERROR_STMT(
                    dwError,
                    pDirectoryContext->pDbContext->pDelObjectStmt);

cleanup:

    if (pDirectoryContext->pDbContext->pDelObjectStmt)
    {
        sqlite3_reset(pDirectoryContext->pDbContext->pDelObjectStmt);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &gSamGlobals.rwLock);

    DIRECTORY_FREE_STRING(pszObjectDN);

    if (pDN)
    {
        SamDbFreeDN(pDN);
    }

    return dwError;

error:

    goto cleanup;
}
