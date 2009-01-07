/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        response.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSMB)
 *
 *        Common Client Response Code
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "includes.h"

DWORD
SMBResponseCreate(
    uint16_t       wMid,
    SMB_RESPONSE **ppResponse
    )
{
    DWORD dwError = 0;
    PSMB_RESPONSE pResponse = NULL;
    BOOLEAN bDestroyCondition = FALSE;
    BOOLEAN bDestroyMutex = FALSE;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_RESPONSE),
                    (PVOID*)&pResponse);
    BAIL_ON_SMB_ERROR(dwError);

    pthread_mutex_init(&pResponse->mutex, NULL);

    bDestroyMutex = TRUE;

    pResponse->state = SMB_RESOURCE_STATE_INITIALIZING;
    pResponse->error.type = ERROR_SMB;
    pResponse->error.smb = SMB_ERROR_SUCCESS;

    dwError = pthread_cond_init(&pResponse->event, NULL);
    BAIL_ON_SMB_ERROR(dwError);

    bDestroyCondition = TRUE;

    pResponse->pTree = NULL;
    pResponse->mid = wMid;
    pResponse->pPacket = NULL;

    *ppResponse = pResponse;

cleanup:

    return dwError;

error:

    if (bDestroyCondition)
    {
        pthread_cond_destroy(&pResponse->event);
    }

    if (bDestroyMutex)
    {
        pthread_mutex_destroy(&pResponse->mutex);
    }

    SMB_SAFE_FREE_MEMORY(pResponse);

    *ppResponse = NULL;

    goto cleanup;
}

/* This function does not remove a socket from it's parent hash; it merely
   frees the memory if the refcount is zero. */
VOID
SMBResponseFree(
    PSMB_RESPONSE pResponse
    )
{
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pResponse->mutex);

    pthread_cond_destroy(&pResponse->event);

    pthread_mutex_destroy(&pResponse->mutex);

    if (pResponse->pTree)
    {
        SMBTreeRelease(pResponse->pTree);
    }

    /* @todo: use allocator */
    SMBFreeMemory(pResponse);
}

VOID
SMBResponseInvalidate(
    PSMB_RESPONSE pResponse,
    SMB_ERROR_TYPE errorType,
    uint32_t networkError
    )
{
    pResponse->state = SMB_RESOURCE_STATE_INVALID;
    pResponse->error.type = errorType;
    pResponse->error.smb  = networkError;

    pthread_cond_broadcast(&pResponse->event);
}

VOID
SMBResponseUnlock(
    PSMB_RESPONSE pResponse
    )
{
    BOOLEAN bInLock = TRUE;

    SMB_UNLOCK_MUTEX(bInLock, &pResponse->mutex);
}
