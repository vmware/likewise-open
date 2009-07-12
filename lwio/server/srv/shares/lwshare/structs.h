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
 *        globals.c
 *
 * Abstract:
 *
 *        Likewise I/O (LWIO) - SRV
 *
 *        Share Repository based on Sqlite
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _SRV_SHARE_DB_CONTEXT
{
    sqlite3*      pDbHandle;

    sqlite3_stmt* pInsertStmt;
    sqlite3_stmt* pEnumStmt;
    sqlite3_stmt* pDeleteStmt;
    sqlite3_stmt* pCountStmt;
    sqlite3_stmt* pFindStmt;

    struct _SRV_SHARE_DB_CONTEXT *pNext;

} SRV_SHARE_DB_CONTEXT, *PSRV_SHARE_DB_CONTEXT;

typedef struct _SRV_SHARE_DB_ENUM_CONTEXT
{

    ULONG ulOffset;
    ULONG ulLimit;

} SRV_SHARE_DB_ENUM_CONTEXT, *PSRV_SHARE_DB_ENUM_CONTEXT;

typedef struct _SRV_SHARE_DB_GLOBALS
{
    pthread_mutex_t      mutex;

    SRV_SHARE_REPOSITORY_FUNCTION_TABLE fnTable;

    pthread_rwlock_t      dbMutex;
    pthread_rwlock_t*     pDbMutex;
    ULONG                 ulMaxNumDbContexts;
    ULONG                 ulNumDbContexts;
    PSRV_SHARE_DB_CONTEXT pDbContextList;

} SRV_SHARE_DB_GLOBALS, *PSRV_SHARE_DB_GLOBALS;

#endif /* __STRUCTS_H__ */
