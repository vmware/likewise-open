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
 *          Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 *
 */

#ifndef __VMDIR_DB_TABLE_H__
#define __VMDIR_DB_TABLE_H__

#define VMDIR_DB_SCHEMA_VERSION 3

#if !defined(VMDIR_DB_UID_RID_OFFSET)
#define VMDIR_DB_UID_RID_OFFSET     (1000)
#endif
#if !defined(VMDIR_DB_GID_RID_OFFSET)
#define VMDIR_DB_GID_RID_OFFSET     VMDIR_DB_UID_RID_OFFSET
#endif

#define VMDIR_DB_UID_FROM_RID(rid)  (VMDIR_DB_UID_RID_OFFSET + (rid))
#define VMDIR_DB_GID_FROM_RID(rid)  (VMDIR_DB_GID_RID_OFFSET + (rid))

#define VMDIR_DB_ID_FROM_RID_OFFSET (LW_MAX(VMDIR_DB_UID_RID_OFFSET, VMDIR_DB_GID_RID_OFFSET))

#define VMDIR_DB_MIN_RID            (1000)
#define VMDIR_DB_MAX_RID            (0xffffffff -                                 \
                                   (VMDIR_DB_MIN_RID + VMDIR_DB_ID_FROM_RID_OFFSET) \
                                   - 1)
#define VMDIR_DB_MIN_UID            VMDIR_DB_UID_FROM_RID(VMDIR_DB_MIN_RID)
#define VMDIR_DB_MAX_UID            VMDIR_DB_UID_FROM_RID(VMDIR_DB_MAX_RID)
#define VMDIR_DB_MIN_GID            VMDIR_DB_GID_FROM_RID(VMDIR_DB_MIN_RID)
#define VMDIR_DB_MAX_GID            VMDIR_DB_GID_FROM_RID(VMDIR_DB_MAX_RID)

#define VMDIR_DB_CONFIG_TABLE              "samdbconfig"
#define VMDIR_DB_OBJECTS_TABLE             "samdbobjects"
#define VMDIR_DB_MEMBERS_TABLE             "samdbmembers"

#define VMDIR_DB_COL_RECORD_ID             "ObjectRecordId"
#define VMDIR_DB_COL_GROUP_RECORD_ID       "GroupRecordId"
#define VMDIR_DB_COL_MEMBER_RECORD_ID      "MemberRecordId"
#define VMDIR_DB_COL_OBJECT_SID            "ObjectSID"
#define VMDIR_DB_COL_SECURITY_DESCRIPTOR   "SecurityDescriptor"
#define VMDIR_DB_COL_DISTINGUISHED_NAME    "DistinguishedName"
#define VMDIR_DB_COL_PARENT_DN             "ParentDN"
#define VMDIR_DB_COL_OBJECT_CLASS          "ObjectClass"
#define VMDIR_DB_COL_DOMAIN                "Domain"
#define VMDIR_DB_COL_NETBIOS_NAME          "NetBIOSName"
#define VMDIR_DB_COL_COMMON_NAME           "CommonName"
#define VMDIR_DB_COL_SAM_ACCOUNT_NAME      "SamAccountName"
#define VMDIR_DB_COL_USER_PRINCIPAL_NAME   "UserPrincipalName"
#define VMDIR_DB_COL_DESCRIPTION           "Description"
#define VMDIR_DB_COL_COMMENT               "Comment"
#define VMDIR_DB_COL_UID                   "UID"
#define VMDIR_DB_COL_PASSWORD              "Password"
#define VMDIR_DB_COL_ACCOUNT_FLAGS         "AccountFlags"
#define VMDIR_DB_COL_GECOS                 "Gecos"
#define VMDIR_DB_COL_HOME_DIR              "Homedir"
#define VMDIR_DB_COL_HOME_DRIVE            "Homedrive"
#define VMDIR_DB_COL_LOGON_SCRIPT          "LogonScript"
#define VMDIR_DB_COL_PROFILE_PATH          "ProfilePath"
#define VMDIR_DB_COL_WORKSTATIONS          "Workstations"
#define VMDIR_DB_COL_SHELL                 "LoginShell"
#define VMDIR_DB_COL_PASSWORD_LAST_SET     "PasswordLastSet"
#define VMDIR_DB_COL_ALLOW_PASSWORD_CHANGE "AllowPasswordChange"
#define VMDIR_DB_COL_FORCE_PASSWORD_CHANGE "ForcePasswordChange"
#define VMDIR_DB_COL_FULL_NAME             "FullName"
#define VMDIR_DB_COL_PARAMETERS            "Parameters"
#define VMDIR_DB_COL_ACCOUNT_EXPIRY        "AccountExpiry"
#define VMDIR_DB_COL_LM_HASH               "LMHash"
#define VMDIR_DB_COL_NT_HASH               "NTHash"
#define VMDIR_DB_COL_PRIMARY_GROUP         "PrimaryGroup"
#define VMDIR_DB_COL_GID                   "GID"
#define VMDIR_DB_COL_COUNTRY_CODE          "CountryCode"
#define VMDIR_DB_COL_CODE_PAGE             "CodePage"
#define VMDIR_DB_COL_MAX_PWD_AGE           "MaxPwdAge"
#define VMDIR_DB_COL_MIN_PWD_AGE           "MinPwdAge"
#define VMDIR_DB_COL_PWD_PROMPT_TIME       "PwdPromptTime"
#define VMDIR_DB_COL_LAST_LOGON            "LastLogon"
#define VMDIR_DB_COL_LAST_LOGOFF           "LastLogoff"
#define VMDIR_DB_COL_LOCKOUT_TIME          "LockoutTime"
#define VMDIR_DB_COL_LOGON_COUNT           "LogonCount"
#define VMDIR_DB_COL_BAD_PASSWORD_COUNT    "BadPwdCount"
#define VMDIR_DB_COL_LOGON_HOURS           "LogonHours"
#define VMDIR_DB_COL_ROLE                  "Role"
#define VMDIR_DB_COL_MIN_PWD_LENGTH        "MinPwdLength"
#define VMDIR_DB_COL_PWD_HISTORY_LENGTH    "PwdHistoryLength"
#define VMDIR_DB_COL_PWD_PROPERTIES        "PwdProperties"
#define VMDIR_DB_COL_FORCE_LOGOFF_TIME     "ForceLogoffTime"
#define VMDIR_DB_COL_PRIMARY_DOMAIN        "PrimaryDomain"
#define VMDIR_DB_COL_SEQUENCE_NUMBER       "SequenceNumber"
#define VMDIR_DB_COL_LOCKOUT_DURATION      "LockoutDuration"
#define VMDIR_DB_COL_LOCKOUT_WINDOW        "LockoutWindow"
#define VMDIR_DB_COL_LOCKOUT_THRESHOLD     "LockoutThreshold"
#define VMDIR_DB_COL_CREATED_TIME          "CreatedTime"

#define VMDIR_DB_QUERY_CREATE_TABLES  \
    "CREATE TABLE " VMDIR_DB_CONFIG_TABLE " (\n"                                 \
                 "UIDCounter INTEGER,\n"                                       \
                 "GIDCounter INTEGER,\n"                                       \
                 "RIDCounter INTEGER,\n"                                       \
                 "Version    INTEGER\n"                                        \
                 ");\n"                                                        \
    "CREATE TABLE " VMDIR_DB_OBJECTS_TABLE " (\n"                                \
                 VMDIR_DB_COL_RECORD_ID " INTEGER PRIMARY KEY AUTOINCREMENT,\n"  \
                 VMDIR_DB_COL_OBJECT_SID            " TEXT COLLATE NOCASE,\n"    \
                 VMDIR_DB_COL_SECURITY_DESCRIPTOR   " BLOB,\n"                   \
                 VMDIR_DB_COL_DISTINGUISHED_NAME    " TEXT COLLATE NOCASE,\n"    \
                 VMDIR_DB_COL_PARENT_DN             " TEXT,\n"                   \
                 VMDIR_DB_COL_OBJECT_CLASS          " INTEGER,\n"                \
                 VMDIR_DB_COL_DOMAIN                " TEXT COLLATE NOCASE,\n"    \
                 VMDIR_DB_COL_NETBIOS_NAME          " TEXT COLLATE NOCASE,\n"    \
                 VMDIR_DB_COL_COMMON_NAME           " TEXT,\n"                   \
                 VMDIR_DB_COL_SAM_ACCOUNT_NAME      " TEXT COLLATE NOCASE,\n"    \
                 VMDIR_DB_COL_USER_PRINCIPAL_NAME   " TEXT COLLATE NOCASE,\n"    \
                 VMDIR_DB_COL_DESCRIPTION           " TEXT,\n"                   \
                 VMDIR_DB_COL_COMMENT               " TEXT,\n"                   \
                 VMDIR_DB_COL_UID                   " INTEGER,\n"                \
                 VMDIR_DB_COL_PASSWORD              " TEXT,\n"                   \
                 VMDIR_DB_COL_ACCOUNT_FLAGS         " INTEGER,\n"                \
                 VMDIR_DB_COL_GECOS                 " TEXT,\n"                   \
                 VMDIR_DB_COL_HOME_DIR              " TEXT,\n"                   \
                 VMDIR_DB_COL_HOME_DRIVE            " TEXT,\n"                   \
                 VMDIR_DB_COL_LOGON_SCRIPT          " TEXT,\n"                   \
                 VMDIR_DB_COL_PROFILE_PATH          " TEXT,\n"                   \
                 VMDIR_DB_COL_WORKSTATIONS          " TEXT,\n"                   \
                 VMDIR_DB_COL_PARAMETERS            " TEXT,\n"                   \
                 VMDIR_DB_COL_SHELL                 " TEXT,\n"                   \
                 VMDIR_DB_COL_PASSWORD_LAST_SET     " INTEGER,\n"                \
                 VMDIR_DB_COL_ALLOW_PASSWORD_CHANGE " INTEGER,\n"                \
                 VMDIR_DB_COL_FORCE_PASSWORD_CHANGE " INTEGER,\n"                \
                 VMDIR_DB_COL_FULL_NAME             " TEXT,\n"                   \
                 VMDIR_DB_COL_ACCOUNT_EXPIRY        " INTEGER,\n"                \
                 VMDIR_DB_COL_LM_HASH               " BLOB,\n"                   \
                 VMDIR_DB_COL_NT_HASH               " BLOB,\n"                   \
                 VMDIR_DB_COL_PRIMARY_GROUP         " INTEGER,\n"                \
                 VMDIR_DB_COL_GID                   " INTEGER,\n"                \
                 VMDIR_DB_COL_COUNTRY_CODE          " INTEGER,\n"                \
                 VMDIR_DB_COL_CODE_PAGE             " INTEGER,\n"                \
                 VMDIR_DB_COL_MAX_PWD_AGE           " INTEGER,\n"                \
                 VMDIR_DB_COL_MIN_PWD_AGE           " INTEGER,\n"                \
                 VMDIR_DB_COL_PWD_PROMPT_TIME       " INTEGER,\n"                \
                 VMDIR_DB_COL_LAST_LOGON            " INTEGER,\n"                \
                 VMDIR_DB_COL_LAST_LOGOFF           " INTEGER,\n"                \
                 VMDIR_DB_COL_LOCKOUT_TIME          " INTEGER,\n"                \
                 VMDIR_DB_COL_LOGON_COUNT           " INTEGER,\n"                \
                 VMDIR_DB_COL_BAD_PASSWORD_COUNT    " INTEGER,\n"                \
                 VMDIR_DB_COL_LOGON_HOURS           " BLOB,\n"                   \
                 VMDIR_DB_COL_ROLE                  " INTEGER,\n"                \
                 VMDIR_DB_COL_MIN_PWD_LENGTH        " INTEGER,\n"                \
                 VMDIR_DB_COL_PWD_HISTORY_LENGTH    " INTEGER,\n"                \
                 VMDIR_DB_COL_PWD_PROPERTIES        " INTEGER,\n"                \
                 VMDIR_DB_COL_FORCE_LOGOFF_TIME     " INTEGER,\n"                \
                 VMDIR_DB_COL_PRIMARY_DOMAIN        " TEXT,\n"                   \
                 VMDIR_DB_COL_SEQUENCE_NUMBER       " INTEGER,\n"                \
                 VMDIR_DB_COL_LOCKOUT_DURATION      " INTEGER,\n"                \
                 VMDIR_DB_COL_LOCKOUT_WINDOW        " INTEGER,\n"                \
                 VMDIR_DB_COL_LOCKOUT_THRESHOLD     " INTEGER,\n"                \
                 VMDIR_DB_COL_CREATED_TIME " DATE DEFAULT (DATETIME('now')),\n"  \
     "UNIQUE(" VMDIR_DB_COL_OBJECT_SID ", " VMDIR_DB_COL_DISTINGUISHED_NAME "),\n" \
     "UNIQUE(" VMDIR_DB_COL_DISTINGUISHED_NAME ", " VMDIR_DB_COL_PARENT_DN "),\n"  \
     "CHECK(" VMDIR_DB_COL_OBJECT_CLASS      " == 1 \n"                          \
            " OR " VMDIR_DB_COL_OBJECT_CLASS " == 2 \n"                          \
            " OR " VMDIR_DB_COL_OBJECT_CLASS " == 3 \n"                          \
            " OR " VMDIR_DB_COL_OBJECT_CLASS " == 4 \n"                          \
            " OR " VMDIR_DB_COL_OBJECT_CLASS " == 5 \n"                          \
            " OR " VMDIR_DB_COL_OBJECT_CLASS " == 6)\n"                          \
                 ");\n"                                                        \
    "CREATE TABLE " VMDIR_DB_MEMBERS_TABLE " (\n"                                \
                 VMDIR_DB_COL_GROUP_RECORD_ID       " INTEGER,\n"                \
                 VMDIR_DB_COL_MEMBER_RECORD_ID      " INTEGER,\n"                \
                 VMDIR_DB_COL_CREATED_TIME " DATE DEFAULT (DATETIME('now')),\n"  \
  "UNIQUE(" VMDIR_DB_COL_GROUP_RECORD_ID ", " VMDIR_DB_COL_MEMBER_RECORD_ID "),\n" \
  "FOREIGN KEY (" VMDIR_DB_COL_GROUP_RECORD_ID ") \n"                            \
  "    REFERENCES " VMDIR_DB_OBJECTS_TABLE " (" VMDIR_DB_COL_RECORD_ID "),\n"      \
                 "FOREIGN KEY (" VMDIR_DB_COL_MEMBER_RECORD_ID ") \n"            \
       "    REFERENCES " VMDIR_DB_OBJECTS_TABLE " (" VMDIR_DB_COL_RECORD_ID ")\n"  \
                 ");\n"                                                        \
    "CREATE TRIGGER samdbobjects_delete_object \n"                             \
    "AFTER  DELETE on " VMDIR_DB_OBJECTS_TABLE "\n"                              \
    "BEGIN\n"                                                                  \
    "  DELETE FROM " VMDIR_DB_MEMBERS_TABLE "\n"                                 \
    "  WHERE " VMDIR_DB_COL_GROUP_RECORD_ID " = old." VMDIR_DB_COL_RECORD_ID ";\n" \
    "END;\n"

typedef enum
{
    VMDIRDB_OBJECT_CLASS_UNKNOWN         = 0,
    VMDIRDB_OBJECT_CLASS_DOMAIN          = 1,
    VMDIRDB_OBJECT_CLASS_BUILTIN_DOMAIN  = 2,
    VMDIRDB_OBJECT_CLASS_CONTAINER       = 3,
    VMDIRDB_OBJECT_CLASS_LOCAL_GROUP     = 4,
    VMDIRDB_OBJECT_CLASS_USER            = 5,
    VMDIRDB_OBJECT_CLASS_LOCALGRP_MEMBER = 6,
    VMDIRDB_OBJECT_CLASS_SENTINEL

} VMDIRDB_OBJECT_CLASS;

typedef enum
{
    VMDIRDB_ATTR_TYPE_UNKNOWN = 0,
    VMDIRDB_ATTR_TYPE_TEXT,
    VMDIRDB_ATTR_TYPE_INT32,
    VMDIRDB_ATTR_TYPE_INT64,
    VMDIRDB_ATTR_TYPE_BOOLEAN,
    VMDIRDB_ATTR_TYPE_BLOB,
    VMDIRDB_ATTR_TYPE_DATETIME,
    VMDIRDB_ATTR_TYPE_SECURITY_DESCRIPTOR
} VMDIRDB_ATTR_TYPE;

#define VMDIR_DB_DIR_ATTR_NAME_MAX_LEN 32
#define VMDIR_DB_COL_NAME_MAX_LEN      32

/* These are the strings exchanged through the Directory API */
#define VMDIR_DB_DIR_ATTR_RECORD_ID \
    {'O','b','j','e','c','t','R','e','c','o','r','d','I','d',0}
#define VMDIR_DB_DIR_ATTR_OBJECT_SID \
    {'O','b','j','e','c','t','S','I','D',0}
#define VMDIR_DB_DIR_ATTR_SECURITY_DESCRIPTOR \
    {'S','e','c','u','r','i','t','y','D','e','s','c','r','i','p','t','o','r',0}
#define VMDIR_DB_DIR_ATTR_DISTINGUISHED_NAME  \
    {'D','i','s','t','i','n','g','u','i','s','h','e','d','N','a','m','e',0}
#define VMDIR_DB_DIR_ATTR_PARENT_DN \
    {'P','a','r','e','n','t','D','N',0}
#define VMDIR_DB_DIR_ATTR_OBJECT_CLASS \
    {'O','b','j','e','c','t','C','l','a','s','s',0}
#define VMDIR_DB_DIR_ATTR_DOMAIN \
    {'D','o','m','a','i','n',0}
#define VMDIR_DB_DIR_ATTR_NETBIOS_NAME \
    {'N','e','t','B','I','O','S','N','a','m','e',0}
#define VMDIR_DB_DIR_ATTR_COMMON_NAME \
    {'C','o','m','m','o','n','N','a','m','e',0}
#define VMDIR_DB_DIR_ATTR_SAM_ACCOUNT_NAME \
    {'S','a','m','A','c','c','o','u','n','t','N','a','m','e',0}
#define VMDIR_DB_DIR_ATTR_USER_PRINCIPAL_NAME \
    {'U','s','e','r','P','r','i','n','c','i','p','a','l','N','a','m','e',0}
#define VMDIR_DB_DIR_ATTR_DESCRIPTION \
    {'D','e','s','c','r','i','p','t','i','o','n',0}
#define VMDIR_DB_DIR_ATTR_COMMENT \
    {'C','o','m','m','e','n','t',0}
#define VMDIR_DB_DIR_ATTR_UID \
    {'U','I','D',0}
#define VMDIR_DB_DIR_ATTR_PASSWORD \
    {'P','a','s','s','w','o','r','d',0}
#define VMDIR_DB_DIR_ATTR_ACCOUNT_FLAGS \
    {'A','c','c','o','u','n','t','F','l','a','g','s',0}
#define VMDIR_DB_DIR_ATTR_GECOS \
    {'G','e','c','o','s',0}
#define VMDIR_DB_DIR_ATTR_HOME_DIR \
    {'H','o','m','e','d','i','r',0}
#define VMDIR_DB_DIR_ATTR_HOME_DRIVE \
    {'H','o','m','e','d','r','i','v','e',0}
#define VMDIR_DB_DIR_ATTR_LOGON_SCRIPT \
    {'L','o','g','o','n','S','c','r','i','p','t',0}
#define VMDIR_DB_DIR_ATTR_PROFILE_PATH \
    {'P','r','o','f','i','l','e','P','a','t','h',0}
#define VMDIR_DB_DIR_ATTR_WORKSTATIONS \
    {'W','o','r','k','s','t','a','t','i','o','n','s',0}
#define VMDIR_DB_DIR_ATTR_PARAMETERS \
    {'P','a','r','a','m','e','t','e','r','s',0}
#define VMDIR_DB_DIR_ATTR_SHELL \
    {'L','o','g','i','n','S','h','e','l','l',0}
#define VMDIR_DB_DIR_ATTR_PASSWORD_LAST_SET \
    {'P','a','s','s','w','o','r','d','L','a','s','t','S','e','t',0}
#define VMDIR_DB_DIR_ATTR_ALLOW_PASSWORD_CHANGE \
    {'A','l','l','o','w','P','a','s','s','w','o','r','d','C','h','a','n','g','e',0}
#define VMDIR_DB_DIR_ATTR_FORCE_PASSWORD_CHANGE \
    {'F','o','r','c','e','P','a','s','s','w','o','r','d','C','h','a','n','g','e',0}
#define VMDIR_DB_DIR_ATTR_FULL_NAME \
    {'F','u','l','l','N','a','m','e',0}
#define VMDIR_DB_DIR_ATTR_ACCOUNT_EXPIRY \
    {'A','c','c','o','u','n','t','E','x','p','i','r','y',0}
#define VMDIR_DB_DIR_ATTR_LM_HASH \
    {'L','M','H','a','s','h',0}
#define VMDIR_DB_DIR_ATTR_NT_HASH \
    {'N','T','H','a','s','h',0}
#define VMDIR_DB_DIR_ATTR_PRIMARY_GROUP \
    {'P','r','i','m','a','r','y','G','r','o','u','p',0}
#define VMDIR_DB_DIR_ATTR_GID \
    {'G','I','D',0}
#define VMDIR_DB_DIR_ATTR_COUNTRY_CODE \
    {'C','o','u','n','t','r','y','C','o','d','e',0}
#define VMDIR_DB_DIR_ATTR_CODE_PAGE \
    {'C','o','d','e','P','a','g','e',0}
#define VMDIR_DB_DIR_ATTR_MAX_PWD_AGE \
    {'M','a','x','P','w','d','A','g','e',0}
#define VMDIR_DB_DIR_ATTR_MIN_PWD_AGE \
    {'M','i','n','P','w','d','A','g','e',0}
#define VMDIR_DB_DIR_ATTR_PWD_PROMPT_TIME \
    {'P','w','d','P','r','o','m','p','t','T','i','m','e',0}
#define VMDIR_DB_DIR_ATTR_LAST_LOGON \
    {'L','a','s','t','L','o','g','o','n',0}
#define VMDIR_DB_DIR_ATTR_LAST_LOGOFF \
    {'L','a','s','t','L','o','g','o','f','f',0}
#define VMDIR_DB_DIR_ATTR_LOCKOUT_TIME \
    {'L','o','c','k','o','u','t','T','i','m','e',0}
#define VMDIR_DB_DIR_ATTR_LOGON_COUNT \
    {'L','o','g','o','n','C','o','u','n','t',0}
#define VMDIR_DB_DIR_ATTR_BAD_PASSWORD_COUNT \
    {'B','a','d','P','w','d','C','o','u','n','t',0}
#define VMDIR_DB_DIR_ATTR_LOGON_HOURS \
    {'L','o','g','o','n','H','o','u','r','s',0}
#define VMDIR_DB_DIR_ATTR_ROLE \
    {'R','o','l','e',0}
#define VMDIR_DB_DIR_ATTR_MIN_PWD_LENGTH \
    {'M','i','n','P','w','d','L','e','n','g','t','h',0}
#define VMDIR_DB_DIR_ATTR_PWD_HISTORY_LENGTH \
    {'P','w','d','H','i','s','t','o','r','y','L','e','n','g','t','h',0}
#define VMDIR_DB_DIR_ATTR_PWD_PROPERTIES \
    {'P','w','d','P','r','o','p','e','r','t','i','e','s',0}
#define VMDIR_DB_DIR_ATTR_FORCE_LOGOFF_TIME \
    {'F','o','r','c','e','L','o','g','o','f','f','T','i','m','e',0}
#define VMDIR_DB_DIR_ATTR_PRIMARY_DOMAIN \
    {'P','r','i','m','a','r','y','D','o','m','a','i','n',0}
#define VMDIR_DB_DIR_ATTR_SEQUENCE_NUMBER \
    {'S','e','q','u','e','n','c','e','N','u','m','b','e','r',0}
#define VMDIR_DB_DIR_ATTR_LOCKOUT_DURATION \
    {'L','o','c','k','o','u','t','D','u','r','a','t','i','o','n',0}
#define VMDIR_DB_DIR_ATTR_LOCKOUT_WINDOW \
    {'L','o','c','k','o','u','t','W','i','n','d','o','w',0}
#define VMDIR_DB_DIR_ATTR_LOCKOUT_THRESHOLD \
    {'L','o','c','k','o','u','t','T','h','r','e','s','h','o','l','d',0}
#define VMDIR_DB_DIR_ATTR_CREATED_TIME \
    {'C','r','e','a','t','e','d','T','i','m','e',0}
#define VMDIR_DB_DIR_ATTR_MEMBERS \
    {'M','e','m','b','e','r','s',0}

typedef DWORD VMDIR_DB_ATTR_FLAGS;

#define VMDIR_DB_ATTR_FLAGS_NONE                       0x00000000
#define VMDIR_DB_ATTR_FLAGS_MANDATORY                  0x00000001
#define VMDIR_DB_ATTR_FLAGS_READONLY                   0x00000002
#define VMDIR_DB_ATTR_FLAGS_GENERATE_IF_NOT_SPECIFIED  0x00000004
#define VMDIR_DB_ATTR_FLAGS_GENERATE_ALWAYS            0x00000008
#define VMDIR_DB_ATTR_FLAGS_GENERATED_BY_DB            0x00000010
#define VMDIR_DB_ATTR_FLAGS_DERIVATIVE                 0x00000020

#define VMDIR_DB_IS_A_ROW_ID         TRUE
#define VMDIR_DB_IS_NOT_A_ROW_ID     FALSE
#define VMDIR_DB_IS_MULTI_VALUED     TRUE
#define VMDIR_DB_IS_NOT_MULTI_VALUED FALSE
#define VMDIR_DB_IS_QUERYABLE        TRUE
#define VMDIR_DB_IS_NOT_QUERYABLE    FALSE

typedef struct _VMDIR_DB_ATTRIBUTE_MAP
{
    wchar16_t       wszDirectoryAttribute[VMDIR_DB_DIR_ATTR_NAME_MAX_LEN];
    CHAR            szDbColumnName[VMDIR_DB_COL_NAME_MAX_LEN];
    VMDIRDB_ATTR_TYPE attributeType;
    BOOLEAN         bIsRowId;
    BOOLEAN         bIsMultiValued;
    BOOLEAN         bIsQueryable;

} VMDIR_DB_ATTRIBUTE_MAP, *PVMDIR_DB_ATTRIBUTE_MAP;

#define VMDIRDB_OBJECT_ATTRIBUTE_MAP            \
    {                                         \
        VMDIR_DB_DIR_ATTR_RECORD_ID,            \
        VMDIR_DB_COL_RECORD_ID,                 \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_A_ROW_ID,                   \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_OBJECT_SID,           \
        VMDIR_DB_COL_OBJECT_SID,                \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_SECURITY_DESCRIPTOR,  \
        VMDIR_DB_COL_SECURITY_DESCRIPTOR,       \
        VMDIRDB_ATTR_TYPE_SECURITY_DESCRIPTOR,  \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_DISTINGUISHED_NAME,   \
        VMDIR_DB_COL_DISTINGUISHED_NAME,        \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_PARENT_DN,            \
        VMDIR_DB_COL_PARENT_DN,                 \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_OBJECT_CLASS,         \
        VMDIR_DB_COL_OBJECT_CLASS,              \
        VMDIRDB_ATTR_TYPE_INT32,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_DOMAIN,               \
        VMDIR_DB_COL_DOMAIN,                    \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_NETBIOS_NAME,         \
        VMDIR_DB_COL_NETBIOS_NAME,              \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_COMMON_NAME,          \
        VMDIR_DB_COL_COMMON_NAME,               \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_SAM_ACCOUNT_NAME,     \
        VMDIR_DB_COL_SAM_ACCOUNT_NAME,          \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_USER_PRINCIPAL_NAME,  \
        VMDIR_DB_COL_USER_PRINCIPAL_NAME,       \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_DESCRIPTION,          \
        VMDIR_DB_COL_DESCRIPTION,               \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_COMMENT,              \
        VMDIR_DB_COL_COMMENT,                   \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_UID,                  \
        VMDIR_DB_COL_UID,                       \
        VMDIRDB_ATTR_TYPE_INT32,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_PASSWORD,             \
        VMDIR_DB_COL_PASSWORD,                  \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_ACCOUNT_FLAGS,        \
        VMDIR_DB_COL_ACCOUNT_FLAGS  ,           \
        VMDIRDB_ATTR_TYPE_INT32,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_GECOS,                \
        VMDIR_DB_COL_GECOS,                     \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_HOME_DIR,             \
        VMDIR_DB_COL_HOME_DIR,                  \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_HOME_DRIVE,           \
        VMDIR_DB_COL_HOME_DRIVE,                \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_LOGON_SCRIPT,         \
        VMDIR_DB_COL_LOGON_SCRIPT,              \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_PROFILE_PATH,         \
        VMDIR_DB_COL_PROFILE_PATH,              \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_WORKSTATIONS,         \
        VMDIR_DB_COL_WORKSTATIONS,              \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_PARAMETERS,           \
        VMDIR_DB_COL_PARAMETERS,                \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_SHELL,                \
        VMDIR_DB_COL_SHELL,                     \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_PASSWORD_LAST_SET,    \
        VMDIR_DB_COL_PASSWORD_LAST_SET,         \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_ALLOW_PASSWORD_CHANGE,\
        VMDIR_DB_COL_ALLOW_PASSWORD_CHANGE,     \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_FORCE_PASSWORD_CHANGE,\
        VMDIR_DB_COL_FORCE_PASSWORD_CHANGE,     \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_FULL_NAME,            \
        VMDIR_DB_COL_FULL_NAME,                 \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_ACCOUNT_EXPIRY,       \
        VMDIR_DB_COL_ACCOUNT_EXPIRY,            \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_LM_HASH,              \
        VMDIR_DB_COL_LM_HASH,                   \
        VMDIRDB_ATTR_TYPE_BLOB,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_NT_HASH,              \
        VMDIR_DB_COL_NT_HASH,                   \
        VMDIRDB_ATTR_TYPE_BLOB,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_PRIMARY_GROUP,        \
        VMDIR_DB_COL_PRIMARY_GROUP,             \
        VMDIRDB_ATTR_TYPE_INT32,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_GID,                  \
        VMDIR_DB_COL_GID,                       \
        VMDIRDB_ATTR_TYPE_INT32,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_COUNTRY_CODE,         \
        VMDIR_DB_COL_COUNTRY_CODE,              \
        VMDIRDB_ATTR_TYPE_INT32,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_CODE_PAGE,            \
        VMDIR_DB_COL_CODE_PAGE,                 \
        VMDIRDB_ATTR_TYPE_INT32,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_MAX_PWD_AGE,          \
        VMDIR_DB_COL_MAX_PWD_AGE,               \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_MIN_PWD_AGE,          \
        VMDIR_DB_COL_MIN_PWD_AGE,               \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_PWD_PROMPT_TIME,      \
        VMDIR_DB_COL_PWD_PROMPT_TIME,           \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_LAST_LOGON,           \
        VMDIR_DB_COL_LAST_LOGON,                \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_LAST_LOGOFF,          \
        VMDIR_DB_COL_LAST_LOGOFF,               \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_LOCKOUT_TIME,         \
        VMDIR_DB_COL_LOCKOUT_TIME,              \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_LOGON_COUNT,          \
        VMDIR_DB_COL_LOGON_COUNT,               \
        VMDIRDB_ATTR_TYPE_INT32,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_BAD_PASSWORD_COUNT,   \
        VMDIR_DB_COL_BAD_PASSWORD_COUNT,        \
        VMDIRDB_ATTR_TYPE_INT32,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_LOGON_HOURS,          \
        VMDIR_DB_COL_LOGON_HOURS,               \
        VMDIRDB_ATTR_TYPE_BLOB,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_ROLE,                 \
        VMDIR_DB_COL_ROLE,                      \
        VMDIRDB_ATTR_TYPE_INT32,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_MIN_PWD_LENGTH,       \
        VMDIR_DB_COL_MIN_PWD_LENGTH,            \
        VMDIRDB_ATTR_TYPE_INT32,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_PWD_HISTORY_LENGTH,   \
        VMDIR_DB_COL_PWD_HISTORY_LENGTH,        \
        VMDIRDB_ATTR_TYPE_INT32,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_PWD_PROPERTIES,       \
        VMDIR_DB_COL_PWD_PROPERTIES,            \
        VMDIRDB_ATTR_TYPE_INT32,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_FORCE_LOGOFF_TIME,    \
        VMDIR_DB_COL_FORCE_LOGOFF_TIME,         \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_PRIMARY_DOMAIN,       \
        VMDIR_DB_COL_PRIMARY_DOMAIN,            \
        VMDIRDB_ATTR_TYPE_TEXT,                 \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_SEQUENCE_NUMBER,      \
        VMDIR_DB_COL_SEQUENCE_NUMBER,           \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_LOCKOUT_DURATION,     \
        VMDIR_DB_COL_LOCKOUT_DURATION,          \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_LOCKOUT_WINDOW,       \
        VMDIR_DB_COL_LOCKOUT_WINDOW,            \
        VMDIRDB_ATTR_TYPE_INT64,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_LOCKOUT_THRESHOLD,    \
        VMDIR_DB_COL_LOCKOUT_THRESHOLD,         \
        VMDIRDB_ATTR_TYPE_INT32,                \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    },                                        \
    {                                         \
        VMDIR_DB_DIR_ATTR_CREATED_TIME,         \
        VMDIR_DB_COL_CREATED_TIME,              \
        VMDIRDB_ATTR_TYPE_DATETIME,             \
        VMDIR_DB_IS_NOT_A_ROW_ID,               \
        VMDIR_DB_IS_NOT_MULTI_VALUED,           \
        VMDIR_DB_IS_QUERYABLE                   \
    }

typedef struct _VMDIRDB_ATTRIBUTE_MAP_INFO
{
    wchar16_t wszAttributeName[VMDIR_DB_DIR_ATTR_NAME_MAX_LEN];
    DWORD     dwAttributeFlags;

} VMDIRDB_ATTRIBUTE_MAP_INFO, *PVMDIRDB_ATTRIBUTE_MAP_INFO;

#define VMDIRDB_TOP_ATTRIBUTE_MAP                                   \
    {                                                             \
        VMDIR_DB_DIR_ATTR_RECORD_ID,                                \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                            \
         VMDIR_DB_ATTR_FLAGS_READONLY  |                            \
         VMDIR_DB_ATTR_FLAGS_GENERATED_BY_DB)                       \
    },                                                            \
    {                                                             \
        VMDIR_DB_DIR_ATTR_OBJECT_SID,                               \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                            \
         VMDIR_DB_ATTR_FLAGS_GENERATE_IF_NOT_SPECIFIED)             \
    },                                                            \
    {                                                             \
        VMDIR_DB_DIR_ATTR_SECURITY_DESCRIPTOR,                      \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                            \
         VMDIR_DB_ATTR_FLAGS_GENERATE_IF_NOT_SPECIFIED)             \
    },                                                            \
    {                                                             \
        VMDIR_DB_DIR_ATTR_DISTINGUISHED_NAME,                       \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                            \
         VMDIR_DB_ATTR_FLAGS_GENERATE_ALWAYS |                      \
         VMDIR_DB_ATTR_FLAGS_DERIVATIVE)                            \
    },                                                            \
    {                                                             \
        VMDIR_DB_DIR_ATTR_PARENT_DN,                                \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                            \
         VMDIR_DB_ATTR_FLAGS_GENERATE_ALWAYS |                      \
         VMDIR_DB_ATTR_FLAGS_DERIVATIVE)                            \
    },                                                            \
    {                                                             \
        VMDIR_DB_DIR_ATTR_OBJECT_CLASS,                             \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                            \
         VMDIR_DB_ATTR_FLAGS_READONLY)                              \
    },                                                            \
    {                                                             \
        VMDIR_DB_DIR_ATTR_DOMAIN,                                   \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                            \
         VMDIR_DB_ATTR_FLAGS_GENERATE_IF_NOT_SPECIFIED |            \
         VMDIR_DB_ATTR_FLAGS_DERIVATIVE)                            \
    },                                                            \
    {                                                             \
        VMDIR_DB_DIR_ATTR_NETBIOS_NAME,                             \
        VMDIR_DB_ATTR_FLAGS_MANDATORY                               \
    },                                                            \
    {                                                             \
        VMDIR_DB_DIR_ATTR_CREATED_TIME,                             \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                            \
         VMDIR_DB_ATTR_FLAGS_READONLY  |                            \
         VMDIR_DB_ATTR_FLAGS_GENERATED_BY_DB)                       \
    }

#define VMDIRDB_USER_ATTRIBUTE_MAP                                 \
    VMDIRDB_TOP_ATTRIBUTE_MAP,                                     \
    {                                                            \
        VMDIR_DB_DIR_ATTR_SAM_ACCOUNT_NAME,                        \
        VMDIR_DB_ATTR_FLAGS_MANDATORY                              \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_USER_PRINCIPAL_NAME,                     \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_COMMON_NAME,                             \
        VMDIR_DB_ATTR_FLAGS_MANDATORY                              \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_UID,                                     \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                           \
         VMDIR_DB_ATTR_FLAGS_READONLY  |                           \
         VMDIR_DB_ATTR_FLAGS_GENERATE_IF_NOT_SPECIFIED)            \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_DESCRIPTION,                             \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_COMMENT,                                 \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_PASSWORD,                                \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_ACCOUNT_FLAGS,                           \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_GECOS,                                   \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_HOME_DIR,                                \
        VMDIR_DB_ATTR_FLAGS_MANDATORY                              \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_SHELL,                                   \
        VMDIR_DB_ATTR_FLAGS_MANDATORY                              \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_PASSWORD_LAST_SET,                       \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_FULL_NAME,                               \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_ACCOUNT_EXPIRY,                          \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_LM_HASH,                                 \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_NT_HASH,                                 \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_PRIMARY_GROUP,                           \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                           \
         VMDIR_DB_ATTR_FLAGS_GENERATE_IF_NOT_SPECIFIED)            \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_LAST_LOGON,                              \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_LAST_LOGOFF,                             \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_LOCKOUT_TIME,                            \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_LOGON_COUNT,                             \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_BAD_PASSWORD_COUNT,                      \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_HOME_DRIVE,                              \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_LOGON_SCRIPT,                            \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_PROFILE_PATH,                            \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_WORKSTATIONS,                            \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_PARAMETERS,                              \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_ALLOW_PASSWORD_CHANGE,                   \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_COUNTRY_CODE,                            \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_CODE_PAGE,                               \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_LOGON_HOURS,                             \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                           \
         VMDIR_DB_ATTR_FLAGS_GENERATE_IF_NOT_SPECIFIED)            \
    }

#define VMDIRDB_CONTAINER_ATTRIBUTE_MAP                            \
    VMDIRDB_TOP_ATTRIBUTE_MAP,                                     \
    {                                                            \
        VMDIR_DB_DIR_ATTR_COMMON_NAME,                             \
        VMDIR_DB_ATTR_FLAGS_MANDATORY | VMDIR_DB_ATTR_FLAGS_READONLY \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_SAM_ACCOUNT_NAME,                        \
        VMDIR_DB_ATTR_FLAGS_MANDATORY | VMDIR_DB_ATTR_FLAGS_READONLY \
    }

#define VMDIRDB_LOCAL_GROUP_ATTRIBUTE_MAP                          \
    VMDIRDB_TOP_ATTRIBUTE_MAP,                                     \
    {                                                            \
        VMDIR_DB_DIR_ATTR_SAM_ACCOUNT_NAME,                        \
        VMDIR_DB_ATTR_FLAGS_MANDATORY                              \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_COMMON_NAME,                             \
        VMDIR_DB_ATTR_FLAGS_MANDATORY                              \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_DESCRIPTION,                             \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_PASSWORD,                                \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_GID,                                     \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                           \
         VMDIR_DB_ATTR_FLAGS_READONLY  |                           \
         VMDIR_DB_ATTR_FLAGS_GENERATE_IF_NOT_SPECIFIED)            \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_FULL_NAME,                               \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    }

#define VMDIRDB_LOCALGRP_MEMBER_ATTRIBUTE_MAP                      \
    {                                                            \
        VMDIR_DB_DIR_ATTR_RECORD_ID,                               \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                           \
         VMDIR_DB_ATTR_FLAGS_READONLY  |                           \
         VMDIR_DB_ATTR_FLAGS_GENERATED_BY_DB)                      \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_DISTINGUISHED_NAME,                      \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY)                            \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_OBJECT_SID,                              \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                           \
         VMDIR_DB_ATTR_FLAGS_READONLY)                             \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_OBJECT_CLASS,                            \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                           \
         VMDIR_DB_ATTR_FLAGS_READONLY)                             \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_DOMAIN,                                  \
        (VMDIR_DB_ATTR_FLAGS_READONLY)                             \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_NETBIOS_NAME,                            \
        (VMDIR_DB_ATTR_FLAGS_READONLY)                             \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_CREATED_TIME,                            \
        (VMDIR_DB_ATTR_FLAGS_MANDATORY |                           \
         VMDIR_DB_ATTR_FLAGS_READONLY  |                           \
         VMDIR_DB_ATTR_FLAGS_GENERATED_BY_DB)                      \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_SAM_ACCOUNT_NAME,                        \
        (VMDIR_DB_ATTR_FLAGS_READONLY)                             \
    }

#define VMDIRDB_DOMAIN_ATTRIBUTE_MAP                               \
    VMDIRDB_TOP_ATTRIBUTE_MAP,                                     \
    {                                                            \
        VMDIR_DB_DIR_ATTR_COMMON_NAME,                             \
        VMDIR_DB_ATTR_FLAGS_MANDATORY                              \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_SAM_ACCOUNT_NAME,                        \
        VMDIR_DB_ATTR_FLAGS_MANDATORY                              \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_COMMENT,                                 \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_MAX_PWD_AGE,                             \
        VMDIR_DB_ATTR_FLAGS_MANDATORY                              \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_MIN_PWD_AGE,                             \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_PWD_PROMPT_TIME,                         \
        VMDIR_DB_ATTR_FLAGS_MANDATORY                              \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_ROLE,                                    \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_MIN_PWD_LENGTH,                          \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_PWD_HISTORY_LENGTH,                      \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_PWD_PROPERTIES,                          \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_FORCE_LOGOFF_TIME,                       \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_PRIMARY_DOMAIN,                          \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_SEQUENCE_NUMBER,                         \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_LOCKOUT_DURATION,                        \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_LOCKOUT_WINDOW,                          \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    },                                                           \
    {                                                            \
        VMDIR_DB_DIR_ATTR_LOCKOUT_THRESHOLD,                       \
        VMDIR_DB_ATTR_FLAGS_NONE                                   \
    }

#ifndef wszVMDIR_DB_DIR_ATTR_EOL
#define wszVMDIR_DB_DIR_ATTR_EOL NULL
#endif

#ifndef ATTR_OBJECT_CLASS
#define ATTR_OBJECT_CLASS                   "objectclass"
#endif
#ifndef ATTR_CN
#define ATTR_CN                             "cn"
#endif
#ifndef ATTR_DN
#define ATTR_DN                             "dn"
#endif
#ifndef ATTR_USER_PASSWORD
#define ATTR_USER_PASSWORD "userPassword"
#endif
#ifndef ATTR_SAM_ACCOUNT_NAME
#define ATTR_SAM_ACCOUNT_NAME               "sAMAccountName"
#endif
#ifndef ATTR_ACCT_FLAGS
#define ATTR_ACCT_FLAGS                     "userAccountControl"
#endif
#ifndef ATTR_DISTINGUISHED_NAME
#define ATTR_DISTINGUISHED_NAME             "distinguishedName"
#endif
#ifndef ATTR_DNS_HOSTNAME
#define ATTR_DNS_HOSTNAME                   "dNSHostName"
#endif
#ifndef ATTR_SERVICE_PRINCIPAL_NAME
#define ATTR_SERVICE_PRINCIPAL_NAME         "servicePrincipalName"
#endif
#ifndef ATTR_DESCRIPTION
#define ATTR_DESCRIPTION                    "description"
#endif
#ifndef ATTR_OS_NAME
#define ATTR_OS_NAME                        "operatingSystem"
#endif
#ifndef ATTR_OS_VERSION
#define ATTR_OS_VERSION                     "operatingSystemVersion"
#endif
#ifndef ATTR_OS_SERVICE_PACK
#define ATTR_OS_SERVICE_PACK                "operatingSystemServicePack"
#endif
#ifndef ATTR_KRB_UPN
#define ATTR_KRB_UPN                        "userPrincipalName"
#endif
#ifndef ATTR_NAME_MEMBER
#define ATTR_NAME_MEMBER                    "member"
#endif

#endif /* __VMDIR_DB_TABLE_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
