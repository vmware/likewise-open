/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        logging.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Logging
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "includes.h"

SMB_API
NTSTATUS
SMBSetLogLevel(
    HANDLE      hSMBConnection,
    SMBLogLevel logLevel
    )
{
    LWIO_LOG_INFO logInfo = {0};

    logInfo.maxAllowedLogLevel = logLevel;

    return SMBSetLogInfo(
                    hSMBConnection,
                    &logInfo);
}

SMB_API
NTSTATUS
SMBSetLogInfo(
    HANDLE hSMBConnection,
    PLWIO_LOG_INFO pLogInfo
    )
{
    NTSTATUS status = 0;
    PIO_CONTEXT pConnection = NULL;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    pConnection = (PIO_CONTEXT)hSMBConnection;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    in.tag = SMB_SET_LOG_INFO;
    in.data = NULL;

    status = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));

    switch (out.tag)
    {
        case SMB_SET_LOG_INFO_SUCCESS:
            break;

        case SMB_SET_LOG_INFO_FAILED:
            status = ((PSMB_STATUS_REPLY) out.data)->dwError;
            break;

        default:
            status = EINVAL;
            break;
    }
    BAIL_ON_LWIO_ERROR(status);

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:

    goto cleanup;
}

SMB_API
NTSTATUS
SMBGetLogInfo(
    HANDLE         hSMBConnection,
    PLWIO_LOG_INFO* ppLogInfo
    )
{
    NTSTATUS status = 0;
    SMB_REQUEST request = {0};
    PIO_CONTEXT pConnection = NULL;
    LWMsgCall* pCall = NULL;
    LWMsgParams in = LWMSG_PARAMS_INITIALIZER;
    LWMsgParams out = LWMSG_PARAMS_INITIALIZER;

    pConnection = (PIO_CONTEXT)hSMBConnection;

    status = LwIoContextAcquireCall(pConnection, &pCall);
    BAIL_ON_NT_STATUS(status);

    /* FIXME: what is this for? */
    request.dwCurTime = time(NULL);
    in.tag = SMB_GET_LOG_INFO;
    in.data = &request;

    status = MAP_LWMSG_STATUS(lwmsg_call_dispatch(pCall, &in, &out, NULL, NULL));
    BAIL_ON_NT_STATUS(status);

    switch (out.tag)
    {
        case SMB_GET_LOG_INFO_SUCCESS:
            *ppLogInfo = out.data;
            out.data = NULL;
            break;

        case SMB_GET_LOG_INFO_FAILED:
            status = ((PSMB_STATUS_REPLY) out.data)->dwError;
            break;

        default:
            status = STATUS_INTERNAL_ERROR;
            break;
    }
    BAIL_ON_LWIO_ERROR(status);

cleanup:

    if (pCall)
    {
        lwmsg_call_destroy_params(pCall, &out);
        lwmsg_call_release(pCall);
    }

    return status;

error:

    goto cleanup;
}
