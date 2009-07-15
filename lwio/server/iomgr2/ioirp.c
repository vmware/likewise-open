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

#include "iop.h"

typedef ULONG IRP_FLAGS;

#define IRP_FLAG_PENDING    0x00000001
#define IRP_FLAG_COMPLETE   0x00000002
#define IRP_FLAG_CANCEL     0x00000004

typedef struct _IRP_INTERNAL {
    IRP Irp;
    LONG ReferenceCount;
    IRP_FLAGS Flags;
    LW_LIST_LINKS FileObjectLinks;
    struct {
        PIO_IRP_CALLBACK Callback;
        PVOID CallbackContext;
    } Cancel;
    struct {
        BOOLEAN IsAsyncCall;
        union {
            struct {
                PIO_ASYNC_COMPLETE_CALLBACK Callback;
                PVOID CallbackContext;
                PIO_STATUS_BLOCK pIoStatusBlock;
                PIO_FILE_HANDLE pCreateFileHandle;
            } Async;
            struct {
                PLW_RTL_EVENT Event;
            } Sync;
        };
    } Completion;
} IRP_INTERNAL, *PIRP_INTERNAL;

// TODO -- make inline -- just want type-safe macro.
PIRP_INTERNAL
IopIrpGetInternal(
    IN PIRP pIrp
    )
{
    return (PIRP_INTERNAL) pIrp;
}

PIO_ASYNC_CANCEL_CONTEXT
IopIrpGetAsyncCancelContextFromIrp(
    IN PIRP Irp
    )
{
    return (PIO_ASYNC_CANCEL_CONTEXT) Irp;
}

PIRP
IopIrpGetIrpFromAsyncCancelContext(
    IN PIO_ASYNC_CANCEL_CONTEXT Context
    )
{
    return (PIRP) Context;
}

VOID
IopIrpAcquireCancelLock(
    IN PIRP pIrp
    )
{
    LwRtlLockMutex(&pIrp->FileHandle->pDevice->CancelMutex);
}

VOID
IopIrpReleaseCancelLock(
    IN PIRP pIrp
    )
{
    LwRtlUnlockMutex(&pIrp->FileHandle->pDevice->CancelMutex);
}

NTSTATUS
IopIrpCreate(
    OUT PIRP* ppIrp,
    IN IRP_TYPE Type,
    IN PIO_FILE_OBJECT pFileObject
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    PIRP_INTERNAL irpInternal = NULL;

    // Note that we allocate enough space for the internal fields.
    status = IO_ALLOCATE(&pIrp, IRP, sizeof(IRP_INTERNAL));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    irpInternal = IopIrpGetInternal(pIrp);
    irpInternal->ReferenceCount = 1;

    pIrp->Type = Type;
    pIrp->FileHandle = pFileObject;
    IopFileObjectReference(pFileObject);
    pIrp->DeviceHandle = pFileObject->pDevice;
    pIrp->DriverHandle = pFileObject->pDevice->Driver;

    // XXX - LOCK...
    LwListInsertTail(&pFileObject->IrpList,
                     &irpInternal->FileObjectLinks);

cleanup:
    if (status)
    {
        IopIrpDereference(&pIrp);
    }

    *ppIrp = pIrp;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

// should become static
static
VOID
IopIrpFree(
    IN OUT PIRP* ppIrp
    )
{
    PIRP pIrp = *ppIrp;

    if (pIrp)
    {
        PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);

        LWIO_ASSERT(0 == irpInternal->ReferenceCount);
        LWIO_ASSERT(STATUS_PENDING != pIrp->IoStatusBlock.Status);

        switch (pIrp->Type)
        {
            case IRP_TYPE_CREATE:
            case IRP_TYPE_CREATE_NAMED_PIPE:
                IoSecurityDereferenceSecurityContext(&pIrp->Args.Create.SecurityContext);
                RtlWC16StringFree(&pIrp->Args.Create.FileName.FileName);
                break;
            case IRP_TYPE_QUERY_DIRECTORY:
                if (pIrp->Args.QueryDirectory.FileSpec)
                {
                    LwRtlUnicodeStringFree(&pIrp->Args.QueryDirectory.FileSpec->Pattern);
                    IO_FREE(&pIrp->Args.QueryDirectory.FileSpec);
                }
                break;
            default:
                break;
        }

        LwListRemove(&irpInternal->FileObjectLinks);
        IopFileObjectDereference(&pIrp->FileHandle);

        IoMemoryFree(pIrp);
        *ppIrp = NULL;
    }
}

static
VOID
IopIrpSetAsyncCompletionCallback(
    IN PIRP pIrp,
    IN PIO_ASYNC_COMPLETE_CALLBACK Callback,
    IN OPTIONAL PVOID CallbackContext,
    IN PIO_STATUS_BLOCK pIoStatusBlock,
    IN OPTIONAL PIO_FILE_HANDLE pCreateFileHandle
    )
{
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);

    LWIO_ASSERT(Callback);
    LWIO_ASSERT(pIoStatusBlock);
    LWIO_ASSERT(IS_BOTH_OR_NEITHER(pCreateFileHandle, IRP_TYPE_CREATE == pIrp->Type));

    irpInternal->Completion.IsAsyncCall = TRUE;
    irpInternal->Completion.Async.Callback = Callback;
    irpInternal->Completion.Async.CallbackContext = CallbackContext;
    irpInternal->Completion.Async.pIoStatusBlock = pIoStatusBlock;
    irpInternal->Completion.Async.pCreateFileHandle = pCreateFileHandle;
}

static
VOID
IopIrpSetSyncCompletionCallback(
    IN PIRP pIrp,
    IN PLW_RTL_EVENT pEvent
    )
{
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);

    irpInternal->Completion.IsAsyncCall = FALSE;
    irpInternal->Completion.Sync.Event = pEvent;
}

#if 0
BOOLEAN
IoIrpIsCancelled(
    IN PIRP pIrp
    )
{
    BOOLEAN isCancelled = FALSE;
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);

    IopIrpAcquireCancelLock(pIrp);
    isCancelled = IsSetFlag(irpInternal->Flags, IRP_FLAG_CANCEL);
    IopIrpReleaseCancelLock(pIrp);

    return isCancelled;
}
#endif

VOID
IoIrpMarkPending(
    IN PIRP pIrp,
    IN PIO_IRP_CALLBACK CancelCallback,
    IN OPTIONAL PVOID CancelCallbackContext
    )
{
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);

    LWIO_ASSERT(CancelCallback);

    IopIrpAcquireCancelLock(pIrp);

    LWIO_ASSERT(!irpInternal->Cancel.Callback);
    LWIO_ASSERT(!IsSetFlag(irpInternal->Flags, IRP_FLAG_PENDING));
    LWIO_ASSERT(!IsSetFlag(irpInternal->Flags, IRP_FLAG_COMPLETE));
    LWIO_ASSERT(!IsSetFlag(irpInternal->Flags, IRP_FLAG_CANCEL));

    SetFlag(irpInternal->Flags, IRP_FLAG_PENDING);
    irpInternal->Cancel.Callback = CancelCallback;
    irpInternal->Cancel.CallbackContext = CancelCallbackContext;

    IopIrpReleaseCancelLock(pIrp);

    //
    // Take a reference that will be released by IoIrpComplete.
    //

    IopIrpReference(pIrp);
}

#if 0
VOID
IoIrpSetCancelCallback(
    IN PIRP pIrp,
    IN OPTIONAL PIO_IRP_CALLBACK CancelCallback,
    IN OPTIONAL PVOID CancelCallbackContext
    )
{
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);

    IopIrpAcquireCancelLock(pIrp);

    LWIO_ASSERT(IsSetFlag(irpInternal->Flags, IRP_FLAG_PENDING));
    LWIO_ASSERT(!IsSetFlag(irpInternal->Flags, IRP_FLAG_CANCEL));
    LWIO_ASSERT(!IsSetFlag(irpInternal->Flags, IRP_FLAG_COMPLETE));

    if (CancelCallback)
    {
        LWIO_ASSERT(!irpInternal->Cancel.Callback);
    }
    else
    {
        LWIO_ASSERT(irpInternal->Cancel.Callback);
    }

    irpInternal->Cancel.Callback = CancelCallback;
    irpInternal->Cancel.CallbackContext = CancelCallbackContext;

    IopIrpReleaseCancelLock(pIrp);
}
#endif

// must have set IO status block in IRP.
VOID
IoIrpComplete(
    IN OUT PIRP pIrp
    )
{
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);

    IopIrpAcquireCancelLock(pIrp);

    LWIO_ASSERT_MSG(IsSetFlag(irpInternal->Flags, IRP_FLAG_PENDING), "IRP must have been pending to be completed this way.");
    LWIO_ASSERT_MSG(!IsSetFlag(irpInternal->Flags, IRP_FLAG_COMPLETE), "IRP cannot be completed more than once.");

    SetFlag(irpInternal->Flags, IRP_FLAG_COMPLETE);

    IopIrpReleaseCancelLock(pIrp);

    if (irpInternal->Completion.IsAsyncCall)
    {
        if (((IRP_TYPE_CREATE == pIrp->Type) ||
             (IRP_TYPE_CREATE_NAMED_PIPE == pIrp->Type)) &&
            (STATUS_SUCCESS == pIrp->IoStatusBlock.Status))
        {
            *irpInternal->Completion.Async.pCreateFileHandle = pIrp->FileHandle;
            IopFileObjectReference(pIrp->FileHandle);
        }
        *irpInternal->Completion.Async.pIoStatusBlock = pIrp->IoStatusBlock;
        irpInternal->Completion.Async.Callback(
                irpInternal->Completion.Async.CallbackContext);
        // IopIrpDereference(&pIrp);
    }
    else
    {
        LwRtlSetEvent(irpInternal->Completion.Sync.Event);
    }

    //
    // Release reference from IoIrpMarkPending().
    //

    IopIrpDereference(&pIrp);
}

VOID
IopIrpReference(
    IN PIRP pIrp
    )
{
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);
    InterlockedIncrement(&irpInternal->ReferenceCount);
}

VOID
IopIrpDereference(
    IN OUT PIRP* ppIrp
    )
{
    if (*ppIrp)
    {
        PIRP_INTERNAL irpInternal = IopIrpGetInternal(*ppIrp);
        LONG count = InterlockedDecrement(&irpInternal->ReferenceCount);
        LWIO_ASSERT(count >= 0);
        if (0 == count)
        {
            IopIrpFree(ppIrp);
        }
        *ppIrp = NULL;
    }
}

BOOLEAN
IopIrpCancel(
    IN PIRP pIrp
    )
{
    PIRP_INTERNAL irpInternal = IopIrpGetInternal(pIrp);
    BOOLEAN isCancellable = FALSE;
    BOOLEAN isAcquired = FALSE;

    if (!pIrp)
    {
        GOTO_CLEANUP();
    }

    IopIrpReference(pIrp);

    IopIrpAcquireCancelLock(pIrp);
    isAcquired = TRUE;

    // TODO -- Add IRP_FLAG_CANCEL_PENDING?

    if (!IsSetFlag(irpInternal->Flags, IRP_FLAG_CANCEL | IRP_FLAG_COMPLETE))
    {
        SetFlag(irpInternal->Flags, IRP_FLAG_CANCEL);
        if (irpInternal->Cancel.Callback)
        {
            isCancellable = TRUE;
            irpInternal->Cancel.Callback(
                    pIrp,
                    irpInternal->Cancel.CallbackContext);
        }
    }
    else
    {
        // If already cancelled or complete, we consider it as cancellable.
        isCancellable = TRUE;
    }

cleanup:

    if (isAcquired)
    {
        IopIrpReleaseCancelLock(pIrp);
    }

    if (pIrp)
    {
        IopIrpDereference(&pIrp);
    }

    return isCancellable;
}

NTSTATUS
IopIrpDispatch(
    IN PIRP pIrp,
    IN OUT OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    IN OPTIONAL PIO_STATUS_BLOCK pIoStatusBlock,
    IN OPTIONAL PIO_FILE_HANDLE pCreateFileHandle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int EE = 0;
    BOOLEAN isAsyncCall = FALSE;
    LW_RTL_EVENT event = LW_RTL_EVENT_ZERO_INITIALIZER;
    PIRP pExtraIrpReference = NULL;

    if (AsyncControlBlock)
    {
        LWIO_ASSERT(!AsyncControlBlock->AsyncCancelContext);

        isAsyncCall = TRUE;
        IopIrpSetAsyncCompletionCallback(
                pIrp,
                AsyncControlBlock->Callback,
                AsyncControlBlock->CallbackContext,
                pIoStatusBlock,
                pCreateFileHandle);

        // Reference IRP since we may need to return an an async cancel context.
        IopIrpReference(pIrp);
        pExtraIrpReference = pIrp;
    }
    else
    {
        isAsyncCall = FALSE;

        status = LwRtlInitializeEvent(&event);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        IopIrpSetSyncCompletionCallback(
                pIrp,
                &event);
    }

    status = IopDeviceCallDriver(pIrp->DeviceHandle, pIrp);
    if (!isAsyncCall)
    {
        if (STATUS_PENDING == status)
        {
            LwRtlWaitEvent(&event, NULL);

            LWIO_ASSERT(pIrp->IoStatusBlock.Status != STATUS_PENDING);
            status = pIrp->IoStatusBlock.Status;
        }
    }

    //
    // At this point, we are either complete or this is
    // an async call that returned STATUS_PENDING.
    //

    LWIO_ASSERT((STATUS_PENDING == status) || (pIrp->IoStatusBlock.Status == status));

    if (STATUS_PENDING == status)
    {
        LWIO_ASSERT(isAsyncCall);

        AsyncControlBlock->AsyncCancelContext = IopIrpGetAsyncCancelContextFromIrp(pIrp);
    }
    else
    {
        if (isAsyncCall)
        {
            //
            // Remove async cancel context reference added earlier since we
            // are returning synchronously w/o an async cancel context.
            //

            IopIrpDereference(&pExtraIrpReference);
        }
    }

cleanup:
    if (STATUS_PENDING != status)
    {
        pIrp->IoStatusBlock.Status = status;
    }

    LwRtlCleanupEvent(&event);

    LWIO_ASSERT(NT_PENDING_OR_SUCCESS_OR_NOT(status));
    return status;
}
