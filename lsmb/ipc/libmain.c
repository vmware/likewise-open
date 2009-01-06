#include "includes.h"

DWORD
SMBIPCGetProtocolSpec(
    LWMsgProtocolSpec** ppProtocolSpec
    )
{
    *ppProtocolSpec = &gLSMBProtocolSpec[0];

    return 0;
}

DWORD
SMBIPCMapLWMsgStatus(
    LWMsgStatus status
    )
{
    DWORD dwError = 0;

    switch (status)
    {
        case LWMSG_STATUS_SUCCESS:

                dwError = SMB_ERROR_SUCCESS;
                break;

        case LWMSG_STATUS_ERROR:

                dwError = SMB_ERROR_LWMSG_ERROR;
                break;

        case LWMSG_STATUS_AGAIN:

                dwError = EAGAIN;
                break;

        case LWMSG_STATUS_MEMORY:

                dwError = SMB_ERROR_OUT_OF_MEMORY;
                break;

        case LWMSG_STATUS_MALFORMED:

                dwError = SMB_ERROR_MALFORMED_REQUEST;
                break;

        case LWMSG_STATUS_EOF:

                dwError = SMB_ERROR_LWMSG_EOF;
                break;

        case LWMSG_STATUS_NOT_FOUND:

                dwError = SMB_ERROR_NO_SUCH_ITEM;
                break;

        case LWMSG_STATUS_UNIMPLEMENTED:

                dwError = SMB_ERROR_NOT_IMPLEMENTED;
                break;

        case LWMSG_STATUS_INVALID_PARAMETER:

                dwError = SMB_ERROR_INVALID_PARAMETER;
                break;

        case LWMSG_STATUS_OVERFLOW:

                dwError = SMB_ERROR_OVERFLOW;
                break;

        case LWMSG_STATUS_UNDERFLOW:

                dwError = SMB_ERROR_UNDERFLOW;
                break;

        case LWMSG_STATUS_SYSTEM:

                dwError = SMB_ERROR_SYSTEM;
                break;

        case LWMSG_STATUS_TIMEOUT:

                dwError = ETIMEDOUT;
                break;

        case LWMSG_STATUS_SECURITY:

                dwError = EACCES;
                break;

        case LWMSG_STATUS_INTERRUPT:

                dwError = EINTR;
                break;

        default:

                SMB_LOG_ERROR("Failed to map lwmsg error [%d]", status);

                dwError = SMB_ERROR_LWMSG_ERROR;

                break;
    }

    return dwError;
}
