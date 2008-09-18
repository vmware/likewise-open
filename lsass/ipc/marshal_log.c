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
 *        marshal_log.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Marshal/Unmarshal API for Messages related to log info
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "ipc.h"

DWORD
LsaMarshalLogInfo(
    LsaLogLevel  maxAllowedLogLevel,
    LsaLogTarget logTarget,
    PCSTR        pszPath,
    PSTR         pszBuffer,
    PDWORD       pdwBufLen
    )
{
    DWORD dwError = 0;
    DWORD dwRequiredBufLen = 0;
    LSA_LOG_INFO_MSG logMsg = {0};
    DWORD dwOffset = 0;
    
    dwRequiredBufLen = LsaComputeLogInfoBufferSize(
                    maxAllowedLogLevel,
                            logTarget,
                            pszPath);
    if (!pszBuffer)
    {
        *pdwBufLen = dwRequiredBufLen;
        goto cleanup;
    }
    
    if (*pdwBufLen < dwRequiredBufLen) {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    logMsg.logTarget = logTarget;
    logMsg.maxAllowedLogLevel = maxAllowedLogLevel;
    
    dwOffset = sizeof(logMsg);
    
    if (!IsNullOrEmptyString(pszPath))
    {
        logMsg.path.length = strlen(pszPath);
        logMsg.path.offset = dwOffset;
        
        memcpy(pszBuffer + logMsg.path.offset,
               pszPath,
               logMsg.path.length);
    }
    
    memcpy(pszBuffer, &logMsg, sizeof(logMsg));
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaUnmarshalLogInfo(
    PCSTR pszMsgBuf,
    DWORD dwMsgLen,
    PLSA_LOG_INFO* ppLogInfo
    )
{
    DWORD dwError = 0;
    PLSA_LOG_INFO pLogInfo = NULL;
    LSA_LOG_INFO_MSG logMsg = {0};

    if (dwMsgLen < sizeof(logMsg)) {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(&logMsg, pszMsgBuf, sizeof(logMsg));
    
    dwError = LsaAllocateMemory(
                    sizeof(LSA_LOG_INFO),
                    (PVOID*)&pLogInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    pLogInfo->logTarget = logMsg.logTarget;
    pLogInfo->maxAllowedLogLevel = logMsg.maxAllowedLogLevel;
    if (logMsg.path.length)
    {
        dwError = LsaStrndup(
                        pszMsgBuf + logMsg.path.offset,
                        logMsg.path.length,
                        &pLogInfo->pszPath);
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *ppLogInfo = pLogInfo;
    
cleanup:

    return dwError;
    
error:

    *ppLogInfo = NULL;
    
    if (pLogInfo)
    {
        LsaFreeLogInfo(pLogInfo);
    }
    
    goto cleanup;    
}

DWORD
LsaComputeLogInfoBufferSize(
    LsaLogLevel  logLevel,
    LsaLogTarget logTarget,
    PCSTR        pszPath
    )
{
    return sizeof(LSA_LOG_INFO_MSG) + (IsNullOrEmptyString(pszPath) ? 0 : strlen(pszPath));
}
