/*
 * Copyright Likewise Software    2004-2009
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
 *        string.c
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        Utilities
 *
 *        Strings
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SrvMbsToWc16s(
    IN  PCSTR  pszString,
    OUT PWSTR* ppwszString
    )
{
    return LwRtlWC16StringAllocateFromCString(ppwszString, pszString);
}

NTSTATUS
SrvWc16sToMbs(
    IN  PCWSTR pwszString,
    OUT PSTR*  ppszString
    )
{
    return LwRtlCStringAllocateFromWC16String(ppszString, pwszString);
}

NTSTATUS
SrvAllocateStringW(
    PWSTR  pwszInputString,
    PWSTR* ppwszOutputString
    )
{
    return LwRtlWC16StringDuplicate(ppwszOutputString, pwszInputString);
}

NTSTATUS
SrvAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    va_list args;

    va_start(args, pszFormat);

    ntStatus = RtlCStringAllocatePrintfV(
                      ppszOutputString,
                      pszFormat,
                      args);

    va_end(args);

    return ntStatus;
}

/**
 * @brief Get the hex dump of the bytes passed in
 *
 * @param[in] pBuffer                Bytes to be dumped
 * @param[in] ulBufLen               Length of bytes to be dumped
 * @param[in] ulMaxLength            Maximum number of bytes to be dumped
 * @param[out] ppszHexString         Hex string of the bytes to be dumped
 * @param[in,out] pulHexStringLength Number of actual bytes dumped
 *
 */
NTSTATUS
SrvGetHexDump(
    PBYTE  pBuffer,
    ULONG  ulBufLen,
    ULONG  ulMaxLength,
    PSTR*  ppszHexString,
    PULONG pulHexStringLength
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR     pszHexString = NULL;
    ULONG    ulLen = 0;

    if ((ulLen = SMB_MIN(ulMaxLength, ulBufLen)) > 0) // hex dump
    {
        PSTR  pszHexCursor = NULL;
        PBYTE pBufCursor = pBuffer;
        ULONG i = 0;
        ULONG j = 0;

        size_t sBufferLen =
                    (ulLen * 2)+     /* 2  hex chars per byte   */
                    ((ulLen/16)*17)+ /* 17 extra chars per line */
                    (ulLen%16)+ 1;

        ntStatus = SrvAllocateMemory(sBufferLen, (PVOID*)&pszHexString);
        BAIL_ON_NT_STATUS(ntStatus);

        pszHexCursor = pszHexString;

        for (; i < ulLen; i++, pBufCursor++)
        {
            CHAR hexChar[] = {'0','1','2','3','4','5','6','7','8',
                              '9','A','B','C','D','E','F'};

            if (j != 0)
            {
                *pszHexCursor++ = ' ';
            }
            if (j == 8)
            {
                *pszHexCursor++ = ' ';
            }

            *pszHexCursor++ = hexChar[*pBufCursor & 0x0F];
            *pszHexCursor++ = hexChar[(*pBufCursor & 0xF0) >> 4];

            if (j++ == 15)
            {
                *pszHexCursor++ = '^';
                j = 0;
            }
        }
    }

    *ppszHexString      = pszHexString;
    *pulHexStringLength = ulLen;

cleanup:

    return ntStatus;

error:

    *ppszHexString      = NULL;
    *pulHexStringLength = 0;

    SRV_SAFE_FREE_MEMORY(pszHexString);

    goto cleanup;
}
