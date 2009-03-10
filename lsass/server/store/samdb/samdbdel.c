#include "includes.h"

DWORD
SamDbDeleteObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN
    )
{
    DWORD dwError = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;
    PWSTR pwszObjectName = NULL;
    DWORD dwType = 0;

    dwError = SamDbParseDN(ObjectDN, &pwszObjectName, &dwType);
    BAIL_ON_LSA_ERROR(dwError);

    switch (dwType) {

        case SAMDB_USER:

            SamDbDeleteUser(
                    pDirectoryContext,
                    pwszObjectName
                    );
            break;

        case SAMDB_GROUP:

            SamDbDeleteGroup(
                    pDirectoryContext,
                    pwszObjectName
                    );
            break;
    }

error:

    return dwError;
}

DWORD
SamDbDeleteUser(
    HANDLE hDirectory,
    PWSTR pszObjectName
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
SamDbDeleteGroup(
    HANDLE hDirectory,
    PWSTR pszObjectName
    )
{
    DWORD dwError = 0;

    return dwError;
}
