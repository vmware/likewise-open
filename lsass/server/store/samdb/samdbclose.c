#include "includes.h"

DWORD
SamDbClose(
    HANDLE hDirectory
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    if (!pDirContext)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    SamDbFreeDirectoryContext(pDirContext);

error:

    return dwError;
}
