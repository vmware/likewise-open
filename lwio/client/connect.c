/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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

#include "includes.h"

NTSTATUS
LwIoOpenContext(
    PIO_CONTEXT* ppContext
    )
{
    PIO_CONTEXT pContext = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    static LWMsgTime connectTimeout = {2, 0};

    Status = LwIoAllocateMemory(
        sizeof(*pContext),
        OUT_PPVOID(&pContext));
    BAIL_ON_NT_STATUS(Status);

    Status = NtIpcLWMsgStatusToNtStatus(
        lwmsg_connection_new(
            gpLwIoProtocol,
            &pContext->pAssoc));
    BAIL_ON_NT_STATUS(Status);
    
    Status = NtIpcLWMsgStatusToNtStatus(
        lwmsg_connection_set_endpoint(
            pContext->pAssoc,
            LWMSG_CONNECTION_MODE_LOCAL,
            LWIO_SERVER_FILENAME));
    BAIL_ON_NT_STATUS(Status);

    if (getenv("LW_DISABLE_CONNECT_TIMEOUT") == NULL)
    {
        Status = NtIpcLWMsgStatusToNtStatus(
            lwmsg_assoc_set_timeout(
                pContext->pAssoc,
                LWMSG_TIMEOUT_ESTABLISH,
                &connectTimeout));
        BAIL_ON_NT_STATUS(Status);
    }

    Status = NtIpcLWMsgStatusToNtStatus(
        lwmsg_assoc_establish(pContext->pAssoc));
    BAIL_ON_NT_STATUS(Status);

    *ppContext = pContext;

cleanup:

    return Status;

error:

    if (pContext)
    {
        LwIoCloseContext(pContext);
    }

    *ppContext = NULL;

    goto cleanup;
}

NTSTATUS
LwIoCloseContext(
    PIO_CONTEXT pContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (pContext)
    {
        if (pContext->pAssoc)
        {
            LWMsgStatus status = lwmsg_assoc_close(pContext->pAssoc);
            if (status)
            {
                LWIO_LOG_ERROR("Failed to close association [Error code:%d]", status);
            }

            lwmsg_assoc_delete(pContext->pAssoc);
        }

        LwIoFreeMemory(pContext);
    }

    return Status;
}
