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
 *        Library Main
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "includes.h"

NTSTATUS
LwShareRepositoryInit(
    OUT PSRV_SHARE_REPOSITORY_FUNCTION_TABLE* ppFnTable
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    pthread_rwlock_init(&gShareRepository_lwshare.dbMutex, NULL);
    gShareRepository_lwshare.pDbMutex = &gShareRepository_lwshare.dbMutex;

    gShareRepository_lwshare.ulMaxNumDbContexts = LWIO_SRV_MAX_NUM_DB_CONTEXTS;
    gShareRepository_lwshare.ulNumDbContexts = 0;
    gShareRepository_lwshare.pDbContextList = NULL;

    status = SrvShareDbInit();
    BAIL_ON_NT_STATUS(status);

    *ppFnTable = &gShareRepository_lwshare.fnTable;

cleanup:

    return status;

error:

    *ppFnTable = NULL;

    goto cleanup;
}

NTSTATUS
LwShareRepositoryShutdown(
    IN PSRV_SHARE_REPOSITORY_FUNCTION_TABLE pFnTable
    )
{
    SrvShareDbShutdown();

    return STATUS_SUCCESS;
}

