#include "includes.h"

DWORD
SMBOpenServer(
    PHANDLE phConnection
    )
{
    DWORD dwError = 0;
    PSMB_SERVER_CONNECTION pConnection = NULL;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_INVALID_POINTER(gpSMBProtocol);

    dwError = SMBAllocateMemory(
                    sizeof(SMB_SERVER_CONNECTION),
                    (PVOID*)&pConnection);
    BAIL_ON_SMB_ERROR(dwError);

    status = lwmsg_connection_new(
                    gpSMBProtocol,
                    &pConnection->pAssoc);
    BAIL_ON_SMB_ERROR(status);

    status = lwmsg_connection_set_endpoint(
                    pConnection->pAssoc,
                    LWMSG_CONNECTION_MODE_LOCAL,
                    SMB_SERVER_FILENAME);
    BAIL_ON_SMB_ERROR(status);

    status = lwmsg_connection_establish(pConnection->pAssoc);
    BAIL_ON_SMB_ERROR(status);

    *phConnection = (HANDLE)pConnection;

cleanup:

    return dwError;

error:

    if (pConnection)
    {
        SMBCloseServer((HANDLE)pConnection);
    }

    *phConnection = (HANDLE)NULL;

    goto cleanup;
}

DWORD
SMBCloseServer(
    HANDLE hConnection
    )
{
    DWORD dwError = 0;
    PSMB_SERVER_CONNECTION pConnection = NULL;

    pConnection = (PSMB_SERVER_CONNECTION)hConnection;

    if (pConnection)
    {
        if (pConnection->pAssoc)
        {
            LWMsgStatus status = lwmsg_assoc_close(pConnection->pAssoc);
            if (status)
            {
                SMB_LOG_ERROR("Failed to close association [Error code:%d]", status);
            }

            lwmsg_assoc_delete(pConnection->pAssoc);
        }

        SMBFreeMemory(pConnection);
    }

    return dwError;
}
