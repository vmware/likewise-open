#include "includes.h"

DWORD
SamDbSearchObject(
    HANDLE            hDirectory,
    PWSTR             pwszBase,
    ULONG             ulScope,
    PWSTR             pwszFilter,
    PWSTR             wszAttributes[],
    ULONG             ulAttributesOnly,
    PATTRIBUTE_VALUE* ppDirectoryValues,
    PDWORD            pdwNumValues
    )
{
    DWORD dwError = 0;
    SAMDB_ENTRY_TYPE entryType = 0;

    dwError = SamDbConvertFiltertoTable(
                    pwszFilter,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    switch (entryType) {

        case SAMDB_ENTRY_TYPE_USER:

            dwError = SamDbSearchUsers(
                            hDirectory,
                            pwszBase,
                            ulScope,
                            wszAttributes,
                            ulAttributesOnly,
                            ppDirectoryValues,
                            pdwNumValues
                            );
            break;

        case SAMDB_ENTRY_TYPE_GROUP:

            dwError = SamDbSearchGroups(
                            hDirectory,
                            pwszBase,
                            ulScope,
                            wszAttributes,
                            ulAttributesOnly,
                            ppDirectoryValues,
                            pdwNumValues
                            );
            break;

        case SAMDB_ENTRY_TYPE_DOMAIN:

            dwError = SamDbSearchDomains(
                            hDirectory,
                            pwszBase,
                            ulScope,
                            wszAttributes,
                            ulAttributesOnly,
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
    HANDLE            hDirectory,
    PWSTR             pwszBase,
    ULONG             ulScope,
    PWSTR             wszAttributes[],
    ULONG             ulAttributesOnly,
    PATTRIBUTE_VALUE* ppDirectoryValues,
    PDWORD            pdwNumValues
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
SamDbSearchDomains(
    HANDLE            hDirectory,
    PWSTR             pwszBase,
    ULONG             ulScope,
    PWSTR             wszAttributes[],
    ULONG             ulAttributesOnly,
    PATTRIBUTE_VALUE* ppDirectoryValues,
    PDWORD            pdwNumValues
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
SamDbSearchGroups(
    HANDLE            hDirectory,
    PWSTR             Base,
    ULONG             Scope,
    PWSTR             Attributes[],
    ULONG             AttributesOnly,
    PATTRIBUTE_VALUE* ppDirectoryValues,
    PDWORD            pdwNumValues
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
SamDbConvertFiltertoTable(
    PWSTR             pwszFilter,
    SAMDB_ENTRY_TYPE* pdwTable
    )
{
    DWORD dwError = 0;
    PSTR  pszFilter = NULL;
    PSTR  pszCursor = NULL;
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;

    dwError = LsaWc16sToMbs(
                    pwszFilter,
                    &pszFilter);
    BAIL_ON_SAMDB_ERROR(dwError);

    pszCursor = pszFilter;
    while (pszCursor && *pszCursor && isspace((int)*pszCursor))
    {
        pszCursor++;
    }

    if (IsNullOrEmptyString(pszCursor) ||
        (*pszCursor != '('))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pszCursor++;

    if (IsNullOrEmptyString(pszCursor) ||
        strncasecmp(pszCursor, "objectclass", sizeof("objectclass")-1))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pszCursor += sizeof("objectclass") - 1;

    while (pszCursor && *pszCursor && isspace((int)*pszCursor))
    {
        pszCursor++;
    }

    if (IsNullOrEmptyString(pszCursor) || (*pszCursor != '='))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pszCursor++;

    if (IsNullOrEmptyString(pszCursor))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    if (!strncasecmp(pszCursor, "user", sizeof("user") - 1))
    {
        entryType = SAMDB_ENTRY_TYPE_USER;
        pszCursor += sizeof("user") - 1;
    }
    else if (!strncasecmp(pszCursor, "group", sizeof("group") - 1))
    {
        entryType = SAMDB_ENTRY_TYPE_GROUP;
        pszCursor += sizeof("group") - 1;
    }
    else if (!strncasecmp(pszCursor, "domain", sizeof("domain") - 1))
    {
        entryType = SAMDB_ENTRY_TYPE_DOMAIN;
        pszCursor += sizeof("domain") - 1;
    }
    else
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    while (pszCursor && *pszCursor && isspace((int)*pszCursor))
    {
        pszCursor++;
    }

    if (IsNullOrEmptyString(pszCursor) || (*pszCursor != ')'))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pszCursor++;

    while (pszCursor && *pszCursor && isspace((int)*pszCursor))
    {
        pszCursor++;
    }

    if (!IsNullOrEmptyString(pszCursor))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *pdwTable = entryType;

cleanup:

    return dwError;

error:

    *pdwTable = SAMDB_ENTRY_TYPE_UNKNOWN;

    goto cleanup;
}

