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
 *        tracing.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Tracing API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "client.h"

DWORD
LsaSetTraceFlags(
    HANDLE          hLsaConnection,
    PLSA_TRACE_INFO pTraceFlagArray,
    DWORD           dwNumFlags
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(pTraceFlagArray);

    dwError = LsaMarshalTraceFlags(
                    pTraceFlagArray,
                    dwNumFlags,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBuildMessage(
                LSA_Q_SET_TRACE_INFO,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMarshalTraceFlags(
                    pTraceFlagArray,
                    dwNumFlags,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_SAFE_FREE_MESSAGE(pMessage);

    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pMessage->header.messageType) {
        case LSA_R_SET_TRACE_INFO:
        {
            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;

            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError
                                );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaEnumTraceFlags(
    HANDLE           hLsaConnection,
    PLSA_TRACE_INFO* ppTraceFlagArray,
    PDWORD           pdwNumFlags
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    PLSA_TRACE_INFO pTraceFlagArray = NULL;
    DWORD dwNumFlags = 0;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(ppTraceFlagArray);
    BAIL_ON_INVALID_POINTER(pdwNumFlags);

    dwError = LsaBuildMessage(
                LSA_Q_ENUM_TRACE_INFO,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_SAFE_FREE_MESSAGE(pMessage);

    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pMessage->header.messageType) {
        case LSA_R_ENUM_TRACE_INFO:
        {
            dwError = LsaUnmarshalTraceFlags(
                            pMessage->pData,
                            pMessage->header.messageLength,
                            &pTraceFlagArray,
                            &dwNumFlags);
            BAIL_ON_LSA_ERROR(dwError);

            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;

            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError
                                );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *ppTraceFlagArray = pTraceFlagArray;
    *pdwNumFlags = dwNumFlags;

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;

error:

    LSA_SAFE_FREE_MEMORY(pTraceFlagArray);

    if (ppTraceFlagArray)
    {
        *ppTraceFlagArray = NULL;
    }

    if (pdwNumFlags)
    {
        *pdwNumFlags = 0;
    }

    goto cleanup;
}

DWORD
LsaGetTraceFlag(
    HANDLE           hLsaConnection,
    DWORD            dwTraceFlag,
    PLSA_TRACE_INFO* ppTraceFlag
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    PLSA_TRACE_INFO pTraceFlag = NULL;
    PLSA_TRACE_INFO pTraceFlagArray = NULL;
    PLSA_TRACE_INFO pTraceInfo = NULL;
    DWORD dwNumFlags = 0;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(ppTraceFlag);

    dwError = LsaMarshalQueryTraceFlag(
                    dwTraceFlag,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBuildMessage(
                LSA_Q_GET_TRACE_INFO,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMarshalQueryTraceFlag(
                    dwTraceFlag,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_SAFE_FREE_MESSAGE(pMessage);

    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pMessage->header.messageType) {
        case LSA_R_GET_TRACE_INFO:
        {
            dwError = LsaUnmarshalTraceFlags(
                            pMessage->pData,
                            pMessage->header.messageLength,
                            &pTraceFlagArray,
                            &dwNumFlags);
            BAIL_ON_LSA_ERROR(dwError);

            break;
        }
        case LSA_ERROR:
        {
            DWORD dwSrvError = 0;

            dwError = LsaUnmarshalError(
                                pMessage->pData,
                                pMessage->header.messageLength,
                                &dwSrvError,
                                &pszError
                                );
            BAIL_ON_LSA_ERROR(dwError);
            dwError = dwSrvError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }
        default:
        {
            dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (dwNumFlags != 1)
    {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(LSA_TRACE_INFO),
                    (PVOID*)&pTraceFlag);
    BAIL_ON_LSA_ERROR(dwError);

    pTraceInfo = &pTraceFlagArray[0];
    pTraceFlag->bStatus = pTraceInfo->bStatus;
    pTraceFlag->dwTraceFlag = pTraceInfo->dwTraceFlag;

    *ppTraceFlag = pTraceFlag;

cleanup:

    LSA_SAFE_FREE_MEMORY(pTraceFlagArray);

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;

error:

    if (ppTraceFlag)
    {
        *ppTraceFlag = NULL;
    }

    LSA_SAFE_FREE_MEMORY(pTraceFlag);

    goto cleanup;
}
