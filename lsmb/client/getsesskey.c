#include "includes.h"

SMB_API
DWORD
SMBGetSessionKey(
    HANDLE hConnection,
    HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    )
{
    DWORD  dwError = 0;
    PSMB_API_HANDLE pAPIHandle = (PSMB_API_HANDLE)hFile;
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PSMB_SERVER_CONNECTION pConnection = (PSMB_SERVER_CONNECTION) hConnection;
    PSMB_GET_SESSION_KEY_RESPONSE pSessResponse = NULL;

    BAIL_IF_NOT_FILE_HANDLE(hFile);

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                                   pConnection->pAssoc,
                                   SMB_GET_SESSION_KEY,
                                   pAPIHandle->variant.hIPCHandle,
                                   &replyType,
                                   &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
    case SMB_GET_SESSION_KEY_SUCCESS:
        break;
        
    case SMB_GET_SESSION_KEY_FAILED:
        dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;
        break;

    default:
        dwError = EINVAL;
        break;
    }
    BAIL_ON_SMB_ERROR(dwError);

    pSessResponse = pResponse;

    *pdwSessionKeyLength = pSessResponse->dwSessionKeyLength;
    *ppSessionKey = pSessResponse->pSessionKey;

    /* Unset pointer to session key in message so it does not get freed */
    pSessResponse->pSessionKey = NULL;

cleanup:

    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, replyType, pResponse);
    }

    return dwError;

error:

    goto cleanup;
}

SMB_API
VOID
SMBFreeSessionKey(
    PBYTE pSessionKey
    )
{
    SMBFreeMemory(pSessionKey);
}
