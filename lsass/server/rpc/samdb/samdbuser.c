


#define DB_QUERY_CREATE_USERS_TABLE                               \
    "create table lwiusers (UserRecordId integer PRIMARY KEY,     \
                            Name             varchar(256),        \
                            Passwd           varchar(256),        \
                            Uid              integer,             \
                            Gid              integer,             \
                            UserInfoFlags    integer,             \
                            Gecos            varchar(256),        \
                            HomeDir          varchar(1024),       \
                            Shell            varchar(128),        \
                            PasswdChangeTime integer,             \
                            FullName         varchar(256),        \
                            AccountExpiry    integer,             \
                            LMOwf_1          integer,             \
                            LMOwf_2          integer,             \
                            LMOwf_3          integer,             \
                            LMOwf_4          integer,             \
                            NTOwf_1          integer,             \
                            NTOwf_2          integer,             \
                            NTOwf_3          integer,             \
                            NTOwf_4          integer,             \
                            CreatedTime      date                 \
                           )"


NTSTATUS
SamDbInitUserTable()
{
    DWORD dwError = 0;

    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_USERS_TABLE,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_USERS_INSERT_TRIGGER,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_USERS_DELETE_TRIGGER,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return(dwError);
}
