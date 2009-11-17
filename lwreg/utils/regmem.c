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
 *        regmem.c
 *
 * Abstract:
 *
 *        Registry Memory Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 *
 */
#include "includes.h"

static
DWORD
RegHexCharToByte(
    CHAR cHexChar,
    UCHAR* pucByte
    );

static
void
RegStripLeadingWhitespace(
    PSTR pszString
    );

static
void
RegStripTrailingWhitespace(
    PSTR pszString
    );


#if 0
DWORD
RegAppendAndFreePtrs(
    IN OUT PDWORD pdwDestCount,
    IN OUT PVOID** pppDestPtrArray,
    IN OUT PDWORD pdwAppendCount,
    IN OUT PVOID** pppAppendPtrArray
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwCurrentCount = *pdwDestCount;
    DWORD dwAppendSize = *pdwAppendCount * sizeof(PVOID);
    DWORD dwNewSize = dwCurrentCount * sizeof(PVOID) + dwAppendSize;
    DWORD dwNewCount = dwNewSize / sizeof(PVOID);
    PVOID *ppDestPtrArray = *pppDestPtrArray;

    if (dwNewCount < dwCurrentCount)
    {
        dwError = ERANGE;
        BAIL_ON_REG_ERROR(dwError);
    }

    if (ppDestPtrArray == NULL)
    {
        LW_REG_ASSERT(dwCurrentCount == 0);
        *pppDestPtrArray = *pppAppendPtrArray;
        *pppAppendPtrArray = NULL;
        *pdwDestCount = *pdwAppendCount;
        *pdwAppendCount = 0;
    }
    else
    {
        dwError = RegReallocMemory(
            ppDestPtrArray,
            (PVOID*)&ppDestPtrArray,
            dwNewSize);
        BAIL_ON_REG_ERROR(dwError);
        /* The old pointer was freed and now invalid, so the output parameter
         * needs to be assigned here, even if the rest of the function fails. */
        *pppDestPtrArray = ppDestPtrArray;

        // Append the new data and zero it out from the src array
        memcpy(ppDestPtrArray + dwCurrentCount,
                *pppAppendPtrArray,
                dwAppendSize);
        *pdwDestCount = dwNewCount;
        LWREG_SAFE_FREE_MEMORY(*pppAppendPtrArray);
        *pdwAppendCount = 0;
    }

cleanup:

    return dwError;

error:
    // Leave pppDestPtrArray, pdwDestCount, pdwAppendCount, and
    // pppAppendPtrArray as is, so that the passed in data is not lost.

    goto cleanup;
}
#endif

DWORD
RegInitializeStringBuffer(
    REG_STRING_BUFFER *pBuffer,
    size_t sCapacity)
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    PSTR pszBuffer = NULL;

    pBuffer->sLen = 0;
    pBuffer->sCapacity = 0;

    if (sCapacity > DWORD_MAX - 1)
    {
        dwError = LWREG_ERROR_INVALID_PARAMETER;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = LW_RTL_ALLOCATE((PVOID*)&pszBuffer, CHAR, sCapacity + 1);
    BAIL_ON_REG_ERROR(dwError);

    pBuffer->pszBuffer = pszBuffer;
    pBuffer->sCapacity = sCapacity;

cleanup:
    return dwError;

error:
    pBuffer->pszBuffer = NULL;

    goto cleanup;
}

DWORD
RegAppendStringBuffer(
    REG_STRING_BUFFER *pBuffer,
    PCSTR pszAppend)
{
    DWORD dwError = 0;
    size_t sAppendLen = 0;
    size_t sNewCapacity = 0;

    if (pszAppend != NULL)
        sAppendLen = strlen(pszAppend);

    if (sAppendLen + pBuffer->sLen > pBuffer->sCapacity ||
            pBuffer->pszBuffer == NULL)
    {
        sNewCapacity = (pBuffer->sCapacity + sAppendLen) * 2;

        if (sNewCapacity > DWORD_MAX - 1)
        {
            dwError = LWREG_ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
        }

        if (sNewCapacity < pBuffer->sCapacity)
        {
            dwError = LWREG_ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = RegReallocMemory(
            pBuffer->pszBuffer,
            (PVOID *)&pBuffer->pszBuffer,
            sNewCapacity + 1);
        BAIL_ON_REG_ERROR(dwError);

        pBuffer->sCapacity = sNewCapacity;
    }

    memcpy(
        pBuffer->pszBuffer + pBuffer->sLen,
        pszAppend,
        sAppendLen);
    pBuffer->sLen += sAppendLen;
    pBuffer->pszBuffer[pBuffer->sLen] = '\0';

cleanup:
    return dwError;

error:
    goto cleanup;
}

void
RegFreeStringBufferContents(
    REG_STRING_BUFFER *pBuffer)
{
    LWREG_SAFE_FREE_MEMORY(pBuffer->pszBuffer);

    pBuffer->sLen = 0;
    pBuffer->sCapacity = 0;
}

VOID
RegFreeMemory(
    PVOID pMemory
    )
{
	LwRtlMemoryFree(pMemory);
}

DWORD
RegReallocMemory(
    IN PVOID pMemory,
    OUT PVOID* ppNewMemory,
    IN DWORD dwSize
    )
{
    DWORD dwError = 0;
    PVOID pNewMemory = NULL;

    if (pMemory == NULL) {
       pNewMemory = malloc(dwSize);
       memset(pNewMemory, 0, dwSize);
    }else {
       pNewMemory = realloc(pMemory, dwSize);
    }
    if (!pNewMemory){
       dwError = LWREG_ERROR_OUT_OF_MEMORY;
       *ppNewMemory = NULL;
    }else {
       *ppNewMemory = pNewMemory;
    }

    return dwError;
}

void
RegFreeString(
    PSTR pszString
    )
{
	LwRtlMemoryFree(pszString);
}

void
RegFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    )
{
    DWORD i;

    if ( ppStringArray )
    {
        for(i = 0; i < dwCount; i++)
        {
            if (ppStringArray[i])
            {
		LwRtlMemoryFree(ppStringArray[i]);
            }
        }

        LwRtlMemoryFree(ppStringArray);
    }

    return;
}

DWORD
RegStrndup(
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
        BAIL_ON_REG_ERROR(dwError);
    }

    for (copylen = 0; copylen < size && pszInputString[copylen]; copylen++);

    dwError = LW_RTL_ALLOCATE((PVOID*)&pszOutputString, CHAR, copylen+1);
    BAIL_ON_REG_ERROR(dwError);

    memcpy(pszOutputString, pszInputString, copylen);
    pszOutputString[copylen] = 0;

    *ppszOutputString = pszOutputString;

cleanup:
    return dwError;

error:
    LWREG_SAFE_FREE_STRING(pszOutputString);
    goto cleanup;
}

DWORD
RegHexStrToByteArray(
    IN PCSTR pszHexString,
    IN OPTIONAL DWORD* pdwHexStringLength,
    OUT UCHAR** ppucByteArray,
    OUT DWORD*  pdwByteArrayLength
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwHexChars = 0;
    UCHAR* pucByteArray = NULL;
    DWORD dwByteArrayLength = 0;

    BAIL_ON_INVALID_POINTER(pszHexString);

    if (pdwHexStringLength)
    {
        dwHexChars = *pdwHexStringLength;
    }
    else
    {
        dwHexChars = strlen(pszHexString);
    }
    dwByteArrayLength = dwHexChars / 2;

    if ((dwHexChars & 0x00000001) != 0)
    {
       dwError = LWREG_ERROR_INVALID_PARAMETER;
       BAIL_ON_REG_ERROR(dwError);
    }

    dwError = LW_RTL_ALLOCATE((PVOID*)&pucByteArray, UCHAR,
			          sizeof(*pucByteArray)* dwByteArrayLength);
    BAIL_ON_REG_ERROR(dwError);

    for (i = 0; i < dwByteArrayLength; i++)
    {
        CHAR hexHi = pszHexString[2*i];
        CHAR hexLow = pszHexString[2*i + 1];

        UCHAR ucHi = 0;
        UCHAR ucLow = 0;

        dwError = RegHexCharToByte(hexHi, &ucHi);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegHexCharToByte(hexLow, &ucLow);
        BAIL_ON_REG_ERROR(dwError);

        pucByteArray[i] = (ucHi * 16) + ucLow;
    }

    *ppucByteArray = pucByteArray;
    *pdwByteArrayLength = dwByteArrayLength;

cleanup:

    return dwError;

error:

    LWREG_SAFE_FREE_MEMORY(pucByteArray);
    *ppucByteArray = NULL;
    *pdwByteArrayLength = 0;

    goto cleanup;
}

DWORD
RegByteArrayToHexStr(
    IN UCHAR* pucByteArray,
    IN DWORD dwByteArrayLength,
    OUT PSTR* ppszHexString
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PSTR pszHexString = NULL;

    dwError = LW_RTL_ALLOCATE((PVOID*)&pszHexString, CHAR,
			          sizeof(*pszHexString)* (dwByteArrayLength*2 + 1));
    BAIL_ON_REG_ERROR(dwError);

    for (i = 0; i < dwByteArrayLength; i++)
    {
        sprintf(pszHexString+(2*i), "%.2X", pucByteArray[i]);
    }

    *ppszHexString = pszHexString;

cleanup:

    return dwError;

error:
    LWREG_SAFE_FREE_STRING(pszHexString);

    *ppszHexString = NULL;
    goto cleanup;
}

DWORD
RegStrDupOrNull(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    )
{
    if (pszInputString == NULL)
    {
        *ppszOutputString = NULL;
        return LWREG_ERROR_SUCCESS;
    }
    else
    {
        return LwRtlCStringDuplicate(ppszOutputString, pszInputString);
    }
}

void
RegStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    )
{
    if (LW_IS_NULL_OR_EMPTY_STR(pszString))
        return;

    if (bLeading) {
        RegStripLeadingWhitespace(pszString);
    }

    if (bTrailing) {
        RegStripTrailingWhitespace(pszString);
    }
}

static
DWORD
RegHexCharToByte(
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
       dwError = LWREG_ERROR_INVALID_PARAMETER;
       BAIL_ON_REG_ERROR(dwError);
    }

    *pucByte = ucByte;

cleanup:

    return dwError;

error:

    *pucByte = 0;

    goto cleanup;
}

static
void
RegStripLeadingWhitespace(
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

static
void
RegStripTrailingWhitespace(
    PSTR pszString
    )
{
    PSTR pszLastSpace = NULL;
    PSTR pszTmp = pszString;

    if (LW_IS_NULL_OR_EMPTY_STR(pszString))
        return;

    while (pszTmp != NULL && *pszTmp != '\0') {
        pszLastSpace = (isspace((int)*pszTmp) ? (pszLastSpace ? pszLastSpace : pszTmp) : NULL);
        pszTmp++;
    }

    if (pszLastSpace != NULL) {
        *pszLastSpace = '\0';
    }
}
