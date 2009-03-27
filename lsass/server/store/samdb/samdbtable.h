/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samdbtable.h
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      Database Schema
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#ifndef __SAM_DB_TABLE_H__
#define __SAM_DB_TABLE_H__

#define SAM_DB_SCHEMA_VERSION 1
#define SAM_DB_MIN_UID        1000
#define SAM_DB_MIN_GID        1000
#define SAM_DB_MIN_RID        1000

#define SAM_DB_CONFIG_TABLE  "samdbconfig"
#define SAM_DB_OBJECTS_TABLE "samdbobjects"
#define SAM_DB_MEMBERS_TABLE "samdbmembers"

#define SAM_DB_QUERY_CREATE_TABLES  \
    "CREATE TABLE " SAM_DB_CONFIG_TABLE " (\n"                                       \
                 "UIDCounter INTEGER,\n"                                             \
                 "GIDCounter INTEGER,\n"                                             \
                 "RIDCounter INTEGER,\n"                                             \
                 "Version    INTEGER\n"                                              \
                 ");\n"                                                              \
    "CREATE TABLE " SAM_DB_OBJECTS_TABLE " (\n"                                      \
                 "ObjectRecordId    INTEGER PRIMARY KEY AUTOINCREMENT,\n"            \
                 "ObjectSID         TEXT COLLATE NOCASE,\n"                          \
                 "DistinguishedName TEXT COLLATE NOCASE,\n"                          \
                 "ParentDN          TEXT,\n"                                         \
                 "ObjectClass       INTEGER,\n"                                      \
                 "Domain            TEXT NOT NULL,\n"                                \
                 "NetBIOSName       TEXT,\n"                                         \
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
                 ");\n"                                                                  \
    "CREATE TRIGGER samdbobjects_delete_object \n"                                       \
    "AFTER  DELETE on " SAM_DB_OBJECTS_TABLE "\n"                                        \
    "BEGIN\n"                                                                            \
    "  DELETE FROM " SAM_DB_MEMBERS_TABLE " WHERE GroupRecordId = old.ObjectRecordId;\n" \
    "END;\n"

#endif /* __SAM_DB_TABLE_H__ */
