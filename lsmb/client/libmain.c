#include "includes.h"

DWORD
SMBInitialize(
    VOID
    )
{
    DWORD dwError = 0;
    LWMsgProtocolSpec* pProtocolSpec = NULL;
    LWMsgProtocol* pProtocol = NULL;

    dwError = SMBIPCGetProtocolSpec(
                    &pProtocolSpec);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_protocol_new(NULL, &pProtocol));
    BAIL_ON_SMB_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_protocol_add_protocol_spec(
                    pProtocol,
                    pProtocolSpec));
    BAIL_ON_SMB_ERROR(dwError);

    gpSMBProtocol = pProtocol;

cleanup:

    return dwError;

error:

    gpSMBProtocol = NULL;

    if (pProtocol)
    {
        lwmsg_protocol_delete(pProtocol);
    }

    goto cleanup;
}

DWORD
SMBShutdown(
    VOID
    )
{
    if (gpSMBProtocol)
    {
        lwmsg_protocol_delete(gpSMBProtocol);
        gpSMBProtocol = NULL;
    }

    return 0;
}
