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

#ifndef _WIN32

#define PATH_SEPARATOR_STR "/"

#else

#define PATH_SEPARATOR_STR "\\"

#endif

typedef DWORD (*PFN_SRVSVC_FOREACH_STACK_ITEM)(
                    PVOID pItem,
                    PVOID pUserData
                    );

typedef struct __SRVSVC_STACK
{
    PVOID pItem;

    struct __SRVSVC_STACK * pNext;

} SRVSVC_STACK, *PSRVSVC_STACK;

/*
 * Config parsing callbacks
 */
typedef DWORD (*PFNCONFIG_START_SECTION)(
                        PCSTR    pszSectionName,
                        PBOOLEAN pbSkipSection,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNCONFIG_COMMENT)(
                        PCSTR    pszComment,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNCONFIG_NAME_VALUE_PAIR)(
                        PCSTR    pszName,
                        PCSTR    pszValue,
                        PBOOLEAN pbContinue
                        );

typedef DWORD (*PFNCONFIG_END_SECTION)(
                        PCSTR pszSectionName,
                        PBOOLEAN pbContinue
                        );

typedef struct _LOGFILEINFO {
    CHAR szLogPath[PATH_MAX+1];
    FILE* logHandle;
} LOGFILEINFO, *PLOGFILEINFO;

typedef struct _SYSLOGINFO {
    CHAR szIdentifier[PATH_MAX+1];
    DWORD dwOption;
    DWORD dwFacility;
} SYSLOGINFO, *PSYSLOGINFO;

typedef struct _LOGINFO {
    pthread_mutex_t lock;
    DWORD dwLogLevel;
    DWORD logTarget;
    union _logdata {
        LOGFILEINFO logfile;
        SYSLOGINFO syslog;
    } data;
    BOOLEAN  bLoggingInitiated;
} LOGINFO, *PLOGINFO;

DWORD
SrvSvcAllocateMemory(
    DWORD  dwSize,
    PVOID* ppMemory
    );

DWORD
SrvSvcReallocMemory(
    PVOID  pMemory,
    PVOID* ppNewMemory,
    DWORD  dwSize
    );

void
SrvSvcSrvFreeMemory(
    PVOID pMemory
    );

DWORD
SrvSvcAllocateString(
    PCSTR pszInputString,
    PSTR *ppszOutputString
    );

VOID
SrvSvcFreeString(
    PSTR pszString
    );

DWORD
SrvSvcStrndup(
    PCSTR  pszInputString,
    size_t size,
    PSTR * ppszOutputString
    );

PCSTR
TableCategoryToStr(
    DWORD tableCategory
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
SrvSvcEscapeString(
    PSTR pszOrig,
    PSTR * ppszEscapedString
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
SrvSvcStackPush(
    PVOID pItem,
    PSRVSVC_STACK* ppStack
    );

PVOID
SrvSvcStackPop(
    PSRVSVC_STACK* ppStack
    );

PVOID
SrvSvcStackPeek(
    PSRVSVC_STACK pStack
    );

DWORD
SrvSvcStackForeach(
    PSRVSVC_STACK pStack,
    PFN_SRVSVC_FOREACH_STACK_ITEM pfnAction,
    PVOID pUserData
    );

PSRVSVC_STACK
SrvSvcStackReverse(
    PSRVSVC_STACK pStack
    );

VOID
SrvSvcStackFree(
    PSRVSVC_STACK pStack
    );

DWORD
SrvSvcParseConfigFile(
    PCSTR                     pszFilePath,
    PFNCONFIG_START_SECTION   pfnStartSectionHandler,
    PFNCONFIG_COMMENT         pfnCommentHandler,
    PFNCONFIG_NAME_VALUE_PAIR pfnNameValuePairHandler,
    PFNCONFIG_END_SECTION     pfnEndSectionHandler
    );

DWORD
SrvSvcParseDays(
    PCSTR  pszTimeInterval,
    PDWORD pdwTimeInterval
    );

DWORD
SrvSvcParseDiskUsage(
    PCSTR  pszDiskUsage,
    PDWORD pdwDiskUsage
    );

DWORD
SrvSvcParseMaxEntries(
    PCSTR  pszMaxEntries,
    PDWORD pdwMaxEntries
    );

DWORD
SrvSvcRemoveFile(
    PCSTR pszPath
    );

DWORD
SrvSvcCheckFileExists(
    PCSTR    pszPath,
    PBOOLEAN pbFileExists
    );

DWORD
SrvSvcMoveFile(
    PCSTR pszSrcPath,
    PCSTR pszDstPath
    );

DWORD
SrvSvcChangePermissions(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
SrvSvcChangeOwner(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid
    );

DWORD
SrvSvcChangeOwnerAndPermissions(
    PCSTR pszPath,
    uid_t uid,
    gid_t gid,
    mode_t dwFileMode
    );

DWORD
SrvSvcGetCurrentDirectoryPath(
    PSTR* ppszPath
    );

DWORD
SrvSvcChangeDirectory(
    PCSTR pszPath
    );

DWORD
SrvSvcRemoveDirectory(
    PCSTR pszPath
    );

DWORD
SrvSvcGetFileSize(
	PCSTR pszPath,
	PDWORD pdwFileSize
	);

DWORD
SrvSvcCheckDirectoryExists(
    PCSTR pszPath,
    PBOOLEAN pbDirExists
    );

DWORD
SrvSvcCreateDirectory(
    PCSTR pszPath,
    mode_t dwFileMode
    );

DWORD
SrvSvcGetHostname(
    PSTR* ppszHostname
    );

extern FILE*   gBasicLogStreamFD;
extern DWORD   gLogLevel;
extern LOGINFO gSrvSvcLogInfo;

VOID
SrvSvcLogMessage(
    DWORD dwLogLevel,
    PCSTR pszFormat,
    ...
    );

DWORD
SrvSvcInitLoggingToSyslog(
    DWORD dwLogLevel,
    PCSTR pszIdentifier,
    DWORD dwOption,
    DWORD dwFacility
    );

DWORD
SrvSvcSetLogLevel(
    DWORD dwLogLevel
    );

DWORD
SrvSvcInitLoggingToFile(
    DWORD dwLogLevel,
    PCSTR pszLogFilePath
    );

VOID
SrvSvcCloseLog(
    VOID
    );


#define IsNullOrEmptyString(pszStr)     \
    (pszStr == NULL || *pszStr == '\0')



#endif /* __SRVSVCUTILS_H__ */
