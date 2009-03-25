
#define DB_QUERY_CREATE_OBJECT_TABLE  \
    "create table sambdobjects (       \
                ObjectRecordId     integer PRIMARY KEY, \
                ObjectSID        text unique,         \
                distinguishedName text unique       \
                parentRecordId    integer
                objectClass       integer
                CreatedTime       date
/* Domain Attributes
                dc                text  \
/* User and Group Attributes */
                cn               text    \
                samAccountName    text   unique             \
                description       text   \
/* user attributes */
                Uid              integer,             \
                UserPasswd           text,                \
                UserInfoFlags    integer,             \
                Gecos            text,                \
                HomeDir          text,                \
                Shell            text,                \
                PasswdChangeTime integer,             \
                FullName         text,                \
                AccountExpiry    integer,             \
                LMOwf_1          integer,             \
                LMOwf_2          integer,             \
                LMOwf_3          integer,             \
                LMOwf_4          integer,             \
                NTOwf_1          integer,             \
                NTOwf_2          integer,             \
                NTOwf_3          integer,             \
                NTOwf_4          integer,             \
/*Group Attributes*/
                Gid            integer,             \
                GroupPasswd         text,                \
               )"
