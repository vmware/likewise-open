#ifndef __SAM_DB_TABLE_H__
#define __SAM_DB_TABLE_H__

#define SAM_DB_QUERY_CREATE_TABLES  \
    "create table sambdobjects (\n" \
                 "ObjectRecordId    integer PRIMARY KEY,\n" \
                 "ObjectSID         text unique,\n"         \
                 "DistinguishedName text unique,\n"         \
                 "ParentDN          text,\n"                \
                 "ObjectClass       integer,\n"             \
                 "Domain            text,\n"                \
                 "CommonName        text,\n"                \
                 "SamAccountName    text unique,\n"         \
                 "Description       text,\n"                \
                 "UID               integer,\n"             \
                 "UserPasswd        text,\n"                \
                 "UserInfoFlags     integer,\n"             \
                 "Gecos             text,\n"                \
                 "HomeDir           text,\n"                \
                 "Shell             text,\n"                \
                 "PasswdChangeTime  integer,\n"             \
                 "FullName          text,\n"                \
                 "AccountExpiry     integer,\n"             \
                 "LMOwf_1           integer,\n"             \
                 "LMOwf_2           integer,\n"             \
                 "LMOwf_3           integer,\n"             \
                 "LMOwf_4           integer,\n"             \
                 "NTOwf_1           integer,\n"             \
                 "NTOwf_2           integer,\n"             \
                 "NTOwf_3           integer,\n"             \
                 "NTOwf_4           integer,\n"             \
                 "GID               integer,\n"             \
                 "GroupPasswd       text,\n"                \
                 "CreatedTime       date\n"                 \
                 ");\n"                                     \
    "create table samdbmembers (\n"                         \
                 "GroupRecordId groupId,\n"                 \
                 "MemberRecordId memberId,\n"               \
                 "CreatedTime    date\n)"

#endif /* __SAM_DB_TABLE_H__ */
