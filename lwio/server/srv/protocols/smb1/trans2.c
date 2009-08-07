#include "includes.h"

NTSTATUS
SrvProcessTransaction2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1    pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                       iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1         pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
    ULONG ulOffset         = pSmbRequest->usHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
    PTRANSACTION_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PUSHORT                     pBytecount = NULL;     // Do not free
    PUSHORT                     pSetup = NULL;         // Do not free
    PBYTE                       pParameters = NULL;    // Do not free
    PBYTE                       pData = NULL;          // Do not free

    ntStatus = WireUnmarshallTransactionRequest(
                    pBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    &pRequestHeader,
                    &pSetup,
                    &pBytecount,
                    NULL, /* No transaction name */
                    &pParameters,
                    &pData);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pSetup == NULL)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    switch (*pSetup)
    {
        case SMB_SUB_COMMAND_TRANS2_FIND_FIRST2 :

            ntStatus = SrvProcessTrans2FindFirst2(
                          pExecContext,
                          pRequestHeader,
                          pSetup,
                          pBytecount,
                          pParameters,
                          pData);

            break;

        case SMB_SUB_COMMAND_TRANS2_FIND_NEXT2 :

            ntStatus = SrvProcessTrans2FindNext2(
                          pExecContext,
                          pRequestHeader,
                          pSetup,
                          pBytecount,
                          pParameters,
                          pData);

            break;

        case SMB_SUB_COMMAND_TRANS2_QUERY_FS_INFORMATION :

            ntStatus = SrvProcessTrans2QueryFilesystemInformation(
                          pExecContext,
                          pRequestHeader,
                          pSetup,
                          pBytecount,
                          pParameters,
                          pData);

            break;

        case SMB_SUB_COMMAND_TRANS2_QUERY_PATH_INFORMATION :

            ntStatus = SrvProcessTrans2QueryPathInformation(
                          pExecContext,
                          pRequestHeader,
                          pSetup,
                          pBytecount,
                          pParameters,
                          pData);

            break;

        case SMB_SUB_COMMAND_TRANS2_QUERY_FILE_INFORMATION :

            ntStatus = SrvProcessTrans2QueryFileInformation(
                          pExecContext,
                          pRequestHeader,
                          pSetup,
                          pBytecount,
                          pParameters,
                          pData);

            break;

        case SMB_SUB_COMMAND_TRANS2_SET_FILE_INFORMATION :

            ntStatus = SrvProcessTrans2SetFileInformation(
                            pExecContext,
                            pRequestHeader,
                            pSetup,
                            pBytecount,
                            pParameters,
                            pData);

            break;

        case SMB_SUB_COMMAND_TRANS2_OPEN2 :
        case SMB_SUB_COMMAND_TRANS2_FSCTL :
        case SMB_SUB_COMMAND_TRANS2_IOCTL2 :
        case SMB_SUB_COMMAND_TRANS2_FIND_NOTIFY_FIRST :
        case SMB_SUB_COMMAND_TRANS2_FIND_NOTIFY_NEXT :
        case SMB_SUB_COMMAND_TRANS2_CREATE_DIRECTORY :
        case SMB_SUB_COMMAND_TRANS2_SESSION_SETUP :
        case SMB_SUB_COMMAND_TRANS2_GET_DFS_REFERRAL :
        case SMB_SUB_COMMAND_TRANS2_REPORT_DFS_INCONSISTENCY :
        case SMB_SUB_COMMAND_TRANS2_SET_PATH_INFORMATION :

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        default:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

