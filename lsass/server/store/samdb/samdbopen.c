#include "includes.h"

DWORD
SamDbOpen(
    PHANDLE phDb
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = NULL;

    dwError = sqlite3_open(SAM_DB, &pDbHandle);
    BAIL_ON_LSA_ERROR(dwError);

    *phDb = (HANDLE)pDbHandle;

cleanup:

    return dwError;

error:

    *(phDb) = (HANDLE)NULL;

    if (pDbHandle) {
        sqlite3_close(pDbHandle);
    }

    goto cleanup;
}
