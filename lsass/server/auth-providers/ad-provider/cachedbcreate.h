/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        cachedbcreate.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        AD info cache Db Provider User/Group Database Create String
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#ifndef __CACHEDBCREATE_H__
#define __CACHEDBCREATE_H__

#define LSASS_DB_DIR CACHEDIR "/db"
#define LSASS_DB     LSASS_DB_DIR "/lsass-adcache.db"

#define AD_CACHEDB_TABLE_NAME_CACHE_TAGS       "lwicachetags"
#define AD_CACHEDB_TABLE_NAME_OBJECTS          "lwiobjects"
#define AD_CACHEDB_TABLE_NAME_USERS            "lwiusers"
#define AD_CACHEDB_TABLE_NAME_VERIFIERS        "lwipasswordverifiers"
#define AD_CACHEDB_TABLE_NAME_GROUPS           "lwigroups"
#define AD_CACHEDB_TABLE_NAME_MEMBERSHIP       "lwigroupmembership2"
#define AD_CACHEDB_TABLE_NAME_LINKED_CELLS     "lwilinkedcells"
#define AD_CACHEDB_TABLE_NAME_PROVIDER_DATA    "lwiproviderdata"
#define AD_CACHEDB_TABLE_NAME_TRUSTS           "lwidomaintrusts"

#define _AD_CACHEDB_SQL_DROP_TABLE(Table) \
    "DROP TABLE IF EXISTS " Table ";\n"

#define _AD_CACHEDB_SQL_DROP_INDEX(Table, Column) \
    "DROP INDEX IF EXISTS " Table "_" Column ";\n"

#define _AD_CACHEDB_SQL_CREATE_TABLE(Table) \
    "CREATE TABLE IF NOT EXISTS " Table " "

#define _AD_CACHEDB_SQL_CREATE_INDEX(Table, Column) \
    "CREATE INDEX IF NOT EXISTS " Table "_" Column " ON " Table "(" Column ");\n"

#define AD_CACHEDB_CREATE_TABLES \
    "\n" \
    _AD_CACHEDB_SQL_CREATE_TABLE(AD_CACHEDB_TABLE_NAME_CACHE_TAGS) "(\n" \
    "    CacheId integer primary key autoincrement,\n" \
    "    LastUpdated integer\n" \
    "    );\n" \
    "\n" \
    _AD_CACHEDB_SQL_CREATE_TABLE(AD_CACHEDB_TABLE_NAME_OBJECTS) "(\n" \
    "    CacheId integer,\n" \
    "    ObjectSid text PRIMARY KEY,\n" \
    "    DN text,\n" \
    "    Enabled integer,\n" \
    "    NetbiosDomainName text,\n" \
    "    SamAccountName text,\n" \
    "    Type integer,\n" \
    "    UNIQUE (NetbiosDomainName, SamAccountName),\n" \
    "    UNIQUE (DN),\n" \
    "    CHECK ( Enabled == 0 OR Enabled == 1),\n" \
    "    CHECK ( Type == 1 OR Type == 2)\n" \
    "    );\n" \
    _AD_CACHEDB_SQL_CREATE_INDEX(AD_CACHEDB_TABLE_NAME_OBJECTS, "CacheId") \
    "\n" \
    _AD_CACHEDB_SQL_CREATE_TABLE(AD_CACHEDB_TABLE_NAME_USERS) "(\n" \
    "    ObjectSid text PRIMARY KEY,\n" \
    "    Uid integer,\n" \
    "    Gid integer,\n" \
    "    UPN text,\n" \
    "    AliasName text,\n" \
    "    Passwd text,\n" \
    "    Gecos text,\n" \
    "    Shell text,\n" \
    "    Homedir text,\n" \
    "    PwdLastSet integer,\n" \
    "    GeneratedUPN integer,\n" \
    "    AccountExpires integer,\n" \
    "    PasswordExpired integer,\n" \
    "    PasswordNeverExpires integer,\n" \
    "    PromptPasswordChange integer,\n" \
    "    UserCanChangePassword integer,\n" \
    "    AccountDisabled integer,\n" \
    "    AccountExpired integer,\n" \
    "    AccountLocked integer,\n" \
    "    UNIQUE (Uid),\n" \
    "    UNIQUE (AliasName)\n" \
    "    );\n" \
    _AD_CACHEDB_SQL_CREATE_INDEX(AD_CACHEDB_TABLE_NAME_USERS, "UPN") \
    "\n" \
    _AD_CACHEDB_SQL_CREATE_TABLE(AD_CACHEDB_TABLE_NAME_VERIFIERS) "(\n" \
    "    CacheId integer,\n" \
    "    ObjectSid text PRIMARY KEY,\n" \
    "    PasswordVerifier text\n" \
    "    );\n" \
    _AD_CACHEDB_SQL_CREATE_INDEX(AD_CACHEDB_TABLE_NAME_VERIFIERS, "CacheId") \
    "\n" \
    _AD_CACHEDB_SQL_CREATE_TABLE(AD_CACHEDB_TABLE_NAME_GROUPS) "(\n" \
    "    ObjectSid text PRIMARY KEY,\n" \
    "    Gid integer,\n" \
    "    AliasName text,\n" \
    "    Passwd text,\n" \
    "    UNIQUE (Gid),\n" \
    "    UNIQUE (AliasName)\n" \
    "    );\n" \
    "\n" \
    _AD_CACHEDB_SQL_DROP_INDEX("lwigroupmembership", "CacheId") \
    _AD_CACHEDB_SQL_DROP_INDEX("lwigroupmembership", "ParentSid") \
    _AD_CACHEDB_SQL_DROP_INDEX("lwigroupmembership", "ChildSid") \
    _AD_CACHEDB_SQL_DROP_TABLE("lwigroupmembership") \
    "\n" \
    _AD_CACHEDB_SQL_CREATE_TABLE(AD_CACHEDB_TABLE_NAME_MEMBERSHIP) "(\n" \
    "    CacheId integer,\n" \
    "    ParentSid text,\n" \
    "    ChildSid text,\n" \
    "    IsInPac integer,\n" \
    "    IsInPacOnly integer,\n" \
    "    IsInLdap integer,\n" \
    "    IsDomainPrimaryGroup integer,\n" \
    "    UNIQUE (ParentSid, ChildSid)\n" \
    "    );\n" \
    _AD_CACHEDB_SQL_CREATE_INDEX(AD_CACHEDB_TABLE_NAME_MEMBERSHIP, "CacheId") \
    _AD_CACHEDB_SQL_CREATE_INDEX(AD_CACHEDB_TABLE_NAME_MEMBERSHIP, "ParentSid") \
    _AD_CACHEDB_SQL_CREATE_INDEX(AD_CACHEDB_TABLE_NAME_MEMBERSHIP, "ChildSid") \
    "\n" \
    _AD_CACHEDB_SQL_CREATE_TABLE(AD_CACHEDB_TABLE_NAME_LINKED_CELLS) "(\n" \
    "    RowIndex integer PRIMARY KEY,\n" \
    "    CellDN text,\n" \
    "    Domain text,\n" \
    "    IsForestCell integer\n" \
    "    );\n" \
    "\n" \
    _AD_CACHEDB_SQL_CREATE_TABLE(AD_CACHEDB_TABLE_NAME_PROVIDER_DATA) "(\n" \
    "    DirectoryMode integer,\n" \
    "    ADConfigurationMode integer,\n" \
    "    ADMaxPwdAge integer,\n" \
    "    Domain text PRIMARY KEY,\n" \
    "    ShortDomain text,\n" \
    "    ComputerDN text,\n" \
    "    CellDN text\n" \
    "    );\n" \
    "\n" \
    _AD_CACHEDB_SQL_CREATE_TABLE(AD_CACHEDB_TABLE_NAME_TRUSTS) "(\n" \
    "    RowIndex integer PRIMARY KEY,\n" \
    "    DnsDomainName text,\n" \
    "    NetbiosDomainName text,\n" \
    "    Sid text,\n" \
    "    Guid text,\n" \
    "    TrusteeDnsDomainName text,\n" \
    "    TrustFlags integer,\n" \
    "    TrustType integer,\n" \
    "    TrustAttributes integer,\n" \
    "    TrustDirection integer,\n" \
    "    TrustMode integer,\n" \
    "    ForestName text,\n" \
    "    Flags integer\n" \
    "    );\n" \
    ""

#endif /* __CACHEDBCREATE_H__ */

