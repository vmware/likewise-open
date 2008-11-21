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
 *        marshal_tracing.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Marshal/Unmarshal API for Messages related to tracing info
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#include "ipc.h"

DWORD
LsaMarshalTraceFlags(
    PLSA_TRACE_INFO pTraceFlagArray,
    DWORD           dwNumFlags,
    PSTR            pszBuffer,
    PDWORD          pdwMsgLen
    )
{
    DWORD dwError = 0;
    PSTR  pszWriteMarker = NULL;
    DWORD iFlag = 0;
    DWORD dwMsgLen = sizeof(DWORD);

    dwMsgLen += dwNumFlags * sizeof(LSA_TRACE_INFO);

    if (!pszBuffer)
    {
        *pdwMsgLen = dwMsgLen;
        goto cleanup;
    }

    pszWriteMarker = pszBuffer;
    memcpy(pszWriteMarker, &dwNumFlags, sizeof(dwNumFlags));
    pszWriteMarker += sizeof(dwNumFlags);

    for (; iFlag < dwNumFlags; iFlag++)
    {
        PLSA_TRACE_INFO pInfo = &pTraceFlagArray[iFlag];

        memcpy(pszWriteMarker, pInfo, sizeof(LSA_TRACE_INFO));

        pszWriteMarker += sizeof(LSA_TRACE_INFO);
    }

cleanup:

    return dwError;
}

DWORD
LsaUnmarshalTraceFlags(
    PCSTR            pszBuffer,
    DWORD            dwMsgLen,
    PLSA_TRACE_INFO* ppTraceFlagArray,
    PDWORD           pdwNumFlags
    )
{
    DWORD dwError = 0;
    DWORD dwNumFlags = 0;
    PCSTR pszReadMarker = NULL;
    DWORD dwNumRemainingBytes = 0;
    DWORD iFlag = 0;
    PLSA_TRACE_INFO pTraceFlagArray = NULL;

    if (dwMsgLen < sizeof(DWORD))
    {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pszReadMarker = pszBuffer;
    dwNumRemainingBytes = dwMsgLen;

    memcpy(&dwNumFlags, pszReadMarker, sizeof(DWORD));
    dwNumRemainingBytes -= sizeof(DWORD);
    pszReadMarker += sizeof(DWORD);

    if (!dwNumFlags)
    {
        goto done;
    }

    dwError = LsaAllocateMemory(
                    dwNumFlags * sizeof(LSA_TRACE_INFO),
                    (PVOID*)&pTraceFlagArray);
    BAIL_ON_LSA_ERROR(dwError);

    for(; iFlag < dwNumFlags; iFlag++)
    {
        PLSA_TRACE_INFO pTraceFlag = &pTraceFlagArray[iFlag];

        if (dwNumRemainingBytes < sizeof(LSA_TRACE_INFO))
        {
            dwError = LSA_ERROR_INVALID_MESSAGE;
            BAIL_ON_LSA_ERROR(dwError);
        }

        memcpy(pTraceFlag, pszReadMarker, sizeof(LSA_TRACE_INFO));

        pszReadMarker += sizeof(LSA_TRACE_INFO);
        dwNumRemainingBytes -= sizeof(LSA_TRACE_INFO);
    }

done:

    *ppTraceFlagArray = pTraceFlagArray;
    *pdwNumFlags = dwNumFlags;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_MEMORY(pTraceFlagArray);

    *ppTraceFlagArray = NULL;
    *pdwNumFlags = 0;

    goto cleanup;
}

DWORD
LsaMarshalQueryTraceFlag(
    DWORD           dwTraceFlag,
    PSTR            pszBuffer,
    PDWORD          pdwMsgLen
    )
{
    DWORD dwError = 0;
    DWORD dwMsgLen = sizeof(DWORD);

    if (!pszBuffer)
    {
        *pdwMsgLen = dwMsgLen;
        goto cleanup;
    }

    memcpy(pszBuffer, &dwTraceFlag, sizeof(DWORD));

cleanup:

    return dwError;
}

DWORD
LsaUnmarshalQueryTraceFlag(
    PSTR            pszBuffer,
    DWORD           dwMsgLen,
    PDWORD          pdwTraceFlag
    )
{
    DWORD dwError = 0;
    DWORD dwTraceFlag = 0;

    if (dwMsgLen < sizeof(DWORD))
    {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    memcpy(&dwTraceFlag, pszBuffer, sizeof(DWORD));

    *pdwTraceFlag = dwTraceFlag;

cleanup:

    return dwError;

error:

    *pdwTraceFlag = 0;

    goto cleanup;
}
