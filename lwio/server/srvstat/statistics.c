/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        statistics.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Reference Statistics Logging Module (SRV)
 *
 *        Logging functions
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
LwioSrvStatCreateRequestContext(
    PSRV_STAT_CONNECTION_INFO  pConnection,        /* IN              */
    SRV_STAT_SMB_VERSION       protocolVersion,    /* IN              */
    ULONG                      ulRequestLength,    /* IN              */
    PHANDLE                    phContext           /*    OUT          */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = NULL;

    BAIL_ON_INVALID_POINTER(pConnection);
    BAIL_ON_INVALID_POINTER(phContext);

    ntStatus = RTL_ALLOCATE(
                    &pContext,
                    SRV_STAT_REQUEST_CONTEXT,
                    sizeof(SRV_STAT_REQUEST_CONTEXT));
    BAIL_ON_NT_STATUS(ntStatus);

    pContext->protocolVersion = protocolVersion;

    ntStatus = LwioSrvStatGetCurrentNTTime(&pContext->requestStartTime);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(&pContext->connInfo, pConnection, sizeof(*pConnection));

    *phContext = (HANDLE)pContext;

cleanup:

    return ntStatus;

error:

    if (phContext)
    {
        *phContext = NULL;
    }

    if (pContext)
    {
        LwioSrvStatCloseRequestContext(pContext);
    }

    goto cleanup;
}


NTSTATUS
LwioSrvStatPushMessage(
    HANDLE                    hContext,            /* IN              */
    ULONG                        ulOpcode,         /* IN              */
    PBYTE                        pMessage,         /* IN     OPTIONAL */
    ULONG                        ulMessageLen      /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;

    BAIL_ON_INVALID_POINTER(pContext);

error:

    return ntStatus;
}

NTSTATUS
LwioSrvStatSetSubOpcode(
    HANDLE                    hContext,            /* IN              */
    ULONG                     ulSubOpcode          /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;

    BAIL_ON_INVALID_POINTER(pContext);

error:

    return ntStatus;
}

NTSTATUS
LwioSrvStatSetIOCTL(
    HANDLE                    hContext,            /* IN              */
    ULONG                     ulIoCtlCode          /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;

    BAIL_ON_INVALID_POINTER(pContext);

error:

    return ntStatus;
}

NTSTATUS
LwioSrvStatSetSessionInfo(
    HANDLE                    hContext,            /* IN              */
    PSRV_STAT_SESSION_INFO    pSessionInfo         /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;

    BAIL_ON_INVALID_POINTER(pContext);

error:

    return ntStatus;
}

NTSTATUS
LwioSrvStatPopMessage(
    HANDLE                    hContext,            /* IN              */
    ULONG                     ulOpCode,            /* IN              */
    NTSTATUS                  msgStatus            /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;

    BAIL_ON_INVALID_POINTER(pContext);

error:

    return ntStatus;
}

NTSTATUS
LwioSrvStatSetResponseInfo(
    HANDLE                    hContext,            /* IN              */
    ULONG                     ulResponseLength     /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;

    BAIL_ON_INVALID_POINTER(pContext);

error:

    return ntStatus;
}

NTSTATUS
LwioSrvStatCloseRequestContext(
    HANDLE                    hContext            /* IN              */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_STAT_REQUEST_CONTEXT pContext = (PSRV_STAT_REQUEST_CONTEXT)hContext;

    BAIL_ON_INVALID_POINTER(pContext);

    ntStatus = LwioSrvStatGetCurrentNTTime(&pContext->requestEndTime);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pContext)
    {
        RtlMemoryFree(pContext);
    }

    return ntStatus;

error:

    goto cleanup;
}
