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

#define LWIO_SRV_SHARES_DB_TABLE_NAME  "shares"
#define LWIO_SRV_SHARES_DB_COL_NAME    "name"
#define LWIO_SRV_SHARES_DB_COL_PATH    "path"
#define LWIO_SRV_SHARES_DB_COL_COMMENT "comment"
#define LWIO_SRV_SHARES_DB_COL_SECDESC "secdesc"
#define LWIO_SRV_SHARES_DB_COL_SERVICE "service"

#define DB_QUERY_CREATE_SHARES_TABLE                                           \
    "create table " LWIO_SRV_SHARES_DB_TABLE_NAME                              \
                  "(" LWIO_SRV_SHARES_DB_COL_NAME    " text primary key,"      \
                      LWIO_SRV_SHARES_DB_COL_PATH    " text,"                  \
                      LWIO_SRV_SHARES_DB_COL_COMMENT " text  collate nocase,"  \
                      LWIO_SRV_SHARES_DB_COL_SECDESC " blob,"                  \
                      LWIO_SRV_SHARES_DB_COL_SERVICE " text,"                  \
                    " UNIQUE (" LWIO_SRV_SHARES_DB_COL_NAME ","                \
				    LWIO_SRV_SHARES_DB_COL_PATH "),"               \
                    " CHECK(" LWIO_SRV_SHARES_DB_COL_SERVICE "== \"A:\" or "   \
				  LWIO_SRV_SHARES_DB_COL_SERVICE "== \"LPT1:\" or "\
				  LWIO_SRV_SHARES_DB_COL_SERVICE "== \"IPC\" or "  \
				  LWIO_SRV_SHARES_DB_COL_SERVICE "== \"COMM\" )"   \
                    ")"

#endif /* __DBQUERY_H__ */

