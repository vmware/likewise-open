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
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "lsassd.h"
#include "config.h"
#include "lwnet.h"
#include "eventlog.h"
#include "lsasrvutils.h"

/* Needed for dcethread_fork() */
#include <dce/dcethread.h>

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    pthread_t listenerThreadId;
    pthread_t* pListenerThreadId = NULL;
    void* threadResult = NULL;

    dwError = LsaSrvSetDefaults();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvParseArgs(argc,
                              argv,
                              &gServerInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaInitLogging_r(
                    LsaGetProgramName(argv[0]),
                    gServerInfo.logTarget,
                    gServerInfo.maxAllowedLogLevel,
                    gServerInfo.szLogFilePath);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_VERBOSE("Logging started");

    dwError = LsaInitTracing_r();
    BAIL_ON_LSA_ERROR(dwError);

    if (atexit(LsaSrvExitHandler) < 0) {
       dwError = errno;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if (LsaSrvShouldStartAsDaemon()) {
       dwError = LsaSrvStartAsDaemon();
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LWNetExtendEnvironmentForKrb5Affinity(FALSE);
    BAIL_ON_LSA_ERROR(dwError);

    // Test system to see if dependent configuration tasks are completed prior to starting our process.
    dwError = LsaSrvStartupPreCheck();
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvCreatePIDFile();

    dwError = NTLMGssInitializeServer();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBlockSelectedSignals();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvInitialize();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvStartListenThread(&listenerThreadId, &pListenerThreadId);
    BAIL_ON_LSA_ERROR(dwError);

    // Handle signals, blocking until we are supposed to exit.
    dwError = LsaSrvHandleSignals();
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_LOG_VERBOSE("Lsa main cleaning up");

    LsaSrvStopProcess();

    if (pListenerThreadId)
    {
        pthread_join(listenerThreadId, &threadResult);
    }

    LsaSrvApiShutdown();

    NTLMGssTeardownServer();

    LSA_LOG_INFO("LSA Service exiting...");

    LsaSrvSetProcessExitCode(dwError);

    LsaShutdownLogging_r();

    LsaShutdownTracing_r();

    return dwError;

error:

    LSA_LOG_ERROR("LSA Process exiting due to error [Code:%d]", dwError);

    LsaSrvLogProcessFailureEvent(dwError);

    goto cleanup;
}

DWORD
LsaSrvStartupPreCheck(
    VOID
    )
{
    DWORD dwError = 0;
#ifdef __LWI_DARWIN__
    PSTR  pszHostname = NULL;
    int  iter = 0;

    // Make sure that the local hostname has been setup by the system
    for (iter = 0; iter < STARTUP_PRE_CHECK_WAIT; iter++)
    {
        LSA_SAFE_FREE_STRING(pszHostname);
        dwError = LsaDnsGetHostInfo(&pszHostname);
        BAIL_ON_LSA_ERROR(dwError);

        if (!strcasecmp(pszHostname, "localhost"))
        {
            sleep(10);
        }
        else
        {
            /* Hostname now looks correct */
            LSA_LOG_INFO("LSA Process start up check for hostname complete [hostname:%s]", pszHostname);
            break;
        }
    }

    if (iter >= STARTUP_PRE_CHECK_WAIT)
    {
        dwError = LSA_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK;
        LSA_LOG_ERROR("LSA start up pre-check failed to get updated hostname after %d seconds of waiting [Code:%d]",
                      STARTUP_PRE_CHECK_WAIT*10,
                      dwError);
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (iter = 0; iter < STARTUP_NETLOGON_WAIT; iter++)
    {
        dwError = LsaSrvVerifyNetLogonStatus();

        if (dwError)
        {
            sleep(1);
        }
        else
        {
            /* NetLogon is responsive */
            LSA_LOG_INFO("LSA Process start up check for NetLogon complete");
            break;
        }
    }

    if (iter >= STARTUP_NETLOGON_WAIT)
    {
        dwError = LSA_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK;
        LSA_LOG_ERROR("LSA start up pre-check failed to be able to use NetLogonD after %d seconds of waiting [Code:%d]",
                      STARTUP_NETLOGON_WAIT,
                      dwError);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Now that we are running, we need to flush the DirectoryService process of any negative cache entries
    dwError = LsaSrvFlushDirectoryServiceCache();
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_STRING(pszHostname);

    return dwError;

error:

    LSA_LOG_ERROR("LSA Process exiting due to error checking hostname at startup [Code:%d]", dwError);

    goto cleanup;
#else
    return dwError;
#endif
}

#if defined (__LWI_DARWIN__)
DWORD
LsaSrvFlushDirectoryServiceCache(
    VOID
    )
{
    DWORD dwError = 0;
    int i;
    const char* cacheUtils[] = {
        "/usr/sbin/lookupd", /* Before Mac OS X 10.5 */
        "/usr/bin/dscacheutil" /* On Mac OS X 10.5 */
    };
    const char* cacheUtilCmd[] = {
        "/usr/sbin/lookupd -flushcache", /* Before Mac OS X 10.5 */
        "/usr/bin/dscacheutil -flushcache" /* On Mac OS X 10.5 */
    };

    LSA_LOG_VERBOSE("Going to flush the Mac DirectoryService cache ...");

    for (i = 0; i < (sizeof(cacheUtils) / sizeof(cacheUtils[0])); i++)
    {
        const char* util = cacheUtils[i];
        const char* command = cacheUtilCmd[i];
        BOOLEAN exists;

        /* Sanity check */
        if (!util)
        {
            continue;
        }

        dwError = LsaCheckFileExists(util, &exists);
        BAIL_ON_LSA_ERROR(dwError);

        if (!exists)
        {
            continue;
        }

        system(command);

        /* Bail regardless */
        goto error;
    }

    LSA_LOG_ERROR("Could not locate cache flush utility");
    dwError = LSA_ERROR_MAC_FLUSH_DS_CACHE_FAILED;

error:

    return dwError;
}
#endif

#if defined (__LWI_DARWIN__)
DWORD
LsaSrvVerifyNetLogonStatus(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDomain = NULL;

    dwError = LWNetGetCurrentDomain(&pszDomain);
    LSA_LOG_INFO("LsaSrvVerifyNetLogonStatus call to LWNet API returned %ld", dwError);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pszDomain)
    {
        LWNetFreeString(pszDomain);
    }

    return dwError;

error:

    if (dwError == LWNET_ERROR_NOT_JOINED_TO_AD)
    {
        dwError = 0;
    }

    goto cleanup;
}
#endif

DWORD
LsaSrvSetDefaults(
    VOID
    )
{
    DWORD dwError = 0;

    gpServerInfo->maxAllowedLogLevel = LSA_LOG_LEVEL_ERROR;

    *(gpServerInfo->szLogFilePath) = '\0';

    strcpy(gpServerInfo->szCachePath, CACHEDIR);
    strcpy(gpServerInfo->szPrefixPath, PREFIXDIR);

    return (dwError);
}

DWORD
LsaSrvParseArgs(
    int argc,
    PSTR argv[],
    PLSASERVERINFO pLsaServerInfo
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_LOGFILE,
        PARSE_MODE_LOGLEVEL
    } ParseMode;

    DWORD dwError = 0;
    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;
    BOOLEAN bLogTargetSet = FALSE;

    do {

      pArg = argv[iArg++];
      if (pArg == NULL || *pArg == '\0') {
        break;
      }

      switch(parseMode) {

      case PARSE_MODE_OPEN:

        {
          if (strcmp(pArg, "--logfile") == 0)    {
            parseMode = PARSE_MODE_LOGFILE;
          }
          else if ((strcmp(pArg, "--help") == 0) ||
                   (strcmp(pArg, "-h") == 0)) {
            ShowUsage(LsaGetProgramName(argv[0]));
            exit(0);
          }
          else if (strcmp(pArg, "--start-as-daemon") == 0) {
            pLsaServerInfo->dwStartAsDaemon = 1;

            // If other arguments set before this set the log target
            // don't over-ride that setting
            if (!bLogTargetSet)
            {
                pLsaServerInfo->logTarget = LSA_LOG_TARGET_SYSLOG;
            }
          }
          else if (strcmp(pArg, "--loglevel") == 0) {
            parseMode = PARSE_MODE_LOGLEVEL;
          } else {
            LSA_LOG_ERROR("Unrecognized command line option [%s]",
                          pArg);
            ShowUsage(LsaGetProgramName(argv[0]));
            exit(1);
          }

          break;
        }

      case PARSE_MODE_LOGFILE:

        {
          strcpy(pLsaServerInfo->szLogFilePath, pArg);

          LsaStripWhitespace(pLsaServerInfo->szLogFilePath, TRUE, TRUE);

          if (!strcmp(pLsaServerInfo->szLogFilePath, "."))
          {
              pLsaServerInfo->logTarget = LSA_LOG_TARGET_CONSOLE;
          }
          else
          {
              pLsaServerInfo->logTarget = LSA_LOG_TARGET_FILE;
          }

          bLogTargetSet = TRUE;

          parseMode = PARSE_MODE_OPEN;

          break;
        }

      case PARSE_MODE_LOGLEVEL:

        {
          if (!strcasecmp(pArg, "error")) {

            pLsaServerInfo->maxAllowedLogLevel = LSA_LOG_LEVEL_ERROR;

          } else if (!strcasecmp(pArg, "warning")) {

            pLsaServerInfo->maxAllowedLogLevel = LSA_LOG_LEVEL_WARNING;

          } else if (!strcasecmp(pArg, "info")) {

            pLsaServerInfo->maxAllowedLogLevel = LSA_LOG_LEVEL_INFO;

          } else if (!strcasecmp(pArg, "verbose")) {

            pLsaServerInfo->maxAllowedLogLevel = LSA_LOG_LEVEL_VERBOSE;

          } else if (!strcasecmp(pArg, "debug")) {

            pLsaServerInfo->maxAllowedLogLevel = LSA_LOG_LEVEL_DEBUG;

          } else {

            LSA_LOG_ERROR("Error: Invalid log level [%s]", pArg);
            ShowUsage(LsaGetProgramName(argv[0]));
            exit(1);

          }

          parseMode = PARSE_MODE_OPEN;

          break;
        }

      }

    } while (iArg < argc);

    if (pLsaServerInfo->dwStartAsDaemon)
    {
        if (pLsaServerInfo->logTarget == LSA_LOG_TARGET_CONSOLE)
        {
            LSA_LOG_ERROR("%s", "Error: Cannot log to console when executing as a daemon");

            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        if (pLsaServerInfo->logTarget != LSA_LOG_TARGET_FILE)
        {
            pLsaServerInfo->logTarget = LSA_LOG_TARGET_CONSOLE;
        }
    }

error:

    return dwError;
}

PSTR
LsaGetProgramName(
    PSTR pszFullProgramPath
    )
{
    if (pszFullProgramPath == NULL || *pszFullProgramPath == '\0') {
        return NULL;
    }

    // start from end of the string
    PSTR pszNameStart = pszFullProgramPath + strlen(pszFullProgramPath);
    do {
        if (*(pszNameStart - 1) == '/') {
            break;
        }

        pszNameStart--;

    } while (pszNameStart != pszFullProgramPath);

    return pszNameStart;
}

VOID
ShowUsage(
    PCSTR pszProgramName
    )
{
    printf("Usage: %s [--start-as-daemon]\n"
           "          [--logfile logFilePath]\n"
           "          [--loglevel {error, warning, info, verbose}]\n", pszProgramName);
}

VOID
LsaSrvExitHandler(
    VOID
    )
{
    DWORD dwError = 0;
    DWORD dwExitCode = 0;
    CHAR  szErrCodeFilePath[PATH_MAX+1];
    PSTR  pszCachePath = NULL;
    BOOLEAN  bFileExists = 0;
    FILE* fp = NULL;

    dwError = LsaSrvGetCachePath(&pszCachePath);
    BAIL_ON_LSA_ERROR(dwError);

    sprintf(szErrCodeFilePath, "%s/lsasd.err", pszCachePath);

    dwError = LsaCheckFileExists(szErrCodeFilePath, &bFileExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (bFileExists) {
        dwError = LsaRemoveFile(szErrCodeFilePath);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvGetProcessExitCode(&dwExitCode);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwExitCode) {
       fp = fopen(szErrCodeFilePath, "w");
       if (fp == NULL) {
          dwError = errno;
          BAIL_ON_LSA_ERROR(dwError);
       }
       fprintf(fp, "%d\n", dwExitCode);
    }

error:

    LSA_SAFE_FREE_STRING(pszCachePath);

    if (fp != NULL) {
       fclose(fp);
    }
}

DWORD
LsaSrvInitialize(
    VOID
    )
{
    DWORD dwError = 0;
    PCSTR pszConfigFilePath = CONFIGDIR "/lsassd.conf";

    dwError = LsaInitCacheFolders();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvApiInit(pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaInitCacheFolders(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR  pszCachePath = NULL;
    BOOLEAN bExists = FALSE;

    dwError = LsaSrvGetCachePath(&pszCachePath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCheckDirectoryExists(
                        pszCachePath,
                        &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists) {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        dwError = LsaCreateDirectory(pszCachePath, cacheDirMode);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LSA_SAFE_FREE_STRING(pszCachePath);

    return dwError;

error:

    goto cleanup;
}

BOOLEAN
LsaSrvShouldStartAsDaemon(
    VOID
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    bResult = (gpServerInfo->dwStartAsDaemon != 0);

    LSA_UNLOCK_SERVERINFO(bInLock);

    return bResult;
}

DWORD
LsaSrvStartAsDaemon(
    VOID
    )
{
    DWORD dwError = 0;
    pid_t pid;
    int fd = 0;
    int iFd = 0;

    /* Use dcethread_fork() rather than fork() because we link with DCE/RPC */
    if ((pid = dcethread_fork()) != 0) {
        // Parent terminates
        exit (0);
    }

    // Let the first child be a session leader
    setsid();

    // Ignore SIGHUP, because when the first child terminates
    // it would be a session leader, and thus all processes in
    // its session would receive the SIGHUP signal. By ignoring
    // this signal, we are ensuring that our second child will
    // ignore this signal and will continue execution.
    dwError = LsaSrvIgnoreSIGHUP();
    BAIL_ON_LSA_ERROR(dwError);

    // Spawn a second child
    if ((pid = fork()) != 0) {
        // Let the first child terminate
        // This will ensure that the second child cannot be a session leader
        // Therefore, the second child cannot hold a controlling terminal
        exit(0);
    }

    // This is the second child executing
    dwError = chdir("/");
    BAIL_ON_LSA_ERROR(dwError);

    // Clear our file mode creation mask
    umask(0);

    for (iFd = 0; iFd < 3; iFd++)
        close(iFd);

    for (iFd = 0; iFd < 3; iFd++)    {

        fd = open("/dev/null", O_RDWR, 0);
        if (fd < 0) {
            fd = open("/dev/null", O_WRONLY, 0);
        }
        if (fd < 0) {
            exit(1);
        }
        if (fd != iFd) {
            exit(1);
        }
    }

    return (dwError);

 error:

    return (dwError);
}

DWORD
LsaSrvGetProcessExitCode(
    PDWORD pdwExitCode
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    *pdwExitCode = gpServerInfo->dwExitCode;

    LSA_UNLOCK_SERVERINFO(bInLock);

    return dwError;
}

VOID
LsaSrvSetProcessExitCode(
    DWORD dwExitCode
    )
{
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    gpServerInfo->dwExitCode = dwExitCode;

    LSA_UNLOCK_SERVERINFO(bInLock);
}

DWORD
LsaSrvGetCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    if (IsNullOrEmptyString(gpServerInfo->szCachePath)) {
      dwError = LSA_ERROR_INVALID_CACHE_PATH;
      BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateString(gpServerInfo->szCachePath, &pszPath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:

    LSA_UNLOCK_SERVERINFO(bInLock);

    return dwError;

 error:

    LSA_SAFE_FREE_STRING(pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

DWORD
LsaSrvGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    PSTR pszPath = NULL;
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    if (IsNullOrEmptyString(gpServerInfo->szPrefixPath)) {
      dwError = LSA_ERROR_INVALID_PREFIX_PATH;
      BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateString(gpServerInfo->szPrefixPath, &pszPath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszPath = pszPath;

 cleanup:

    LSA_UNLOCK_SERVERINFO(bInLock);

    return dwError;

 error:

    LSA_SAFE_FREE_STRING(pszPath);

    *ppszPath = NULL;

    goto cleanup;
}

VOID
LsaSrvCreatePIDFile(
    VOID
    )
{
    int result = -1;
    pid_t pid;
    char contents[PID_FILE_CONTENTS_SIZE];
    size_t len;
    int fd = -1;

    pid = LsaSrvGetPidFromPidFile();
    if (pid > 0) {
        fprintf(stderr, "Daemon already running as %d\n", (int) pid);
        result = -1;
        goto error;
    }

    fd = open(PID_FILE, O_CREAT | O_WRONLY | O_EXCL, 0644);
    if (fd < 0) {
        fprintf(stderr, "Could not create pid file: %s\n", strerror(errno));
        result = 1;
        goto error;
    }

    pid = getpid();
    snprintf(contents, sizeof(contents)-1, "%d\n", (int) pid);
    contents[sizeof(contents)-1] = 0;
    len = strlen(contents);

    result = (int) write(fd, contents, len);
    if ( result != (int) len ) {
        fprintf(stderr, "Could not write to pid file: %s\n", strerror(errno));
        result = -1;
        goto error;
    }

    result = 0;

 error:
    if (fd != -1) {
        close(fd);
    }

    if (result < 0) {
        exit(1);
    }
}

pid_t
LsaSrvGetPidFromPidFile(
    VOID
    )
{
    pid_t pid = 0;
    int fd = -1;
    int result;
    char contents[PID_FILE_CONTENTS_SIZE];

    fd = open(PID_FILE, O_RDONLY, 0644);
    if (fd < 0) {
        goto error;
    }

    result = read(fd, contents, sizeof(contents)-1);
    if (result <= 0) {
        goto error;
    }
    contents[result-1] = 0;

    result = atoi(contents);
    if (result <= 0) {
        result = -1;
        goto error;
    }

    pid = (pid_t) result;
    result = kill(pid, 0);
    if (result != 0 || errno == ESRCH) {
        unlink(PID_FILE);
        pid = 0;
    }

 error:
    if (fd != -1) {
        close(fd);
    }

    return pid;
}

DWORD
LsaBlockSelectedSignals(
    VOID
    )
{
    DWORD dwError = 0;
    sigset_t default_signal_mask;
    sigset_t old_signal_mask;

    sigemptyset(&default_signal_mask);
    sigaddset(&default_signal_mask, SIGINT);
    sigaddset(&default_signal_mask, SIGTERM);
    sigaddset(&default_signal_mask, SIGHUP);
    sigaddset(&default_signal_mask, SIGQUIT);
    sigaddset(&default_signal_mask, SIGPIPE);

    dwError = pthread_sigmask(SIG_BLOCK,  &default_signal_mask, &old_signal_mask);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

BOOLEAN
LsaSrvShouldProcessExit(
    VOID
    )
{
    BOOLEAN bExit = FALSE;
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    bExit = gpServerInfo->bProcessShouldExit;

    LSA_UNLOCK_SERVERINFO(bInLock);

    return bExit;
}

VOID
LsaSrvSetProcessToExit(
    BOOLEAN bExit
    )
{
    BOOLEAN bInLock = FALSE;

    LSA_LOCK_SERVERINFO(bInLock);

    gpServerInfo->bProcessShouldExit = bExit;

    LSA_UNLOCK_SERVERINFO(bInLock);
}

VOID
LsaSrvLogProcessFailureEvent(
    DWORD dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszLsassdFailureDescription = NULL;
    PSTR pszData = NULL;

    dwError = LsaAllocateStringPrintf(
                 &pszLsassdFailureDescription,
                 "The LSASSD process encountered a serious error with code '%d' which caused it to stop running.",
                 dwErrCode);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);

    LsaSrvLogServiceFailureEvent(
            SERVICESTOP_EVENT_CATEGORY,
            pszLsassdFailureDescription,
            pszData);

cleanup:

    LSA_SAFE_FREE_STRING(pszLsassdFailureDescription);
    LSA_SAFE_FREE_STRING(pszData);

    return;

error:

    goto cleanup;
}


