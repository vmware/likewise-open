/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        ipc_tracing.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process communication (Server) API for Tracing Info
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "ipc.h"

DWORD
LsaSrvIpcSetTraceInfo(
    HANDLE      hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD  dwError = 0;
    PLSAMESSAGE pResponse = NULL;
    PLSASERVERCONNECTIONCONTEXT pContext  =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    PLSA_TRACE_INFO pTraceFlagArray = NULL;
    DWORD dwNumFlags = 0;

    dwError = LsaUnmarshalTraceFlags(
                    pMessage->pData,
                    pMessage->header.messageLength,
                    &pTraceFlagArray,
                    &dwNumFlags);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvSetTraceFlags(
                    hServer,
                    pTraceFlagArray,
                    dwNumFlags);
    if (!dwError) {

       dwError = LsaBuildMessage(
                    LSA_R_SET_TRACE_INFO,
                    0, /* Empty message */
                    1,
                    1,
                    &pResponse);
       BAIL_ON_LSA_ERROR(dwError);

    } else {

        dwError = LsaSrvIpcMarshalError(
                        dwError,
                        &pResponse);
        BAIL_ON_LSA_ERROR(dwError);

    }

    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_MEMORY(pTraceFlagArray);
    LSA_SAFE_FREE_MESSAGE(pResponse);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvIpcGetTraceInfo(
    HANDLE      hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD  dwError = 0;
    PLSAMESSAGE pResponse = NULL;
    PLSASERVERCONNECTIONCONTEXT pContext  =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    DWORD dwTraceFlag = 0;
    PLSA_TRACE_INFO pTraceInfo = NULL;
    DWORD dwMsgLen = 0;

    dwError = LsaUnmarshalQueryTraceFlag(
                    pMessage->pData,
                    pMessage->header.messageLength,
                    &dwTraceFlag);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvGetTraceInfo(
                    hServer,
                    dwTraceFlag,
                    &pTraceInfo);
    if (!dwError) {

        dwError = LsaMarshalTraceFlags(
                        pTraceInfo,
                        1,
                        NULL,
                        &dwMsgLen);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaBuildMessage(
                        LSA_R_GET_TRACE_INFO,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaMarshalTraceFlags(
                        pTraceInfo,
                        1,
                        pResponse->pData,
                        &dwMsgLen);
        BAIL_ON_LSA_ERROR(dwError);

    } else {

        dwError = LsaSrvIpcMarshalError(
                        dwError,
                        &pResponse);
        BAIL_ON_LSA_ERROR(dwError);

    }

    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_MEMORY(pTraceInfo);
    LSA_SAFE_FREE_MESSAGE(pResponse);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvIpcEnumTraceInfo(
    HANDLE      hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD  dwError = 0;
    PLSAMESSAGE pResponse = NULL;
    PLSASERVERCONNECTIONCONTEXT pContext  =
         (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;
    DWORD dwNumFlags = 0;
    PLSA_TRACE_INFO pTraceFlagArray = NULL;
    DWORD dwMsgLen = 0;

    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvEnumTraceFlags(
                    hServer,
                    &pTraceFlagArray,
                    &dwNumFlags);
    if (!dwError) {

        dwError = LsaMarshalTraceFlags(
                        pTraceFlagArray,
                        dwNumFlags,
                        NULL,
                        &dwMsgLen);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaBuildMessage(
                        LSA_R_ENUM_TRACE_INFO,
                        dwMsgLen,
                        1,
                        1,
                        &pResponse);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaMarshalTraceFlags(
                        pTraceFlagArray,
                        dwNumFlags,
                        pResponse->pData,
                        &dwMsgLen);
        BAIL_ON_LSA_ERROR(dwError);

    } else {

        dwError = LsaSrvIpcMarshalError(
                        dwError,
                        &pResponse);
        BAIL_ON_LSA_ERROR(dwError);

    }

    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_MEMORY(pTraceFlagArray);
    LSA_SAFE_FREE_MESSAGE(pResponse);

    return dwError;

error:

    goto cleanup;
}
