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
 *        dbquery.h
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 *        Server sub-system
 *
 *        Server share database query templates
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __DBQUERY_H__
#define __DBQUERY_H__

#define DB_QUERY_CREATE_SHARES_TABLE                    \
    "create table shares (name    text primary key,     \
                          path    text,                 \
                          comment text  collate nocase, \
                          sid     text nocase,          \
                          service text,                 \
                          unique (name, path),          \
                          check(service == \"A:\" or service == \"LPT1:\" or service == \"IPC\" or service == \"COMM\" ) \
                          )"

#define DB_QUERY_LOOKUP_SHARE_BY_NAME  \
    "select name,      \
            path,      \
            comment,   \
            sid,       \
            service    \
      from  shares     \
      where name = %Q"

#define DB_QUERY_COUNT_EXISTING_SHARES \
    "select count(*) from shares"

#define DB_QUERY_FIND_SHARES_LIMIT \
    "select     name,       \
                path,       \
                comment,    \
                sid,        \
                service     \
       from     shares      \
       order by name       \
       limit %d offset %d"

#define DB_QUERY_INSERT_SHARE  \
    "insert into shares \
     (name,      \
      path,      \
      comment,   \
      sid,       \
      service    \
     )           \
     values( %Q, \
             %Q, \
             %Q, \
             %Q, \
             %Q  \
           )"

#define DB_QUERY_DELETE_SHARE \
    "delete from shares where name = %Q"

#endif /* __DBQUERY_H__ */

