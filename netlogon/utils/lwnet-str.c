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
 *        lwnet-str.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 *
 *        String Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#include "includes.h"

#if !HAVE_DECL_ISBLANK
int isblank(int c)
{
    return c == '\t' || c == ' ';
}
#endif

#if !defined(HAVE_STRTOLL)

long long int
strtoll(
    const char* nptr,
    char**      endptr,
    int         base
    )
{
#if defined(HAVE___STRTOLL)
    return __strtoll(nptr, endptr, base);
#else
#error strtoll support is not available
#endif
}

#endif /* defined(HAVE_STRTOLL) */

#if !defined(HAVE_STRTOULL)

unsigned long long int
strtoull(
    const char* nptr,
    char**      endptr,
    int         base
    )
{
#if defined(HAVE___STRTOULL)
    return __strtoull(nptr, endptr, base);
#else
#error strtoull support is not available
#endif
}

#endif /* defined(HAVE_STRTOULL) */


DWORD
LWNetAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    )
{
    DWORD dwError = 0;
    va_list args;
    
    va_start(args, pszFormat);
    
    dwError = LWNetAllocateStringPrintfV(
                      ppszOutputString,
                      pszFormat,
                      args);
    
    va_end(args);

    return dwError;
}

DWORD
LWNetAllocateStringPrintfV(
    PSTR*   ppszOutputString,
    PCSTR   pszFormat,
    va_list args
    )
{
    DWORD dwError = 0;
    PSTR  pszSmallBuffer = NULL;
    DWORD dwBufsize = 0;
    INT   requiredLength = 0;
    DWORD dwNewRequiredLength = 0;
    PSTR  pszOutputString = NULL;
    va_list args2;

    va_copy(args2, args);

    dwBufsize = 4;
    /* Use a small buffer in case libc does not like NULL */
    do
    {
        dwError = LWNetAllocateMemory(
                        dwBufsize, 
                        (PVOID*) &pszSmallBuffer);
        BAIL_ON_LWNET_ERROR(dwError);
        
        requiredLength = vsnprintf(
                              pszSmallBuffer,
                              dwBufsize,
                              pszFormat,
                              args);
        if (requiredLength < 0)
        {
            dwBufsize *= 2;
        }
        LWNetFreeMemory(pszSmallBuffer);
        pszSmallBuffer = NULL;
        
    } while (requiredLength < 0);

    if (requiredLength >= (UINT32_MAX - 1))
    {
        dwError = ENOMEM;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetAllocateMemory(
                    requiredLength + 2,
                    (PVOID*)&pszOutputString);
    BAIL_ON_LWNET_ERROR(dwError);

    dwNewRequiredLength = vsnprintf(
                            pszOutputString,
                            requiredLength + 1,
                            pszFormat,
                            args2);
    if (dwNewRequiredLength < 0)
    {
        dwError = errno;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    else if (dwNewRequiredLength > requiredLength)
    {
        /* unexpected, ideally should log something, or use better error code */
        dwError = ENOMEM;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    else if (dwNewRequiredLength < requiredLength)
    {
        /* unexpected, ideally should log something -- do not need an error, though */
    }
    
    *ppszOutputString = pszOutputString;

cleanup:

    va_end(args2);
    
    return dwError;
    
error:

    LWNET_SAFE_FREE_MEMORY(pszOutputString);
    
    *ppszOutputString = NULL;

    goto cleanup;
}

void
LWNetStripLeadingWhitespace(
    PSTR pszString
)
{
    PSTR pszNew = pszString;
    PSTR pszTmp = pszString;

    if (pszString == NULL || *pszString == '\0' || !isspace((int)*pszString)) {
        return;
    }

    while (pszTmp != NULL && *pszTmp != '\0' && isspace((int)*pszTmp)) {
        pszTmp++;
    }

    while (pszTmp != NULL && *pszTmp != '\0') {
        *pszNew++ = *pszTmp++;
    }
    *pszNew = '\0';
}


DWORD
LWNetStrIsAllSpace(
    PCSTR pszString,
    PBOOLEAN pbIsAllSpace
    )
{
    DWORD dwError = 0;
    PCSTR pszTmp = NULL;
    BOOLEAN bIsAllSpace = TRUE;
    
    BAIL_ON_INVALID_POINTER(pszString);
    
    for (pszTmp = pszString; *pszTmp; pszTmp++)
    {
        if (!isspace((int)*pszTmp))
        {
            bIsAllSpace = FALSE;
            break;
        }
    }
    
    
    *pbIsAllSpace = bIsAllSpace;
    
cleanup:

    return dwError;

error:

    *pbIsAllSpace = FALSE;
    goto cleanup;
}

void
LWNetStripTrailingWhitespace(
    PSTR pszString
)
{
    PSTR pszLastSpace = NULL;
    PSTR pszTmp = pszString;

    if (IsNullOrEmptyString(pszString))
        return;

    while (pszTmp != NULL && *pszTmp != '\0') {
        pszLastSpace = (isspace((int)*pszTmp) ? (pszLastSpace ? pszLastSpace : pszTmp) : NULL);
        pszTmp++;
    }

    if (pszLastSpace != NULL) {
        *pszLastSpace = '\0';
    }
}

void
LWNetStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
)
{
    if (IsNullOrEmptyString(pszString))
        return;

    if (bLeading) {
        LWNetStripLeadingWhitespace(pszString);
    }

    if (bTrailing) {
        LWNetStripTrailingWhitespace(pszString);
    }
}

void
LWNetStrToUpper(
    PSTR pszString
)
{
    if (IsNullOrEmptyString(pszString))
        return;

    while (*pszString != '\0') {
        *pszString = toupper(*pszString);
        pszString++;
    }
}

void
LWNetStrToLower(
    PSTR pszString
)
{
    if (IsNullOrEmptyString(pszString))
        return;

    while (*pszString != '\0') {
        *pszString = tolower(*pszString);
        pszString++;
    }
}

DWORD
LWNetEscapeString(
    PSTR pszOrig,
    PSTR * ppszEscapedString
)
{
    DWORD dwError = 0;
    int nQuotes = 0;
    PSTR pszTmp = pszOrig;
    PSTR pszNew = NULL;
    PSTR pszNewTmp = NULL;

    if ( !ppszEscapedString || !pszOrig ) {
        dwError = EINVAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    while(pszTmp && *pszTmp)
    {
        if (*pszTmp=='\'') {
            nQuotes++;
        }
        pszTmp++;
    }

    if (!nQuotes) {
        dwError = LWNetAllocateString(pszOrig, &pszNew);
        BAIL_ON_LWNET_ERROR(dwError);
    } else {
        /*
         * We are going to escape each single quote and enclose it in two other
         * single-quotes
         */
        dwError = LWNetAllocateMemory( strlen(pszOrig)+3*nQuotes+1, (PVOID*)&pszNew );
        BAIL_ON_LWNET_ERROR(dwError);

        pszTmp = pszOrig;
        pszNewTmp = pszNew;

        while(pszTmp && *pszTmp)
        {
            if (*pszTmp=='\'') {
                *pszNewTmp++='\'';
                *pszNewTmp++='\\';
                *pszNewTmp++='\'';
                *pszNewTmp++='\'';
                pszTmp++;
            }
            else {
                *pszNewTmp++ = *pszTmp++;
            }
        }
        *pszNewTmp = '\0';
    }

    *ppszEscapedString = pszNew;

cleanup:

    return dwError;

error:

    LWNET_SAFE_FREE_MEMORY(pszNew);

    *ppszEscapedString = NULL;

    goto cleanup;
}

DWORD
LWNetStrndup(
    PCSTR pszInputString,
    size_t size,
    PSTR * ppszOutputString
)
{
    DWORD dwError = 0;
    size_t copylen = 0;
    PSTR pszOutputString = NULL;

    if (!pszInputString || !ppszOutputString){
        dwError = EINVAL;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    copylen = strlen(pszInputString);
    if (copylen > size)
        copylen = size;

    dwError = LWNetAllocateMemory(copylen+1, (PVOID *)&pszOutputString);
    BAIL_ON_LWNET_ERROR(dwError);

    memcpy(pszOutputString, pszInputString, copylen);
    pszOutputString[copylen] = 0;

    *ppszOutputString = pszOutputString;
    
cleanup:
    return dwError;
    
error:
    LWNET_SAFE_FREE_STRING(pszOutputString);
    goto cleanup;
}

DWORD
LWNetMbsToWc16s(
    PCSTR pszInput,
    PWSTR* ppszOutput
    )
{
    DWORD dwError = 0;
    PWSTR pszOutput = NULL;
    
    if (!pszInput) {
        dwError = LWNET_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    pszOutput = ambstowc16s(pszInput);
    if (!pszOutput) {
        dwError = LWNET_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    *ppszOutput = pszOutput;
    
cleanup:

    return dwError;
    
error:
    
    *ppszOutput = NULL;

    goto cleanup;
}

DWORD
LWNetWc16sToMbs(
    PCWSTR pwszInput,
    PSTR*  ppszOutput
    )
{
    DWORD dwError = 0;
    PSTR pszOutput = NULL;
    
    if (!pwszInput) {
        dwError = LWNET_ERROR_INVALID_PARAMETER;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    pszOutput = awc16stombs(pwszInput);
    if (!pszOutput) {
        dwError = LWNET_ERROR_STRING_CONV_FAILED;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    *ppszOutput = pszOutput;
    
cleanup:

    return dwError;
    
error:

    *ppszOutput = NULL;

    goto cleanup;
}

VOID
LWNetStrCharReplace(
    PSTR pszStr, 
    CHAR oldCh,
    CHAR newCh)
{
    if (oldCh != newCh)
    {
        while (pszStr && *pszStr)
        {
            if (*pszStr == oldCh){
                *pszStr = newCh;
            }
            pszStr++;
        }
    }
}

DWORD
LWNetStrDupOrNull(
    PCSTR pszInputString, 
    PSTR *ppszOutputString
    )
{
    if (pszInputString == NULL)
    {
        *ppszOutputString = NULL;
        return LWNET_ERROR_SUCCESS;
    }
    else
    {
        return LWNetAllocateString(pszInputString, ppszOutputString);
    }
}


PCSTR
LWNetEmptyStrForNull(
    PCSTR pszInputString
    )
{
    if (pszInputString == NULL)
        return "";
    else
        return pszInputString;
}


DWORD
LWNetHexStrToByteArray(
    PCSTR   pszHexString,
    UCHAR** ppucByteArray,
    DWORD*  pdwByteArrayLength
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwHexChars = strlen(pszHexString);
    UCHAR* pucByteArray = NULL;
    DWORD dwByteArrayLength = dwHexChars / 2;
    
    if ((dwHexChars & 0x00000001) != 0) 
    {
       dwError = LWNET_ERROR_INVALID_PARAMETER;
       BAIL_ON_LWNET_ERROR(dwError);
    }
    
    dwError = LWNetAllocateMemory(
                  sizeof(UCHAR)*(dwByteArrayLength), 
                  (PVOID*)&pucByteArray
                  );
    BAIL_ON_LWNET_ERROR(dwError);
    
    for (i = 0; i < dwByteArrayLength; i++)
    {
        CHAR hexHi = pszHexString[2*i];
        CHAR hexLow = pszHexString[2*i + 1];
        
        UCHAR ucHi = 0;
        UCHAR ucLow = 0;
      
        dwError = LWNetHexCharToByte(hexHi, &ucHi);
        BAIL_ON_LWNET_ERROR(dwError);
      
        dwError = LWNetHexCharToByte(hexLow, &ucLow);
        BAIL_ON_LWNET_ERROR(dwError);
      
        pucByteArray[i] = (ucHi * 16) + ucLow;
    }
    
    *ppucByteArray = pucByteArray;
    *pdwByteArrayLength = dwByteArrayLength;
    
cleanup:
    
    return dwError;

error:
    
    LWNET_SAFE_FREE_MEMORY(pucByteArray);
    *ppucByteArray = NULL;
    *pdwByteArrayLength = 0;
    
    goto cleanup;
}

DWORD
LWNetByteArrayToHexStr(
    UCHAR* pucByteArray,
    DWORD dwByteArrayLength,
    PSTR* ppszHexString
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR pszHexString = NULL;
    
    dwError = LWNetAllocateMemory(
                (dwByteArrayLength*2 + 1) * sizeof(CHAR), 
                (PVOID*)&pszHexString);
    BAIL_ON_LWNET_ERROR(dwError);
    
    for (i = 0; i < dwByteArrayLength; i++)
    {
        sprintf((char*)pszHexString+(2*i), "%.2X", pucByteArray[i]);
    }
    
    *ppszHexString = pszHexString;
    
cleanup:

    return dwError;

error:

    *ppszHexString = NULL;
    goto cleanup;
}


DWORD
LWNetHexCharToByte(
    CHAR cHexChar,
    UCHAR* pucByte
    )
{
    DWORD dwError = 0;
    UCHAR ucByte = 0;
    
    if (cHexChar >= '0' && cHexChar <= '9')
    {
       ucByte = (UCHAR)(cHexChar - '0');
    }
    else if (cHexChar >= 'a' && cHexChar <= 'f')
    {
       ucByte = 10 + (UCHAR)(cHexChar - 'a');
    }
    else if (cHexChar >= 'A' && cHexChar <= 'F')
    {
       ucByte = 10 + (UCHAR)(cHexChar - 'A');
    }
    else 
    {
       dwError = LWNET_ERROR_INVALID_PARAMETER;
       BAIL_ON_LWNET_ERROR(dwError);
    }
    
    *pucByte = ucByte;
    
cleanup:

    return dwError;

error:
    
    *pucByte = 0;

    goto cleanup;
}

