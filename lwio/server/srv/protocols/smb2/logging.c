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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        logging.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Logging
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

VOID
SrvLogRequest_SMB_V2(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT pExecContext = NULL;
    PSTR  pszBuffer = NULL;
    ULONG ulLen = 0;
    va_list msgList;

    va_start(msgList, ulLine);

    pExecContext = va_arg(msgList, PSRV_EXEC_CONTEXT);

    if (pExecContext)
    {
        ntStatus = SrvGetHexDump(
                        (PBYTE)pExecContext->pSmbRequest->pSMB2Header,
                        pExecContext->pSmbRequest->bufferUsed - sizeof(NETBIOS_HEADER),
                        SrvLogContextGetMaxLogLength(pLogContext),
                        &pszBuffer,
                        &ulLen);
        BAIL_ON_NT_STATUS(ntStatus);

        if (logLevel >= LWIO_LOG_LEVEL_DEBUG)
        {
            LWIO_LOG_ALWAYS_CUSTOM(
                    logLevel,
                    "[%s() %s:%u] SMB Request:[%u/%u bytes][%s]",
                    LWIO_SAFE_LOG_STRING(pszFunction),
                    LWIO_SAFE_LOG_STRING(pszFile),
                    ulLine,
                    ulLen,
                    pExecContext->pSmbRequest->bufferUsed - sizeof(NETBIOS_HEADER),
                    LWIO_SAFE_LOG_STRING(pszBuffer));
        }
        else
        {
            LWIO_LOG_ALWAYS_CUSTOM(
                    logLevel,
                    "SMB Request:[%u/%u bytes][%s]",
                    ulLen,
                    pExecContext->pSmbRequest->bufferUsed - sizeof(NETBIOS_HEADER),
                    LWIO_SAFE_LOG_STRING(pszBuffer));
        }
    }

error:

    va_end(msgList);

    SRV_SAFE_FREE_MEMORY(pszBuffer);
}
