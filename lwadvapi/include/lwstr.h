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
 *        lwstr.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) String Utilities
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */
#ifndef __LWSTR_H__
#define __LWSTR_H__

#define LW_IS_NULL_OR_EMPTY_STR(str) (!(str) || !(*(str)))

#define LW_SAFE_CLEAR_FREE_STRING(str)       \
        do {                                  \
           if (str) {                         \
              if (*str) {                     \
                 memset(str, 0, strlen(str)); \
              }                               \
              LwFreeString(str);             \
              (str) = NULL;                   \
           }                                  \
        } while(0);

#define LW_SAFE_FREE_STRING(str) \
        do {                      \
           if (str) {             \
              LwFreeString(str);  \
              (str) = NULL;       \
           }                      \
        } while(0);


DWORD
LwAllocateString(
    PCSTR  pszInputString,
    PSTR* ppszOutputString
    );

void
LwFreeString(
    PSTR pszString
    );

DWORD
LwAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    );

DWORD
LwAllocateStringPrintfV(
    PSTR*   ppszOutputString,
    PCSTR   pszFormat,
    va_list args
    );

VOID
LwStripLeadingWhitespace(
    PSTR pszString
    );

DWORD
LwStrIsAllSpace(
    PCSTR pszString,
    PBOOLEAN pbIsAllSpace
    );

VOID
LwStripTrailingWhitespace(
    PSTR pszString
    );

VOID
LwStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    );

VOID
LwStrToUpper(
    PSTR pszString
    );

VOID
LwStrnToUpper(
    PSTR  pszString,
    DWORD dwLen
    );

VOID
LwStrToLower(
    PSTR pszString
    );

VOID
LwStrnToLower(
    PSTR  pszString,
    DWORD dwLen
    );

DWORD
LwEscapeString(
    PSTR pszOrig,
    PSTR * ppszEscapedString
    );

DWORD
LwStrndup(
    PCSTR pszInputString,
    size_t size,
    PSTR * ppszOutputString
    );

VOID
LwStrCharReplace(
    PSTR pszStr, 
    CHAR oldCh,
    CHAR newCh
    );

DWORD
LwStrDupOrNull(
    PCSTR pszInputString, 
    PSTR *ppszOutputString
    );

PCSTR
LwEmptyStrForNull(
    PCSTR pszInputString
    );

VOID
LwStrChr(
    PCSTR pszInputString,
    CHAR c,
    PSTR *ppszOutputString
    );

void
LwFreeNullTerminatedStringArray(
    PSTR * ppStringArray
    );

DWORD
LwHexCharToByte(
    CHAR cHexChar,
    UCHAR* pucByte
    );

DWORD
LwHexStrToByteArray(
    IN PCSTR pszHexString,
    IN OPTIONAL DWORD* pdwHexStringLength,
    OUT UCHAR** ppucByteArray,
    OUT DWORD*  pdwByteArrayLength
    );

DWORD
LwMbsToWc16s(
    PCSTR pszInput,
    PWSTR* ppszOutput
    );

DWORD
LwWc16sToMbs(
    PCWSTR pwszInput,
    PSTR*  ppszOutput
    );

#endif
