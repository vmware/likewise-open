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
 *        notify.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV1
 *
 *        Change Notify
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
VOID
SrvFreeNotifyState(
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    );

NTSTATUS
SrvBuildNotifyState(
    PLWIO_SRV_CONNECTION             pConnection,
    PLWIO_SRV_SESSION                pSession,
    PLWIO_SRV_TREE                   pTree,
    PLWIO_SRV_FILE                   pFile,
    USHORT                           usMid,
    ULONG                            ulPid,
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1* ppNotifyState
    )
{
    NTSTATUS                        ntStatus     = STATUS_SUCCESS;
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_CHANGE_NOTIFY_STATE_SMB_V1),
                    (PVOID*)&pNotifyState);
    BAIL_ON_NT_STATUS(ntStatus);

    pNotifyState->refCount = 1;

    pthread_mutex_init(&pNotifyState->mutex, NULL);
    pNotifyState->pMutex = &pNotifyState->mutex;

    pNotifyState->pConnection = pConnection;
    InterlockedIncrement(&pNotifyState->refCount);

    pNotifyState->usUid = pSession->uid;
    pNotifyState->usTid = pTree->tid;
    pNotifyState->usFid = pFile->fid;
    pNotifyState->usMid = usMid;
    pNotifyState->ulPid = ulPid;

    *ppNotifyState = pNotifyState;

cleanup:

    return ntStatus;

error:

    *ppNotifyState = NULL;

    if (pNotifyState)
    {
        SrvFreeNotifyState(pNotifyState);
    }

    goto cleanup;
}

VOID
SrvReleaseNotifyState(
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    )
{
    if (InterlockedDecrement(&pNotifyState->refCount) == 0)
    {
        SrvFreeNotifyState(pNotifyState);
    }
}

static
VOID
SrvFreeNotifyState(
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    )
{
    if (pNotifyState->pAcb && pNotifyState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pNotifyState->pAcb->AsyncCancelContext);
    }

    if (pNotifyState->pConnection)
    {
        SrvConnectionRelease(pNotifyState->pConnection);
    }

    if (pNotifyState->pMutex)
    {
        pthread_mutex_destroy(&pNotifyState->mutex);
    }

    SrvFreeMemory(pNotifyState);
}
