#include "includes.h"

#define DB_QUERY_CREATE_GROUPMEMBERSHIP_TABLE \
    "create table samdbgroupmembers (         \
                    Gid integer,              \
                    Uid integer               \
                    )"

#define DB_QUERY_CREATE_GROUPS_TABLE \
    "create table samdbgroups (      \
                    Gid           integer PRIMARY KEY, \
                    Name          varchar(256),        \
                    Passwd        varchar(256),        \
                    CreatedTime   date                 \
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
          delete from samdbgroupmembers where Gid = old.Gid;   \
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

