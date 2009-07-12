#include "includes.h"

NTSTATUS
SrvContextCreate(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pRequest,
    PLWIO_SRV_CONTEXT*  ppContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONTEXT pContext = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(LWIO_SRV_CONTEXT),
                    (PVOID*)&pContext);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pConnection->protocolVer)
    {
        case SMB_PROTOCOL_VERSION_1:

            // Update the sequence whether we end up signing or not
            ntStatus = SrvConnectionGetNextSequence(
                            pConnection,
                            pRequest,
                            &pRequest->sequence);
            BAIL_ON_NT_STATUS(ntStatus);

            break;

        default:

            break;
    }

    if (pConnection->serverProperties.bRequireSecuritySignatures &&
        pConnection->pSessionKey)
    {
        switch (pConnection->protocolVer)
        {
            case SMB_PROTOCOL_VERSION_1:

                ntStatus = SMBPacketVerifySignature(
                                pRequest,
                                pRequest->sequence,
                                pConnection->pSessionKey,
                                pConnection->ulSessionKeyLength);

                break;

            case SMB_PROTOCOL_VERSION_2:

#if 0
                ntStatus = SMB2PacketVerifySignature(
                                pRequest,
                                pConnection->pSessionKey,
                                pConnection->ulSessionKeyLength);
#endif

                break;

            default:

                ntStatus = STATUS_INTERNAL_ERROR;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pContext->pConnection = pConnection;

    InterlockedIncrement(&pConnection->refCount);

    pContext->pRequest = pRequest;

    *ppContext = pContext;

cleanup:

    return ntStatus;

error:

    *ppContext = NULL;

    goto cleanup;
}

VOID
SrvContextFree(
    PVOID pContext
    )
{
    PLWIO_SRV_CONTEXT pIOContext = (PLWIO_SRV_CONTEXT)pContext;

    if (pIOContext->pConnection)
    {
        if (pIOContext->pRequest)
        {
            SMBPacketFree(
                    pIOContext->pConnection->hPacketAllocator,
                    pIOContext->pRequest);
        }

        SrvConnectionRelease(pIOContext->pConnection);
    }

    LwRtlMemoryFree(pIOContext);
}

