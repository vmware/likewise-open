
#include "includes.h"

DWORD
SamDBAddObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN
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
            dwError = SamDbDeleteUser(
                        pDirectoryContext,
                        pwszObjectName
                        );
            break;

        case SAMDB_GROUP:
            dwError = SamDbDeleteGroup(
                        pDirectoryContext,
                        pwszObjectName
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
    DIRECTORY_MODS Modifications[]
    )
{
    DWORD dwError = 0;
    return dwError;
}


DWORD
SamDbAddGroup(
    HANDLE hDirectory,
    PWSTR pszObjectName
    )
{
    DWORD dwError = 0;

    return dwError;
}
