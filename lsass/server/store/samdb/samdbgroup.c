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
                    unique(DomainRecordId, Name),       \
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

