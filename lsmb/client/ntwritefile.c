
#include "includes.h"

NT_CLIENT_API
DWORD
NTWriteFile(
    HANDLE      hConnection,
    HANDLE      hFile,
    PVOID       pBuffer,
    DWORD       dwNumBytesToWrite,
    PDWORD      pdwNumBytesWritten,
    POVERLAPPED pOverlapped
    )
{
    DWORD  dwError = 0;
    PNT_SERVER_CONNECTION pConnection = (PNT_SERVER_CONNECTION)hConnection;
    PNT_API_HANDLE pAPIHandle = (PNT_API_HANDLE)hFile;
    NT_WRITE_FILE_REQUEST request = {0};
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PNT_WRITE_FILE_RESPONSE pNPResponse = NULL;

    BAIL_ON_INVALID_POINTER(pBuffer);
    BAIL_ON_INVALID_POINTER(pdwNumBytesWritten);
    BAIL_IF_NOT_FILE_HANDLE(hFile);

    request.dwBytesToWrite = dwNumBytesToWrite;
    request.pBuffer = (PBYTE)pBuffer;
    request.hFile = pAPIHandle->variant.hIPCHandle;

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                                   pConnection->pAssoc,
                                   NT_WRITE_FILE,
                                   &request,
                                   &replyType,
                                   &pResponse));
    BAIL_ON_NT_ERROR(dwError);

    switch (replyType)
    {
        case NT_WRITE_FILE_SUCCESS:

            break;

        case NT_WRITE_FILE_FAILED:

            BAIL_ON_INVALID_POINTER(pResponse);

            dwError = ((PNT_STATUS_REPLY)pResponse)->dwError;

            break;

        default:

            dwError = EINVAL;

            break;
    }
    BAIL_ON_NT_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(pResponse);

    pNPResponse = (PNT_WRITE_FILE_RESPONSE)pResponse;

    *pdwNumBytesWritten = pNPResponse->dwBytesWritten;

cleanup:

    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, replyType, pResponse);
    }

    return dwError;

error:

    if (pdwNumBytesWritten)
    {
        *pdwNumBytesWritten = 0;
    }

    goto cleanup;
}
