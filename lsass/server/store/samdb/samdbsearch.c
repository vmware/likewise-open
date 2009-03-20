#include "includes.h"

DWORD
SamDbSearchObject(
    HANDLE            hDirectory,
    PWSTR             pwszBase,
    ULONG             ulScope,
    PWSTR             pwszFilter,
    PWSTR             wszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
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
                            ppDirectoryEntries,
                            pdwNumEntries
                            );
            break;

        case SAMDB_ENTRY_TYPE_GROUP:

            dwError = SamDbSearchGroups(
                            hDirectory,
                            pwszBase,
                            ulScope,
                            wszAttributes,
                            ulAttributesOnly,
                            ppDirectoryEntries,
                            pdwNumEntries
                            );
            break;

        case SAMDB_ENTRY_TYPE_DOMAIN:

            dwError = SamDbSearchDomains(
                            hDirectory,
                            pwszBase,
                            ulScope,
                            wszAttributes,
                            ulAttributesOnly,
                            ppDirectoryEntries,
                            pdwNumEntries
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
    PWSTR             pwszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
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
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PWSTR pwszObjectName = NULL;
    PWSTR pwszDomain = NULL;
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    PSAM_DB_DOMAIN_INFO* ppDomainInfoList = NULL;
    DWORD dwNumDomains = 0;
    PDIRECTORY_ENTRY pDirectoryEntries = NULL;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    if (pwszBase)
    {
        dwError = SamDbParseDN(
                        pwszBase,
                        &pwszObjectName,
                        &pwszDomain,
                        &entryType);
        BAIL_ON_SAMDB_ERROR(dwError);

        if (entryType != SAMDB_ENTRY_TYPE_DOMAIN)
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_SAMDB_ERROR(dwError);
        }
    }

    dwError = SamDbFindDomains(
                    hDirectory,
                    pwszDomain,
                    &ppDomainInfoList,
                    &dwNumDomains);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumDomains)
    {
        dwError = SamDbBuildDomainDirectoryEntries(
                        pDirContext,
                        wszAttributes,
                        ulAttributesOnly,
                        ppDomainInfoList,
                        dwNumDomains,
                        &pDirectoryEntries);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    *ppDirectoryEntries = pDirectoryEntries;
    *pdwNumEntries = dwNumDomains;

cleanup:

    if (pwszObjectName)
    {
        DirectoryFreeMemory(pwszObjectName);
    }
    if (pwszDomain)
    {
        DirectoryFreeMemory(pwszDomain);
    }
    if (ppDomainInfoList)
    {
        SamDbFreeDomainInfoList(ppDomainInfoList, dwNumDomains);
    }

    return dwError;

error:

    *ppDirectoryEntries = NULL;
    *pdwNumEntries = 0;

    if (pDirectoryEntries)
    {
        DirectoryFreeEntries(pDirectoryEntries, dwNumDomains);
    }

    goto cleanup;
}

DWORD
SamDbBuildDomainDirectoryEntries(
    PSAM_DIRECTORY_CONTEXT pDirContext,
    PWSTR                  wszAttributes[],
    ULONG                  ulAttributesOnly,
    PSAM_DB_DOMAIN_INFO*   ppDomainInfoList,
    DWORD                  dwNumDomains,
    PDIRECTORY_ENTRY*      ppDirectoryEntries
    )
{
    DWORD dwError = 0;
    DWORD iDomain = 0;
    DWORD iAttr = 0;
    DWORD dwNumAttrs = 0;
    PDIRECTORY_ENTRY pDirectoryEntries = NULL;

    dwError = DirectoryAllocateMemory(
                    sizeof(DIRECTORY_ENTRY) * dwNumDomains,
                    (PVOID*)&pDirectoryEntries);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (ulAttributesOnly) {
        while (wszAttributes[dwNumAttrs]) {
            dwNumAttrs++;
        }
    } else {
        /* total number of attributes in SAM_DB_DOMAIN_INFO */
        dwNumAttrs = 3;
    }

    for (; iDomain < dwNumDomains; iDomain++)
    {
        PDIRECTORY_ENTRY pDirEntry = &pDirectoryEntries[iDomain];
        PSAM_DB_DOMAIN_INFO pDomainInfo = *(ppDomainInfoList + iDomain);

        dwError = DirectoryAllocateMemory(
                        sizeof(DIRECTORY_ATTRIBUTE) * dwNumAttrs,
                        (PVOID*)&pDirEntry->pDirectoryAttributes);
        BAIL_ON_SAMDB_ERROR(dwError);

        pDirEntry->ulNumAttributes = dwNumAttrs;

        for (; iAttr < dwNumAttrs; iAttr++)
        {
            NTSTATUS ntStatus = 0;
            PWSTR pwszAttrName = wszAttributes[iAttr];
            PSAMDB_ATTRIBUTE_LOOKUP_ENTRY pEntry = NULL;
            PDIRECTORY_ATTRIBUTE pDirAttr = &pDirEntry->pDirectoryAttributes[iAttr];
            PATTRIBUTE_VALUE pAttrValue = NULL;

            ntStatus = LwRtlRBTreeFind(
                            pDirContext->pAttrLookup->pAttrTree,
                            pwszAttrName,
                            (PVOID*)&pEntry);
            if (ntStatus)
            {
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = DirectoryAllocateStringW(
                            pEntry->pwszAttributeName,
                            &pDirAttr->pwszAttributeName);
            BAIL_ON_SAMDB_ERROR(dwError);

            dwError = DirectoryAllocateMemory(
                            sizeof(ATTRIBUTE_VALUE),
                            (PVOID*)&pDirAttr->pAttributeValues);
            BAIL_ON_SAMDB_ERROR(dwError);

            pDirAttr->ulNumValues = 1;

            pAttrValue = &pDirAttr->pAttributeValues[0];

            switch (pEntry->dwId)
            {
                case SAMDB_DOMAIN_TABLE_COLUMN_NETBIOS_NAME:

                    pAttrValue->Type = pEntry->attrType;

                    if (!ulAttributesOnly ||
                        !wc16scmp(wszAttributes[0], pDirAttr->pwszAttributeName))
                    {
                        pAttrValue->pwszStringValue = pDomainInfo->pwszNetBIOSName;
                        pDomainInfo->pwszNetBIOSName = NULL;
                    }

                    break;

                case SAMDB_DOMAIN_TABLE_COLUMN_MACHINE_SID:

                    pAttrValue->Type = pEntry->attrType;

                    if (!ulAttributesOnly ||
                        !wc16scmp(wszAttributes[0], pDirAttr->pwszAttributeName))
                    {
                        pAttrValue->pwszStringValue = pDomainInfo->pwszDomainSID;
                        pDomainInfo->pwszDomainSID = NULL;
                    }

                    break;

                case SAMDB_DOMAIN_TABLE_COLUMN_DOMAIN_NAME:

                    pAttrValue->Type = pEntry->attrType;

                    if (!ulAttributesOnly ||
                        !wc16scmp(wszAttributes[0], pDirAttr->pwszAttributeName))
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
    }

    *ppDirectoryEntries = pDirectoryEntries;

cleanup:

    return dwError;

error:

    *ppDirectoryEntries = NULL;

    if (pDirectoryEntries)
    {
        DirectoryFreeEntries(pDirectoryEntries, dwNumDomains);
    }

    goto cleanup;
}

DWORD
SamDbSearchGroups(
    HANDLE            hDirectory,
    PWSTR             pwszBase,
    ULONG             ulScope,
    PWSTR             pwszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
