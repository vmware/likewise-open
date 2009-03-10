
#include "includes.h"

DWORD
SamDbAddObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN,
    DIRECTORY_MOD Modifications[]
    )
{
    DWORD dwError = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;
    PWSTR pwszObjectName = NULL;
    DWORD dwType = 0;

    dwError = SamDbParseDN(ObjectDN,&pwszObjectName, &dwType);
    BAIL_ON_LSA_ERROR(dwError);

    switch (dwType) {

        case SAMDB_USER:
            dwError = SamDbAddUser(
                        pDirectoryContext,
                        pwszObjectName,
                        Modifications
                        );
            break;

        case SAMDB_GROUP:
            dwError = SamDbAddGroup(
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
SamDbAddUser(
    HANDLE hDirectory,
    PWSTR pszObjectName,
    DIRECTORY_MOD Modifications[]
    )
{
    DWORD dwError = 0;
    return dwError;
}


DWORD
SamDbAddGroup(
    HANDLE hDirectory,
    PWSTR pszObjectName,
    DIRECTORY_MOD Modifications[]
    )
{
    DWORD dwError = 0;

    return dwError;
}
