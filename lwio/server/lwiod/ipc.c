/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include "includes.h"
#include "ntipcmsg.h"
#include "goto.h"
#include "ntlogmacros.h"

static
LWMsgStatus
LwIoDaemonIpcRefreshConfiguration(
    IN LWMsgAssoc* pAssoc,
    IN const LWMsgMessage* pRequest,
    OUT LWMsgMessage* pResponse,
    IN void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PCSTR pszConfigPath = SMB_CONFIG_FILE_PATH;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = SMBSrvRefreshConfig(pszConfigPath);

    /* Transmit refresh error to client but do not fail out of dispatch loop */
    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_REFRESH_CONFIG_FAILED;
        pResponse->object = (PVOID) pStatusResponse;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_REFRESH_CONFIG_SUCCESS;
    pResponse->object = (PVOID) pStatusResponse;

cleanup:

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwIoDaemonIpcSetLogInfo(
    IN LWMsgAssoc* pAssoc,
    IN const LWMsgMessage* pRequest,
    OUT LWMsgMessage* pResponse,
    IN void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_STATUS_REPLY pStatusResponse = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LWIO_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(pRequest->object);

    dwError = SMBLogSetInfo_r((PLWIO_LOG_INFO)pRequest->object);

    /* Transmit failure to client but do not bail out of dispatch loop */
    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_SET_LOG_INFO_FAILED;
        pResponse->object = pStatusResponse;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_SET_LOG_INFO_SUCCESS;
    pResponse->object = pStatusResponse;

cleanup:

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
LwIoDaemonIpcGetLogInfo(
    IN LWMsgAssoc* pAssoc,
    IN const LWMsgMessage* pRequest,
    OUT LWMsgMessage* pResponse,
    IN void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PLWIO_LOG_INFO pLogInfo = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_LWIO_ERROR(dwError);

    dwError = SMBLogGetInfo_r(&pLogInfo);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_GET_LOG_INFO_FAILED;
        pResponse->object = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_GET_LOG_INFO_SUCCESS;
    pResponse->object = pLogInfo;

cleanup:

    LWIO_SAFE_FREE_MEMORY(pStatusResponse);

    return status;

error:

    goto cleanup;
}

static LWMsgDispatchSpec gLwIoDaemonIpcDispatchSpec[] =
{
    LWMSG_DISPATCH(SMB_REFRESH_CONFIG, LwIoDaemonIpcRefreshConfiguration),
    LWMSG_DISPATCH(SMB_SET_LOG_INFO,   LwIoDaemonIpcSetLogInfo),
    LWMSG_DISPATCH(SMB_GET_LOG_INFO,   LwIoDaemonIpcGetLogInfo),
    LWMSG_DISPATCH_END
};

NTSTATUS
LwIoDaemonIpcAddDispatch(
    IN OUT LWMsgServer* pServer
    )
{
    NTSTATUS status = 0;
    int EE = 0;

    status = NtIpcLWMsgStatusToNtStatus(lwmsg_server_add_dispatch_spec(
                    pServer,
                    gLwIoDaemonIpcDispatchSpec));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}
