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
 *        sharedb.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Share Repository based on sqlite
 *
 *        Database Context
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "includes.h"

static
VOID
SrvShareDbFreeContext(
	PSRV_SHARE_DB_CONTEXT pDbContext
	);

NTSTATUS
SrvShareDbAcquireContext(
	PSRV_SHARE_DB_CONTEXT* ppDbContext
    )
{
	NTSTATUS ntStatus = 0;
	BOOLEAN bInLock = FALSE;
	PSRV_SHARE_DB_GLOBALS pGlobals = &gShareRepository_lwshare;
	PSRV_SHARE_DB_CONTEXT pDbContext = NULL;

	LWIO_LOCK_MUTEX(bInLock, &pGlobals->mutex);

	if (pGlobals->ulNumDbContexts)
	{
		pDbContext = pGlobals->pDbContextList;
		pGlobals->pDbContextList = pGlobals->pDbContextList->pNext;
		pDbContext->pNext = NULL;

		pGlobals->ulNumDbContexts--;

		LWIO_UNLOCK_MUTEX(bInLock, &pGlobals->mutex);
	}
	else
	{
		PCSTR pszShareDbPath = LWIO_SRV_SHARE_DB;

        ntStatus = SrvAllocateMemory(
						sizeof(SRV_SHARE_DB_CONTEXT),
						(PVOID*)&pDbContext);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = sqlite3_open(
						pszShareDbPath,
                        &pDbContext->pDbHandle);
        if (ntStatus)
        {
		LWIO_LOG_ERROR("Sqlite3 Error (code: %d): %s",
		                        ntStatus,
		                        "Failed to open database handle""");
		ntStatus = STATUS_INTERNAL_DB_ERROR;
        }
        BAIL_ON_NT_STATUS(ntStatus);
	}

	*ppDbContext = pDbContext;

cleanup:

	LWIO_UNLOCK_MUTEX(bInLock, &pGlobals->mutex);

	return ntStatus;

error:

	*ppDbContext = NULL;

	if (pDbContext)
	{
		SrvShareDbFreeContext(pDbContext);
	}

	goto cleanup;
}

VOID
SrvShareDbReleaseContext(
	PSRV_SHARE_DB_CONTEXT pDbContext
    )
{
	BOOLEAN bInLock = FALSE;
	PSRV_SHARE_DB_GLOBALS pGlobals = &gShareRepository_lwshare;

	LWIO_LOCK_MUTEX(bInLock, &pGlobals->mutex);

	if (pGlobals->ulNumDbContexts < pGlobals->ulMaxNumDbContexts)
	{
		SrvShareDbFreeContext(pDbContext);
	}
	else
	{
		pDbContext->pNext = pGlobals->pDbContextList;
		pGlobals->pDbContextList = pDbContext;

		pGlobals->ulNumDbContexts++;
	}

	LWIO_UNLOCK_MUTEX(bInLock, &pGlobals->mutex);
}

static
VOID
SrvShareDbFreeContext(
	PSRV_SHARE_DB_CONTEXT pDbContext
	)
{
	if (pDbContext->pInsertStmt)
	{
		sqlite3_finalize(pDbContext->pInsertStmt);
	}
	if (pDbContext->pEnumStmt)
	{
		sqlite3_finalize(pDbContext->pEnumStmt);
	}
	if (pDbContext->pDeleteStmt)
	{
		sqlite3_finalize(pDbContext->pDeleteStmt);
	}
	if (pDbContext->pCountStmt)
	{
		sqlite3_finalize(pDbContext->pCountStmt);
	}
	if (pDbContext->pFindStmt)
	{
		sqlite3_finalize(pDbContext->pFindStmt);
	}
	if (pDbContext->pDbHandle)
	{
		sqlite3_close(pDbContext->pDbHandle);
	}

	SrvFreeMemory(pDbContext);
}
