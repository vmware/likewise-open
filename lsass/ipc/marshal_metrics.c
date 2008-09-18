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
 *        marshal_metrics.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Marshal/Unmarshal API for Messages related to Metrics
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "ipc.h"

DWORD
LsaMarshalMetricsInfo(
    DWORD  dwInfoLevel,
    PVOID  pMetricPack,
    PSTR   pszBuffer,
    PDWORD pdwBufLen
    )
{
    DWORD dwError = 0;
    LSA_METRICS_HEADER header = {0};
    DWORD dwRequiredBufferLen = sizeof(header);
    DWORD dwOffset = sizeof(header);
    
    BAIL_ON_INVALID_POINTER(pMetricPack);
    
    switch (dwInfoLevel)
    {
        case 0:

             dwRequiredBufferLen += sizeof(LSA_METRIC_PACK_0);
             break;

        case 1:

             dwRequiredBufferLen += sizeof(LSA_METRIC_PACK_1);
             break;

        default:

             dwError = LSA_ERROR_INVALID_METRIC_INFO_LEVEL;
             BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pszBuffer)
    {
        *pdwBufLen = dwRequiredBufferLen;
        goto cleanup;
    }
    
    if (*pdwBufLen < dwRequiredBufferLen)
    {
        dwError = LSA_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch (dwInfoLevel)
    {
        case 0:

             memcpy(pszBuffer + dwOffset, pMetricPack, sizeof(LSA_METRIC_PACK_0));
             break;

        case 1:

             memcpy(pszBuffer + dwOffset, pMetricPack, sizeof(LSA_METRIC_PACK_1));
             break;

        default:

             dwError = LSA_ERROR_INVALID_METRIC_INFO_LEVEL;
             BAIL_ON_LSA_ERROR(dwError);
    }

    header.dwInfoLevel = dwInfoLevel;

    memcpy(pszBuffer, &header, sizeof(header));
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaUnmarshalMetricsInfo(
    PCSTR  pszMsgBuf,
    DWORD  dwMsgLen,
    PDWORD pdwInfoLevel,
    PVOID* ppMetricPack
    )
{
    DWORD dwError = 0;
    LSA_METRICS_HEADER header = {0};
    PVOID pMetricPack = NULL;
    DWORD dwReadOffset = 0;
    DWORD dwBytesRemaining = dwMsgLen;
    
    if (dwMsgLen < sizeof(header))
    {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    memcpy(&header, pszMsgBuf, sizeof(header));
    dwBytesRemaining -= sizeof(header);
    dwReadOffset += sizeof(header);
    
    switch(header.dwInfoLevel)
    {
        case 0:

             if (dwBytesRemaining < sizeof(LSA_METRIC_PACK_0))
             {
                dwError = LSA_ERROR_INVALID_MESSAGE;
                BAIL_ON_LSA_ERROR(dwError);
             }

             dwError = LsaAllocateMemory(
                           sizeof(LSA_METRIC_PACK_0),
                           (PVOID*)&pMetricPack);
             BAIL_ON_LSA_ERROR(dwError);
 
             memcpy(pMetricPack, pszMsgBuf + dwReadOffset, sizeof(LSA_METRIC_PACK_0));

             break;

        case 1:

             if (dwBytesRemaining < sizeof(LSA_METRIC_PACK_1))
             {
                dwError = LSA_ERROR_INVALID_MESSAGE;
                BAIL_ON_LSA_ERROR(dwError);
             }

             dwError = LsaAllocateMemory(
                           sizeof(LSA_METRIC_PACK_1),
                           (PVOID*)&pMetricPack);
             BAIL_ON_LSA_ERROR(dwError);

             memcpy(pMetricPack, pszMsgBuf + dwReadOffset, sizeof(LSA_METRIC_PACK_1));

             break;

        default:

             dwError = LSA_ERROR_INVALID_METRIC_INFO_LEVEL;
             BAIL_ON_LSA_ERROR(dwError);
    }
    
    *pdwInfoLevel = header.dwInfoLevel;
    *ppMetricPack = pMetricPack;
    
cleanup:

    return dwError;
    
error:

    *pdwInfoLevel = 0;
    *ppMetricPack = NULL;
    
    LSA_SAFE_FREE_MEMORY(pMetricPack);

    goto cleanup;    
}
