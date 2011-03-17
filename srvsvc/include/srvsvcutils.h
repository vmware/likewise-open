/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        srvsvcutils.h
 *
 * Abstract:
 *
 *        Likewise Server Service Service (LWSRVSVC)
 *
 *        Public header
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __SRVSVCUTILS_H__
#define __SRVSVCUTILS_H__

#include <lw/rtllog.h>


#define IsNullOrEmptyString(pszStr)     \
    (pszStr == NULL || *pszStr == '\0')

#define SRVSVC_SAFE_FREE_STRING(str) \
        do {                      \
           if (str) {             \
              SrvSvcFreeString(str); \
              (str) = NULL;       \
           }                      \
        } while(0);

/*
 * Logging
 */

typedef enum
{
    SRVSVC_LOG_LEVEL_ALWAYS = LW_RTL_LOG_LEVEL_ALWAYS,
    SRVSVC_LOG_LEVEL_ERROR = LW_RTL_LOG_LEVEL_ERROR,
    SRVSVC_LOG_LEVEL_WARNING = LW_RTL_LOG_LEVEL_WARNING,
    SRVSVC_LOG_LEVEL_INFO = LW_RTL_LOG_LEVEL_INFO,
    SRVSVC_LOG_LEVEL_VERBOSE = LW_RTL_LOG_LEVEL_VERBOSE,
    SRVSVC_LOG_LEVEL_DEBUG = LW_RTL_LOG_LEVEL_DEBUG,
    SRVSVC_LOG_LEVEL_TRACE = LW_RTL_LOG_LEVEL_TRACE
} SRVSVC_LOG_LEVEL;

#define SRVSVC_SAFE_LOG_STRING(x) \
  LW_RTL_LOG_SAFE_STRING(x)

#define SRVSVC_LOG_ALWAYS(Format, ...) \
    LW_RTL_LOG_ALWAYS(Format, ## __VA_ARGS__)

#define SRVSVC_LOG_ERROR(Format, ...) \
    LW_RTL_LOG_ERROR(Format, ## __VA_ARGS__)

#define SRVSVC_LOG_WARNING(Format, ...) \
    LW_RTL_LOG_WARNING(Format, ## __VA_ARGS__)

#define SRVSVC_LOG_INFO(Format, ...) \
    LW_RTL_LOG_INFO(Format, ## __VA_ARGS__)

#define SRVSVC_LOG_VERBOSE(Format, ...) \
    LW_RTL_LOG_VERBOSE(Format, ## __VA_ARGS__)

#define SRVSVC_LOG_DEBUG(Format, ...) \
    LW_RTL_LOG_DEBUG(Format, ## __VA_ARGS__)

#define SRVSVC_LOG_TRACE(Format, ...) \
    LW_RTL_LOG_TRACE(Format, ## __VA_ARGS__)


DWORD
SrvSvcSrvAllocateMemory(
    DWORD  dwSize,
    PVOID* ppMemory
    );

void
SrvSvcSrvFreeMemory(
    PVOID pMemory
    );

DWORD
SrvSvcStrndup(
    PCSTR  pszInputString,
    size_t size,
    PSTR * ppszOutputString
    );

VOID
SrvSvcFreeString(
    PSTR pszString
    );

BOOLEAN
SrvSvcIsWhiteSpace(
    char c
    );

/*
 * Modify PSTR in-place to convert sequences
 * of whitespace characters into
 * single spaces (0x20)
 */
DWORD
SrvSvcCompressWhitespace(
    PSTR pszString
    );

/*
 * Convert a 16-bit string to an 8-bit string
 * Allocate new memory in the process
 */
DWORD
SrvSvcLpwStrToLpStr(
    PCWSTR pszwString,
    PSTR*  ppszString
    );

VOID
SrvSvcStripWhitespace(
    PSTR pszString,
    BOOLEAN bLeading,
    BOOLEAN bTrailing
    );

VOID
SrvSvcStrToUpper(
    PSTR pszString
    );

VOID
SrvSvcStrToLower(
    PSTR pszString
    );

DWORD
SrvSvcAllocateStringPrintf(
    PSTR* ppszOutputString,
    PCSTR pszFormat,
    ...
    );

DWORD
SrvSvcAllocateStringPrintfV(
    PSTR*   ppszOutputString,
    PCSTR   pszFormat,
    va_list args
    );

DWORD
SrvSvcAllocateString(
    PCSTR  pszInputString,
    PSTR*  ppszOutputString
    );


#endif /* __SRVSVCUTILS_H__ */
