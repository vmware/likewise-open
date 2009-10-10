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
ConvertMultiStrsToByteArrayW(
    PSTR* ppszInMultiSz,
    PBYTE *outBuf,
    SSIZE_T *outBufLen
    )
{
    DWORD dwError = 0;
    DWORD count = 0;
    PWSTR pszUcs2String = NULL;
    size_t ucs2StringLen = 0;
    DWORD multiStringLen = 0;
    PCHAR pszMultiString = NULL;
    PCHAR ptrMultiString = NULL;

    BAIL_ON_INVALID_POINTER(ppszInMultiSz);
    BAIL_ON_INVALID_POINTER(outBuf);

    /* Determine length of all multi strings in bytes */
    for (count=0, multiStringLen=0; ppszInMultiSz[count]; count++)
    {
        ucs2StringLen = strlen(ppszInMultiSz[count]);
        multiStringLen += (ucs2StringLen+1) * 2;
    }

    /*
     * These are the double '\0' terminations at the end of every string.
     */
    multiStringLen += (count + 2) * 2;
    dwError = LwAllocateMemory(sizeof(CHAR) * multiStringLen,
                               (LW_PVOID) &pszMultiString);
    BAIL_ON_REG_ERROR(dwError);

    ptrMultiString = pszMultiString;
    for (count=0; ppszInMultiSz[count]; count++)
    {
        dwError = LwMbsToWc16s(ppszInMultiSz[count], &pszUcs2String);
        BAIL_ON_REG_ERROR(dwError);
        dwError = LwWc16sLen(pszUcs2String, &ucs2StringLen);
        BAIL_ON_REG_ERROR(dwError);

        ucs2StringLen = (ucs2StringLen+1) * 2;
        memcpy(ptrMultiString, pszUcs2String, ucs2StringLen);
        LwFreeMemory(pszUcs2String);
        ptrMultiString += ucs2StringLen;
    }
    *++ptrMultiString = '\0';
    *++ptrMultiString = '\0';

   *outBuf = (PBYTE) pszMultiString;
   *outBufLen = ptrMultiString - pszMultiString;
cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
ConvertMultiStrsToByteArrayA(
    PSTR* ppszInMultiSz,
    PBYTE *outBuf,
    SSIZE_T *outBufLen
    )
{
    DWORD dwError = 0;
    DWORD count = 0;
    size_t ansiStringLen = 0;
    DWORD multiStringLen = 0;
    PCHAR pszMultiString = NULL;
    PCHAR ptrMultiString = NULL;

    BAIL_ON_INVALID_POINTER(ppszInMultiSz);
    BAIL_ON_INVALID_POINTER(outBuf);

    /* Determine length of all multi strings in bytes */
    for (count=0, multiStringLen=0; ppszInMultiSz[count]; count++)
    {
        ansiStringLen = strlen(ppszInMultiSz[count]);
        multiStringLen += (ansiStringLen+1);
    }

    /*
     * These are the double '\0' terminations at the end of every string.
     */
    multiStringLen += (count + 2) * 2;
    dwError = LwAllocateMemory(sizeof(CHAR) * multiStringLen,
                               (LW_PVOID) &pszMultiString);
    BAIL_ON_REG_ERROR(dwError);

    ptrMultiString = pszMultiString;
    for (count=0; ppszInMultiSz[count]; count++)
    {
        ansiStringLen = strlen(ppszInMultiSz[count]) + 1;
        memcpy(ptrMultiString, ppszInMultiSz[count], ansiStringLen);
        ptrMultiString += ansiStringLen;
    }
    *++ptrMultiString = '\0';
    *++ptrMultiString = '\0';

   *outBuf = (PBYTE) pszMultiString;
   *outBufLen = ptrMultiString - pszMultiString;
cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
ConvertByteArrayToMultiStrsW(
    PBYTE pInBuf,
    SSIZE_T bufLen,
    PSTR **pppszOutMultiSz
    )
{
    DWORD dwError;
    size_t strLen = 0;
    DWORD count = 0;
    PSTR pszUTF8 = NULL;
    PSTR *ppszOutMultiSz = NULL;
    PWSTR pwszInString = NULL;

    BAIL_ON_INVALID_POINTER(pInBuf);
    BAIL_ON_INVALID_POINTER(pppszOutMultiSz);


    /* Loop through multistring once to count number of entries */
    pwszInString = (PWSTR) pInBuf;
    do
    {
        dwError = LwWc16sLen(pwszInString, &strLen);
        BAIL_ON_REG_ERROR(dwError);

        if (strLen)
        {
            pwszInString += strLen + 1;
            count++;
        }
    } while (strLen);
    dwError = LwAllocateMemory(sizeof(PCHAR) * (count + 1),
                               (LW_PVOID) &ppszOutMultiSz);
    BAIL_ON_REG_ERROR(dwError);

    /* Loop through multistring again to convert to UTF8 */
    count = 0;
    pwszInString = (PWSTR) pInBuf;
    do
    {
        dwError = LwWc16sToMbs(pwszInString, &pszUTF8);
        BAIL_ON_REG_ERROR(dwError);
        dwError = LwWc16sLen(pwszInString, &strLen);
        BAIL_ON_REG_ERROR(dwError);

        if (strLen)
        {
            ppszOutMultiSz[count++] = pszUTF8;
            pszUTF8 = NULL;
            pwszInString += strLen + 1;
        }
    } while (strLen);

    *pppszOutMultiSz = ppszOutMultiSz;

cleanup:
    LW_SAFE_FREE_STRING(pszUTF8);

    return dwError;

error:
    ConvertMultiStrsFree(ppszOutMultiSz);
    goto cleanup;
}

DWORD
ConvertByteArrayToMultiStrsA(
    PBYTE pInBuf,
    SSIZE_T bufLen,
    PSTR **pppszOutMultiSz
    )
{
    DWORD dwError;
    size_t sLen = 0;
    DWORD count = 0;
    PSTR pszUTF8 = NULL;
    PSTR *ppszOutMultiSz = NULL;
    PSTR pszInString = NULL;

    BAIL_ON_INVALID_POINTER(pInBuf);
    BAIL_ON_INVALID_POINTER(pppszOutMultiSz);

    /* Loop through multistring once to count number of entries */
    pszInString = (PSTR) pInBuf;
    do
    {
        sLen = strlen(pszInString);

        if (sLen)
        {
            pszInString += sLen + 1;
            count++;
        }
    } while (sLen);
    dwError = LwAllocateMemory(sizeof(PCHAR) * (count + 1),
                               (LW_PVOID) &ppszOutMultiSz);
    BAIL_ON_REG_ERROR(dwError);

    /* Loop through multistring again to convert to UTF8 */
    count = 0;
    pszInString = (PSTR) pInBuf;
    do
    {
        dwError = LwAllocateString(pszInString, &pszUTF8);
        BAIL_ON_REG_ERROR(dwError);

        sLen = strlen(pszInString);

        if (sLen)
        {
            ppszOutMultiSz[count++] = pszUTF8;
            pszUTF8 = NULL;
            pszInString += sLen + 1;
        }
    } while (sLen);

    *pppszOutMultiSz = ppszOutMultiSz;

cleanup:
    LW_SAFE_FREE_STRING(pszUTF8);
    return dwError;

error:
    ConvertMultiStrsFree(ppszOutMultiSz);
    goto cleanup;
}


void
ConvertMultiStrsFree(
    PCHAR *pszMultiSz)
{
    DWORD count = 0;

    if (pszMultiSz)
    {
        for (count = 0; pszMultiSz[count]; count++)
        {
            LwFreeMemory(pszMultiSz[count]);
        }
        LwFreeMemory(pszMultiSz);
    }
}


