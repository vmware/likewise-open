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
 *        create.c
 *
 * Abstract:
 *
 *        IO Test Driver
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"
#include "lwthreads.h"


static
VOID
ItCreateInternal(
    IN PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    UNICODE_STRING path = { 0 };
    PIT_CCB pCcb = NULL;

    RtlUnicodeStringInit(&path, pIrp->Args.Create.FileName.FileName);

    status = ItpCreateCcb(&pCcb, &path);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IoFileSetContext(pIrp->FileHandle, pCcb);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pCcb = NULL;

cleanup:
    ItpDestroyCcb(&pCcb);

    pIrp->IoStatusBlock.Status = status;
}

static
VOID
ItpAsyncCreateCompleteWorkCallback(
    IN PIOTEST_WORK_ITEM pWorkItem,
    IN PVOID pContext
    )
{
    PIT_IRP_CONTEXT pIrpContext = (PIT_IRP_CONTEXT) pContext;

    if (pIrpContext->IsCancelled)
    {
        pIrpContext->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;
    }
    else
    {
        ItCreateInternal(pIrpContext->pIrp);
    }
    IoIrpComplete(pIrpContext->pIrp);
    ItDestroyIrpContext(&pIrpContext);
}

static
VOID
ItpCancelAsyncCreate(
    IN PIRP pIrp,
    IN PVOID CallbackContext
    )
{
    PIT_IRP_CONTEXT pIrpContext = (PIT_IRP_CONTEXT) CallbackContext;
    PIT_DRIVER_STATE pState = NULL;
    BOOLEAN wasInQueue = FALSE;

    pState = ItGetDriverState(pIrp);

    wasInQueue = ItRemoveWorkQueue(pState->pWorkQueue, pIrpContext->pWorkItem);

    if (wasInQueue)
    {
        NTSTATUS status = STATUS_SUCCESS;

        pIrpContext->IsCancelled = TRUE;

        status = ItAddWorkQueue(
                        pState->pWorkQueue,
                        pIrpContext->pWorkItem,
                        pIrpContext,
                        0,
                        ItpAsyncCreateCompleteWorkCallback);
        LWIO_ASSERT(!status);
    }
}


static
NTSTATUS
ItDispatchAsyncCreate(
    OUT PIT_IRP_CONTEXT* ppIrpContext,
    IN PIRP pIrp,
    IN ULONG WaitSeconds
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PIT_IRP_CONTEXT pIrpContext = NULL;
    PIT_DRIVER_STATE pState = NULL;

    status = ItCreateIrpContext(&pIrpContext, pIrp);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pState = ItGetDriverState(pIrp);

    status = ItAddWorkQueue(
                    pState->pWorkQueue,
                    pIrpContext->pWorkItem,
                    pIrpContext,
                    WaitSeconds,
                    ItpAsyncCreateCompleteWorkCallback);
    LWIO_ASSERT(!status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (status)
    {
        ItDestroyIrpContext(&pIrpContext);
    }

    *ppIrpContext = pIrpContext;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
ItDispatchCreate(
    IN PIRP pIrp
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    int EE = 0;
    UNICODE_STRING path = { 0 };
    UNICODE_STRING allowPath = { 0 };
    UNICODE_STRING asyncPath = { 0 };
    UNICODE_STRING testSyncPath = { 0 };
    UNICODE_STRING testAsyncPath = { 0 };

    RtlUnicodeStringInit(&path, pIrp->Args.Create.FileName.FileName);

    status = RtlUnicodeStringAllocateFromCString(&allowPath, IOTEST_INTERNAL_PATH_ALLOW);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlUnicodeStringAllocateFromCString(&asyncPath, IOTEST_INTERNAL_PATH_ASYNC);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlUnicodeStringAllocateFromCString(&testSyncPath, IOTEST_INTERNAL_PATH_TEST_SYNC);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = RtlUnicodeStringAllocateFromCString(&testAsyncPath, IOTEST_INTERNAL_PATH_TEST_ASYNC);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // Only succeed for certain paths.
    if (RtlUnicodeStringIsEqual(&path, &allowPath, FALSE))
    {
        // Ok
    }
    else if (RtlUnicodeStringIsEqual(&path, &asyncPath, FALSE))
    {
        PIT_IRP_CONTEXT pIrpContext = NULL;

        status = ItDispatchAsyncCreate(&pIrpContext, pIrp, 5);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        IoIrpMarkPending(pIrp, ItpCancelAsyncCreate, pIrpContext);
        status = STATUS_PENDING;

        GOTO_CLEANUP_EE(EE);
    }
    else if (RtlUnicodeStringIsEqual(&path, &testSyncPath, FALSE))
    {
        // do test first
        status = ItTestSyncCreate();
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    else if (RtlUnicodeStringIsEqual(&path, &testAsyncPath, FALSE))
    {
        // do test first
        status = ItTestAsyncCreate(TRUE, TRUE);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    else
    {
        status = STATUS_OBJECT_PATH_NOT_FOUND;
        GOTO_CLEANUP_EE(EE);
    }

    ItCreateInternal(pIrp);
    status = pIrp->IoStatusBlock.Status;

cleanup:
    RtlUnicodeStringFree(&allowPath);
    RtlUnicodeStringFree(&asyncPath);
    RtlUnicodeStringFree(&testSyncPath);
    RtlUnicodeStringFree(&testAsyncPath);

    pIrp->IoStatusBlock.Status = status;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
ItDispatchClose(
    IN PIRP pIrp
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    PIT_CCB pCcb = NULL;

    status = ItpGetCcb(&pCcb, pIrp);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    ItpDestroyCcb(&pCcb);

cleanup:    
    pIrp->IoStatusBlock.Status = status;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}
