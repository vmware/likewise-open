#include "includes.h"

DWORD
SamDbModifyObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN,
    DIRECTORY_MOD Modifications[]
    )
{
    DWORD dwError = 0;
    PWSTR pwszObjectName = NULL;
    DWORD dwType = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;

    dwError = SamDbParseDN(ObjectDN,&pwszObjectName, &dwType);
    BAIL_ON_LSA_ERROR(dwError);

    switch (dwType) {

        case SAMDB_USER:
            dwError = SamDbModifyUser(
                        pDirectoryContext,
                        pwszObjectName,
                        Modifications
                        );
            break;

        case SAMDB_GROUP:
            dwError = SamDbModifyGroup(
                        pDirectoryContext,
                        pwszObjectName,
                        Modifications
                        );
            break;
    }

error:

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
