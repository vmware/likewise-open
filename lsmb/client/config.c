#include "includes.h"

DWORD
SMBRefreshConfiguration(
    HANDLE hConnection
    )
{
    DWORD dwError = 0;
    SMB_REQUEST request = {0};
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PSMB_SERVER_CONNECTION pConnection = NULL;

    pConnection = (PSMB_SERVER_CONNECTION)hConnection;

    request.dwCurTime = time(NULL);

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                    pConnection->pAssoc,
                    SMB_REFRESH_CONFIG,
                    &request,
                    &replyType,
                    &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
        case SMB_REFRESH_CONFIG_SUCCESS:

            SMB_LOG_INFO("Configuration refresh succeeded");

            break;

        case SMB_REFRESH_CONFIG_FAILED:

            BAIL_ON_INVALID_POINTER(pResponse);

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
