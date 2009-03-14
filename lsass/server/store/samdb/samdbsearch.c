#include "includes.h"

DWORD
SamDbSearchObject(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Filter,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PATTRIBUTE_VALUE* ppDirectoryValues,
    PDWORD pdwNumValues
    )
{
    DWORD dwError = 0;
    SAMDB_ENTRY_TYPE entryType = 0;

    dwError = SamDbConvertFiltertoTable(
                    Filter,
                    &entryType
                    );
    BAIL_ON_SAMDB_ERROR(dwError);

    switch (entryType) {

        case SAMDB_ENTRY_TYPE_USER:
            dwError = SamDbSearchUsers(
                            hDirectory,
                            Base,
                            Scope,
                            Attributes,
                            AttributesOnly,
                            ppDirectoryValues,
                            pdwNumValues
                            );
            break;

        case SAMDB_ENTRY_TYPE_GROUP:
            dwError = SamDbSearchGroups(
                            hDirectory,
                            Base,
                            Scope,
                            Attributes,
                            AttributesOnly,
                            ppDirectoryValues,
                            pdwNumValues
                            );
            break;

        case SAMDB_ENTRY_TYPE_DOMAIN:
            dwError = SamDbSearchDomains(
                            hDirectory,
                            Base,
                            Scope,
                            Attributes,
                            AttributesOnly,
                            ppDirectoryValues,
                            pdwNumValues
                            );
            break;

        default:

            dwError = LSA_ERROR_INVALID_PARAMETER;

            break;
    }

error:

    return dwError;
}

DWORD
SamDbSearchUsers(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PATTRIBUTE_VALUE * ppDirectoryValues,
    PDWORD pdwNumValues
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
SamDbSearchGroups(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PATTRIBUTE_VALUE * ppDirectoryValues,
    PDWORD pdwNumValues
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
SamDbSearchDomains(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PATTRIBUTE_VALUE * ppDirectoryValues,
    PDWORD pdwNumValues
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
SamDbConvertFiltertoTable(
    PWSTR pwszFilter,
    PDWORD pdwTable
    )
{
    return 0;
}

