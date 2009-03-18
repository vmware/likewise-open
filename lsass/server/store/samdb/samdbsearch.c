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
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PWSTR pwszObjectName = NULL;
    PWSTR pwszDomain = NULL;
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    PSAM_DB_DOMAIN_INFO pDomainInfo = NULL;
    DWORD dwNumAttrs = sizeof(wszAttributes)/sizeof(wszAttributes[0]);
    DWORD iAttr = 0;
    PATTRIBUTE_VALUE pDirectoryValues = NULL;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    dwError = SamDbParseDN(
                    pwszBase,
                    &pwszObjectName,
                    &pwszDomain,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    if ((entryType != SAMDB_ENTRY_TYPE_DOMAIN) ||
        (pwszObjectName || *pwszObjectName))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbFindDomain(
                    hDirectory,
                    pwszDomain,
                    &pDomainInfo);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = DirectoryAllocateMemory(
                    sizeof(ATTRIBUTE_VALUE) * dwNumAttrs,
                    (PVOID*)&pDirectoryValues);
    BAIL_ON_SAMDB_ERROR(dwError);

    for (; iAttr < dwNumAttrs; iAttr++)
    {
        NTSTATUS ntStatus = 0;
        PWSTR pwszAttrName = wszAttributes[iAttr];
        PSAMDB_ATTRIBUTE_LOOKUP_ENTRY pEntry = NULL;
        PATTRIBUTE_VALUE pAttrValue = &pDirectoryValues[iAttr];

        ntStatus = LwRtlRBTreeFind(
                        pDirContext->pAttrLookup->pAttrTree,
                        pwszAttrName,
                        (PVOID*)&pEntry);
        if (ntStatus)
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        switch (pEntry->dwId)
        {
            case SAMDB_DOMAIN_TABLE_COLUMN_NETBIOS_NAME:

                pAttrValue->Type = pEntry->attrType;

                if (!ulAttributesOnly)
                {
                    pAttrValue->pwszStringValue = pDomainInfo->pwszNetBIOSName;
                    pDomainInfo->pwszNetBIOSName = NULL;
                }

                break;

            case SAMDB_DOMAIN_TABLE_COLUMN_MACHINE_SID:

                pAttrValue->Type = pEntry->attrType;

                if (!ulAttributesOnly)
                {
                    pAttrValue->pwszStringValue = pDomainInfo->pwszDomainSID;
                    pDomainInfo->pwszDomainSID = NULL;
                }

                break;

            case SAMDB_DOMAIN_TABLE_COLUMN_DOMAIN_NAME:

                pAttrValue->Type = pEntry->attrType;

                if (!ulAttributesOnly)
                {
                    pAttrValue->pwszStringValue = pDomainInfo->pwszDomainName;
                    pDomainInfo->pwszDomainName = NULL;
                }

                break;

            default:

                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);
        }
    }

    *ppDirectoryValues = pDirectoryValues;
    *pdwNumValues = dwNumAttrs;

cleanup:

    if (pwszObjectName)
    {
        DirectoryFreeMemory(pwszObjectName);
    }
    if (pwszDomain)
    {
        DirectoryFreeMemory(pwszDomain);
    }
    if (pDomainInfo)
    {
        SamDbFreeDomainInfo(pDomainInfo);
    }

    return dwError;

error:

    *ppDirectoryValues = NULL;
    *pdwNumValues = 0;

    if (pDirectoryValues)
    {
        DirectoryFreeAttributeValues(pDirectoryValues, dwNumAttrs);
    }

    goto cleanup;
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

    if (pszFilter)
    {
        DirectoryFreeMemory(pszFilter);
    }

    return dwError;

error:

    *pdwTable = SAMDB_ENTRY_TYPE_UNKNOWN;

    goto cleanup;
}

