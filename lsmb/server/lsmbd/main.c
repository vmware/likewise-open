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
 *        Likewise Security and Authentication Subsystem (SMBSS)
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "includes.h"

static
DWORD
SMBSrvSetDefaults(
    VOID
    );

static
DWORD
SMBSrvParseArgs(
    int argc,
    PSTR argv[],
    PSMBSERVERINFO pSMBServerInfo
    );

static
PSTR
SMBGetProgramName(
    PSTR pszFullProgramPath
    );

static
VOID
ShowUsage(
    PCSTR pszProgramName
    );

static
VOID
SMBSrvExitHandler(
    VOID
    );

static
DWORD
SMBSrvInitialize(
    VOID
    );

static
DWORD
SMBInitCacheFolders(
    VOID
    );

static
BOOLEAN
SMBSrvShouldStartAsDaemon(
    VOID
    );

static
DWORD
SMBSrvStartAsDaemon(
    VOID
    );

static
DWORD
SMBSrvGetProcessExitCode(
    PDWORD pdwExitCode
    );

static
VOID
SMBSrvSetProcessExitCode(
    DWORD dwExitCode
    );

static
VOID
SMBSrvCreatePIDFile(
    VOID
    );

static
pid_t
SMBSrvGetPidFromPidFile(
    VOID
    );

static
DWORD
SMBSrvExecute(
    VOID
    );

static
DWORD
SMBHandleSignals(
    VOID
    );

static
VOID
SMBSrvInterruptHandler(
    int sig
    );

static
VOID
SMBSrvBlockSignals(
    VOID
    );

static
int
SMBSrvGetNextSignal(
    VOID
    );

static
VOID
SMBSrvGetBlockedSignals(
    sigset_t* pBlockedSignals
    );

static
VOID
SMBSrvGetBlockedSignalsSansInterrupt(
    sigset_t* pBlockedSignals
    );

int
main(
    int argc,
    char* argv[]
    )
{
    DWORD dwError = 0;

    dwError = SMBSrvSetDefaults();
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvParseArgs(argc,
                              argv,
                              &gServerInfo);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBInitLogging_r(
                    SMBGetProgramName(argv[0]),
                    gServerInfo.logTarget,
                    gServerInfo.maxAllowedLogLevel,
                    gServerInfo.szLogFilePath);
    BAIL_ON_SMB_ERROR(dwError);

    SMB_LOG_VERBOSE("Logging started");

    if (atexit(SMBSrvExitHandler) < 0) {
       dwError = errno;
       BAIL_ON_SMB_ERROR(dwError);
    }

    if (SMBSrvShouldStartAsDaemon()) {
       dwError = SMBSrvStartAsDaemon();
       BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = LWNetExtendEnvironmentForKrb5Affinity(FALSE);
    BAIL_ON_SMB_ERROR(dwError);

    SMBSrvCreatePIDFile();

    dwError = SMBSrvInitialize();
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvExecute();
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_LOG_VERBOSE("SMB main cleaning up");

    IOMgrShutdown();

    SMB_LOG_INFO("SMB Service exiting...");

    SMBSrvSetProcessExitCode(dwError);

    SMBShutdownLogging_r();

    return dwError;

error:

    SMB_LOG_ERROR("SMB Process exiting due to error [Code:%d]", dwError);

    goto cleanup;
}

static
DWORD
SMBSrvSetDefaults(
    VOID
    )
{
    DWORD dwError = 0;

    gpServerInfo->maxAllowedLogLevel = SMB_LOG_LEVEL_ERROR;

    *(gpServerInfo->szLogFilePath) = '\0';

    strcpy(gpServerInfo->szCachePath, CACHEDIR);
    strcpy(gpServerInfo->szPrefixPath, PREFIXDIR);

    return (dwError);
}

static
DWORD
SMBSrvParseArgs(
    int argc,
    PSTR argv[],
    PSMBSERVERINFO pSMBServerInfo
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
            ShowUsage(SMBGetProgramName(argv[0]));
            exit(0);
          }
          else if (strcmp(pArg, "--start-as-daemon") == 0) {
            pSMBServerInfo->dwStartAsDaemon = 1;

            // If other arguments set before this set the log target
            // don't over-ride that setting
            if (!bLogTargetSet)
            {
                pSMBServerInfo->logTarget = SMB_LOG_TARGET_SYSLOG;
            }
          }
          else if (strcmp(pArg, "--loglevel") == 0) {
            parseMode = PARSE_MODE_LOGLEVEL;
          } else {
            SMB_LOG_ERROR("Unrecognized command line option [%s]",
                          pArg);
            ShowUsage(SMBGetProgramName(argv[0]));
            exit(1);
          }

          break;
        }

      case PARSE_MODE_LOGFILE:

        {
          strcpy(pSMBServerInfo->szLogFilePath, pArg);

          SMBStripWhitespace(pSMBServerInfo->szLogFilePath, TRUE, TRUE);

          if (!strcmp(pSMBServerInfo->szLogFilePath, "."))
          {
              pSMBServerInfo->logTarget = SMB_LOG_TARGET_CONSOLE;
          }
          else
          {
              pSMBServerInfo->logTarget = SMB_LOG_TARGET_FILE;
          }

          bLogTargetSet = TRUE;

          parseMode = PARSE_MODE_OPEN;

          break;
        }

      case PARSE_MODE_LOGLEVEL:

        {
          if (!strcasecmp(pArg, "error")) {

            pSMBServerInfo->maxAllowedLogLevel = SMB_LOG_LEVEL_ERROR;

          } else if (!strcasecmp(pArg, "warning")) {

            pSMBServerInfo->maxAllowedLogLevel = SMB_LOG_LEVEL_WARNING;

          } else if (!strcasecmp(pArg, "info")) {

            pSMBServerInfo->maxAllowedLogLevel = SMB_LOG_LEVEL_INFO;

          } else if (!strcasecmp(pArg, "verbose")) {

            pSMBServerInfo->maxAllowedLogLevel = SMB_LOG_LEVEL_VERBOSE;

          } else if (!strcasecmp(pArg, "debug")) {

            pSMBServerInfo->maxAllowedLogLevel = SMB_LOG_LEVEL_DEBUG;

          } else {

            SMB_LOG_ERROR("Error: Invalid log level [%s]", pArg);
            ShowUsage(SMBGetProgramName(argv[0]));
            exit(1);

          }

          parseMode = PARSE_MODE_OPEN;

          break;
        }

      }

    } while (iArg < argc);

    if (pSMBServerInfo->dwStartAsDaemon)
    {
        if (pSMBServerInfo->logTarget == SMB_LOG_TARGET_CONSOLE)
        {
            SMB_LOG_ERROR("%s", "Error: Cannot log to console when executing as a daemon");

            dwError = SMB_ERROR_INVALID_PARAMETER;
            BAIL_ON_SMB_ERROR(dwError);
        }
    }
    else
    {
        if (pSMBServerInfo->logTarget != SMB_LOG_TARGET_FILE)
        {
            pSMBServerInfo->logTarget = SMB_LOG_TARGET_CONSOLE;
        }
    }

error:

    return dwError;
}

static
PSTR
SMBGetProgramName(
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

static
VOID
ShowUsage(
    PCSTR pszProgramName
    )
{
    printf("Usage: %s [--start-as-daemon]\n"
           "          [--logfile logFilePath]\n"
           "          [--loglevel {error, warning, info, verbose}]\n", pszProgramName);
}

static
VOID
SMBSrvExitHandler(
    VOID
    )
{
    DWORD dwError = 0;
    DWORD dwExitCode = 0;
    CHAR  szErrCodeFilePath[PATH_MAX+1];
    BOOLEAN  bFileExists = 0;
    FILE* fp = NULL;

    sprintf(szErrCodeFilePath, "%s/lsasd.err", CACHEDIR);

    dwError = SMBCheckFileExists(szErrCodeFilePath, &bFileExists);
    BAIL_ON_SMB_ERROR(dwError);

    if (bFileExists) {
        dwError = SMBRemoveFile(szErrCodeFilePath);
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = SMBSrvGetProcessExitCode(&dwExitCode);
    BAIL_ON_SMB_ERROR(dwError);

    if (dwExitCode) {
       fp = fopen(szErrCodeFilePath, "w");
       if (fp == NULL) {
          dwError = errno;
          BAIL_ON_SMB_ERROR(dwError);
       }
       fprintf(fp, "%d\n", dwExitCode);
    }

error:

    if (fp != NULL) {
       fclose(fp);
    }
}

static
DWORD
SMBSrvInitialize(
    VOID
    )
{
    DWORD dwError = 0;
    PCSTR pszConfigPath = SMB_CONFIG_FILE_PATH;

    dwError = SMBSrvSetupInitialConfig();
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvRefreshConfig(
                    pszConfigPath);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBInitCacheFolders();
    BAIL_ON_SMB_ERROR(dwError);

    dwError = IOMgrInitialize(pszConfigPath);
    BAIL_ON_SMB_ERROR(dwError);

error:

    return dwError;
}

static
DWORD
SMBInitCacheFolders(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;

    dwError = SMBCheckDirectoryExists(
                        CACHEDIR,
                        &bExists);
    BAIL_ON_SMB_ERROR(dwError);

    if (!bExists) {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        dwError = SMBCreateDirectory(CACHEDIR, cacheDirMode);
        BAIL_ON_SMB_ERROR(dwError);
    }

error:

    return dwError;
}

static
BOOLEAN
SMBSrvShouldStartAsDaemon(
    VOID
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_SERVERINFO(bInLock);

    bResult = (gpServerInfo->dwStartAsDaemon != 0);

    SMB_UNLOCK_SERVERINFO(bInLock);

    return bResult;
}

static
DWORD
SMBSrvStartAsDaemon(
    VOID
    )
{
    DWORD dwError = 0;
    pid_t pid;
    int fd = 0;
    int iFd = 0;

    if ((pid = fork()) != 0) {
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
    if (signal(SIGHUP, SIG_IGN) < 0) {
        dwError = errno;
        BAIL_ON_SMB_ERROR(dwError);
    }

    // Spawn a second child
    if ((pid = fork()) != 0) {
        // Let the first child terminate
        // This will ensure that the second child cannot be a session leader
        // Therefore, the second child cannot hold a controlling terminal
        exit(0);
    }

    // This is the second child executing
    dwError = chdir("/");
    BAIL_ON_SMB_ERROR(dwError);

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

static
DWORD
SMBSrvGetProcessExitCode(
    PDWORD pdwExitCode
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_SERVERINFO(bInLock);

    *pdwExitCode = gpServerInfo->dwExitCode;

    SMB_UNLOCK_SERVERINFO(bInLock);

    return dwError;
}

static
VOID
SMBSrvSetProcessExitCode(
    DWORD dwExitCode
    )
{
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_SERVERINFO(bInLock);

    gpServerInfo->dwExitCode = dwExitCode;

    SMB_UNLOCK_SERVERINFO(bInLock);
}

static
VOID
SMBSrvCreatePIDFile(
    VOID
    )
{
    DWORD dwError = 0;
    int result = -1;
    pid_t pid;
    char contents[PID_FILE_CONTENTS_SIZE];
    size_t len;
    int fd = -1;
    struct stat myStat = {0};
    struct stat runningStat = {0};

    pid = SMBSrvGetPidFromPidFile();
    if (pid > 0)
    {
        dwError = SMBSrvGetExecutableStatByPid(
                    getpid(),
                    &myStat);
        if (dwError != 0)
        {
            fprintf(stderr, "Unable to stat the executable of this program. Make sure this program was invoked with an absolute path.\n");
            result = -1;
            goto error;
        }

        dwError = SMBSrvGetExecutableStatByPid(
                    pid,
                    &runningStat);
        if (dwError == ENOENT || dwError == ESRCH)
        {
            runningStat.st_dev = -1;
            runningStat.st_ino = -1;
            dwError = 0;
        }
        else if(dwError != 0)
        {
            fprintf(stderr, "Unable to stat the executable of pid %ld\n", (long)pid);
            result = -1;
            goto error;
        }

        if (runningStat.st_dev == myStat.st_dev &&
                runningStat.st_ino == myStat.st_ino)
        {
            fprintf(stderr, "Daemon already running as %d\n", (int) pid);
            result = -1;
            goto error;
        }
        else
        {
            fprintf(
                    stderr,
                    "Warning: the pid file already exists and contains pid %d, but this pid is not owned by this program. Most likely, this daemon shutdown uncleanly on its last run.\n",
                    (int) pid);
            if (remove(PID_FILE) < 0)
            {
                fprintf(stderr, "Unable to clear existing pid file\n");
                result = -1;
                goto error;
            }
        }
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

static
pid_t
SMBSrvGetPidFromPidFile(
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

static
DWORD
SMBSrvExecute(
    VOID
    )
{
    DWORD dwError = 0;
    LWMsgServer* pServer = NULL;
    LWMsgProtocolSpec* pProtocolSpec = NULL;
    LWMsgProtocol* pProtocol = NULL;
    LWMsgTime timeout = { 30, 0 }; /* 30 seconds */

    dwError = SMBIPCGetProtocolSpec(&pProtocolSpec);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_protocol_new(NULL, &pProtocol));
    BAIL_ON_SMB_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_protocol_add_protocol_spec(
                                   pProtocol,
                                   pProtocolSpec));
    BAIL_ON_SMB_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_server_new(pProtocol, &pServer));
    BAIL_ON_SMB_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_server_add_dispatch_spec(
                    pServer,
                    gLSMBdispatch));
    BAIL_ON_SMB_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_server_set_endpoint(
                    pServer,
                    LWMSG_SERVER_MODE_LOCAL,
                    SMB_SERVER_FILENAME,
                    (S_IRWXU | S_IRWXG | S_IRWXO)));
    BAIL_ON_SMB_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_server_set_max_clients(
                    pServer,
                    512));
    BAIL_ON_SMB_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_server_set_max_dispatch(
                    pServer,
                    8));
    BAIL_ON_SMB_ERROR(dwError);

    dwError = MAP_LWMSG_STATUS(lwmsg_server_set_timeout(
                    pServer,
                    &timeout));
    BAIL_ON_SMB_ERROR(dwError);

    SMBSrvBlockSignals();

    dwError = MAP_LWMSG_STATUS(lwmsg_server_start(pServer));
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBHandleSignals();
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    if (pServer)
    {
        LWMsgStatus status2 = lwmsg_server_stop(pServer);

        if (status2)
        {
            SMB_LOG_ERROR("Error stopping server. [Error code:%d]", status2);
        }

        lwmsg_server_delete(pServer);
    }

    return dwError;

error:

    SMB_LOG_ERROR("SMB Server stopping due to error [code: %d]", dwError);

    goto cleanup;
}

static
DWORD
SMBHandleSignals(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bDone = FALSE;
    struct sigaction action;
    sigset_t catch_signal_mask;

    // After starting up threads, we now want to handle SIGINT async
    // instead of using sigwait() on it.  The reason for this is so
    // that a debugger (such as gdb) can break in properly.
    // See http://sourceware.org/ml/gdb/2007-03/msg00145.html and
    // http://bugzilla.kernel.org/show_bug.cgi?id=9039.

    memset(&action, 0, sizeof(action));
    action.sa_handler = SMBSrvInterruptHandler;

    if (sigaction(SIGINT, &action, NULL) != 0)
    {
        dwError = errno;
        BAIL_ON_SMB_ERROR(dwError);
    }

    // Unblock SIGINT
    sigemptyset(&catch_signal_mask);
    sigaddset(&catch_signal_mask, SIGINT);

    dwError = pthread_sigmask(SIG_UNBLOCK, &catch_signal_mask, NULL);
    BAIL_ON_SMB_ERROR(dwError);

    while (!bDone)
    {
        switch (SMBSrvGetNextSignal())
        {
            case SIGINT:
            case SIGTERM:

                bDone = TRUE;
                break;

            case SIGHUP:

                {
                    DWORD dwError2 = 0;
                    PCSTR pszConfigPath = SMB_CONFIG_FILE_PATH;

                    dwError2 = SMBSrvRefreshConfig(pszConfigPath);
                    if (dwError2)
                    {
                        SMB_LOG_ERROR("Failed to refresh configuration [code:%d]", dwError2);
                    }
                }

                break;

            default:

                break;
        }
    }

error:

    return dwError;
}

static
VOID
SMBSrvInterruptHandler(
    int sig
    )
{
    if (sig == SIGINT)
    {
        raise(SIGTERM);
    }
}

static
VOID
SMBSrvBlockSignals(
    VOID
    )
{
    sigset_t blockedSignals;

    SMBSrvGetBlockedSignals(&blockedSignals);

    pthread_sigmask(SIG_BLOCK, &blockedSignals, NULL);
}

static
int
SMBSrvGetNextSignal(
    VOID
    )
{
    sigset_t blockedSignals;
    int sig = 0;

    SMBSrvGetBlockedSignalsSansInterrupt(&blockedSignals);

    sigwait(&blockedSignals, &sig);

    return sig;
}

static
VOID
SMBSrvGetBlockedSignals(
    sigset_t* pBlockedSignals
    )
{
    sigemptyset(pBlockedSignals);
    sigaddset(pBlockedSignals, SIGTERM);
    sigaddset(pBlockedSignals, SIGINT);
    sigaddset(pBlockedSignals, SIGPIPE);
    sigaddset(pBlockedSignals, SIGHUP);
}

static
VOID
SMBSrvGetBlockedSignalsSansInterrupt(
    sigset_t* pBlockedSignals
    )
{
    sigemptyset(pBlockedSignals);
    sigaddset(pBlockedSignals, SIGTERM);
    sigaddset(pBlockedSignals, SIGPIPE);
    sigaddset(pBlockedSignals, SIGHUP);
}

BOOLEAN
SMBSrvShouldProcessExit(
    VOID
    )
{
    BOOLEAN bExit = FALSE;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_SERVERINFO(bInLock);

    bExit = gpServerInfo->bProcessShouldExit;

    SMB_UNLOCK_SERVERINFO(bInLock);

    return bExit;
}

VOID
SMBSrvSetProcessToExit(
    BOOLEAN bExit
    )
{
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_SERVERINFO(bInLock);

    gpServerInfo->bProcessShouldExit = bExit;

    SMB_UNLOCK_SERVERINFO(bInLock);
}

