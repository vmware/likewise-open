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
 *        sqldb_create.h
 *
 * Abstract:
 *
 *        Likewise Registry
 *
 *        Sqlite registry backend
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */
#ifndef __SQLCACHE_CREATE_H__
#define __SQLCACHE_CREATE_H__

#define REG_DB_TABLE_NAME_CACHE_TAGS     "regcachetags"
#define REG_DB_TABLE_NAME_ENTRIES        "regentry1"

#define _REG_DB_SQL_DROP_TABLE(Table) \
    "DROP TABLE IF EXISTS " Table ";\n"

#define _REG_DB_SQL_DROP_INDEX(Table, Column) \
    "DROP INDEX IF EXISTS " Table "_" Column ";\n"

#define _REG_DB_SQL_CREATE_TABLE(Table) \
    "CREATE TABLE IF NOT EXISTS " Table " "

#define _REG_DB_SQL_CREATE_INDEX(Table, Column) \
    "CREATE INDEX IF NOT EXISTS " Table "_" Column " ON " Table "(" Column ");\n"

#define REG_DB_CREATE_TABLES \
    "\n" \
    _REG_DB_SQL_CREATE_TABLE(REG_DB_TABLE_NAME_CACHE_TAGS) "(\n" \
    "    CacheId integer primary key autoincrement,\n" \
    "    LastUpdated integer\n" \
    "    );\n" \
    "\n" \
    _REG_DB_SQL_CREATE_TABLE(REG_DB_TABLE_NAME_ENTRIES) "(\n" \
    "    CacheId integer,\n" \
    "    KeyName text COLLATE NOCASE,\n" \
    "    ValueName text COLLATE NOCASE,\n" \
    "    Type integer,\n" \
    "    Value blob COLLATE NOCASE,\n" \
    "    UNIQUE (KeyName, ValueName, Type)\n" \
    "    );\n" \
    _REG_DB_SQL_CREATE_INDEX(REG_DB_TABLE_NAME_ENTRIES, "CacheId") \
    "\n" \
    ""

#endif /* __SQLCACHE_CREATE_H__ */

