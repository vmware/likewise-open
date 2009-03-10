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
    DWORD dwTable = 0;

    dwError = SamDbConvertFiltertoTable(
                    Filter,
                    &dwTable
                    );
    BAIL_ON_LSA_ERROR(dwError);

    switch (dwTable) {

        case SAMDB_USER:
            SamDbSearchUsers(
                    hDirectory,
                    Base,
                    Scope,
                    Attributes,
                    AttributesOnly,
                    ppDirectoryValues,
                    pdwNumValues
                    );
            break;

        case SAMDB_GROUP:
            SamDbSearchGroups(
                    hDirectory,
                    Base,
                    Scope,
                    Attributes,
                    AttributesOnly,
                    ppDirectoryValues,
                    pdwNumValues
                    );
            break;

        case SAMDB_DOMAIN:
            SamDbSearchDomains(
                    hDirectory,
                    Base,
                    Scope,
                    Attributes,
                    AttributesOnly,
                    ppDirectoryValues,
                    pdwNumValues
                    );
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

