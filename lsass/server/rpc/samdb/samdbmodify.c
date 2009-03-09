#include "includes.h"

NTSTATUS
SamDBModifyObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN,
    PDIRECTORY_MODS Modifications[]
    )
{
    NTSTATUS ntStatus = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;

    ntStatus = SamDbParseDN(ObjectDN,&pszObjectName, &dwType);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (dwType) {

        case SAMDB_USER:
            SamDbModifyUser(
                    hDirectory,
                    pszObjectName,
                    Modifications
                    );
            break;

        case SAMDB_GROUP:
            SamDbDeleteGroup(
                    hDirectory,
                    pszObjectName,
                    Modifications
                    );
            break;
    }

    return ntStatus;
}

NTSTATUS
SamDbModifyUser(
    HANDLE hDirectory,
    PWSTR pszObjectName,
    PDIRECTORY_MODS Modifications[]
    )
{
    NTSTATUS ntStatus = 0;

    return ntStatus;
}


NTSTATUS
SamDbModifyGroup(
    HANDLE hDirectory,
    PWSTR pszObjectName
    PDIRECTORY_MODS Modifications[]
    )
{
    NTSTATUS ntStatus = 0;

    return ntStatus;
}
