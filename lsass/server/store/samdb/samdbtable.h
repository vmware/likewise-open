#ifndef __SAM_DB_TABLE_H__
#define __SAM_DB_TABLE_H__

#define SAM_DB_OBJECTS_TABLE "samdbobjects"
#define SAM_DB_MEMBERS_TABLE "samdbmembers"

#define SAM_DB_QUERY_CREATE_TABLES  \
    "CREATE TABLE " SAM_DB_OBJECTS_TABLE " (\n"                                      \
                 "ObjectRecordId    INTEGER PRIMARY KEY AUTOINCREMENT,\n"            \
                 "ObjectSID         TEXT COLLATE NOCASE,\n"                          \
                 "DistinguishedName TEXT COLLATE NOCASE,\n"                          \
                 "ParentDN          TEXT,\n"                                         \
                 "ObjectClass       INTEGER,\n"                                      \
                 "Domain            TEXT,\n"                                         \
                 "CommonName        TEXT,\n"                                         \
                 "SamAccountName    TEXT COLLATE NOCASE,\n"                          \
                 "Description       TEXT,\n"                                         \
                 "UID               INTEGER,\n"                                      \
                 "Password          TEXT,\n"                                         \
                 "UserInfoFlags     INTEGER,\n"                                      \
                 "Gecos             TEXT,\n"                                         \
                 "HomeDir           TEXT,\n"                                         \
                 "Shell             TEXT,\n"                                         \
                 "PasswdChangeTime  INTEGER,\n"                                      \
                 "FullName          TEXT,\n"                                         \
                 "AccountExpiry     INTEGER,\n"                                      \
                 "LMHash            BLOB,\n"                                         \
                 "NTHash            BLOB,\n"                                         \
                 "GID               INTEGER,\n"                                      \
                 "CreatedTime       DATE NOT NULL DEFAULT (DATETIME('now')),\n"      \
                 "UNIQUE(ObjectSID, DistinguishedName),\n"                           \
                 "UNIQUE(DistinguishedName, ParentDN),\n"                            \
                 "CHECK(ObjectClass == 1 OR \n"                                      \
                 "      ObjectClass == 2 OR \n"                                      \
                 "      ObjectClass == 3 OR \n"                                      \
                 "      ObjectClass == 4)\n"                                         \
                 ");\n"                                                              \
    "CREATE TABLE " SAM_DB_MEMBERS_TABLE " (\n"                                      \
                 "GroupRecordId  INTEGER,\n"                                         \
                 "MemberRecordId INTEGER,\n"                                         \
                 "CreatedTime    DATE NOT NULL DEFAULT (DATETIME('now')),\n"         \
                 "UNIQUE(GroupRecordId, MemberRecordId),\n"                          \
                 "FOREIGN KEY (GroupRecordId) REFERENCES " SAM_DB_OBJECTS_TABLE " (ObjectRecordId),\n" \
                 "FOREIGN KEY (MemberRecordId) REFERENCES " SAM_DB_OBJECTS_TABLE " (ObjectRecordId)\n" \
                 ");\n"

#endif /* __SAM_DB_TABLE_H__ */
