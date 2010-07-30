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
 *        response.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        Common Client Response Code
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "rdr.h"

NTSTATUS
SMBResponseCreate(
    uint16_t       wMid,
    SMB_RESPONSE **ppResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_RESPONSE pResponse = NULL;
    BOOLEAN bDestroyCondition = FALSE;

    ntStatus = SMBAllocateMemory(
                    sizeof(SMB_RESPONSE),
                    (PVOID*)&pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponse->state = SMB_RESOURCE_STATE_INITIALIZING;

    ntStatus = pthread_cond_init(&pResponse->event, NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    bDestroyCondition = TRUE;

    pResponse->mid = wMid;
    pResponse->pPacket = NULL;

    *ppResponse = pResponse;

cleanup:

    return ntStatus;

error:

    if (bDestroyCondition)
    {
        pthread_cond_destroy(&pResponse->event);
    }

    LWIO_SAFE_FREE_MEMORY(pResponse);

    *ppResponse = NULL;

    goto cleanup;
}

VOID
SMBResponseFree(
    PSMB_RESPONSE pResponse
    )
{
    if (pResponse->pSocket)
    {
        RdrSocketRemoveResponse(pResponse->pSocket, pResponse);
    }

    pthread_cond_destroy(&pResponse->event);

    SMBFreeMemory(pResponse);
}

VOID
SMBResponseInvalidate_InLock(
    PSMB_RESPONSE pResponse,
    NTSTATUS ntStatus
    )
{
    pResponse->state = SMB_RESOURCE_STATE_INVALID;
    pResponse->error = ntStatus;

    if (pResponse->pContext)
    {
        RdrContinueContext(pResponse->pContext, ntStatus, NULL);
    }

    pthread_cond_broadcast(&pResponse->event);
}
