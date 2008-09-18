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

#define AD_CACHEDB_CREATE_TABLES \
"\n" \
"create table lwicachetags (\n" \
"        CacheId integer primary key autoincrement,\n" \
"        LastUpdated integer\n" \
"        );\n" \
"\n" \
"create table lwiobjects (\n" \
"        CacheId integer,\n" \
"        ObjectSid text PRIMARY KEY,\n" \
"        DN text,\n" \
"        Enabled integer,\n" \
"        NetbiosDomainName text,\n" \
"        SamAccountName text,\n" \
"        Type integer,\n" \
"        UNIQUE (NetbiosDomainName, SamAccountName),\n" \
"        UNIQUE (DN),\n" \
"        CHECK ( Enabled == 0 OR Enabled == 1),\n" \
"        CHECK ( Type == 1 OR Type == 2)\n" \
"        );\n" \
"\n" \
"create table lwiusers (\n" \
"        ObjectSid text PRIMARY KEY,\n" \
"        Uid integer,\n" \
"        Gid integer,\n" \
"        UPN text,\n" \
"        AliasName text,\n" \
"        Passwd text,\n" \
"        Gecos text,\n" \
"        Shell text,\n" \
"        Homedir text,\n" \
"        PwdLastSet integer,\n" \
"        GeneratedUPN integer,\n" \
"        AccountExpires integer,\n" \
"        PasswordExpired integer,\n" \
"        PasswordNeverExpires integer,\n" \
"        PromptPasswordChange integer,\n" \
"        UserCanChangePassword integer,\n" \
"        AccountDisabled integer,\n" \
"        AccountExpired integer,\n" \
"        AccountLocked integer,\n" \
"        UNIQUE (Uid)\n" \
"        );\n" \
"\n" \
"create table lwipasswordverifiers (\n" \
"        CacheId integer,\n" \
"        ObjectSid text PRIMARY KEY,\n" \
"        PasswordVerifier text\n" \
"        );\n" \
"\n" \
"create table lwigroups (\n" \
"        ObjectSid text PRIMARY KEY,\n" \
"        Gid integer,\n" \
"        AliasName text,\n" \
"        Passwd text,\n" \
"        UNIQUE (Gid)\n" \
"        );\n" \
"\n" \
"create table lwigroupmembership (\n" \
"        CacheId integer,\n" \
"        ParentSid text,\n" \
"        ChildSid text,\n" \
"        UNIQUE (ParentSid, ChildSid)\n" \
"        );\n" \
"\n" \
"create table lwilinkedcells (\n" \
"        RowIndex integer PRIMARY KEY,\n" \
"        CellDN text,\n" \
"        Domain text,\n" \
"        IsForestCell integer\n" \
"        );\n" \
"\n" \
"create table lwiproviderdata (\n" \
"        DirectoryMode integer,\n" \
"        ADConfigurationMode integer,\n" \
"        ADMaxPwdAge integer,\n" \
"        Domain text PRIMARY KEY,\n" \
"        ShortDomain text,\n" \
"        ServerName text,\n" \
"        ComputerDN text,\n" \
"        CellDN text\n" \
"        );\n" \
"\n" \
"create table lwidomaintrusts (\n" \
"        RowIndex integer PRIMARY KEY,\n" \
"        DnsDomainName text,\n" \
"        NetbiosDomainName text,\n" \
"        Sid text,\n" \
"        Guid text,\n" \
"        TrusteeDnsDomainName text,\n" \
"        TrustFlags integer,\n" \
"        TrustType integer,\n" \
"        TrustAttributes integer,\n" \
"        ForestName text,\n" \
"        Flags integer\n" \
"        );\n" \
""

#endif /* __CACHEDBCREATE_H__ */

