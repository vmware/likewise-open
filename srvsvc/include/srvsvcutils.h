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
    SRVSVC_LOG_LEVEL_ALWAYS = 0,
    SRVSVC_LOG_LEVEL_ERROR,
    SRVSVC_LOG_LEVEL_WARNING,
    SRVSVC_LOG_LEVEL_INFO,
    SRVSVC_LOG_LEVEL_VERBOSE,
    SRVSVC_LOG_LEVEL_DEBUG
} SRVSVC_LOG_LEVEL;

typedef enum
{
    SRVSVC_LOG_TARGET_DISABLED = 0,
    SRVSVC_LOG_TARGET_CONSOLE,
    SRVSVC_LOG_TARGET_FILE,
    SRVSVC_LOG_TARGET_SYSLOG
} SRVSVC_LOG_TARGET;

typedef VOID (*PFN_SRVSVC_LOG_MESSAGE)(
                            HANDLE           hLog,
                            SRVSVC_LOG_LEVEL logLevel,
                            PCSTR            pszFormat,
                            va_list          msgList
                            );

typedef struct _SRVSVC_LOG_INFO
{
    SRVSVC_LOG_LEVEL  maxAllowedLogLevel;
    SRVSVC_LOG_TARGET logTarget;
    PSTR              pszPath;
} SRVSVC_LOG_INFO, *PSRVSVC_LOG_INFO;

#if defined(LW_ENABLE_THREADS)

extern pthread_mutex_t gSrvSvcLogLock;

#define SRVSVC_LOCK_LOGGER   pthread_mutex_lock(&gSrvSvcLogLock)
#define SRVSVC_UNLOCK_LOGGER pthread_mutex_unlock(&gSrvSvcLogLock)

#define _SRVSVC_LOG_PREFIX_THREAD(Format) \
    "0x%lx:" Format, ((unsigned long)pthread_self())

#else

#define SRVSVC_LOCK_LOGGER
#define SRVSVC_UNLOCK_LOGGER

#define _SRVSVC_LOG_PREFIX_THREAD(Format) \
    Format

#endif

#define _SRVSVC_LOG_PREFIX_LOCATION(Format, Function, File, Line) \
    _SRVSVC_LOG_PREFIX_THREAD("[%s() %s:%d] " Format), \
    (Function), \
    (File), \
    (Line)

#define _SRVSVC_LOG_WITH_THREAD(Level, Format, ...) \
    _SRVSVC_LOG_MESSAGE(Level, \
                      _SRVSVC_LOG_PREFIX_THREAD(Format), \
                      ## __VA_ARGS__)

#define _SRVSVC_LOG_WITH_LOCATION(Level, Format, Function, File, Line, ...) \
    _SRVSVC_LOG_MESSAGE(Level, \
                  _SRVSVC_LOG_PREFIX_LOCATION(Format, Function, File, Line), \
                  ## __VA_ARGS__)

#define _SRVSVC_LOG_WITH_DEBUG(Level, Format, ...) \
    _SRVSVC_LOG_WITH_LOCATION(Level, Format, \
                            __FUNCTION__, __FILE__, __LINE__, \
                            ## __VA_ARGS__)

extern HANDLE                 ghSrvSvcLog;
extern SRVSVC_LOG_LEVEL       gSrvSvcMaxLogLevel;
extern PFN_SRVSVC_LOG_MESSAGE gpfnSrvSvcLogger;

#define _SRVSVC_LOG_MESSAGE(Level, Format, ...) \
    SrvSvcLogMessage(gpfnSrvSvcLogger, ghSrvSvcLog, Level, Format, ## __VA_ARGS__)

#define _SRVSVC_LOG_IF(Level, Format, ...)                              \
    do {                                                                \
        SRVSVC_LOCK_LOGGER;                                             \
        if (gpfnSrvSvcLogger && (gSrvSvcMaxLogLevel >= (Level)))        \
        {                                                               \
            if (gSrvSvcMaxLogLevel >= SRVSVC_LOG_LEVEL_DEBUG)           \
            {                                                           \
                _SRVSVC_LOG_WITH_DEBUG(Level, Format, ## __VA_ARGS__);  \
            }                                                           \
            else                                                        \
            {                                                           \
                _SRVSVC_LOG_WITH_THREAD(Level, Format, ## __VA_ARGS__); \
            }                                                           \
        }                                                               \
        SRVSVC_UNLOCK_LOGGER;                                           \
    } while (0)

#define SRVSVC_SAFE_LOG_STRING(x) \
    ( (x) ? (x) : "<null>" )

#define SRVSVC_LOG_ALWAYS(szFmt, ...) \
    _SRVSVC_LOG_IF(SRVSVC_LOG_LEVEL_ALWAYS, szFmt, ## __VA_ARGS__)

#define SRVSVC_LOG_ERROR(szFmt, ...) \
    _SRVSVC_LOG_IF(SRVSVC_LOG_LEVEL_ERROR, szFmt, ## __VA_ARGS__)

#define SRVSVC_LOG_WARNING(szFmt, ...) \
    _SRVSVC_LOG_IF(SRVSVC_LOG_LEVEL_WARNING, szFmt, ## __VA_ARGS__)

#define SRVSVC_LOG_INFO(szFmt, ...) \
    _SRVSVC_LOG_IF(SRVSVC_LOG_LEVEL_INFO, szFmt, ## __VA_ARGS__)

#define SRVSVC_LOG_VERBOSE(szFmt, ...) \
    _SRVSVC_LOG_IF(SRVSVC_LOG_LEVEL_VERBOSE, szFmt, ## __VA_ARGS__)

#define SRVSVC_LOG_DEBUG(szFmt, ...) \
    _SRVSVC_LOG_IF(SRVSVC_LOG_LEVEL_DEBUG, szFmt, ## __VA_ARGS__)

#define SRVSVC_LOG_TRACE(szFmt, ...) \
    _SRVSVC_LOG_IF(SRVSVC_LOG_LEVEL_TRACE, szFmt, ## __VA_ARGS__)

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

DWORD
SrvSvcInitLogging(
    PCSTR             pszProgramName,
    SRVSVC_LOG_TARGET logTarget,
    SRVSVC_LOG_LEVEL  maxAllowedLogLevel,
    PCSTR             pszPath
    );

DWORD
SrvSvcLogGetInfo(
    PSRVSVC_LOG_INFO* ppLogInfo
    );

DWORD
SrvSvcLogSetInfo(
    PSRVSVC_LOG_INFO pLogInfo
    );

VOID
SrvSvcLogMessage(
    PFN_SRVSVC_LOG_MESSAGE pfnLogger,
    HANDLE                 hLog,
    SRVSVC_LOG_LEVEL       logLevel,
    PCSTR                  pszFormat,
    ...
    );

DWORD
SrvSvcShutdownLogging(
    VOID
    );

DWORD
SrvSvcValidateLogLevel(
    DWORD dwLogLevel
    );

DWORD
SrvSvcConfigGetLsaLpcSocketPath(
    PSTR* ppszPath
    );

#endif /* __SRVSVCUTILS_H__ */
