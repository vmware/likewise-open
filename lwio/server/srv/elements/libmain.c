/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        libmain.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        File Sharing Essential Elements
 *
 *        Library Main
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SrvElementsInit(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int      iIter = 0;

    status = SrvElementsResourcesInit();
    BAIL_ON_NT_STATUS(status);

    status = WireGetCurrentNTTime(&gSrvElements.llBootTime);
    BAIL_ON_NT_STATUS(status);

    while (!RAND_status() && (iIter++ < 10))
    {
        uuid_t uuid;
        CHAR   szUUID[37];

        memset(szUUID, 0, sizeof(szUUID));

        uuid_generate(uuid);
        uuid_unparse(uuid, szUUID);

        RAND_seed(szUUID, sizeof(szUUID));
    }

    status = SrvTimerInit(&gSrvElements.timer);
    BAIL_ON_NT_STATUS(status);

    pthread_rwlock_init(&gSrvElements.statsLock, NULL);
    gSrvElements.pStatsLock = &gSrvElements.statsLock;

error:

    return status;
}

NTSTATUS
SrvTimerPostRequest(
    IN  LONG64                 llExpiry,
    IN  PVOID                  pUserData,
    IN  PFN_SRV_TIMER_CALLBACK pfnTimerExpiredCB,
    OUT PSRV_TIMER_REQUEST*    ppTimerRequest
    )
{
    return SrvTimerPostRequestSpecific(
                &gSrvElements.timer,
                llExpiry,
                pUserData,
                pfnTimerExpiredCB,
                ppTimerRequest);
}

NTSTATUS
SrvTimerCancelRequest(
    IN  PSRV_TIMER_REQUEST pTimerRequest,
    PVOID*                 ppUserData
    )
{
    return SrvTimerCancelRequestSpecific(
                &gSrvElements.timer,
                pTimerRequest,
                ppUserData);
}

NTSTATUS
SrvElementsGetBootTime(
    PULONG64 pullBootTime
    )
{
    LONG64   llBootTime = 0LL;
    BOOLEAN  bInLock    = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gSrvElements.mutex);

    llBootTime = gSrvElements.llBootTime;

    LWIO_UNLOCK_MUTEX(bInLock, &gSrvElements.mutex);

    *pullBootTime = llBootTime;

    return STATUS_SUCCESS;
}

BOOLEAN
SrvElementsGetShareNameEcpEnabled(
    VOID
    )
{
    return gSrvElements.bShareNameEcpEnabled;
}

NTSTATUS
SrvElementsGetStats(
    PSRV_ELEMENTS_STATISTICS pStats
    )
{
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSrvElements.statsLock);

    *pStats = gSrvElements.stats;

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvElements.statsLock);

    return STATUS_SUCCESS;
}

NTSTATUS
SrvElementsResetStats(
    VOID
    )
{
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSrvElements.statsLock);

    memset(&gSrvElements.stats, 0, sizeof(gSrvElements.stats));

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvElements.statsLock);

    return STATUS_SUCCESS;
}

NTSTATUS
SrvElementsShutdown(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = SrvTimerIndicateStop(&gSrvElements.timer);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvTimerFreeContents(&gSrvElements.timer);

    if (gSrvElements.pHintsBuffer != NULL)
    {
        SrvFreeMemory(gSrvElements.pHintsBuffer);
        gSrvElements.pHintsBuffer = NULL;
        gSrvElements.ulHintsLength = 0;
    }

    if (gSrvElements.pStatsLock)
    {
        pthread_rwlock_destroy(&gSrvElements.statsLock);
        gSrvElements.pStatsLock = NULL;
    }

    SrvElementsResourcesShutdown();

error:

    return ntStatus;
}

