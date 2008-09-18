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
 *        Likewise Netlogon (LWNET)
 * 
 *        AD info cache Db Provider User/Group Database Create String
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#ifndef __CACHEDBCREATE_H__
#define __CACHEDBCREATE_H__

/* 
 * legacy_file_format is disabled, as this is necessary for sqlite indexing to 
 * take advantage of sorted columns.
 */

#define NETLOGON_DB_TABLE_NAME "lwnetcache3"
#define NETLOGON_KRB5_DB_TABLE_NAME "lwnetcache_krb5"

// Include line break in SQL statements for pretty-prining while debugging

#define LWNET_CACHEDB_SQL_CREATE_NETLOGON_DB_TABLE \
    "CREATE TABLE IF NOT EXISTS " NETLOGON_DB_TABLE_NAME " (\n" \
    "    QueryDnsDomainName TEXT,\n" \
    "    QuerySiteName TEXT,\n" \
    "    QueryType INTEGER,\n" \
    "\n" \
    "    LastPinged INTEGER,\n" \
    "    LastDiscovered INTEGER,\n" \
    "\n" \
    "    PingTime INTEGER,\n" \
    "    DomainControllerAddressType INTEGER,\n" \
    "    Flags INTEGER,\n" \
    "    Version INTEGER,\n" \
    "    LMToken INTEGER,\n" \
    "    NTToken INTEGER,\n" \
    "    DomainControllerName TEXT,\n" \
    "    DomainControllerAddress TEXT,\n" \
    "    DomainGUID TEXT,\n" \
    "    NetBIOSDomainName TEXT,\n" \
    "    FullyQualifiedDomainName TEXT,\n" \
    "    DnsForestName TEXT,\n" \
    "    DCSiteName TEXT,\n" \
    "    ClientSiteName TEXT,\n" \
    "    NetBIOSHostName TEXT,\n" \
    "    UserName TEXT,\n" \
    "\n" \
    "    PRIMARY KEY (\n" \
    "        QueryDnsDomainName ASC,\n" \
    "        QuerySiteName ASC,\n" \
    "        QueryType ASC\n" \
    "        )\n" \
    "    );\n" \
    ""

#define LWNET_CACHEDB_SQL_CREATE_NETLOGON_KRB5_DB_TABLE \
    "CREATE TABLE IF NOT EXISTS " NETLOGON_KRB5_DB_TABLE_NAME " (\n" \
    "    Realm TEXT,\n" \
    "\n" \
    "    LastUpdated INTEGER,\n" \
    "\n" \
    "    ServerCount INTEGER,\n" \
    "    ServerList TEXT,\n" \
    "\n" \
    "    PRIMARY KEY (\n" \
    "        Realm ASC\n" \
    "        )\n" \
    "    );\n" \
    ""

#define LWNET_CACHEDB_SQL_DUMP_NETLOGON_DB_TABLE \
    "SELECT\n" \
    "    QueryDnsDomainName,\n" \
    "    QuerySiteName,\n" \
    "    QueryType,\n" \
    "\n" \
    "    LastPinged,\n" \
    "    LastDiscovered,\n" \
    "\n" \
    "    PingTime,\n" \
    "    DomainControllerAddressType,\n" \
    "    Flags,\n" \
    "    Version,\n" \
    "    LMToken,\n" \
    "    NTToken,\n" \
    "    DomainControllerName,\n" \
    "    DomainControllerAddress,\n" \
    "    DomainGUID,\n" \
    "    NetBIOSDomainName,\n" \
    "    FullyQualifiedDomainName,\n" \
    "    DnsForestName,\n" \
    "    DCSiteName,\n" \
    "    ClientSiteName,\n" \
    "    NetBIOSHostName,\n" \
    "    UserName\n" \
    "\n" \
    "FROM " NETLOGON_DB_TABLE_NAME "\n" \
    "ORDER BY LastPinged\n" \
    ";\n" \
    ""

#define LWNET_CACHEDB_SQL_DUMP_NETLOGON_KRB5_DB_TABLE \
    "SELECT\n" \
    "    Realm,\n" \
    "\n" \
    "    LastUpdated INTEGER,\n" \
    "\n" \
    "    ServerCount INTEGER,\n" \
    "    ServerList TEXT\n" \
    "\n" \
    "FROM " NETLOGON_KRB5_DB_TABLE_NAME "\n" \
    "ORDER BY LastUpdated\n" \
    ";\n" \
    ""

#define LWNET_CACHEDB_SQL_SETUP \
    "\n" \
    "DROP INDEX IF EXISTS indexPingTime;\n" \
    "DROP TABLE IF EXISTS lwnetcache;\n" \
    "DROP TABLE IF EXISTS lwnetcache2;\n" \
    LWNET_CACHEDB_SQL_CREATE_NETLOGON_DB_TABLE \
    LWNET_CACHEDB_SQL_CREATE_NETLOGON_KRB5_DB_TABLE \
    ""


#endif /* __CACHEDBCREATE_H__ */

