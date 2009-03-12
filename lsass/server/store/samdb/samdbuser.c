/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        samdbuser.c
 *
 * Abstract:
 *
 *
 *      Likewise SAM Database Provider
 *
 *      SamDbInitUserTable
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */
#include "includes.h"

#define DB_QUERY_CREATE_USERS_TABLE                               \
    "create table lwiusers (UserRecordId integer PRIMARY KEY,     \
                            Name             varchar(256),        \
                            Passwd           varchar(256),        \
                            Uid              integer,             \
                            Gid              integer,             \
                            UserInfoFlags    integer,             \
                            Gecos            varchar(256),        \
                            HomeDir          varchar(1024),       \
                            Shell            varchar(128),        \
                            PasswdChangeTime integer,             \
                            FullName         varchar(256),        \
                            AccountExpiry    integer,             \
                            LMOwf_1          integer,             \
                            LMOwf_2          integer,             \
                            LMOwf_3          integer,             \
                            LMOwf_4          integer,             \
                            NTOwf_1          integer,             \
                            NTOwf_2          integer,             \
                            NTOwf_3          integer,             \
                            NTOwf_4          integer,             \
                            CreatedTime      date                 \
                           )"


#define DB_QUERY_CREATE_USERS_INSERT_TRIGGER                   \
    "create trigger lwiusers_createdtime                       \
     after insert on lwiusers                                  \
     begin                                                     \
          update lwiusers                                      \
          set CreatedTime = DATETIME('NOW')                    \
          where rowid = new.rowid;                             \
                                                               \
          insert into lwigroupmembers (Uid, Gid)               \
          values (new.Uid, new.Gid);                           \
     end"

#define DB_QUERY_CREATE_USERS_DELETE_TRIGGER                   \
    "create trigger lwiusers_delete_record                     \
     after delete on lwiusers                                  \
     begin                                                     \
          delete from lwigroupmembers where Uid = old.Uid;     \
     end"


NTSTATUS
SamDbInitUserTable(
    HANDLE hDb
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR pszError = NULL;

    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_USERS_TABLE,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_USERS_INSERT_TRIGGER,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_USERS_DELETE_TRIGGER,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_SAMDB_ERROR(dwError);

cleanup:

    return dwError;

error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}

