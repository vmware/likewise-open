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
 *        srvtimer.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Timer
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
PVOID
SrvTimerMain(
	PVOID pData
	);

static
VOID
SrvTimerFree(
	IN  PSRV_TIMER_REQUEST pTimerRequest
	);

static
BOOLEAN
SrvTimerMustStop(
	PSRV_TIMER_CONTEXT pContext
    );

static
VOID
SrvTimerStop(
	PSRV_TIMER_CONTEXT pContext
	);

NTSTATUS
SrvTimerInit(
	PSRV_TIMER pTimer
	)
{
	NTSTATUS status = STATUS_SUCCESS;

    memset(&pTimer->context, 0, sizeof(pTimer->context));

    pthread_mutex_init(&pTimer->context.mutex, NULL);
    pTimer->context.pMutex = &pTimer->context.mutex;

    pTimer->context.bStop = FALSE;

    status = pthread_create(
                    &pTimer->timerThread,
                    NULL,
                    &SrvTimerMain,
                    &pTimer->context);
    BAIL_ON_NT_STATUS(status);

    pTimer->pTimerThread = &pTimer->timerThread;

error:

	return status;
}

static
PVOID
SrvTimerMain(
	PVOID pData
	)
{
    NTSTATUS status = 0;
    PSRV_TIMER_CONTEXT pContext = (PSRV_TIMER_CONTEXT)pData;

    LWIO_LOG_DEBUG("Srv timer starting");

    while (!SrvTimerMustStop(pContext))
    {
	// TODO: Call expired timers

	BAIL_ON_NT_STATUS(status);
    }

cleanup:

    LWIO_LOG_DEBUG("Srv timer stopping");

    return NULL;

error:

	LWIO_LOG_ERROR("Srv timer due to error [%d]", status);

    goto cleanup;
}

NTSTATUS
SrvTimerPostRequestSpecific(
	IN  PSRV_TIMER             pTimer,
	IN  struct timespec        timespan,
	IN  PVOID                  pUserData,
	IN  PFN_SRV_TIMER_CALLBACK pfnTimerExpiredCB,
	OUT PSRV_TIMER_REQUEST*    ppTimerRequest
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	PSRV_TIMER_REQUEST pTimerRequest = NULL;

	if (!timespan.tv_sec && !timespan.tv_nsec)
	{
		status = STATUS_INVALID_PARAMETER_1;
		BAIL_ON_NT_STATUS(status);
	}
	if (!pfnTimerExpiredCB)
	{
		status = STATUS_INVALID_PARAMETER_3;
		BAIL_ON_NT_STATUS(status);
	}

	status = SrvAllocateMemory(
					sizeof(SRV_TIMER_REQUEST),
					(PVOID*)&pTimerRequest);
	BAIL_ON_NT_STATUS(status);

	pTimerRequest->refCount = 1;
	pTimerRequest->timespan = timespan;
	pTimerRequest->pUserData = pUserData;
	pTimerRequest->pfnTimerExpiredCB = pfnTimerExpiredCB;

	// TODO: Enqueue timer

	*ppTimerRequest = pTimerRequest;

cleanup:

	return status;

error:

	*ppTimerRequest = pTimerRequest;

	SrvTimerRelease(pTimerRequest);

	goto cleanup;
}

NTSTATUS
SrvTimerCancelRequestSpecific(
	IN  PSRV_TIMER         pTimer,
	IN  PSRV_TIMER_REQUEST pTimerRequest
	)
{
	NTSTATUS status = STATUS_SUCCESS;

	// TODO: Dequeue timer

	return status;
}

NTSTATUS
SrvTimerRelease(
	IN  PSRV_TIMER_REQUEST pTimerRequest
	)
{
	if (InterlockedDecrement(&pTimerRequest->refCount) == 0)
	{
		SrvTimerFree(pTimerRequest);
	}

	return STATUS_SUCCESS;
}

static
VOID
SrvTimerFree(
	IN  PSRV_TIMER_REQUEST pTimerRequest
	)
{
	SrvFreeMemory(pTimerRequest);
}

VOID
SrvTimerIndicateStop(
	PSRV_TIMER pTimer
	)
{
	if (pTimer->pTimerThread)
	{
		SrvTimerStop(&pTimer->context);
	}
}

VOID
SrvTimerFreeContents(
    PSRV_TIMER pTimer
    )
{
    if (pTimer->pTimerThread)
    {
        SrvTimerStop(&pTimer->context);

        pthread_join(pTimer->timerThread, NULL);
    }

    if (pTimer->context.pMutex)
    {
        pthread_mutex_destroy(pTimer->context.pMutex);
        pTimer->context.pMutex = NULL;
    }
}

static
BOOLEAN
SrvTimerMustStop(
	PSRV_TIMER_CONTEXT pContext
    )
{
    BOOLEAN bStop = FALSE;

    pthread_mutex_lock(&pContext->mutex);

    bStop = pContext->bStop;

    pthread_mutex_unlock(&pContext->mutex);

    return bStop;
}

static
VOID
SrvTimerStop(
	PSRV_TIMER_CONTEXT pContext
	)
{
    pthread_mutex_lock(&pContext->mutex);

    pContext->bStop = TRUE;

    pthread_mutex_unlock(&pContext->mutex);
}
