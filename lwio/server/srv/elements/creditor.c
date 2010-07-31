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
 *        creditor.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Credit Management
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
NTSTATUS
SrvCreditorInitialize_inlock(
    PSRV_CREDITOR pCreditor
    );

static
NTSTATUS
SrvCreditorAcquireCredits(
    USHORT  usCreditsRequested,
    PUSHORT pusCreditsGranted
    );

static
VOID
SrvCreditorReturnCredits(
    USHORT usCredits
    );

NTSTATUS
SrvCreditorCreate(
    PSRV_CREDITOR* ppCreditor
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_CREDITOR pCreditor = NULL;

    ntStatus = SrvAllocateMemory(sizeof(SRV_CREDITOR), (PVOID*)&pCreditor);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pCreditor->mutex, NULL);
    pCreditor->pMutex = &pCreditor->mutex;

    *ppCreditor = pCreditor;

cleanup:

    return ntStatus;

error:

    *ppCreditor = NULL;

    if (pCreditor)
    {
        SrvCreditorFree(pCreditor);
    }

    goto cleanup;
}

USHORT
SrvCreditorGetCredits(
    PSRV_CREDITOR pCreditor,
    USHORT        usCreditsRequested
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    USHORT  usCreditsGranted = 1;
    BOOLEAN bInLock = FALSE;

    if (pCreditor)
    {
        LWIO_LOCK_MUTEX(bInLock, &pCreditor->mutex);

        ntStatus = SrvCreditorInitialize_inlock(pCreditor);
        BAIL_ON_NT_STATUS(ntStatus);

        if ((usCreditsRequested > pCreditor->usCreditsAcquired) &&
            (pCreditor->usCreditLimit > pCreditor->usCreditsAcquired))
        {
            USHORT   usNewCreditsGranted = 0;
            USHORT   usCreditsToAcquire =
                        (SMB_MIN(usCreditsRequested, pCreditor->usCreditLimit) -
                                pCreditor->usCreditsAcquired);

            ntStatus = SrvCreditorAcquireCredits(
                            usCreditsToAcquire,
                            &usNewCreditsGranted);
            if (ntStatus == STATUS_SUCCESS)
            {
                pCreditor->usCreditsAcquired += usNewCreditsGranted;

                usCreditsGranted = pCreditor->usCreditsAcquired;

                LWIO_LOG_DEBUG("Acquired new credits (%u). Current credits (%u).",
                               usCreditsToAcquire,
                               pCreditor->usCreditsAcquired);
            }
            else
            {
                LWIO_LOG_DEBUG("Failed to acquire additional credits (%u). "
                               "Current credits: (%u)",
                               usCreditsToAcquire,
                               pCreditor->usCreditsAcquired);
            }
        }

        LWIO_UNLOCK_MUTEX(bInLock, &pCreditor->mutex);
    }

cleanup:

    return usCreditsGranted;

error:

    usCreditsGranted = 0;

    goto cleanup;
}

static
NTSTATUS
SrvCreditorInitialize_inlock(
    PSRV_CREDITOR pCreditor
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!pCreditor->bInitialized)
    {
        pCreditor->usCreditLimit = SrvElementsConfigGetClientCreditLimit();
    }

    return ntStatus;
}

VOID
SrvCreditorFree(
    PSRV_CREDITOR pCreditor
    )
{
    if (pCreditor->usCreditsAcquired)
    {
        SrvCreditorReturnCredits(pCreditor->usCreditsAcquired);
    }

    if (pCreditor->pMutex)
    {
        pthread_mutex_destroy(&pCreditor->mutex);
        pCreditor->pMutex = NULL;
    }

    SrvFreeMemory(pCreditor);
}

static
NTSTATUS
SrvCreditorAcquireCredits(
    USHORT  usCreditsRequested,
    PUSHORT pusCreditsGranted
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock = FALSE;
    USHORT   usCreditsGranted = usCreditsRequested;

    LWIO_LOCK_MUTEX(bInLock, &gSrvElements.mutex);

    if (!gSrvElements.ulGlobalCreditLimit)
    {
        ntStatus = STATUS_TOO_MANY_CONTEXT_IDS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (gSrvElements.ulGlobalCreditLimit >= usCreditsRequested)
    {
        gSrvElements.ulGlobalCreditLimit -= usCreditsRequested;
    }
    else
    {
        usCreditsGranted = gSrvElements.ulGlobalCreditLimit;
        gSrvElements.ulGlobalCreditLimit = 0;
    }

    LWIO_UNLOCK_MUTEX(bInLock, &gSrvElements.mutex);

    *pusCreditsGranted = usCreditsGranted;

cleanup:

    return ntStatus;

error:

    *pusCreditsGranted = 0;

    goto cleanup;
}

static
VOID
SrvCreditorReturnCredits(
    USHORT usCredits
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gSrvElements.mutex);

    gSrvElements.ulGlobalCreditLimit += usCredits;

    LWIO_UNLOCK_MUTEX(bInLock, &gSrvElements.mutex);
}
