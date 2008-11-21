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
 *        groups.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Group Lookup and Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "client.h"

LSASS_API
DWORD
LsaSetLogLevel(
    HANDLE      hLsaConnection,
    LsaLogLevel logLevel
    )
{
    LSA_LOG_INFO logInfo = {0};

    logInfo.maxAllowedLogLevel = logLevel;

    return LsaSetLogInfo(
                    hLsaConnection,
                    &logInfo);
}

LSASS_API
DWORD
LsaSetLogInfo(
    HANDLE hLsaConnection,
    PLSA_LOG_INFO pLogInfo
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(pLogInfo);

    dwError = LsaMarshalLogInfo(
                    pLogInfo->maxAllowedLogLevel,
                    pLogInfo->logTarget,
                    pLogInfo->pszPath,
                    NULL,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBuildMessage(
                LSA_Q_SET_LOGINFO,
                dwMsgLen,
                1,
                1,
                &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMarshalLogInfo(
                    pLogInfo->maxAllowedLogLevel,
                    pLogInfo->logTarget,
                    pLogInfo->pszPath,
                    pMessage->pData,
                    &dwMsgLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSendMessage(hLsaConnection, pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_SAFE_FREE_MESSAGE(pMessage);

    dwError = LsaGetNextMessage(hLsaConnection, &pMessage);
    BAIL_ON_LSA_ERROR(dwError);

    switch (pMessage->header.messageType) {
        case LSA_R_SET_LOGINFO:
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

LSASS_API
DWORD
LsaGetLogInfo(
    HANDLE         hLsaConnection,
    PLSA_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PLSAMESSAGE pMessage = NULL;
    DWORD   dwMsgLen = 0;
    PSTR    pszError = NULL;
    PLSA_LOG_INFO pLogInfo = NULL;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_POINTER(ppLogInfo);

    dwError = LsaBuildMessage(
                LSA_Q_GET_LOGINFO,
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
        case LSA_R_GET_LOGINFO:
        {
            dwError = LsaUnmarshalLogInfo(
                                    pMessage->pData,
                                    pMessage->header.messageLength,
                                    &pLogInfo);
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

    *ppLogInfo = pLogInfo;

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    LSA_SAFE_FREE_STRING(pszError);

    return dwError;

error:

    if (ppLogInfo)
    {
        *ppLogInfo = NULL;
    }

    if (pLogInfo)
    {
        LsaFreeLogInfo(pLogInfo);
    }

    goto cleanup;
}


