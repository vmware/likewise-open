#include "includes.h"

DWORD
SamDbDeleteObject(
    HANDLE hDirectory,
    PWSTR  pwszObjectDN
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = NULL;
    PSAM_DB_DN pDN = NULL;

    pDirectoryContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    dwError = SamDbParseDN(
                    pwszObjectDN,
                    &pDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    // TODO

cleanup:

    if (pDN)
    {
        SamDbFreeDN(pDN);
    }

    return dwError;

error:

    goto cleanup;
}


