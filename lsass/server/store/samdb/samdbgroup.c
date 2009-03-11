#include "includes.h"

#if 0
#define DB_QUERY_CREATE_GROUPMEMBERSHIP_TABLE                  \
    "create table lwigroupmembers (Gid integer,                \
                                   Uid integer                 \
                                  )"

#define DB_QUERY_CREATE_GROUPS_TABLE                           \
    "create table lwigroups (GroupRecordId integer PRIMARY KEY,\
                             Name          varchar(256),       \
                             Passwd        varchar(256),       \
                             Gid           integer,            \
                             CreatedTime   date                \
                            )"

#define DB_QUERY_CREATE_GROUPS_INSERT_TRIGGER                  \
    "create trigger lwigroups_createdtime                      \
     after insert on lwigroups                                 \
     begin                                                     \
          update lwigroups set CreatedTime = DATETIME('NOW')   \
          where rowid = new.rowid;                             \
     end"

#define DB_QUERY_CREATE_GROUPS_DELETE_TRIGGER                  \
    "create trigger lwigroups_delete_record                    \
     after delete on lwigroups                                 \
     begin                                                     \
          delete from lwigroupmembers where Gid = old.Gid;     \
     end"

NTSTATUS
SamDbInitGroupTable(
    HANDLE hDb
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR pszError = NULL;

    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_GROUPMEMBERSHIP_TABLE,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_GROUPS_TABLE,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_GROUPS_INSERT_TRIGGER,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_GROUPS_DELETE_TRIGGER,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

error:

    return dwError;
}

#endif
