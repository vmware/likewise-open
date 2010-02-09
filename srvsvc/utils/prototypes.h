/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise Server Service Services
 *
 *        System Functions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

// consolelog.c

DWORD
SrvSvcOpenConsoleLog(
    SRVSVC_LOG_LEVEL maxAllowedLogLevel,
    PHANDLE     phLog
    );

DWORD
SrvSvcCloseConsoleLog(
    HANDLE hLog
    );

VOID
SrvSvcLogToConsole(
    HANDLE      hLog,
    SRVSVC_LOG_LEVEL logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    );

VOID
SrvSvcFreeConsoleLogInfo(
    PSRVSVC_CONSOLE_LOG pConsoleLog
    );

// filelog.c

DWORD
SrvSvcOpenFileLog(
    PCSTR            pszFilePath,
    SRVSVC_LOG_LEVEL maxAllowedLogLevel,
    PHANDLE          phLog
    );

DWORD
SrvSvcGetFileLogInfo(
    HANDLE hLog,
    PSRVSVC_LOG_INFO* ppLogInfo
    );

DWORD
SrvSvcCloseFileLog(
    HANDLE hLog
    );

VOID
SrvSvcLogToFile(
    HANDLE      hLog,
    SRVSVC_LOG_LEVEL logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    );

VOID
SrvSvcFreeFileLogInfo(
    PSRVSVC_FILE_LOG pFileLog
    );

// logger.c

DWORD
SrvSvcSetupLogging(
    HANDLE                 hLog,
    SRVSVC_LOG_LEVEL       maxAllowedLogLevel,
    PFN_SRVSVC_LOG_MESSAGE pfnLogger
    );

VOID
SrvSvcResetLogging(
    VOID
    );

// loginfo.c

VOID
SrvSvcFreeLogInfo(
    PSRVSVC_LOG_INFO pLogInfo
    );

// sysfuncs.c

#if !HAVE_DECL_ISBLANK
int isblank(int c);
#endif

void
SrvSvc_vsyslog(
    int priority,
    const char *format,
    va_list ap
    );

#if !defined(HAVE_RPL_MALLOC)

void*
rpl_malloc(
    size_t n
    );

#endif /* ! HAVE_RPL_MALLOC */

#if !defined(HAVE_RPL_REALLOC)

void*
rpl_realloc(
    void* buf,
    size_t n
    );

// syslog.c

DWORD
SrvSvcOpenSyslog(
    PCSTR       pszIdentifier,
    SRVSVC_LOG_LEVEL maxAllowedLogLevel,
    DWORD       dwOptions,
    DWORD       dwFacility,
    PHANDLE     phLog
    );

VOID
SrvSvcSetSyslogMask(
    SRVSVC_LOG_LEVEL logLevel
    );

VOID
SrvSvcLogToSyslog(
    HANDLE      hLog,
    SRVSVC_LOG_LEVEL logLevel,
    PCSTR       pszFormat,
    va_list     msgList
    );

DWORD
SrvSvcCloseSyslog(
    HANDLE hLog
    );

VOID
SrvSvcFreeSysLogInfo(
    PSRVSVC_SYS_LOG pSysLog
    );

#endif /* ! HAVE_RPL_REALLOC */


