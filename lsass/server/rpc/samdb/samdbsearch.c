#include "includes.h"

NTSTATUS
SamDbSearchObject(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Filter,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PDIRECTORY_VALUES * ppDirectoryValues
    PDWORD pdwNumValues
    )
{
    NTSTATUS ntStatus = 0;
    PDIRECTORY_CONTEXT pDirectoryContext = hBindHandle;

    ntStatus = SamDbConvertFiltertoTable(
                    Filter,
                    &dwTable
                    );
    switch (dwTable) {

        case SAMDB_USER:
            SamDbSearchUsers(
                    hDirectory,
                    Attributes,
                    AttributesOnly,
                    ppDirectoryValues,
                    pdwNumValues
                    );
            break;

        case SAMDB_GROUP:
            SamDbSearchGroups(
                    hDirectory,
                    Attributes,
                    AttributesOnly,
                    ppDirectoryValues,
                    pdwNumValues
                    );
            break;

        case SAMDB_DOMAIN:
            SamDbSearchDomains(
                    hDirectory,
                    Attributes,
                    AttributesOnly,
                    ppDirectoryValues,
                    pdwNumValues
                    );
            break;
    }

    return ntStatus;

}
