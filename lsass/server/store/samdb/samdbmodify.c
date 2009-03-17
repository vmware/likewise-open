#include "includes.h"

DWORD
SamDbModifyObject(
    HANDLE hBindHandle,
    PWSTR  pwszObjectDN,
    DIRECTORY_MOD Modifications[]
    )
{
    DWORD dwError = 0;
    PWSTR pwszObjectName = NULL;
    PWSTR pwszDomain = NULL;
    PSAM_DIRECTORY_CONTEXT pDirectoryContext = (PSAM_DIRECTORY_CONTEXT)hBindHandle;
    SAMDB_ENTRY_TYPE entryType = 0;

    dwError = SamDbParseDN(
                    pwszObjectDN,
                    &pwszObjectName,
                    &pwszDomain,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    switch (entryType) {

        case SAMDB_ENTRY_TYPE_USER:

            dwError = SamDbModifyUser(
                        pDirectoryContext,
                        pwszObjectName,
                        Modifications
                        );
            break;

        case SAMDB_ENTRY_TYPE_GROUP:

            dwError = SamDbModifyGroup(
                        pDirectoryContext,
                        pwszObjectName,
                        Modifications
                        );
            break;

        case SAMDB_ENTRY_TYPE_DOMAIN:

            dwError = SamDbModifyDomain(
                            pDirectoryContext,
                            pwszDomain,
                            Modifications);

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

DWORD
SamDbModifyUser(
    HANDLE hDirectory,
    PWSTR pszObjectName,
    DIRECTORY_MOD Modifications[]
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
SamDbModifyGroup(
    HANDLE hDirectory,
    PWSTR pszObjectName,
    DIRECTORY_MOD Modifications[]
    )
{
    DWORD dwError = 0;

    return dwError;
}
