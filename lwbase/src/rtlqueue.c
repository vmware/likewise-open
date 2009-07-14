/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        rtlqueue.c
 *
 * Abstract:
 *
 *        Base FIFO Queue data structure
 *
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */


#include <lw/base.h>

/* Macros */

#ifndef BAIL_ON_NT_STATUS
#define BAIL_ON_NT_STATUS(err)                      \
    do {                                            \
        if ((err) != STATUS_SUCCESS) {              \
            goto error;                             \
        }                                           \
    } while (0);
#endif

#ifndef BAIL_ON_INVALID_PTR
#define BAIL_ON_INVALID_PTR(ptr, err)                  \
    do {                                               \
        if ((ptr) == NULL) {                           \
            err = STATUS_INVALID_PARAMETER;            \
            goto error;                                \
        }                                              \
    } while (0);
#endif

#ifndef BAIL_ON_NULL_PTR
#define BAIL_ON_NULL_PTR(x, err)                    \
    do {                                            \
        if ((x) == NULL) {                          \
            err = STATUS_INSUFFICIENT_RESOURCES;	\
            goto error;                             \
        }                                           \
    } while(0);
#endif

/* Structures */

typedef struct _LWRTL_QUEUE_NODE {
    struct _LWRTL_QUEUE_NODE *Next;
    struct _LWRTL_QUEUE_NODE *Prev;

    PVOID pData;

} LWRTL_QUEUE_NODE, *PLWRTL_QUEUE_NODE;


typedef struct _LWRTL_QUEUE {
    INT32 MaxSize;
    INT32 CurrentSize;

    PLWRTL_QUEUE_FREE_DATA_FN pfnFreeData;

    PLWRTL_QUEUE_NODE Head;
    PLWRTL_QUEUE_NODE Tail;
} LWRTL_QUEUE;


/* Functions */

/*************************************************************
 ************************************************************/

NTSTATUS
LwRtlQueueInit(
    PLWRTL_QUEUE *ppNewQueue,
    DWORD dwMaxSize,
    PLWRTL_QUEUE_FREE_DATA_FN pfnFreeData
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PLWRTL_QUEUE pQueue = NULL;

    BAIL_ON_INVALID_PTR(ppNewQueue, ntStatus);

    ntStatus = RTL_ALLOCATE(&pQueue, LWRTL_QUEUE, sizeof(LWRTL_QUEUE));
    BAIL_ON_NT_STATUS(ntStatus);

    pQueue->MaxSize = dwMaxSize;
    pQueue->CurrentSize = 0;

    pQueue->pfnFreeData = pfnFreeData;

    pQueue->Head = NULL;
    pQueue->Tail = NULL;

    *ppNewQueue = pQueue;
    pQueue = NULL;

    ntStatus = STATUS_SUCCESS;

cleanup:
    RTL_FREE(&pQueue);

    return ntStatus;

error:
    goto cleanup;
}


/*************************************************************
 ************************************************************/

NTSTATUS
LwRtlQueueSetMaxSize(
    PLWRTL_QUEUE pQueue,
    DWORD dwNewLength
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(pQueue, ntStatus);

    /* We can set the Queue to an unlimited size
       or larger that the current size */

    if ((dwNewLength != 0) && (dwNewLength < pQueue->CurrentSize))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pQueue->MaxSize = dwNewLength;

    ntStatus = STATUS_SUCCESS;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


/*************************************************************
 ************************************************************/

NTSTATUS
LwRtlQueueAddItem(
    PLWRTL_QUEUE pQueue,
    PVOID pItem
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PLWRTL_QUEUE_NODE pNode = NULL;

    BAIL_ON_INVALID_PTR(pQueue, ntStatus);
    BAIL_ON_INVALID_PTR(pItem, ntStatus);

    /* Using >= here to be safe.  Technically, == should
       be fine */

    if (LwRtlQueueIsFull(pQueue)) {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = RTL_ALLOCATE(&pNode, LWRTL_QUEUE_NODE,
                            sizeof(LWRTL_QUEUE_NODE));
    BAIL_ON_NT_STATUS(ntStatus);

    /* Add the to the front of the Queue.  Update the Tail
       if necessary */

    pNode->pData = pItem;
    pNode->Next  = pQueue->Head;
    pNode->Prev = NULL;

    pQueue->Head = pNode;
    pQueue->CurrentSize++;

    if (pQueue->Tail == NULL) {
        pQueue->Tail = pQueue->Head;
    }

    ntStatus = STATUS_SUCCESS;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}



/*************************************************************
 ************************************************************/

NTSTATUS
LwRtlQueueRemoveItem(
    PLWRTL_QUEUE pQueue,
    PVOID *ppItem
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PLWRTL_QUEUE_NODE pNode = NULL;

    BAIL_ON_INVALID_PTR(pQueue, ntStatus);
    BAIL_ON_INVALID_PTR(ppItem, ntStatus);

    if (LwRtlQueueIsEmpty(pQueue)) {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pNode = pQueue->Tail;

    if (pQueue->Head == pQueue->Tail)
    {
        pQueue->Head = NULL;
        pQueue->Tail = NULL;
    }
    else
    {
        pQueue->Tail = pQueue->Tail->Prev;
    }
    pQueue->CurrentSize--;

    *ppItem = pNode->pData;
    RTL_FREE(&pNode);

    ntStatus = STATUS_SUCCESS;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}

/*************************************************************
 ************************************************************/

BOOL
LwRtlQueueIsEmpty(
    PLWRTL_QUEUE pQueue
    )
{
    return (pQueue->CurrentSize == 0);
}

/*************************************************************
 ************************************************************/

BOOL
LwRtlQueueIsFull(
    PLWRTL_QUEUE pQueue
    )
{
    if (pQueue->MaxSize == 0) {
        return FALSE;
    }

    return (pQueue->CurrentSize >= pQueue->MaxSize);
}

/*************************************************************
 ************************************************************/

VOID
LwRtlQueueDestroy(
    PLWRTL_QUEUE *ppQueue
    )
{
    PLWRTL_QUEUE pQueue = NULL;
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PVOID pData = NULL;

    if (!ppQueue) {
        return;
    }

    pQueue = *ppQueue;

    while (pQueue->Head != NULL)
    {
        ntStatus = LwRtlQueueRemoveItem(pQueue, &pData);
        if (ntStatus != STATUS_SUCCESS) {
            /* Avoid an infinite loop in the case of
               an error.  This means we may leak memory. */
            break;
        }

        pQueue->pfnFreeData(&pData);
        pData = NULL;
    }

    RTL_FREE(&pQueue);

    *ppQueue = NULL;

    return;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
