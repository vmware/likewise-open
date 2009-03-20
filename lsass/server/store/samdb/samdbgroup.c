#include "includes.h"

#define DB_QUERY_CREATE_GROUPMEMBERSHIP_TABLE \
    "create table samdbgroupmembers (         \
                    GroupRecordId  integer,   \
                    UserRecordId   integer,   \
                    DomainRecordId integer,   \
                    unique(DomainRecordId, GroupRecordId, UserRecordId) \
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
          delete from samdbgroupmembers where GroupRecordId = old.GroupRecordId;   \
     end"

#define DB_QUERY_NUM_GROUPS_IN_DOMAIN \
    "select count(*) \
       from samdbgroups  sdg, \
            samdbdomains sdd  \
      where sdg.DomainRecordId = sdd.DomainRecordId \
        and sdd.Name = %Q"

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
        BOOL bIsMandatory;
        BOOL bIsModifiable;
    } groupAttrs[] =
    {
        {
            DIRECTORY_ATTR_TAG_GROUP_NAME,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            TRUE,
            FALSE
        },
        {
            DIRECTORY_ATTR_TAG_GID,
            DIRECTORY_ATTR_TYPE_INTEGER,
            TRUE,
            FALSE
        },
        {
            DIRECTORY_ATTR_TAG_GROUP_SID,
            DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR,
            TRUE,
            TRUE
        },
        {
            DIRECTORY_ATTR_TAG_GROUP_PASSWORD,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            FALSE,
            TRUE
        },
        {
            DIRECTORY_ATTR_TAG_GROUP_MEMBERS,
            DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            FALSE,
            TRUE
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
    PWSTR         pwszObjectName,
    DIRECTORY_MOD Modifications[]
    )
{
    DWORD dwError = 0;
    PSAM_DIRECTORY_CONTEXT pDirContext = NULL;
    DWORD dwNumModifications = 0;
    DWORD iModification = 0;

    pDirContext = (PSAM_DIRECTORY_CONTEXT)hDirectory;

    if (!pDirContext || !pwszObjectName || !*pwszObjectName)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_SAMDB_ERROR(dwError);
    }

    dwNumModifications = sizeof(Modifications)/sizeof(Modifications[0]);
    for (; iModification < dwNumModifications; iModification++)
    {
        // PDIRECTORY_MOD pMod = &Modifications[iModification];
    }

cleanup:

    return dwError;

error:

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


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
