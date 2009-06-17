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
	IN  PVOID pData
	);

static
NTSTATUS
SrvTimerGetNextRequest(
	IN  PSRV_TIMER_CONTEXT  pContext,
	OUT PSRV_TIMER_REQUEST* ppTimerRequest
	);

static
NTSTATUS
SrvTimerDetachRequest(
	IN OUT PSRV_TIMER_CONTEXT pContext,
	IN OUT PSRV_TIMER_REQUEST pTimerRequest
	);

static
VOID
SrvTimerFree(
	IN  PSRV_TIMER_REQUEST pTimerRequest
	);

static
BOOLEAN
SrvTimerMustStop(
	IN  PSRV_TIMER_CONTEXT pContext
    );

static
VOID
SrvTimerStop(
	IN  PSRV_TIMER_CONTEXT pContext
	);

NTSTATUS
SrvTimerInit(
	IN  PSRV_TIMER pTimer
	)
{
	NTSTATUS status = STATUS_SUCCESS;

    memset(&pTimer->context, 0, sizeof(pTimer->context));

    pthread_mutex_init(&pTimer->context.mutex, NULL);
    pTimer->context.pMutex = &pTimer->context.mutex;

    pthread_cond_init(&pTimer->context.event, NULL);
    pTimer->context.pEvent = &pTimer->context.event;

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
	IN  PVOID pData
	)
{
    NTSTATUS status = 0;
    PSRV_TIMER_CONTEXT pContext = (PSRV_TIMER_CONTEXT)pData;
    PSRV_TIMER_REQUEST pTimerRequest = NULL;

    LWIO_LOG_DEBUG("Srv timer starting");

    while (!SrvTimerMustStop(pContext))
    {
	int errCode = 0;
	BOOLEAN bRetryWait = FALSE;

	if (pTimerRequest)
	{
		SrvTimerRelease(pTimerRequest);
		pTimerRequest = NULL;
	}

	status = SrvTimerGetNextRequest(pContext, &pTimerRequest);
	if (status == STATUS_NOT_FOUND)
	{
		struct timespec tsLong = { .tv_sec = 86400, .tv_nsec = 0 };

		do
		{
			bRetryWait = FALSE;

			errCode = pthread_cond_timedwait(
								&pContext->event,
								&pContext->mutex,
								&tsLong);

			if (errCode == ETIMEDOUT)
			{
				if (time(NULL) < tsLong.tv_sec)
				{
					bRetryWait = TRUE;
					continue;
				}
			}

                status = LwUnixErrnoToNtStatus(errCode);
                BAIL_ON_NT_STATUS(status);

		} while (bRetryWait && !SrvTimerMustStop(pContext));

		continue;
	}
	BAIL_ON_NT_STATUS(status);

	// Has it already expired ?
	if (difftime(time(NULL), pTimerRequest->timespan.tv_sec) >= 0)
	{
		pTimerRequest->pfnTimerExpiredCB(
								pTimerRequest,
								pTimerRequest->pUserData);

		SrvTimerDetachRequest(pContext, pTimerRequest);

		continue;
	}

	do
	{
		bRetryWait = FALSE;

		errCode = pthread_cond_timedwait(
							&pContext->event,
							&pContext->mutex,
							&pTimerRequest->timespan);
			if (errCode == ETIMEDOUT)
			{
				if (time(NULL) < pTimerRequest->timespan.tv_sec)
				{
					bRetryWait = TRUE;
					continue;
				}
			}

            status = LwUnixErrnoToNtStatus(errCode);
            BAIL_ON_NT_STATUS(status);

	} while (bRetryWait && !SrvTimerMustStop(pContext));

	if (difftime(time(NULL), pTimerRequest->timespan.tv_sec) >= 0)
	{
		BOOLEAN bInLock = FALSE;

		LWIO_LOCK_MUTEX(bInLock, &pTimerRequest->mutex);

		if (pTimerRequest->pfnTimerExpiredCB)
		{
			// Timer has not been canceled
				pTimerRequest->pfnTimerExpiredCB(
									pTimerRequest,
									pTimerRequest->pUserData);
		}

		LWIO_UNLOCK_MUTEX(bInLock, &pTimerRequest->mutex);

		SrvTimerDetachRequest(pContext, pTimerRequest);
	}
    }

cleanup:

	if (pTimerRequest)
	{
		SrvTimerRelease(pTimerRequest);
	}

    LWIO_LOG_DEBUG("Srv timer stopping");

    return NULL;

error:

	LWIO_LOG_ERROR("Srv timer due to error [%d]", status);

    goto cleanup;
}

static
NTSTATUS
SrvTimerGetNextRequest(
	IN  PSRV_TIMER_CONTEXT  pContext,
	OUT PSRV_TIMER_REQUEST* ppTimerRequest
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN bInLock = FALSE;
	PSRV_TIMER_REQUEST pTimerRequest = NULL;

	LWIO_LOCK_MUTEX(bInLock, &pContext->mutex);

	if (!pContext->pRequests)
	{
		status = STATUS_NOT_FOUND;
		BAIL_ON_NT_STATUS(status);
	}

	pTimerRequest = pContext->pRequests;

	InterlockedIncrement(&pTimerRequest->refCount);

	*ppTimerRequest = pTimerRequest;

cleanup:

	LWIO_UNLOCK_MUTEX(bInLock, &pContext->mutex);

	return status;

error:

	*ppTimerRequest = NULL;

	goto cleanup;
}

static
NTSTATUS
SrvTimerDetachRequest(
	IN OUT PSRV_TIMER_CONTEXT pContext,
	IN OUT PSRV_TIMER_REQUEST pTimerRequest
	)
{
	BOOLEAN bInLock = FALSE;

	LWIO_LOCK_MUTEX(bInLock, &pContext->mutex);

	if (pTimerRequest->pPrev)
	{
		pTimerRequest->pPrev->pNext = pTimerRequest->pNext;
	}
	else
	{
		pContext->pRequests = pTimerRequest->pNext;
	}

	LWIO_UNLOCK_MUTEX(bInLock, &pContext->mutex);

	pTimerRequest->pPrev = NULL;
	pTimerRequest->pNext = NULL;

	return STATUS_SUCCESS;
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
	PSRV_TIMER_REQUEST pTimerIter = NULL;
	PSRV_TIMER_REQUEST pPrev = NULL;
	BOOLEAN bInLock = FALSE;

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

	pthread_mutex_init(&pTimerRequest->mutex, NULL);
	pTimerRequest->pMutex = &pTimerRequest->mutex;

	LWIO_LOCK_MUTEX(bInLock, &pTimer->context.mutex);

	for (pTimerIter = pTimer->context.pRequests;
		 pTimerIter && (pTimerIter->timespan.tv_sec <= timespan.tv_sec);
		 pPrev = pTimerIter, pTimerIter = pTimerIter->pNext);

	if (!pPrev)
	{
		pTimerRequest->pNext = pTimer->context.pRequests;
		if (pTimer->context.pRequests)
		{
			pTimer->context.pRequests->pPrev = pTimerRequest;
		}
		pTimer->context.pRequests = pTimerRequest;
	}
	else
	{
		pTimerRequest->pNext = pPrev->pNext;
		pTimerRequest->pPrev = pPrev;
		pPrev->pNext = pTimerRequest;
		if (pTimerRequest->pNext)
		{
			pTimerRequest->pNext->pPrev = pTimerRequest;
		}
	}

	InterlockedIncrement(&pTimerRequest->refCount);

	LWIO_UNLOCK_MUTEX(bInLock, &pTimer->context.mutex);

	pthread_cond_signal(&pTimer->context.event);

	*ppTimerRequest = pTimerRequest;

cleanup:

	LWIO_UNLOCK_MUTEX(bInLock, &pTimer->context.mutex);

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
	BOOLEAN bInLock = FALSE;
	PSRV_TIMER_REQUEST pIter = NULL;

	LWIO_LOCK_MUTEX(bInLock, &pTimer->context.mutex);

	for (pIter = pTimer->context.pRequests;
	     pIter && (pIter != pTimerRequest);
	     pIter = pIter->pNext);

	if (pIter)
	{
		BOOLEAN bInLock2 = FALSE;
		PSRV_TIMER_REQUEST pPrev = pIter->pPrev;

		if (pPrev)
		{
			pPrev->pNext = pIter->pNext;
		}

		if (pIter->pNext)
		{
			pIter->pNext->pPrev = pPrev;
		}

		pIter->pPrev = NULL;
		pIter->pNext = NULL;

		LWIO_LOCK_MUTEX(bInLock2, &pIter->mutex);

		pIter->pfnTimerExpiredCB = NULL;

		LWIO_UNLOCK_MUTEX(bInLock2, &pIter->mutex);
	}
	else
	{
		status = STATUS_NOT_FOUND;
		BAIL_ON_NT_STATUS(status);
	}

	LWIO_UNLOCK_MUTEX(bInLock, &pTimer->context.mutex);

	pthread_cond_signal(&pTimer->context.event);

cleanup:

	if (pIter)
	{
		SrvTimerRelease(pIter);
	}

	LWIO_UNLOCK_MUTEX(bInLock, &pTimer->context.mutex);

	return status;

error:

	goto cleanup;
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
	if (pTimerRequest->pMutex)
	{
		pthread_mutex_destroy(&pTimerRequest->mutex);
	}

	SrvFreeMemory(pTimerRequest);
}

VOID
SrvTimerIndicateStop(
	IN  PSRV_TIMER pTimer
	)
{
	if (pTimer->pTimerThread)
	{
		SrvTimerStop(&pTimer->context);
	}
}

VOID
SrvTimerFreeContents(
    IN  PSRV_TIMER pTimer
    )
{
    if (pTimer->pTimerThread)
    {
        SrvTimerStop(&pTimer->context);

        pthread_join(pTimer->timerThread, NULL);
    }

    if (pTimer->context.pEvent)
    {
	pthread_cond_destroy(&pTimer->context.event);
	pTimer->context.pEvent = NULL;
    }

    while(pTimer->context.pRequests)
    {
	PSRV_TIMER_REQUEST pRequest = pTimer->context.pRequests;

	pTimer->context.pRequests = pTimer->context.pRequests->pNext;

	SrvTimerRelease(pRequest);
    }

    if (pTimer->context.pMutex)
    {
        pthread_mutex_destroy(&pTimer->context.mutex);
        pTimer->context.pMutex = NULL;
    }
}

static
BOOLEAN
SrvTimerMustStop(
	IN  PSRV_TIMER_CONTEXT pContext
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
	IN  PSRV_TIMER_CONTEXT pContext
	)
{
    pthread_mutex_lock(&pContext->mutex);

    pContext->bStop = TRUE;

    pthread_mutex_unlock(&pContext->mutex);
}
