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
 *        lsamem.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Memory Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
LsaAllocateMemory(
    DWORD dwSize,
    PVOID * ppMemory
    )
{
    DWORD dwError = 0;
    PVOID pMemory = NULL;

    pMemory = malloc(dwSize);
    if (!pMemory){
        dwError = ENOMEM;
        *ppMemory = NULL;
    }else {
        memset(pMemory,0, dwSize);
        *ppMemory = pMemory;
    }

    return dwError;
}

DWORD
LsaReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
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
       dwError = ENOMEM;
       *ppNewMemory = NULL;
    }else {
       *ppNewMemory = pNewMemory;
    }

    return dwError;
}

void
LsaFreeMemory(
    PVOID pMemory
    )
{
    free(pMemory);
}


DWORD
LsaAllocateString(
    PCSTR  pszInputString,
    PSTR* ppszOutputString
    )
{
    DWORD dwError = 0;
    DWORD dwLen = 0;
    PSTR  pszOutputString = NULL;
    
    if (!pszInputString) {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwLen = strlen(pszInputString);
       
    dwError = LsaAllocateMemory(dwLen+1, (PVOID *)&pszOutputString);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwLen) {
       memcpy(pszOutputString, pszInputString, dwLen);
    }
    
    *ppszOutputString = pszOutputString;

cleanup:

    return dwError;

error:
    
    LSA_SAFE_FREE_STRING(pszOutputString);

    *ppszOutputString = NULL;

    goto cleanup;
}

void
LsaFreeString(
    PSTR pszString
    )
{
    LsaFreeMemory(pszString);
}

void
LsaFreeStringArray(
    PSTR * ppStringArray,
    DWORD dwCount
    )
{
    DWORD i;

    if ( ppStringArray ) {
        for(i = 0; i < dwCount; i++)
        {
            if (ppStringArray[i]) {
                LsaFreeString(ppStringArray[i]);
            }
        }

        LsaFreeMemory(ppStringArray);
    }

    return;
}

void
LsaFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    )
{
    PSTR* ppTmp = ppStringArray;
    
    while (ppTmp && *ppTmp) {
          
          LsaFreeString(*ppTmp);
          
          ppTmp++;
    }

    LsaFreeMemory(ppStringArray);
}

DWORD
LsaInitializeStringBuffer(
        LSA_STRING_BUFFER *pBuffer,
        size_t sCapacity)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszBuffer = NULL;

    pBuffer->sLen = 0;
    pBuffer->sCapacity = 0;

    if (sCapacity > DWORD_MAX - 1)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
        sCapacity + 1,
        (PVOID *)&pszBuffer);
    BAIL_ON_LSA_ERROR(dwError);

    pBuffer->pszBuffer = pszBuffer;
    pBuffer->sCapacity = sCapacity;

cleanup:
    return dwError;

error:
    pBuffer->pszBuffer = NULL;

    goto cleanup;
}

DWORD
LsaAppendStringBuffer(
        LSA_STRING_BUFFER *pBuffer,
        PCSTR pszAppend)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
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
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (sNewCapacity < pBuffer->sCapacity)
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaReallocMemory(
            pBuffer->pszBuffer,
            (PVOID *)&pBuffer->pszBuffer,
            sNewCapacity + 1);
        BAIL_ON_LSA_ERROR(dwError);

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
LsaFreeStringBufferContents(
        LSA_STRING_BUFFER *pBuffer)
{
    LSA_SAFE_FREE_MEMORY(pBuffer->pszBuffer);

    pBuffer->sLen = 0;
    pBuffer->sCapacity = 0;
}
