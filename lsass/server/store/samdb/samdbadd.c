
#include "includes.h"

DWORD
SamDbAddObject(
    HANDLE hBindHandle,
    PWSTR  pwszObjectDN,
    DIRECTORY_MOD Modifications[]
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = hBindHandle;
    PWSTR pwszObjectName = NULL;
    PWSTR pwszDomain = NULL;
    SAMDB_ENTRY_TYPE entryType = 0;

    dwError = SamDbParseDN(
                    pwszObjectDN,
                    &pwszObjectName,
                    &pwszDomain,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    switch (entryType)
    {
        case SAMDB_ENTRY_TYPE_USER:

            dwError = SamDbAddUser(
                        pDirectoryContext->pDbContext,
                        pwszObjectName,
                        Modifications
                        );
            break;

        case SAMDB_ENTRY_TYPE_GROUP:

            dwError = SamDbAddGroup(
                        pDirectoryContext->pDbContext,
                        pwszObjectName,
                        Modifications
                        );
            break;

        case SAMDB_ENTRY_TYPE_DOMAIN:

            dwError = SamDbAddDomain(
                        pDirectoryContext->pDbContext,
                        pwszObjectName,
                        Modifications
                        );
            break;

        default:

            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_SAMDB_ERROR(dwError);

            break;
    }

cleanup:

    if (pwszObjectName)
    {
        DirectoryFreeMemory(pwszObjectName);
    }

    if (pwszDomain)
    {
        DirectoryFreeMemory(pwszDomain);
    }

    return dwError;

error:

    goto cleanup;
}

