#include "includes.h"

DWORD
SamDbAddGroupAttrLookups(
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup
    )
{
    DWORD dwError = 0;
    struct {
        PSTR pszAttrName;
        DIRECTORY_ATTR_TYPE attrType;
        SAMDB_GROUP_TABLE_COLUMN colType;
        BOOL bIsMandatory;
        BOOL bIsModifiable;
    } groupAttrs[] =
    {
        {
            DIRECTORY_ATTR_TAG_GROUP_NAME,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            SAMDB_GROUP_TABLE_COLUMN_NAME,
            ATTR_IS_MANDATORY,
            ATTR_IS_IMMUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_GID,
            DIRECTORY_ATTR_TYPE_INTEGER,
            SAMDB_GROUP_TABLE_COLUMN_GID,
            ATTR_IS_MANDATORY,
            ATTR_IS_IMMUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_GROUP_SID,
            DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR,
            SAMDB_GROUP_TABLE_COLUMN_SID,
            ATTR_IS_MANDATORY,
            ATTR_IS_IMMUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_GROUP_PASSWORD,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            SAMDB_GROUP_TABLE_COLUMN_PASSWORD,
            ATTR_IS_NOT_MANDATORY,
            ATTR_IS_MUTABLE
        },
        {
            DIRECTORY_ATTR_TAG_GROUP_MEMBERS,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            SAMDB_GROUP_TABLE_COLUMN_MEMBERS,
            ATTR_IS_NOT_MANDATORY,
            ATTR_IS_MUTABLE
        }
    };
    DWORD dwNumAttrs = sizeof(groupAttrs)/sizeof(groupAttrs[0]);
    DWORD iAttr = 0;
    PSAMDB_ATTRIBUTE_LOOKUP_ENTRY pAttrEntry = NULL;

    for(; iAttr < dwNumAttrs; iAttr++)
    {
        dwError = DirectoryAllocateMemory(
                        sizeof(SAMDB_ATTRIBUTE_LOOKUP_ENTRY),
                        (PVOID*)&pAttrEntry);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaMbsToWc16s(
                        groupAttrs[iAttr].pszAttrName,
                        &pAttrEntry->pwszAttributeName);
        BAIL_ON_SAMDB_ERROR(dwError);

        pAttrEntry->bIsMandatory = groupAttrs[iAttr].bIsMandatory;
        pAttrEntry->bIsModifiable = groupAttrs[iAttr].bIsModifiable;
        pAttrEntry->dwId = groupAttrs[iAttr].colType;
        pAttrEntry->attrType = groupAttrs[iAttr].attrType;

        dwError = LwRtlRBTreeAdd(
                        pAttrLookup->pAttrTree,
                        pAttrEntry->pwszAttributeName,
                        pAttrEntry);
        BAIL_ON_SAMDB_ERROR(dwError);

        pAttrEntry = NULL;
    }

cleanup:

    return dwError;

error:

    if (pAttrEntry)
    {
        SamDbFreeAttributeLookupEntry(pAttrEntry);
    }

    goto cleanup;
}

DWORD
SamDbAddGroup(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PSTR  pszGroupDN = NULL;
    PWSTR pwszGroupName = NULL;
    PWSTR pwszDomain = NULL;
    PSTR  pszDomainName = NULL;
    PSTR  pszGroupName = NULL;
    PSTR  pszGroupSID = NULL;
    PSTR  pszPassword = NULL;
    PSTR  pszDomainSID = NULL;
    DWORD dwGID = 0;
    BOOLEAN bGIDSpecified = FALSE;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    BOOLEAN bInLock = FALSE;
    DWORD dwNumMods = 0;
    DWORD iMod = 0;
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    PSAM_DB_DOMAIN_INFO* ppDomainInfoList = NULL;
    DWORD dwNumDomains = 0;
    BOOLEAN bTxStarted = FALSE;
    SAMDB_OBJECT_CLASS objectClass = SAMDB_OBJECT_CLASS_GROUP;
    PCSTR pszQueryTemplate = "INSERT INTO " SAM_DB_OBJECTS_TABLE \
                                "(ObjectRecordId,"        \
                                 "ObjectSID,"             \
                                 "DistinguishedName,"     \
                                 "ObjectClass,"           \
                                 "GID,"                   \
                                 "Password,"              \
                                 "SamAccountName,"        \
                                 "Domain"                 \
                                ")\n"                     \
                                "VALUES ("                \
   /* ObjectRecordId    */       "rowid,"                 \
   /* ObjectSID         */       "%Q,"                    \
   /* DistinguishedName */       "%Q,"                    \
   /* ObjectClass       */       "%d,"                    \
   /* GID               */       "%d,"                    \
   /* Password          */       "%Q,"                    \
   /* SamAccountName    */       "%Q,"                    \
   /* Domain            */       "%Q"                     \
                                ")";

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    while (modifications[dwNumMods].pwszAttrName &&
           modifications[dwNumMods].pAttrValues)
    {
        dwNumMods++;
    }

    dwError = LsaWc16sToMbs(
                    pwszObjectDN,
                    &pszGroupDN);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = SamDbParseDN(
                    pwszObjectDN,
                    &pwszGroupName,
                    &pwszDomain,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (entryType != SAMDB_ENTRY_TYPE_GROUP)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbFindDomains(
                    hDirectory,
                    pwszDomain,
                    &ppDomainInfoList,
                    &dwNumDomains);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumDomains != 1)
    {
        dwError = LSA_ERROR_NO_SUCH_DOMAIN;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pDirContext->rwLock);

    SAM_DB_BEGIN_TRANSACTION(bTxStarted, hDirectory);

    dwError = LsaWc16sToMbs(
                    pwszGroupName,
                    &pszGroupName);
    BAIL_ON_SAMDB_ERROR(dwError);

    for (; iMod < dwNumMods; iMod++)
    {
        NTSTATUS ntStatus = 0;
        PSAMDB_ATTRIBUTE_LOOKUP_ENTRY pLookupEntry = NULL;
        PATTRIBUTE_VALUE pAttrValue = NULL;

        ntStatus = LwRtlRBTreeFind(
                        pDirContext->pAttrLookup->pAttrTree,
                        modifications[iMod].pwszAttrName,
                        (PVOID*)&pLookupEntry);
        if (ntStatus)
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_SAMDB_ERROR(dwError);
        }

        switch (pLookupEntry->dwId)
        {
            case SAMDB_GROUP_TABLE_COLUMN_GID:

                if ((modifications[iMod].ulNumValues != 1) ||
                    (modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_INTEGER)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwGID = pAttrValue->ulValue;
                bGIDSpecified = TRUE;

                break;

            case SAMDB_GROUP_TABLE_COLUMN_PASSWORD:

                if ((modifications[iMod].ulNumValues != 1) ||
                    (modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttrValue->pwszStringValue,
                                &pszPassword);
                BAIL_ON_SAMDB_ERROR(dwError);

                break;

            case SAMDB_GROUP_TABLE_COLUMN_SID:

                if ((modifications[iMod].ulNumValues != 1) ||
                    (modifications[iMod].ulOperationFlags != DIR_MOD_FLAGS_ADD))
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                pAttrValue = &modifications[iMod].pAttrValues[0];
                if (pAttrValue->Type != DIRECTORY_ATTR_TYPE_UNICODE_STRING)
                {
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_SAMDB_ERROR(dwError);
                }

                dwError = LsaWc16sToMbs(
                                pAttrValue->pwszStringValue,
                                &pszGroupSID);
                BAIL_ON_SAMDB_ERROR(dwError);

                break;

            case SAMDB_GROUP_TABLE_COLUMN_MEMBERS:

                // TODO: Validate and add group members in transaction

                break;

            default:

                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);

                break;
        }
    }

    if (!bGIDSpecified)
    {
        dwError = SamDbGetNextAvailableGID(
                        hDirectory,
                        &dwGID);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    if (!pszGroupSID)
    {
        DWORD dwRID = 0;

        dwError = SamDbGetNextAvailableRID(
                        hDirectory,
                        &dwRID);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaWc16sToMbs(
                        ppDomainInfoList[0]->pwszDomainSID,
                        &pszDomainSID);
        BAIL_ON_SAMDB_ERROR(dwError);

        dwError = LsaAllocateStringPrintf(
                        &pszGroupSID,
                        "%s-%u",
                        pszDomainSID,
                        dwRID);
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = LsaWc16sToMbs(
                    ppDomainInfoList[0]->pwszDomainName,
                    &pszDomainName);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (IsNullOrEmptyString(pszGroupSID) ||
        IsNullOrEmptyString(pszGroupName) ||
        !dwGID)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pszQuery = sqlite3_mprintf(
                    pszQueryTemplate,
                    pszGroupSID,
                    pszGroupDN,
                    objectClass,
                    dwGID,
                    pszPassword,
                    pszGroupName,
                    pszDomainName);

    dwError = sqlite3_exec(pDirContext->pDbContext->pDbHandle,
                           pszQuery,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    SAM_DB_END_TRANSACTION(bTxStarted, dwError, hDirectory);

    if (pszQuery)
    {
        sqlite3_free(pszQuery);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &pDirContext->rwLock);

    DIRECTORY_FREE_STRING(pszGroupName);
    DIRECTORY_FREE_STRING(pszGroupSID);
    DIRECTORY_FREE_STRING(pszPassword);
    DIRECTORY_FREE_STRING(pszDomainSID);
    DIRECTORY_FREE_STRING(pszGroupDN);
    DIRECTORY_FREE_STRING(pszDomainName);

    if (ppDomainInfoList)
    {
        SamDbFreeDomainInfoList(ppDomainInfoList, dwNumDomains);
    }

    return dwError;

error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

DWORD
SamDbNumMembersInGroup_inlock(
    HANDLE hDirectory,
    PCSTR  pszGroupName,
    PCSTR  pszDomainName,
    PDWORD pdwNumGroupMembers
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    int   nRows = 0;
    int   nCols = 0;
    PSTR* ppszResult = NULL;
    DWORD dwNumGroupMembers = 0;
    PSTR  pszQueryTemplate =
            "SELECT COUNT(*)" \
            "  FROM " SAM_DB_MEMBERS_TABLE "sdm" \
            " WHERE sdm.GroupRecordId IN " \
            "SELECT ObjectRecordId FROM " SAM_DB_OBJECTS_TABLE "sdo" \
            " WHERE sdo.Domain = %Q" \
            "   AND sdo.SamAccountName = %Q" \
            "   AND sdo.ObjectClass = 3";

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    pszQuery = sqlite3_mprintf(
                    pszQueryTemplate,
                    pszGroupName,
                    pszDomainName);

    dwError = sqlite3_get_table(
                    pDirContext->pDbContext->pDbHandle,
                    pszQuery,
                    &ppszResult,
                    &nRows,
                    &nCols,
                    &pszError);
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows)
    {
        dwNumGroupMembers = 0;
        goto done;
    }

    if (nCols != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwNumGroupMembers = atoi(ppszResult[1]);

done:

    *pdwNumGroupMembers = dwNumGroupMembers;

cleanup:

    if (pszQuery)
    {
        sqlite3_free(pszQuery);
    }

    if (ppszResult)
    {
        sqlite3_free_table(ppszResult);
    }

    return dwError;

error:

    *pdwNumGroupMembers = 0;

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

DWORD
SamDbFindGID_inlock(
    HANDLE hDirectory,
    PWSTR  pwszGroupDN,
    PDWORD pdwGID
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PWSTR pwszGroupName = NULL;
    PWSTR pwszDomainName = NULL;
    PSTR  pszDomainName = NULL;
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    int   nRows = 0;
    int   nCols = 0;
    PSTR* ppszResult = NULL;
    PCSTR pszQueryTemplate =
            "SELECT GID FROM " SAM_DB_OBJECTS_TABLE \
            " WHERE Domain = %Q"         \
            "   AND SamAccountName = %Q" \
            "   AND ObjectClass = 3";

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    dwError = SamDbParseDN(
                    pwszGroupDN,
                    &pwszGroupName,
                    &pwszDomainName,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    if ((entryType != SAMDB_ENTRY_TYPE_GROUP) ||
        (!pwszDomainName || !*pwszDomainName))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = LsaWc16sToMbs(
                    pwszDomainName,
                    &pszDomainName);
    BAIL_ON_SAMDB_ERROR(dwError);

    pszQuery = sqlite3_mprintf(
                    pszQueryTemplate,
                    pszDomainName,
                    pwszGroupName);

    dwError = sqlite3_get_table(
                    pDirContext->pDbContext->pDbHandle,
                    pszQuery,
                    &ppszResult,
                    &nRows,
                    &nCols,
                    &pszError);
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows)
    {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    if (nCols != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pdwGID = atoi(ppszResult[1]);

cleanup:

    if (pszQuery)
    {
        sqlite3_free(pszQuery);
    }

    if (ppszResult)
    {
        sqlite3_free_table(ppszResult);
    }

    DIRECTORY_FREE_STRING(pszDomainName);

    return dwError;

error:

    *pdwGID = 0;

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

DWORD
SamDbDeleteGroup(
    HANDLE hDirectory,
    PWSTR  pwszObjectDN
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PWSTR pwszObjectName = NULL;
    PWSTR pwszDomainName = NULL;
    PSTR  pszGroupName = NULL;
    PSTR  pszDomainName = NULL;
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    BOOLEAN bInLock = FALSE;
    DWORD dwNumGroupMembers = 0;
    PCSTR pszQueryTemplate =
          "DELETE FROM " SAM_DB_OBJECTS_TABLE \
          " WHERE Domain = %Q"                \
          "   AND SamAccountName = %Q"        \
          "   AND ObjectClass = 3";

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    dwError = SamDbParseDN(
                    pwszObjectDN,
                    &pwszObjectName,
                    &pwszDomainName,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    if ((entryType != SAMDB_ENTRY_TYPE_GROUP) ||
        (!pwszDomainName || !*pwszDomainName))
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = LsaWc16sToMbs(
                    pwszDomainName,
                    &pszDomainName);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaWc16sToMbs(
                    pwszObjectName,
                    &pszGroupName);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pDirContext->rwLock);

    dwError = SamDbNumMembersInGroup_inlock(
                    hDirectory,
                    pszGroupName,
                    pszDomainName,
                    &dwNumGroupMembers);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumGroupMembers)
    {
        dwError = LSA_ERROR_GROUP_IN_USE;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pszQuery = sqlite3_mprintf(
                    pszQueryTemplate,
                    pszGroupName,
                    pszDomainName);

    dwError = sqlite3_exec(
                    pDirContext->pDbContext->pDbHandle,
                    pszQuery,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    if (pszQuery)
    {
        sqlite3_free(pszQuery);
    }

    SAMDB_UNLOCK_RWMUTEX(bInLock, &pDirContext->rwLock);

    DIRECTORY_FREE_MEMORY(pwszObjectName);
    DIRECTORY_FREE_MEMORY(pwszDomainName);
    DIRECTORY_FREE_MEMORY(pszGroupName);
    DIRECTORY_FREE_STRING(pszDomainName);

    return dwError;

error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}


