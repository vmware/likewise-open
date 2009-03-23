#include "includes.h"

DWORD
SamDbDeleteObject(
    HANDLE hBindHandle,
    PWSTR  pwszObjectDN
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = NULL;
    PWSTR pwszObjectName = NULL;
    PWSTR pwszDomain = NULL;
    SAMDB_ENTRY_TYPE entryType = 0;

    pDirectoryContext = (PSAM_DIRECTORY_CONTEXT)hBindHandle;

    dwError = SamDbParseDN(
                    pwszObjectDN,
                    &pwszObjectName,
                    &pwszDomain,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    switch (entryType) {

        case SAMDB_ENTRY_TYPE_USER:

            dwError = SamDbDeleteUser(
                            pDirectoryContext,
                            pwszObjectName);

            break;

        case SAMDB_ENTRY_TYPE_GROUP:

            dwError = SamDbDeleteGroup(
                            pDirectoryContext,
                            pwszObjectName);
            break;

        case SAMDB_ENTRY_TYPE_DOMAIN:

            dwError = SamDbDeleteDomain(
                            pDirectoryContext,
                            pwszDomain);

            break;

        default:

            dwError = LSA_ERROR_INVALID_PARAMETER;

            break;
    }

error:

    if (pwszObjectName)
    {
        DirectoryFreeMemory(pwszObjectName);
    }

    if (pwszDomain)
    {
        DirectoryFreeMemory(pwszDomain);
    }

    return dwError;
}


