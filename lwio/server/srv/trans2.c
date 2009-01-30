#include "includes.h"

NTSTATUS
SrvProcessTrans2(
    PLWIO_SRV_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET pSmbRequest = pContext->pRequest;
    PTRANSACTION_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PUSHORT pBytecount = NULL; // Do not free
    PUSHORT pSetup = NULL; // Do not free
    PBYTE   pParameters = NULL; // Do not free
    PBYTE   pData = NULL; // Do not free
    PSMB_PACKET pSmbResponse = NULL;

    ntStatus = WireUnmarshallTransactionRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->bufferLen - pSmbRequest->bufferUsed,
                    (PBYTE)pSmbRequest->pParams - (PBYTE)pSmbRequest->pSMBHeader,
                    &pRequestHeader,
                    &pSetup,
                    &pBytecount,
                    &pParameters,
                    &pData);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    return ntStatus;

error:

    goto cleanup;
}

