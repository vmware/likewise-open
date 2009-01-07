#include "includes.h"

DWORD
SMBSetNamedPipeHandleState(
    HANDLE      hConnection,
    HANDLE      hPipe,
    PDWORD      pdwMode,
    PDWORD      pdwMaxCollectionCount,
    PDWORD      pdwMaxTimeout
    )
{
    DWORD  dwError = 0;
    PSMB_SERVER_CONNECTION pConnection = (PSMB_SERVER_CONNECTION)hConnection;
    PSMB_API_HANDLE pAPIHandle = (PSMB_API_HANDLE)hPipe;
    SMB_SETNAMEDPIPEHANDLESTATE_REQUEST request = {0};
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;

    BAIL_IF_NOT_FILE_HANDLE(hPipe);

    request.hPipe = pAPIHandle->variant.hIPCHandle;
    request.pdwMode = pdwMode;
    request.pdwCollectionCount = pdwMaxCollectionCount;
    request.pdwTimeout = pdwMaxTimeout;

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                                   pConnection->pAssoc,
                                   SMB_SET_NAMED_PIPE_HANDLE_STATE,
                                   &request,
                                   &replyType,
                                   &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
        case SMB_SET_NAMED_PIPE_HANDLE_STATE_SUCCESS:

            break;

        case SMB_SET_NAMED_PIPE_HANDLE_STATE_FAILED:

            dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;

            break;

        default:

            dwError = EINVAL;

            break;
    }
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, replyType, pResponse);
    }

    return dwError;

error:

    goto cleanup;
}
