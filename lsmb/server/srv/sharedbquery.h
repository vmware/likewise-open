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
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        User/Group Database Query Templates
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __DBQUERY_H__
#define __DBQUERY_H__

#define DB_QUERY_LOOKUP_SHARE_BY_NAME  \
    "SELECT ShareName,  \
            Path,       \
            Comment     \
      from  ShareTable  \
      where ShareName = %Q"

#define DB_QUERY_COUNT_EXISTING_SHARES \
    "SELECT count(*) from ShareTable"

#define DB_QUERY_FIND_SHARES_LIMIT \
    "select ShareName,   \
            Path,        \
            Comment      \
    from    ShareTable   \
    order by ShareName   \
    LIMIT %d OFFSET %d"

#define DB_QUERY_CREATE_SHARE_TABLE                            \
    "create table ShareTable (ShareName        varchar(256),   \
                              Path             varchar(1024),  \
                              Comment          varchar(1024)   \
                             )"


#define DB_QUERY_INSERT_SHARE  "INSERT INTO ShareTable \
                                     (ShareName,       \
                                      Path,            \
                                      Comment          \
                                      )                \
                               VALUES( %Q,             \
                                       %Q,             \
                                       %Q              \
                                     )"

#define DB_QUERY_DELETE_SHARE "delete from ShareTable where ShareName = %Q"

#endif /* __DBQUERY_H__ */

