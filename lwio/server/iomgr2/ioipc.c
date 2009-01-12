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

static
VOID
IopIpcCleanupFileHandle(
    PVOID pHandle
    )
{
    IO_FILE_HANDLE fileHandle = (IO_FILE_HANDLE) pHandle;
    assert(fileHandle);
    if (fileHandle)
    {
        NTSTATUS status = IoCloseFile(fileHandle);
        if (status)
        {
            SMB_LOG_ERROR("failed to cleanup handle (status = 0x%08x)", status);
            assert(FALSE);
        }
    }
}

static
NTSTATUS
IopIpcRegisterFileHandle(
    IN LWMsgAssoc* pAssoc,
    IN IO_FILE_HANDLE FileHandle
    )
{
    NTSTATUS status = 0;
    status = NtIpcLWMsgStatusToNtStatus(lwmsg_assoc_register_handle(
                    pAssoc,
                    "IO_FILE_HANDLE",
                    FileHandle,
                    IopIpcCleanupFileHandle));
    assert(!status);
    return status;
}

static
LWMsgStatus
IopIpcCreateFile(
    IN LWMsgAssoc* pAssoc,
    IN const LWMsgMessage* pRequest,
    OUT LWMsgMessage* pResponse,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PNT_IPC_MESSAGE_CREATE_FILE pMessage = (PNT_IPC_MESSAGE_CREATE_FILE) pRequest->object;
    PNT_IPC_MESSAGE_CREATE_FILE_RESULT pReply = NULL;
    const LWMsgMessageTag messageType = NT_IPC_MESSAGE_TYPE_CREATE_FILE;
    const LWMsgMessageTag replyType = NT_IPC_MESSAGE_TYPE_CREATE_FILE_RESULT;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IO_FILE_HANDLE fileHandle = NULL;
    IO_FILE_NAME fileName = { 0 };

    assert(messageType == pRequest->tag);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_CREATE_FILE_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // TODO -- SECURITY TOKEN!!!

#ifdef _NT_IPC_USE_PSEUDO_TYPES
    NtIpcRealFromPseudoIoFileName(&fileName, &pMessage->FileName);
#else
    fileName = pMessage->FileName;
#endif

    // TODO -- EAs, SDs, etc.
    pReply->Status = IoCreateFile(
                            &fileHandle,
                            NULL,
                            &ioStatusBlock,
                            &fileName,
                            pMessage->DesiredAccess,
                            pMessage->AllocationSize,
                            pMessage->FileAttributes,
                            pMessage->ShareAccess,
                            pMessage->CreateDisposition,
                            pMessage->CreateOptions,
                            NULL,
                            NULL,
                            NULL);
#ifdef _NT_IPC_USE_PSEUDO_TYPES
    NtIpcRealToPseudoIoFileHandle(fileHandle, &pReply->FileHandle);
#else
    pReply->FileHandle = fileHandle;
#endif
    pReply->Status = ioStatusBlock.Status;
    pReply->CreateResult = ioStatusBlock.CreateResult;

    // Register handle
    if (fileHandle)
    {
        status = IopIpcRegisterFileHandle(pAssoc, fileHandle);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    pResponse->tag = replyType;
    pResponse->object = pReply;

cleanup:
    if (status)
    {
        if (fileHandle)
        {
            assert(FALSE);
            IoCloseFile(fileHandle);
        }
        IO_FREE(&pReply);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcCloseFile(
    IN LWMsgAssoc* pAssoc,
    IN const LWMsgMessage* pRequest,
    OUT LWMsgMessage* pResponse,
    IN void* pData
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PNT_IPC_MESSAGE_GENERIC_FILE pMessage = (PNT_IPC_MESSAGE_GENERIC_FILE) pRequest->object;
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pReply = NULL;
    const LWMsgMessageTag messageType = NT_IPC_MESSAGE_TYPE_CLOSE_FILE;
    const LWMsgMessageTag replyType = NT_IPC_MESSAGE_TYPE_CLOSE_FILE_RESULT;

    assert(messageType == pRequest->tag);

    status = IO_ALLOCATE(&pReply, NT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT, sizeof(*pReply));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pReply->Status = IoCloseFile(pMessage->FileHandle);
    if (!pReply->Status)
    {
        NtIpcUnregisterFileHandle(pAssoc, pMessage->FileHandle);
    }
    pResponse->tag = replyType;
    pResponse->object = pReply;

cleanup:
    if (status)
    {
        IO_FREE(&pReply);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return NtIpcNtStatusToLWMsgStatus(status);
}

LWMsgStatus
IopIpcReadFile(
    IN LWMsgAssoc* pAssoc,
    IN const LWMsgMessage* pRequest,
    OUT LWMsgMessage* pResponse,
    IN void* pData
    )
{
    // If someone calls this, the server aborts the connection.
    return LWMSG_STATUS_UNIMPLEMENTED;
}

LWMsgStatus
IopIpcWriteFile(
    IN LWMsgAssoc* pAssoc,
    IN const LWMsgMessage* pRequest,
    OUT LWMsgMessage* pResponse,
    IN void* pData
    )
{
    // If someone calls this, the server aborts the connection.
    return LWMSG_STATUS_UNIMPLEMENTED;
}

LWMsgStatus
IopIpcDeviceIoControlFile(
    IN LWMsgAssoc* pAssoc,
    IN const LWMsgMessage* pRequest,
    OUT LWMsgMessage* pResponse,
    IN void* pData
    )
{
    // If someone calls this, the server aborts the connection.
    return LWMSG_STATUS_UNIMPLEMENTED;
}

LWMsgStatus
IopIpcFsControlFile(
    IN LWMsgAssoc* pAssoc,
    IN const LWMsgMessage* pRequest,
    OUT LWMsgMessage* pResponse,
    IN void* pData
    )
{
    // If someone calls this, the server aborts the connection.
    return LWMSG_STATUS_UNIMPLEMENTED;
}

LWMsgStatus
IopIpcFlushBuffersFile(
    IN LWMsgAssoc* pAssoc,
    IN const LWMsgMessage* pRequest,
    OUT LWMsgMessage* pResponse,
    IN void* pData
    )
{
    // If someone calls this, the server aborts the connection.
    return LWMSG_STATUS_UNIMPLEMENTED;
}

LWMsgStatus
IopIpcQueryInformationFile(
    IN LWMsgAssoc* pAssoc,
    IN const LWMsgMessage* pRequest,
    OUT LWMsgMessage* pResponse,
    IN void* pData
    )
{
    // If someone calls this, the server aborts the connection.
    return LWMSG_STATUS_UNIMPLEMENTED;
}

LWMsgStatus
IopIpcSetInformationFile(
    IN LWMsgAssoc* pAssoc,
    IN const LWMsgMessage* pRequest,
    OUT LWMsgMessage* pResponse,
    IN void* pData
    )
{
    // If someone calls this, the server aborts the connection.
    return LWMSG_STATUS_UNIMPLEMENTED;
}

static
LWMsgDispatchSpec gIopIpcDispatchSpec[] =
{
    LWMSG_DISPATCH(NT_IPC_MESSAGE_TYPE_CREATE_FILE,        IopIpcCreateFile),
    LWMSG_DISPATCH(NT_IPC_MESSAGE_TYPE_CLOSE_FILE,         IopIpcCloseFile),
    LWMSG_DISPATCH(NT_IPC_MESSAGE_TYPE_READ_FILE,          IopIpcReadFile),
    LWMSG_DISPATCH(NT_IPC_MESSAGE_TYPE_WRITE_FILE,         IopIpcWriteFile),
    LWMSG_DISPATCH(NT_IPC_MESSAGE_TYPE_DEVICE_IO_CONTROL_FILE, IopIpcDeviceIoControlFile),
    LWMSG_DISPATCH(NT_IPC_MESSAGE_TYPE_FS_CONTROL_FILE,        IopIpcFsControlFile),
    LWMSG_DISPATCH(NT_IPC_MESSAGE_TYPE_FLUSH_BUFFERS_FILE,     IopIpcFlushBuffersFile),
    LWMSG_DISPATCH(NT_IPC_MESSAGE_TYPE_QUERY_INFORMATION_FILE, IopIpcQueryInformationFile),
    LWMSG_DISPATCH(NT_IPC_MESSAGE_TYPE_SET_INFORMATION_FILE,   IopIpcSetInformationFile),
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

