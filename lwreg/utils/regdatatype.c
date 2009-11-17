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
 *        regtime.c
 *
 * Abstract:
 *
 *        Likewise Registry
 *
 *        DataType Convertion Utilities
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 *
 */

#include "includes.h"

DWORD
RegMultiStrsToByteArrayW(
    PWSTR*   ppwszInMultiSz,
    PBYTE*   ppOutBuf,
    SSIZE_T* pOutBufLen
    )
{
    DWORD   dwError   = 0;
    SSIZE_T idx       = 0;
    SSIZE_T OutBufLen = 0;
    PBYTE   pOutBuf   = NULL;
    PBYTE   pCursor   = NULL;

    BAIL_ON_INVALID_POINTER(ppwszInMultiSz);
    BAIL_ON_INVALID_POINTER(ppOutBuf);
    BAIL_ON_INVALID_POINTER(pOutBufLen);

    // Determine total length of all strings in bytes
    for (; ppwszInMultiSz[idx]; idx++)
    {
        size_t len = 0;

        dwError = LwWc16sLen(ppwszInMultiSz[idx], &len);
        BAIL_ON_REG_ERROR(dwError);

        OutBufLen +=  (len + 1) * sizeof(WCHAR);
    }

    OutBufLen += sizeof(WCHAR); // double null at end

    dwError = LwAllocateMemory(OutBufLen, (LW_PVOID*) &pOutBuf);
    BAIL_ON_REG_ERROR(dwError);

    for (idx=0, pCursor = pOutBuf; ppwszInMultiSz[idx]; idx++)
    {
        size_t len = 0;

        dwError = LwWc16sLen(ppwszInMultiSz[idx], &len);
        BAIL_ON_REG_ERROR(dwError);

        len++; // accommodate null

        memcpy(pCursor, (PBYTE)ppwszInMultiSz[idx], len * sizeof(WCHAR));

        pCursor += len * sizeof(WCHAR);
    }

    *((PWSTR)(++pCursor)) = 0;

   *ppOutBuf   = pOutBuf;
   *pOutBufLen = OutBufLen;

cleanup:

    return dwError;

error:

    if (pOutBuf)
    {
        LwFreeMemory(pOutBuf);
    }

    if (ppOutBuf)
    {
        *ppOutBuf = NULL;
    }
    if (pOutBufLen)
    {
        *pOutBufLen = 0;
    }

    goto cleanup;
}

DWORD
RegMultiStrsToByteArrayA(
    PSTR*    ppszInMultiSz,
    PBYTE*   ppOutBuf,
    SSIZE_T* pOutBufLen
    )
{
    DWORD   dwError   = 0;
    SSIZE_T idx       = 0;
    SSIZE_T OutBufLen = 0;
    PBYTE   pOutBuf   = NULL;
    PBYTE   pCursor   = NULL;

    BAIL_ON_INVALID_POINTER(ppszInMultiSz);
    BAIL_ON_INVALID_POINTER(ppOutBuf);
    BAIL_ON_INVALID_POINTER(pOutBufLen);

    // Determine total length of all strings in bytes
    for (; ppszInMultiSz[idx]; idx++)
    {
        OutBufLen += strlen(ppszInMultiSz[idx]) + 1;
    }

    OutBufLen++; // double null at end

    dwError = LwAllocateMemory(OutBufLen, (LW_PVOID*) &pOutBuf);
    BAIL_ON_REG_ERROR(dwError);

    for (idx=0, pCursor = pOutBuf; ppszInMultiSz[idx]; idx++)
    {
        size_t len = strlen(ppszInMultiSz[idx]) + 1;

        memcpy(pCursor, ppszInMultiSz[idx], len);

        pCursor += len;
    }

    *++pCursor = '\0';

   *ppOutBuf   = pOutBuf;
   *pOutBufLen = OutBufLen;

cleanup:

    return dwError;

error:

    if (pOutBuf)
    {
        LwFreeMemory(pOutBuf);
    }

    if (ppOutBuf)
    {
        *ppOutBuf = NULL;
    }
    if (pOutBufLen)
    {
        *pOutBufLen = 0;
    }

    goto cleanup;
}

DWORD
RegByteArrayToMultiStrsW(
    PBYTE   pInBuf,
    SSIZE_T bufLen,
    PWSTR** pppwszStrings
    )
{
    DWORD   dwError      = 0;
    DWORD   dwNumStrings = 0;
    PWSTR*  ppwszStrings = NULL;
    PWSTR   pwszCursor   = NULL;
    size_t  len          = 0;
    SSIZE_T iStr         = 0;

    BAIL_ON_INVALID_POINTER(pInBuf);
    BAIL_ON_INVALID_POINTER(pppwszStrings);

    if (!bufLen || (bufLen % sizeof(WCHAR)))
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_REG_ERROR(dwError);
    }

    // determine number of strings
    pwszCursor = (PWSTR)pInBuf;
	do
    {
        dwError = LwWc16sLen(pwszCursor, &len);
        BAIL_ON_REG_ERROR(dwError);

        if (len)
        {
            pwszCursor += len + 1;
            dwNumStrings++;
        }
    } while (len);

    dwError = LwAllocateMemory(
                    sizeof(PWSTR) * (dwNumStrings + 1),
                    (LW_PVOID*) &ppwszStrings);
    BAIL_ON_REG_ERROR(dwError);

    pwszCursor = (PWSTR)pInBuf;
    for (iStr = 0; iStr < dwNumStrings; iStr++)
    {
        PWSTR   pwszStrBegin = pwszCursor;

	    len = 0;
        while (!IsNullOrEmptyString(pwszCursor))
        {
            len++;
            pwszCursor++;
        }

        dwError = LwAllocateMemory(
                        (len + 1) * sizeof(WCHAR),
                        (LW_PVOID*)&ppwszStrings[iStr]);
        BAIL_ON_REG_ERROR(dwError);

        memcpy( (PBYTE)ppwszStrings[iStr],
                (PBYTE)pwszStrBegin,
                len * sizeof(WCHAR));

        pwszCursor++;
    }

    *pppwszStrings = ppwszStrings;

cleanup:

    return dwError;

error:

    *pppwszStrings = NULL;

    if (ppwszStrings)
    {
        RegFreeMultiStrsW(ppwszStrings);
    }

    goto cleanup;
}

DWORD
RegByteArrayToMultiStrsA(
    PBYTE   pInBuf,
    SSIZE_T bufLen,
    PSTR**  pppszStrings
    )
{
    DWORD   dwError      = 0;
    DWORD   dwNumStrings = 0;
    PSTR*   ppszStrings  = NULL;
    PSTR    pszCursor    = NULL;
    SSIZE_T sLen         = 0;
    SSIZE_T iStr         = 0;

    BAIL_ON_INVALID_POINTER(pInBuf);
    BAIL_ON_INVALID_POINTER(pppszStrings);

    if (!bufLen)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_REG_ERROR(dwError);
    }

    // determine number of strings
    pszCursor = (PSTR)pInBuf;
    do
    {
        sLen = strlen(pszCursor);

        if (sLen)
        {
            pszCursor += sLen + 1;
            dwNumStrings++;
        }
    } while (sLen);

    dwError = LwAllocateMemory(
                    sizeof(PSTR) * (dwNumStrings + 1),
                    (LW_PVOID*) &ppszStrings);
    BAIL_ON_REG_ERROR(dwError);

    pszCursor = (PSTR)pInBuf;
    for (iStr = 0; iStr < dwNumStrings; iStr++)
    {
        SSIZE_T len = 0;
        PSTR    pszStrBegin = pszCursor;

        while (!IsNullOrEmptyString(pszCursor))
        {
            len++;
            pszCursor++;
        }

        dwError = LwAllocateMemory(
                        (len + 1) * sizeof(CHAR),
                        (LW_PVOID*)&ppszStrings[iStr]);
        BAIL_ON_REG_ERROR(dwError);

        memcpy( (PBYTE)ppszStrings[iStr],
                (PBYTE)pszStrBegin,
                len * sizeof(CHAR));

        pszCursor++;
    }

    *pppszStrings = ppszStrings;

cleanup:

    return dwError;

error:

    *pppszStrings = NULL;

    if (ppszStrings)
    {
        RegFreeMultiStrsA(ppszStrings);
    }

    goto cleanup;
}

DWORD
RegConvertByteStreamA2W(
    const PBYTE pData,
    DWORD       cbData,
    PBYTE*      ppOutData,
    PDWORD      pcbOutDataLen
    )
{
    DWORD dwError      = 0;
    PBYTE pOutData     = NULL;
    DWORD cbOutDataLen = 0;
    PCSTR pszCursor    = NULL;
    PWSTR pwszCursor   = NULL;
    PWSTR pwszValue    = NULL;

    cbOutDataLen = cbData * sizeof(WCHAR);

    dwError = LwAllocateMemory(cbOutDataLen, (LW_PVOID*)&pOutData);
    BAIL_ON_REG_ERROR(dwError);

    pszCursor = (PCSTR)pData;
    pwszCursor = (PWSTR)pOutData;

    while (pszCursor && *pszCursor)
    {
        DWORD dwLength = strlen(pszCursor);

        if (pwszValue)
        {
            LwFreeMemory(pwszValue);
            pwszValue = NULL;
        }

        dwError = LwMbsToWc16s(pszCursor, &pwszValue);
        BAIL_ON_REG_ERROR(dwError);

        memcpy((PBYTE)pwszCursor, (PBYTE)pwszValue, dwLength * sizeof(WCHAR));

        pszCursor  += dwLength + 1;
        pwszCursor += dwLength + 1;
    }

    *ppOutData     = pOutData;
    *pcbOutDataLen = cbOutDataLen;

cleanup:

    if (pwszValue)
    {
        LwFreeMemory(pwszValue);
    }

    return dwError;

error:

    *ppOutData     = NULL;
    *pcbOutDataLen = 0;

    if (pOutData)
    {
        LwFreeMemory(pOutData);
    }

    goto cleanup;
}

DWORD
RegConvertByteStreamW2A(
    const PBYTE pData,
    DWORD       cbData,
    PBYTE*      ppOutData,
    PDWORD      pcbOutDataLen
    )
{
    DWORD dwError      = 0;
    PBYTE pOutData     = NULL;
    DWORD cbOutDataLen = 0;
    PSTR  pszCursor    = NULL;
    PWSTR pwszCursor   = NULL;
    PSTR  pszValue     = NULL;

    cbOutDataLen = cbData / sizeof(WCHAR);

    dwError = LwAllocateMemory(cbOutDataLen, (LW_PVOID*)&pOutData);
    BAIL_ON_REG_ERROR(dwError);

    pwszCursor = (PWSTR)pData;
    pszCursor = (PSTR)pOutData;

    while (pwszCursor && *pwszCursor)
    {
        size_t len = 0;

        dwError = LwWc16sLen(pwszCursor, &len);
        BAIL_ON_REG_ERROR(dwError);

        if (pszValue)
        {
            LwFreeMemory(pszValue);
            pszValue = NULL;
        }

        dwError = LwWc16sToMbs(pwszCursor, &pszValue);
        BAIL_ON_REG_ERROR(dwError);

        memcpy(pszCursor, pszValue, len);

        pszCursor  += len + 1;
        pwszCursor += len + 1;
    }

    *ppOutData     = pOutData;
    *pcbOutDataLen = cbOutDataLen;

cleanup:

    if (pszValue)
    {
        LwFreeMemory(pszValue);
    }

    return dwError;

error:

    *ppOutData     = NULL;
    *pcbOutDataLen = 0;

    if (pOutData)
    {
        LwFreeMemory(pOutData);
    }

    goto cleanup;
}

void
RegFreeMultiStrsA(
    PSTR* ppszStrings
    )
{
    SSIZE_T idx = 0;

    while (ppszStrings[idx])
    {
        LwFreeMemory(ppszStrings[idx++]);
    }

    LwFreeMemory(ppszStrings);
}

void
RegFreeMultiStrsW(
    PWSTR* ppwszStrings
    )
{
    SSIZE_T idx = 0;

    while (ppwszStrings[idx])
    {
        LwFreeMemory(ppwszStrings[idx++]);
    }

    LwFreeMemory(ppwszStrings);
}
