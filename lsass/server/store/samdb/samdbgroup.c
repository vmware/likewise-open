#include "includes.h"

#define DB_QUERY_CREATE_GROUPMEMBERSHIP_TABLE \
    "create table samdbgroupmembers (         \
                    GroupRecordId  integer,   \
                    MemberRecordId integer,   \
                    MemberType     integer,   \
                    unique(GroupRecordId, MemberRecordId, MemberType) \
                    check (MemberType == 0 OR MemberType == 1) \
                    )"

#define DB_QUERY_CREATE_GROUPS_TABLE \
    "create table samdbgroups (      \
                    GroupRecordId  integer PRIMARY KEY, \
                    DomainRecordId integer,             \
                    ObjectSID      text unique,         \
                    Gid            integer,             \
                    Name           text,                \
                    Passwd         text,                \
                    CreatedTime    date,                \
                    unique(DomainRecordId, Gid),        \
                    unique(DomainRecordId, Name)        \
                    )"

#define DB_QUERY_INSERT_GROUP \
    "INSERT INTO samdbgroups \
        (                    \
            GroupRecordId,   \
            DomainRecordId,  \
            ObjectSID,       \
            Gid,             \
            Name,            \
            Passwd           \
        )                    \
     VALUES (                \
            NULL,            \
            %d,              \
            %Q,              \
            %d,              \
            %Q,              \
            %Q               \
        )"

#define DB_QUERY_CREATE_GROUPS_INSERT_TRIGGER                  \
    "create trigger samdbgroups_createdtime                    \
     after insert on samdbgroups                               \
     begin                                                     \
          update samdbgroups set CreatedTime = DATETIME('NOW') \
          where rowid = new.rowid;                             \
     end"

#define DB_QUERY_CREATE_GROUPS_DELETE_TRIGGER                  \
    "create trigger samdbgroups_delete_record                  \
     after delete on samdbgroups                               \
     begin                                                     \
       delete from samdbgroupmembers where GroupRecordId = old.GroupRecordId; \
     end"

#define DB_QUERY_NUM_GROUPS_IN_DOMAIN \
    "select count(*) \
       from samdbgroups  sdg, \
            samdbdomains sdd  \
      where sdg.DomainRecordId = sdd.DomainRecordId \
        and sdd.Name = %Q"

#define DB_QUERY_NUM_MEMBERS_IN_GROUP \
    "select count(*) \
       from samdbgroupmembers sgm, \
            samdbgroups sdg \
      where sdg.Name = %Q \
        and sdg.GroupRecordId = sgm.GroupRecordId \
        and sdg.DomainRecordId = %d"

#define DB_QUERY_MAX_GID_BY_DOMAIN \
    "select max(Gid) \
       from samdbgroups sdg \
      where sdg.DomainRecordId = %d"

#define DB_QUERY_GROUP_RECORD_ID \
    "select GroupRecordId \
       from samdbgroups sdg \
      where sdg.Name = %Q \
        and sdg.DomainRecordId = %d"

#define DB_QUERY_DELETE_GROUP \
    "delete \
       from samdbgroups sdg \
      where sdg.Name = %Q \
        and sdg.DomainRecordId = %d"

DWORD
SamDbInitGroupTable(
    PSAM_DB_CONTEXT pDbContext
    )
{
    DWORD dwError = 0;
    PSTR pszError = NULL;

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    DB_QUERY_CREATE_GROUPMEMBERSHIP_TABLE,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    DB_QUERY_CREATE_GROUPS_TABLE,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    DB_QUERY_CREATE_GROUPS_INSERT_TRIGGER,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_exec(
                    pDbContext->pDbHandle,
                    DB_QUERY_CREATE_GROUPS_DELETE_TRIGGER,
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    return dwError;

error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

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
    PWSTR pwszGroupName = NULL;
    PWSTR pwszDomain = NULL;
    PSTR  pszGroupName = NULL;
    PSTR  pszGroupSID = NULL;
    PSTR  pszPassword = NULL;
    PSTR  pszDomainSID = NULL;
    DWORD dwGID = 0;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    BOOLEAN bInLock = FALSE;
    DWORD dwNumMods = 0;
    DWORD iMod = 0;
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    PSAM_DB_DOMAIN_INFO* ppDomainInfoList = NULL;
    DWORD dwNumDomains = 0;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    while (modifications[dwNumMods].pwszAttrName &&
                    modifications[dwNumMods].pAttrValues)
    {
        dwNumMods++;
    }

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

            case SAMDB_GROUP_TABLE_COLUMN_MEMBERS:

                // TODO: Validate and add group members in transaction

                break;

            default:

                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_SAMDB_ERROR(dwError);

                break;
        }
    }

    dwError = SamDbGetNextAvailableGID_inlock(
                    hDirectory,
                    ppDomainInfoList[0]->ulDomainRecordId,
                    &dwGID);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaWc16sToMbs(
                    ppDomainInfoList[0]->pwszDomainSID,
                    &pszDomainSID);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                    &pszGroupSID,
                    "%s-%d",
                    pszDomainSID,
                    dwGID);
    BAIL_ON_SAMDB_ERROR(dwError);

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_INSERT_GROUP,
                    ppDomainInfoList[0]->ulDomainRecordId,
                    pszGroupSID,
                    dwGID,
                    pszGroupName,
                    pszPassword);

    dwError = sqlite3_exec(pDirContext->pDbContext->pDbHandle,
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

    DIRECTORY_FREE_STRING(pszGroupName);
    DIRECTORY_FREE_STRING(pszGroupSID);
    DIRECTORY_FREE_STRING(pszPassword);
    DIRECTORY_FREE_STRING(pszDomainSID);

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
SamDbNumGroupsInDomain_inlock(
    HANDLE hDirectory,
    PSTR   pszDomainName,
    PDWORD pdwNumGroups
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    int   nRows = 0;
    int   nCols = 0;
    PSTR* ppszResult = NULL;
    DWORD dwNumGroups = 0;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_NUM_GROUPS_IN_DOMAIN,
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
        dwNumGroups = 0;
        goto done;
    }

    if (nCols != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwNumGroups = atoi(ppszResult[1]);

done:

    *pdwNumGroups = dwNumGroups;

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

    *pdwNumGroups = 0;

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

DWORD
SamDbNumMembersInGroup_inlock(
    HANDLE hDirectory,
    PSTR   pszGroupName,
    DWORD  dwDomainRecordId,
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

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_NUM_MEMBERS_IN_GROUP,
                    pszGroupName,
                    dwDomainRecordId);

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
SamDbFindGroupRecordId_inlock(
    HANDLE              hDirectory,
    PSAM_DB_DOMAIN_INFO pDefaultDomainInfo,
    PWSTR               pwszGroupDN,
    PDWORD              pdwGroupRecordId
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PWSTR pwszGroupName = NULL;
    PWSTR pwszDomainName = NULL;
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    PSAM_DB_DOMAIN_INFO* ppDomainInfoList = NULL;
    DWORD dwNumDomains = 0;
    PSAM_DB_DOMAIN_INFO pDomainInfo = pDefaultDomainInfo;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    int   nRows = 0;
    int   nCols = 0;
    PSTR* ppszResult = NULL;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    dwError = SamDbParseDN(
                    pwszGroupDN,
                    &pwszGroupName,
                    &pwszDomainName,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (entryType != SAMDB_ENTRY_TYPE_GROUP)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    if (pwszDomainName && *pwszDomainName)
    {
        dwError = SamDbFindDomains(
                        hDirectory,
                        pwszDomainName,
                        &ppDomainInfoList,
                        &dwNumDomains);
        BAIL_ON_SAMDB_ERROR(dwError);

        if (dwNumDomains > 0)
        {
            pDomainInfo = ppDomainInfoList[0];
        }
    }

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_GROUP_RECORD_ID,
                    pDomainInfo->ulDomainRecordId,
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

    *pdwGroupRecordId = atoi(ppszResult[1]);

cleanup:

    if (pszQuery)
    {
        sqlite3_free(pszQuery);
    }

    if (ppszResult)
    {
        sqlite3_free_table(ppszResult);
    }

    if (ppDomainInfoList)
    {
        SamDbFreeDomainInfoList(ppDomainInfoList, dwNumDomains);
    }

    return dwError;

error:

    *pdwGroupRecordId = 0;

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

DWORD
SamDbGetNextAvailableGID_inlock(
    HANDLE hDirectory,
    DWORD  dwDomainRecordId,
    PDWORD pdwGID
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    int   nRows = 0;
    int   nCols = 0;
    PSTR* ppszResult = NULL;
    DWORD dwMaxGID = 0;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_MAX_GID_BY_DOMAIN,
                    dwDomainRecordId);

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
        dwMaxGID = SAMDB_MIN_GID - 1;
        goto done;
    }

    if (nCols != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwMaxGID = atoi(ppszResult[1]);

done:

    *pdwGID = dwMaxGID + 1;

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
    SAMDB_ENTRY_TYPE entryType = SAMDB_ENTRY_TYPE_UNKNOWN;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    BOOLEAN bInLock = FALSE;
    DWORD dwNumGroupMembers = 0;
    PSAM_DB_DOMAIN_INFO* ppDomainInfoList = NULL;
    DWORD dwNumDomains = 0;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    dwError = SamDbParseDN(
                    pwszObjectDN,
                    &pwszObjectName,
                    &pwszDomainName,
                    &entryType);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (entryType != SAMDB_ENTRY_TYPE_GROUP)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = SamDbFindDomains(
                    hDirectory,
                    pwszDomainName,
                    &ppDomainInfoList,
                    &dwNumDomains);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumDomains == 0)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwError = LsaWc16sToMbs(
                    pwszObjectName,
                    &pszGroupName);
    BAIL_ON_SAMDB_ERROR(dwError);

    SAMDB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pDirContext->rwLock);

    dwError = SamDbNumMembersInGroup_inlock(
                    hDirectory,
                    pszGroupName,
                    ppDomainInfoList[0]->ulDomainRecordId,
                    &dwNumGroupMembers);
    BAIL_ON_SAMDB_ERROR(dwError);

    if (dwNumGroupMembers)
    {
        dwError = LSA_ERROR_GROUP_IN_USE;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_DELETE_GROUP,
                    pszGroupName,
                    ppDomainInfoList[0]->ulDomainRecordId);

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

    if (pwszObjectName)
    {
        DirectoryFreeMemory(pwszObjectName);
    }
    if (pwszDomainName)
    {
        DirectoryFreeMemory(pwszDomainName);
    }
    if (pszGroupName)
    {
        DirectoryFreeString(pszGroupName);
    }

    return dwError;

error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }

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

