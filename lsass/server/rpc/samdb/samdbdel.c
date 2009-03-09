#include "includes.h"

NTSTATUS
SamDBDeleteObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN,
    )
{
    NTSTATUS ntStatus = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;

    ntStatus = SamDbParseDN(ObjectDN,&pszObjectName, &dwType);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (dwType) {

        case SAMDB_USER:
            SamDbDeleteUser(
                    hDirectory,
                    pszObjectName
                    );
            break;

        case SAMDB_GROUP:
            SamDbDeleteGroup(
                    hDirectory,
                    pszObjectName
                    );
            break;
    }

    return ntStatus;
}

NTSTATUS
SamDbDeleteUser(
    HANDLE hDirectory,
    PWSTR pszObjectName
    )
{
    NTSTATUS ntStatus = 0;

    return ntStatus;
}


NTSTATUS
SamDbDeleteGroup(
    HANDLE hDirectory,
    PWSTR pszObjectName
    )
{
    NTSTATUS ntStatus = 0;

    return ntStatus;
}
