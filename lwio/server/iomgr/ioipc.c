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
 *        ioipc.c
 *
 * Abstract:
 *
 *        IO lwmsg IPC Implementaion
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "iop.h"
#include "ntipcmsg.h"
#include "ntlogmacros.h"
#include "ioapi.h"
#include "ioipc.h"

typedef struct _IO_IPC_CALL_CONTEXT
{
    IO_STATUS_BLOCK ioStatusBlock;
    IO_ASYNC_CONTROL_BLOCK asyncBlock;
    const LWMsgParams* pIn;
    LWMsgParams* pOut;
    LWMsgCall* pCall;
} IO_IPC_CALL_CONTEXT, *PIO_IPC_CALL_CONTEXT;

static
NTSTATUS
IopIpcCreateCallContext(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    IN LWMsgParams* pOut,
    IN PIO_ASYNC_COMPLETE_CALLBACK pfnCallback,
    OUT PIO_IPC_CALL_CONTEXT* ppContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_IPC_CALL_CONTEXT pContext = NULL;

    status = IO_ALLOCATE(&pContext, IO_IPC_CALL_CONTEXT, sizeof(*pContext));
    BAIL_ON_NT_STATUS(status);

    pContext->pCall = pCall;
    pContext->pIn = pIn;
    pContext->pOut = pOut;
    pContext->asyncBlock.Callback = pfnCallback;
    pContext->asyncBlock.CallbackContext = pContext;

    *ppContext = pContext;

cleanup:

    return status;

error:

    *ppContext = NULL;

    if (pContext)
    {
        IO_FREE(&pContext);
    }

    goto cleanup;
}

static
VOID
IopIpcFreeCallContext(
    PIO_IPC_CALL_CONTEXT pContext
    )
{
    IO_FREE(&pContext);
}

static
void
IopIpcCancelCall(
    LWMsgCall* pCall,
    void* pData
    )
{
    PIO_IPC_CALL_CONTEXT pContext = (PIO_IPC_CALL_CONTEXT) pData;

    IoCancelAsyncCancelContext(pContext->asyncBlock.AsyncCancelContext);
}

static
VOID
IopIpcCompleteGenericCall(
    IN PVOID pData
    )
{
    PIO_IPC_CALL_CONTEXT pContext = (PIO_IPC_CALL_CONTEXT) pData;
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pReply = pContext->pOut->data;

    pReply->Status = pContext->ioStatusBlock.Status;
    pReply->BytesTransferred = pContext->ioStatusBlock.BytesTransferred;

    IoDereferenceAsyncCancelContext(&pContext->asyncBlock.AsyncCancelContext);
    lwmsg_call_complete(pContext->pCall, LWMSG_STATUS_SUCCESS);

    IopIpcFreeCallContext(pContext);
}

static
VOID
IopIpcCompleteRundownCall(
    IN PVOID pData
    )
{
    PIO_IPC_CALL_CONTEXT pContext = (PIO_IPC_CALL_CONTEXT) pData;
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pReply = pContext->pOut->data;

    pReply->Status = pContext->ioStatusBlock.Status;

    IoDereferenceAsyncCancelContext(&pContext->asyncBlock.AsyncCancelContext);
    lwmsg_call_complete(pContext->pCall, LWMSG_STATUS_SUCCESS);

    IopIpcFreeCallContext(pContext);
}

static
VOID
IopIpcCompleteCloseCall(
    IN PVOID pData
    )
{
    PIO_STATUS_BLOCK pIoStatus = (PIO_STATUS_BLOCK) pData;

    if (pIoStatus->Status)
    {
        LWIO_LOG_ERROR("failed to cleanup handle (status = 0x%08x)", pIoStatus->Status);
    }

    RTL_FREE(&pIoStatus);
}

static
VOID
IopIpcCleanupFileHandle(
    PVOID pHandle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_FILE_HANDLE fileHandle = (IO_FILE_HANDLE) pHandle;
    PIO_STATUS_BLOCK pIoStatus = NULL;
    IO_ASYNC_CONTROL_BLOCK async = {0};

    if (RTL_ALLOCATE(&pIoStatus, IO_STATUS_BLOCK, sizeof(*pIoStatus)) != STATUS_SUCCESS)
    {
        status = IoCloseFile(fileHandle);
        GOTO_ERROR_ON_STATUS(status);
    }
    else
    {
        async.Callback = IopIpcCompleteCloseCall;
        async.CallbackContext = pIoStatus;

        status = IoAsyncCloseFile(fileHandle, &async, pIoStatus);
        switch (status)
        {
        case STATUS_SUCCESS:
            RTL_FREE(&pIoStatus);
            break;
        case STATUS_PENDING:
            IoDereferenceAsyncCancelContext(&async.AsyncCancelContext);
            status = STATUS_SUCCESS;
            break;
        default:
            GOTO_ERROR_ON_STATUS(status);
        }
    }

error:

    if (status)
    {
        LWIO_LOG_ERROR("failed to cleanup handle (status = 0x%08x)", status);
    }
}

static
NTSTATUS
IopIpcRegisterFileHandle(
    IN LWMsgCall* pCall,
    IN IO_FILE_HANDLE FileHandle
    )
{
    NTSTATUS status = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    status = NtIpcLWMsgStatusToNtStatus(lwmsg_session_register_handle(
                                            pSession,
                                            "IO_FILE_HANDLE",
                                            FileHandle,
                                            IopIpcCleanupFileHandle));
    return status;
}

static
NTSTATUS
IopIpcRetainFileHandle(
    IN LWMsgCall* pCall,
    IN IO_FILE_HANDLE FileHandle
    )
{
    NTSTATUS status = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    status = NtIpcLWMsgStatusToNtStatus(lwmsg_session_retain_handle(
                                            pSession,
                                            FileHandle));
    assert(!status);
    return status;
}

static
NTSTATUS
IopIpcGetProcessSecurity(
    IN LWMsgCall* pCall,
    OUT uid_t* pUid,
    OUT gid_t* pGid
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);
    LWMsgSecurityToken* token = lwmsg_session_get_peer_security_token(pSession);
    uid_t uid = (uid_t) -1;
    gid_t gid = (gid_t) -1;

    if (token == NULL || strcmp(lwmsg_security_token_get_type(token), "local"))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = NtIpcLWMsgStatusToNtStatus(lwmsg_local_token_get_eid(token, &uid, &gid));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    *pUid = uid;
    *pGid = gid;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

static
LWMsgStatus
IopIpcCreateFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_CREATE_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_CREATE_FILE_RESULT;
    PNT_IPC_MESSAGE_CREATE_FILE pMessage = (PNT_IPC_MESSAGE_CREATE_FILE) pIn->data;
    PNT_IPC_MESSAGE_CREATE_FILE_RESULT pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IO_FILE_HANDLE fileHandle = NULL;
    IO_FILE_NAME fileName = { 0 };
    uid_t uid = 0;
    gid_t gid = 0;
    PIO_CREATE_SECURITY_CONTEXT securityContext = NULL;
    PIO_ECP_LIST pEcpList = NULL;

    assert(messageType == pIn->tag);

    status = IopIpcGetProcessSecurity(
                    pCall,
                    &uid,
                    &gid);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IoSecurityCreateSecurityContextFromUidGid(
                    &securityContext,
                    uid,
                    gid,
                    pMessage->pSecurityToken);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_CREATE_FILE_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    fileName = pMessage->FileName;

    if (pMessage->EcpCount)
    {
        ULONG ecpIndex = 0;

        pReply->Status = IoRtlEcpListAllocate(&pEcpList);
        GOTO_CLEANUP_ON_STATUS_EE(pReply->Status, EE);

        for (ecpIndex = 0; ecpIndex < pMessage->EcpCount; ecpIndex++)
        {
            pReply->Status = IoRtlEcpListInsert(
                                    pEcpList,
                                    pMessage->EcpList[ecpIndex].pszType,
                                    pMessage->EcpList[ecpIndex].pData,
                                    pMessage->EcpList[ecpIndex].Size,
                                    NULL);
            GOTO_CLEANUP_ON_STATUS_EE(pReply->Status, EE);
        }
    }

    // TODO -- SDs, etc.
    pReply->Status = IoCreateFile(
                            &fileHandle,
                            NULL,
                            &ioStatusBlock,
                            securityContext,
                            &fileName,
                            NULL,
                            NULL,
                            pMessage->DesiredAccess,
                            pMessage->AllocationSize,
                            pMessage->FileAttributes,
                            pMessage->ShareAccess,
                            pMessage->CreateDisposition,
                            pMessage->CreateOptions,
                            pMessage->EaBuffer,
                            pMessage->EaLength,
                            pEcpList);

    // Register handle with lwmsg if it was created
    if (fileHandle)
    {
        status = IopIpcRegisterFileHandle(pCall, fileHandle);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        pReply->FileHandle = fileHandle;
        fileHandle = NULL;

        status = IopIpcRetainFileHandle(pCall, pReply->FileHandle);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    pReply->Status = ioStatusBlock.Status;
    pReply->CreateResult = ioStatusBlock.CreateResult;

cleanup:

    if (status)
    {
        if (fileHandle)
        {
            IoCloseFile(fileHandle);
        }
    }

    IoSecurityDereferenceSecurityContext(&securityContext);
    IoRtlEcpListFree(&pEcpList);

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcCloseFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    static const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_CLOSE_FILE_RESULT;
    PNT_IPC_MESSAGE_GENERIC_FILE pMessage = (PNT_IPC_MESSAGE_GENERIC_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pReply = NULL;
    PIO_IPC_CALL_CONTEXT pContext = NULL;

    status = NtIpcUnregisterFileHandle(pCall, pMessage->FileHandle);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IopIpcCreateCallContext(pCall, pIn, pOut, IopIpcCompleteRundownCall, &pContext);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    status = IoRundownFile(
        pMessage->FileHandle,
        &pContext->asyncBlock,
        &pContext->ioStatusBlock);
    switch (status)
    {
    case STATUS_PENDING:
        lwmsg_call_pend(pCall, IopIpcCancelCall, pContext);
        break;
    default:
        pReply->Status = pContext->ioStatusBlock.Status;
        status = STATUS_SUCCESS;
        break;
    }

cleanup:

    if (pContext && status != STATUS_PENDING)
    {
        IopIpcFreeCallContext(pContext);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

// TODO -- ASK BRIAN ABOUT HANDLING on pOut on error (wrt free).

LWMsgStatus
IopIpcReadFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_READ_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_READ_FILE_RESULT;
    PNT_IPC_MESSAGE_READ_FILE pMessage = (PNT_IPC_MESSAGE_READ_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pReply = NULL;
    PIO_IPC_CALL_CONTEXT pContext = NULL;

    assert(messageType == pIn->tag);

    status = IopIpcCreateCallContext(pCall, pIn, pOut, IopIpcCompleteGenericCall, &pContext);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    if (pMessage->Length)
    {
        pReply->Status = IO_ALLOCATE(&pReply->Buffer, VOID, pMessage->Length);
        GOTO_CLEANUP_ON_STATUS_EE(pReply->Status, EE);
    }

    status = IoReadFile(
        pMessage->FileHandle,
        &pContext->asyncBlock,
        &pContext->ioStatusBlock,
        pReply->Buffer,
        pMessage->Length,
        pMessage->ByteOffset,
        pMessage->Key);

    switch (status)
    {
    case STATUS_PENDING:
        lwmsg_call_pend(pCall, IopIpcCancelCall, pContext);
        break;
    default:
        pReply->Status = pContext->ioStatusBlock.Status;
        pReply->BytesTransferred = pContext->ioStatusBlock.BytesTransferred;
        status = STATUS_SUCCESS;
        break;
    }

cleanup:

    if (pContext && status != STATUS_PENDING)
    {
        IopIpcFreeCallContext(pContext);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);

    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcWriteFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_WRITE_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_WRITE_FILE_RESULT;
    PNT_IPC_MESSAGE_WRITE_FILE pMessage = (PNT_IPC_MESSAGE_WRITE_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    assert(messageType == pIn->tag);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    pReply->Status = IoWriteFile(
                            pMessage->FileHandle,
                            NULL,
                            &ioStatusBlock,
                            pMessage->Buffer,
                            pMessage->Length,
                            pMessage->ByteOffset,
                            pMessage->Key);
    pReply->Status = ioStatusBlock.Status;
    pReply->BytesTransferred = ioStatusBlock.BytesTransferred;

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcDeviceIoControlFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_DEVICE_IO_CONTROL_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_DEVICE_IO_CONTROL_FILE_RESULT;
    PNT_IPC_MESSAGE_GENERIC_CONTROL_FILE pMessage = (PNT_IPC_MESSAGE_GENERIC_CONTROL_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    assert(messageType == pIn->tag);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    if (pMessage->OutputBufferLength)
    {
        pReply->Status = IO_ALLOCATE(&pReply->Buffer, VOID, pMessage->OutputBufferLength);
        GOTO_CLEANUP_ON_STATUS_EE(pReply->Status, EE);
    }

    pReply->Status = IoDeviceIoControlFile(
                            pMessage->FileHandle,
                            NULL,
                            &ioStatusBlock,
                            pMessage->ControlCode,
                            pMessage->InputBuffer,
                            pMessage->InputBufferLength,
                            pReply->Buffer,
                            pMessage->OutputBufferLength);
    pReply->Status = ioStatusBlock.Status;
    pReply->BytesTransferred = ioStatusBlock.BytesTransferred;

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcFsControlFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_FS_CONTROL_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_FS_CONTROL_FILE_RESULT;
    PNT_IPC_MESSAGE_GENERIC_CONTROL_FILE pMessage = (PNT_IPC_MESSAGE_GENERIC_CONTROL_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pReply = NULL;
    PIO_IPC_CALL_CONTEXT pContext = NULL;

    assert(messageType == pIn->tag);

    status = IopIpcCreateCallContext(pCall, pIn, pOut, IopIpcCompleteGenericCall, &pContext);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    if (pMessage->OutputBufferLength)
    {
        pReply->Status = IO_ALLOCATE(&pReply->Buffer, VOID, pMessage->OutputBufferLength);
        GOTO_CLEANUP_ON_STATUS_EE(pReply->Status, EE);
    }

    status = IoFsControlFile(
        pMessage->FileHandle,
        &pContext->asyncBlock,
        &pContext->ioStatusBlock,
        pMessage->ControlCode,
        pMessage->InputBuffer,
        pMessage->InputBufferLength,
        pReply->Buffer,
        pMessage->OutputBufferLength);

    switch (status)
    {
    case STATUS_PENDING:
        lwmsg_call_pend(pCall, IopIpcCancelCall, pContext);
        break;
    default:
        pReply->Status = pContext->ioStatusBlock.Status = status;
        pReply->BytesTransferred = pContext->ioStatusBlock.BytesTransferred;
        status = STATUS_SUCCESS;
        break;
    }

cleanup:

    if (pContext && status != STATUS_PENDING)
    {
        IopIpcFreeCallContext(pContext);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);

    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcFlushBuffersFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_FLUSH_BUFFERS_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_FLUSH_BUFFERS_FILE_RESULT;
    PNT_IPC_MESSAGE_GENERIC_FILE pMessage = (PNT_IPC_MESSAGE_GENERIC_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    assert(messageType == pIn->tag);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    pReply->Status = IoFlushBuffersFile(pMessage->FileHandle, NULL, &ioStatusBlock);
    pReply->Status = ioStatusBlock.Status;
    pReply->BytesTransferred = ioStatusBlock.BytesTransferred;

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcQueryInformationFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_QUERY_INFORMATION_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_QUERY_INFORMATION_FILE_RESULT;
    PNT_IPC_MESSAGE_QUERY_INFORMATION_FILE pMessage = (PNT_IPC_MESSAGE_QUERY_INFORMATION_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    assert(messageType == pIn->tag);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    if (pMessage->Length)
    {
        pReply->Status = IO_ALLOCATE(&pReply->Buffer, VOID, pMessage->Length);
        GOTO_CLEANUP_ON_STATUS_EE(pReply->Status, EE);
    }

    pReply->Status = IoQueryInformationFile(
                            pMessage->FileHandle,
                            NULL,
                            &ioStatusBlock,
                            pReply->Buffer,
                            pMessage->Length,
                            pMessage->FileInformationClass);
    pReply->Status = ioStatusBlock.Status;
    pReply->BytesTransferred = ioStatusBlock.BytesTransferred;

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcSetInformationFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_SET_INFORMATION_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_SET_INFORMATION_FILE_RESULT;
    PNT_IPC_MESSAGE_SET_INFORMATION_FILE pMessage = (PNT_IPC_MESSAGE_SET_INFORMATION_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    assert(messageType == pIn->tag);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    pReply->Status = IoSetInformationFile(
                            pMessage->FileHandle,
                            NULL,
                            &ioStatusBlock,
                            pMessage->FileInformation,
                            pMessage->Length,
                            pMessage->FileInformationClass);
    pReply->Status = ioStatusBlock.Status;
    pReply->BytesTransferred = ioStatusBlock.BytesTransferred;

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcQueryDirectoryFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_QUERY_DIRECTORY_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_QUERY_DIRECTORY_FILE_RESULT;
    PNT_IPC_MESSAGE_QUERY_DIRECTORY_FILE pMessage = (PNT_IPC_MESSAGE_QUERY_DIRECTORY_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    assert(messageType == pIn->tag);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    if (pMessage->Length)
    {
        pReply->Status = IO_ALLOCATE(&pReply->Buffer, VOID, pMessage->Length);
        GOTO_CLEANUP_ON_STATUS_EE(pReply->Status, EE);
    }

    pReply->Status = IoQueryDirectoryFile(
                            pMessage->FileHandle,
                            NULL,
                            &ioStatusBlock,
                            pReply->Buffer,
                            pMessage->Length,
                            pMessage->FileInformationClass,
                            pMessage->ReturnSingleEntry,
                            pMessage->FileSpec,
                            pMessage->RestartScan);
    pReply->Status = ioStatusBlock.Status;
    pReply->BytesTransferred = ioStatusBlock.BytesTransferred;

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcReadDirectoryChangeFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_READ_DIRECTORY_CHANGE_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_READ_DIRECTORY_CHANGE_FILE_RESULT;
    PNT_IPC_MESSAGE_READ_DIRECTORY_CHANGE_FILE pMessage = (PNT_IPC_MESSAGE_READ_DIRECTORY_CHANGE_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    assert(messageType == pIn->tag);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    if (pMessage->Length)
    {
        pReply->Status = IO_ALLOCATE(&pReply->Buffer, VOID, pMessage->Length);
        GOTO_CLEANUP_ON_STATUS_EE(pReply->Status, EE);
    }

    pReply->Status = IoReadDirectoryChangeFile(
                            pMessage->FileHandle,
                            NULL,
                            &ioStatusBlock,
                            pReply->Buffer,
                            pMessage->Length,
                            pMessage->WatchTree,
                            pMessage->NotifyFilter,
                            pMessage->MaxBufferSize);
    pReply->Status = ioStatusBlock.Status;
    pReply->BytesTransferred = ioStatusBlock.BytesTransferred;

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcQueryVolumeInformationFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_QUERY_VOLUME_INFORMATION_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_QUERY_VOLUME_INFORMATION_FILE_RESULT;
    PNT_IPC_MESSAGE_QUERY_VOLUME_INFORMATION_FILE pMessage = (PNT_IPC_MESSAGE_QUERY_VOLUME_INFORMATION_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    assert(messageType == pIn->tag);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    if (pMessage->Length)
    {
        pReply->Status = IO_ALLOCATE(&pReply->Buffer, VOID, pMessage->Length);
        GOTO_CLEANUP_ON_STATUS_EE(pReply->Status, EE);
    }

    pReply->Status = IoQueryVolumeInformationFile(
                            pMessage->FileHandle,
                            NULL,
                            &ioStatusBlock,
                            pReply->Buffer,
                            pMessage->Length,
                            pMessage->FsInformationClass);
    pReply->Status = ioStatusBlock.Status;
    pReply->BytesTransferred = ioStatusBlock.BytesTransferred;

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcUnlockFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_UNLOCK_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_UNLOCK_FILE_RESULT;
    PNT_IPC_MESSAGE_UNLOCK_FILE pMessage = (PNT_IPC_MESSAGE_UNLOCK_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    assert(messageType == pIn->tag);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    pReply->Status = IoUnlockFile(
                            pMessage->FileHandle,
                            NULL,
                            &ioStatusBlock,
                            pMessage->ByteOffset,
                            pMessage->Length,
                            pMessage->Key);
    pReply->Status = ioStatusBlock.Status;
    pReply->BytesTransferred = ioStatusBlock.BytesTransferred;

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcLockFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_LOCK_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_LOCK_FILE_RESULT;
    PNT_IPC_MESSAGE_LOCK_FILE pMessage = (PNT_IPC_MESSAGE_LOCK_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    assert(messageType == pIn->tag);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    pReply->Status = IoLockFile(
                            pMessage->FileHandle,
                            NULL,
                            &ioStatusBlock,
                            pMessage->ByteOffset,
                            pMessage->Length,
                            pMessage->Key,
                            pMessage->FailImmediately,
                            pMessage->ExclusiveLock);
    pReply->Status = ioStatusBlock.Status;
    pReply->BytesTransferred = ioStatusBlock.BytesTransferred;

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcQuerySecurityFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_QUERY_SECURITY_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_QUERY_SECURITY_FILE_RESULT;
    PNT_IPC_MESSAGE_QUERY_SECURITY_FILE pMessage = (PNT_IPC_MESSAGE_QUERY_SECURITY_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    assert(messageType == pIn->tag);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    if (pMessage->Length)
    {
        pReply->Status = IO_ALLOCATE(&pReply->Buffer, VOID, pMessage->Length);
        GOTO_CLEANUP_ON_STATUS_EE(pReply->Status, EE);
    }

    pReply->Status = IoQuerySecurityFile(
        pMessage->FileHandle,
        NULL,
        &ioStatusBlock,
        pMessage->SecurityInformation,
        (PSECURITY_DESCRIPTOR_RELATIVE) pReply->Buffer,
        pMessage->Length);
    pReply->Status = ioStatusBlock.Status;
    pReply->BytesTransferred = ioStatusBlock.BytesTransferred;

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcSetSecurityFile(
    IN LWMsgCall* pCall,
    IN const LWMsgParams* pIn,
    OUT LWMsgParams* pOut,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgTag messageType = NT_IPC_MESSAGE_TYPE_SET_SECURITY_FILE;
    const LWMsgTag replyType = NT_IPC_MESSAGE_TYPE_SET_SECURITY_FILE_RESULT;
    PNT_IPC_MESSAGE_SET_SECURITY_FILE pMessage = (PNT_IPC_MESSAGE_SET_SECURITY_FILE) pIn->data;
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    assert(messageType == pIn->tag);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pOut->tag = replyType;
    pOut->data = pReply;

    pReply->Status = IoSetSecurityFile(
                            pMessage->FileHandle,
                            NULL,
                            &ioStatusBlock,
                            pMessage->SecurityInformation,
                            pMessage->SecurityDescriptor,
                            pMessage->Length);
    pReply->Status = ioStatusBlock.Status;
    pReply->BytesTransferred = ioStatusBlock.BytesTransferred;

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

static
LWMsgDispatchSpec gIopIpcDispatchSpec[] =
{
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_CREATE_FILE,        IopIpcCreateFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_CLOSE_FILE,         IopIpcCloseFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_READ_FILE,          IopIpcReadFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_WRITE_FILE,         IopIpcWriteFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_DEVICE_IO_CONTROL_FILE, IopIpcDeviceIoControlFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_FS_CONTROL_FILE,  IopIpcFsControlFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_FLUSH_BUFFERS_FILE,     IopIpcFlushBuffersFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_QUERY_INFORMATION_FILE, IopIpcQueryInformationFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_SET_INFORMATION_FILE,   IopIpcSetInformationFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_QUERY_DIRECTORY_FILE,   IopIpcQueryDirectoryFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_READ_DIRECTORY_CHANGE_FILE,   IopIpcReadDirectoryChangeFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_QUERY_VOLUME_INFORMATION_FILE,   IopIpcQueryVolumeInformationFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_LOCK_FILE,              IopIpcLockFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_UNLOCK_FILE,            IopIpcUnlockFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_QUERY_SECURITY_FILE,    IopIpcQuerySecurityFile),
    LWMSG_DISPATCH_BLOCK(NT_IPC_MESSAGE_TYPE_SET_SECURITY_FILE,      IopIpcSetSecurityFile),
    LWMSG_DISPATCH_END
};

NTSTATUS
IoIpcAddProtocolSpec(
    IN OUT LWMsgProtocol* pProtocol
    )
{
    return NtIpcAddProtocolSpec(pProtocol);
}

NTSTATUS
IoIpcAddDispatch(
    IN OUT LWMsgServer* pServer
    )
{
    NTSTATUS status = 0;
    int EE = 0;

    status = NtIpcLWMsgStatusToNtStatus(lwmsg_server_add_dispatch_spec(
                    pServer,
                    gIopIpcDispatchSpec));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

