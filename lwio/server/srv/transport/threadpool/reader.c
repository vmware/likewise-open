#include "includes.h"

typedef struct _SMB_SOCKET_READER_WORK_SET
{
    fd_set          fdset;
    int             maxFd;
    PSMBDLINKEDLIST pConnectionList;
} SMB_SOCKET_READER_WORK_SET, *PSMB_SOCKET_READER_WORK_SET;

static
NTSTATUS
SrvVerifyContext(
    PSRV_EXEC_CONTEXT pContext
    );

static
NTSTATUS
SrvSocketReaderReadMessage(
    PLWIO_SRV_CONNECTION            pConnection
    );

static
NTSTATUS
SrvSocketReaderProcessInit(
    PLW_TASK pTask,
    PLWIO_SRV_CONNECTION pConnection
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_SOCKET pSocket = (PLWIO_SRV_SOCKET) pConnection->hSocket;

    /* Register fd with thread pool */
    ntStatus = LwRtlSetTaskFd(
        pTask,
        pSocket->fd,
        LW_TASK_EVENT_FD_READABLE | LW_TASK_EVENT_FD_WRITABLE);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

VOID
SrvSocketReaderProcess(
    PLW_TASK pTask,
    PVOID pDataContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64 pllTime
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION pConnection = (PLWIO_SRV_CONNECTION) pDataContext;
    PLWIO_SRV_SOCKET pSocket = (PLWIO_SRV_SOCKET) pConnection->hSocket;
    CHAR szIpAddr[256];

    if (WakeMask & LW_TASK_EVENT_INIT)
    {
        ntStatus = SrvSocketReaderProcessInit(pTask, pConnection);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (WakeMask & LW_TASK_EVENT_CANCEL || SrvConnectionIsInvalid(pConnection))
    {
        SrvConnectionSetInvalid(pConnection);
        SrvConnectionRelease(pConnection);
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
        goto cleanup;
    }

    ntStatus = SrvSocketReaderReadMessage(
        pConnection);

    switch (ntStatus)
    {
    case STATUS_SUCCESS:
        *pWaitMask = LW_TASK_EVENT_YIELD;
        goto cleanup;
    case STATUS_PENDING:
        *pWaitMask = LW_TASK_EVENT_FD_READABLE;
        goto cleanup;
    case STATUS_CONNECTION_RESET:
        LWIO_LOG_DEBUG("Connection reset by peer [fd:%d][%s]",
                       pSocket->fd,
                       SMB_SAFE_LOG_STRING(inet_ntop(
                                               AF_INET,
                                               &pSocket->cliaddr.sin_addr,
                                               szIpAddr,
                                               sizeof(szIpAddr))));
        // Intentional fall through so the connections gets
        // marked as invalid
    default:
        SrvConnectionSetInvalid(pConnection);
        SrvConnectionRelease(pConnection);
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
        goto cleanup;
    }

cleanup:

    return;

error:

    goto cleanup;
}

static
NTSTATUS
SrvSocketReaderReadMessage(
    PLWIO_SRV_CONNECTION            pConnection
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_EXEC_CONTEXT pContext = NULL;
    PSMB_PACKET pPacket = NULL;

    ntStatus = SrvConnectionReadPacket(pConnection, &pPacket);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pPacket)
    {
        ntStatus = SrvBuildExecContext(pConnection, pPacket, FALSE, &pContext);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvVerifyContext(pContext);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvProdConsEnqueue(gSrvThreadpoolTransport.pWorkQueue, pContext);
        BAIL_ON_NT_STATUS(ntStatus);

        pContext = NULL;
    }

cleanup:

    if (pPacket)
    {
        SMBPacketRelease(pConnection->hPacketAllocator, pPacket);
    }

    return ntStatus;

error:

    if (pContext)
    {
        SrvReleaseExecContext(pContext);
    }

    goto cleanup;
}

static
NTSTATUS
SrvVerifyContext(
    PSRV_EXEC_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION pConnection = pContext->pConnection;

    switch (SrvConnectionGetProtocolVersion(pConnection))
    {
        case SMB_PROTOCOL_VERSION_1:

            // Update the sequence whether we end up signing or not
            ntStatus = SrvConnectionGetNextSequence(
                            pConnection,
                            pContext->pSmbRequest,
                            &pContext->pSmbRequest->sequence);

            break;

        default:

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    if (pConnection->serverProperties.bRequireSecuritySignatures &&
        pConnection->pSessionKey)
    {
        switch (SrvConnectionGetProtocolVersion(pConnection))
        {
            case SMB_PROTOCOL_VERSION_1:

                ntStatus = SMBPacketVerifySignature(
                                pContext->pSmbRequest,
                                pContext->pSmbRequest->sequence,
                                pConnection->pSessionKey,
                                pConnection->ulSessionKeyLength);

                break;

            case SMB_PROTOCOL_VERSION_2:

                // Allow only the Cancel request to be not signed
                if (!SMB2PacketIsSigned(pContext->pSmbRequest) &&
                    (pContext->pSmbRequest->pSMB2Header->command != COM2_CANCEL))
                {
                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                ntStatus = SMB2PacketVerifySignature(
                                    pContext->pSmbRequest,
                                    pConnection->pSessionKey,
                                    pConnection->ulSessionKeyLength);

                break;

            default:

                ntStatus = STATUS_INTERNAL_ERROR;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}
