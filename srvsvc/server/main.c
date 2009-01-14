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
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007
 * All rights reserved.
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 * Likewise Server Service (LWSRVSVC)
 *
 */
#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

#include "includes.h"
#include "main_p.h"

SRVSVCSERVERINFO gServerInfo =
{
    PTHREAD_MUTEX_INITIALIZER,  /* Lock              */
    0,                          /* Start as daemon   */
    LOG_LEVEL_ERROR,            /* Max Log Level     */
    "",                         /* Log file path     */
    "",                         /* Config file path  */
    "",                         /* Cache path        */
    "",                         /* Prefix path       */
    0,                          /* Process exit flag */
    0                           /* Process exit code */
};

#define SRVSVC_LOCK_SERVERINFO   pthread_mutex_lock(&gServerInfo.lock)
#define SRVSVC_UNLOCK_SERVERINFO pthread_mutex_unlock(&gServerInfo.lock)

static
DWORD
SRVSVCGetProcessExitCode(
    PDWORD pdwExitCode
    );

static
void
SRVSVCExitHandler(
    void
    )
{
    DWORD dwError = 0;
    DWORD dwExitCode = 0;
    CHAR  szErrCodeFilePath[PATH_MAX+1];
    PSTR  pszCachePath = NULL;
    BOOLEAN  bFileExists = 0;
    FILE* fp = NULL;

    dwError = SRVSVCGetCachePath(&pszCachePath);
    BAIL_ON_SRVSVC_ERROR(dwError);

    sprintf(szErrCodeFilePath, "%s/srvsvcd.err", pszCachePath);

    dwError = SRVSVCCheckFileExists(szErrCodeFilePath, &bFileExists);
    BAIL_ON_SRVSVC_ERROR(dwError);

    if (bFileExists) {
        dwError = SRVSVCRemoveFile(szErrCodeFilePath);
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    dwError = SRVSVCGetProcessExitCode(&dwExitCode);
    BAIL_ON_SRVSVC_ERROR(dwError);

    if (dwExitCode) {
        fp = fopen(szErrCodeFilePath, "w");
        if (fp == NULL) {
            dwError = errno;
            BAIL_ON_SRVSVC_ERROR(dwError);
        }
        fprintf(fp, "%d\n", dwExitCode);
    }

error:

    if (pszCachePath) {
        SRVSVCFreeString(pszCachePath);
    }

    if (fp != NULL) {
        fclose(fp);
    }
}

static
PSTR
get_program_name(
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

BOOLEAN
SRVSVCProcessShouldExit()
{
    BOOLEAN bResult = 0;

    SRVSVC_LOCK_SERVERINFO;

    bResult = gServerInfo.bProcessShouldExit;

    SRVSVC_UNLOCK_SERVERINFO;

    return bResult;
}

void
SRVSVCSetProcessShouldExit(
    BOOLEAN val
    )
{
    SRVSVC_LOCK_SERVERINFO;

    gServerInfo.bProcessShouldExit = val;

    SRVSVC_UNLOCK_SERVERINFO;
}

DWORD
SRVSVCGetProcessExitCode(
    PDWORD pdwExitCode
    )
{
    DWORD dwError = 0;

    SRVSVC_LOCK_SERVERINFO;

    *pdwExitCode = gServerInfo.dwExitCode;

    SRVSVC_UNLOCK_SERVERINFO;

    return (dwError);
}

void
SRVSVCSetProcessExitCode(
    DWORD dwExitCode
    )
{
    SRVSVC_LOCK_SERVERINFO;

    gServerInfo.dwExitCode = dwExitCode;

    SRVSVC_UNLOCK_SERVERINFO;
}

DWORD
SRVSVCGetCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    SRVSVC_LOCK_SERVERINFO;

    dwError = SRVSVCAllocateString(gServerInfo.szCachePath, ppszPath);

    SRVSVC_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
SRVSVCGetConfigPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    SRVSVC_LOCK_SERVERINFO;

    dwError = SRVSVCAllocateString(gServerInfo.szConfigFilePath, ppszPath);

    SRVSVC_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
SRVSVCGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    SRVSVC_LOCK_SERVERINFO;

    dwError = SRVSVCAllocateString(gServerInfo.szPrefixPath, ppszPath);

    SRVSVC_UNLOCK_SERVERINFO;

    return (dwError);
}

void
SRVSVCUnlockServerInfo()
{
    SRVSVC_UNLOCK_SERVERINFO;
}

static
void
ShowUsage(
    const PSTR pszProgramName
    )
{
    printf("Usage: %s [--start-as-daemon]\n"
            "          [--logfile logFilePath]\n"
            "          [--loglevel {0, 1, 2, 3, 4, 5}]\n"
            "          [--configfile configfilepath]\n", pszProgramName);
}

void
get_server_info_r(
    PSRVSVCSERVERINFO pServerInfo
    )
{
    if (pServerInfo == NULL) {
        return;
    }

    SRVSVC_LOCK_SERVERINFO;

    memcpy(pServerInfo, &gServerInfo, sizeof(SRVSVCSERVERINFO));

    pthread_mutex_init(&pServerInfo->lock, NULL);

    SRVSVC_UNLOCK_SERVERINFO;
}

static
DWORD
SRVSVCParseArgs(
    int argc,
    PSTR argv[],
    PSRVSVCSERVERINFO pSRVSVCServerInfo
    )
{
    typedef enum {
        PARSE_MODE_OPEN = 0,
        PARSE_MODE_CONFIGFILE,
        PARSE_MODE_LOGFILE,
        PARSE_MODE_LOGLEVEL
    } ParseMode;

    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;
    DWORD dwLogLevel = 0;

    do
    {
        pArg = argv[iArg++];
        if (pArg == NULL || *pArg == '\0') {
            break;
        }

        switch(parseMode)
        {
            case PARSE_MODE_OPEN:
            {
                if (strcmp(pArg, "--logfile") == 0)    {
                    parseMode = PARSE_MODE_LOGFILE;
                }
                else if ((strcmp(pArg, "--help") == 0) ||
                         (strcmp(pArg, "-h") == 0)) {
                    ShowUsage(get_program_name(argv[0]));
                    exit(0);
                }
                else if (strcmp(pArg, "--start-as-daemon") == 0) {
                    pSRVSVCServerInfo->dwStartAsDaemon = 1;
                }
                else if (strcmp(pArg, "--configfile") == 0) {
                    parseMode = PARSE_MODE_CONFIGFILE;
                }
                else if (strcmp(pArg, "--loglevel") == 0) {
                    parseMode = PARSE_MODE_LOGLEVEL;
                }
                else {
                    SRVSVC_LOG_ERROR("Unrecognized command line option [%s]",
                                    pArg);
                    ShowUsage(get_program_name(argv[0]));
                    exit(1);
                }
            }
            break;
            case PARSE_MODE_LOGFILE:
            {
                strncpy(pSRVSVCServerInfo->szLogFilePath, pArg, PATH_MAX);
                *(pSRVSVCServerInfo->szLogFilePath+PATH_MAX) = '\0';
                parseMode = PARSE_MODE_OPEN;
            }
            break;
            case PARSE_MODE_LOGLEVEL:
            {
                dwLogLevel = atoi(pArg);

                if (dwLogLevel < LOG_LEVEL_ALWAYS || dwLogLevel > LOG_LEVEL_DEBUG) {

                    SRVSVC_LOG_ERROR("Error: Invalid log level [%d]", dwLogLevel);
                    ShowUsage(get_program_name(argv[0]));
                    exit(1);
                }

                pSRVSVCServerInfo->dwLogLevel = dwLogLevel;

                parseMode = PARSE_MODE_OPEN;
            }
            break;
            case PARSE_MODE_CONFIGFILE:
            {
                strncpy(pSRVSVCServerInfo->szConfigFilePath, pArg, PATH_MAX);
                *(pSRVSVCServerInfo->szConfigFilePath+PATH_MAX) = '\0';
                parseMode = PARSE_MODE_OPEN;

            }
            break;
        }
    } while (iArg < argc);

    return 0;
}

static
DWORD
SRVSVCStartAsDaemon()
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
    if (signal(SIGHUP, SIG_IGN) < 0) {
        dwError = errno;
        BAIL_ON_SRVSVC_ERROR(dwError);
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
    BAIL_ON_SRVSVC_ERROR(dwError);

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

#define DAEMON_NAME "srvsvcd"
#define PID_DIR "/var/run"
#define PID_FILE PID_DIR "/" DAEMON_NAME ".pid"

#define PID_FILE_CONTENTS_SIZE ((9 * 2) + 2)

static
pid_t
pid_from_pid_file()
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
void
SRVSVCCreatePIDFile()
{
    int result = -1;
    pid_t pid;
    char contents[PID_FILE_CONTENTS_SIZE];
    size_t len;
    int fd = -1;

    pid = pid_from_pid_file();
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


static
DWORD
SRVSVCSetConfigDefaults()
{
    DWORD dwError = 0;

    SRVSVC_LOCK_SERVERINFO;

    SRVSVC_UNLOCK_SERVERINFO;

    return dwError;
}

static
DWORD
SRVSVCSetServerDefaults()
{
    DWORD dwError = 0;

    SRVSVC_LOCK_SERVERINFO;

    gServerInfo.dwLogLevel = LOG_LEVEL_ERROR;

    *(gServerInfo.szLogFilePath) = '\0';

    memset(gServerInfo.szConfigFilePath, 0, PATH_MAX+1);
    strncpy(gServerInfo.szConfigFilePath, DEFAULT_CONFIG_FILE_PATH, PATH_MAX);
    strcpy(gServerInfo.szCachePath, CACHEDIR);
    strcpy(gServerInfo.szPrefixPath, PREFIXDIR);

    SRVSVC_UNLOCK_SERVERINFO;

    dwError = SRVSVCSetConfigDefaults();

    return dwError;
}

static
void
SRVSVCBlockSelectedSignals()
{
    sigset_t default_signal_mask;
    sigset_t old_signal_mask;

    sigemptyset(&default_signal_mask);
    sigaddset(&default_signal_mask, SIGINT);
    sigaddset(&default_signal_mask, SIGTERM);
    sigaddset(&default_signal_mask, SIGHUP);
    sigaddset(&default_signal_mask, SIGQUIT);
    sigaddset(&default_signal_mask, SIGPIPE);

    pthread_sigmask(SIG_BLOCK,  &default_signal_mask, &old_signal_mask);
}

static
DWORD
SRVSVCInitLogging(
    PSTR pszProgramName
    )
{
    if (gServerInfo.dwStartAsDaemon) {

        return SRVSVCInitLoggingToSyslog(gServerInfo.dwLogLevel,
                                      pszProgramName,
                                      LOG_PID,
                                      LOG_DAEMON);
    }
    else
    {
        return SRVSVCInitLoggingToFile(gServerInfo.dwLogLevel,
                                    gServerInfo.szLogFilePath);
    }
}

static
DWORD
SRVSVCStopSignalHandler()
{
    DWORD dwError = 0;
    uint32_t status = 0;

    rpc_mgmt_stop_server_listening(NULL, (unsigned32*)&status);

    if (pgSignalHandlerThread && !pthread_cancel(gSignalHandlerThread)) {
        pthread_join(gSignalHandlerThread, NULL);
        pgSignalHandlerThread = NULL;
    }

    return (dwError);
}

/* call back functions to get the values from config file */
DWORD
SRVSVCConfigStartSection(
    PCSTR    pszSectionName,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{

    //This callback may not be required,retaining it for future
    SRVSVC_LOG_VERBOSE("SRVSVCConfigStartSection: SECTION Name=%s", pszSectionName);

    *pbSkipSection = FALSE;
    *pbContinue = TRUE;

    return 0;
}

DWORD
SRVSVCConfigComment(
    PCSTR    pszComment,
    PBOOLEAN pbContinue
    )
{
    //This callback may not be required,retaining it for future
    SRVSVC_LOG_VERBOSE("SRVSVCConfigComment: %s",
        (IsNullOrEmptyString(pszComment) ? "" : pszComment));

    *pbContinue = TRUE;

    return 0;
}

DWORD
SRVSVCConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PBOOLEAN pbContinue
    )
{

    //strip the white spaces
    SRVSVCStripWhitespace((PSTR)pszName,1,1);
    SRVSVCStripWhitespace((PSTR)pszValue,1,1);

    SRVSVC_LOG_INFO("SRVSVCConfigNameValuePair: NAME=%s, VALUE=%s",
        (IsNullOrEmptyString(pszName) ? "" : pszName),
        (IsNullOrEmptyString(pszValue) ? "" : pszValue));

    *pbContinue = TRUE;

    return 0;
}

DWORD
SRVSVCConfigEndSection(
    PCSTR pszSectionName,
    PBOOLEAN pbContinue
    )
{
    //This callback may not be required,retaining it for future
    SRVSVC_LOG_VERBOSE("SRVSVCConfigEndSection: SECTION Name=%s", pszSectionName);

    *pbContinue = TRUE;

    return 0;
}


static
DWORD
SRVSVCReadEventLogConfigSettings()
{
    DWORD dwError = 0;
    PSTR pszConfigFilePath = NULL;

    SRVSVC_LOG_INFO("Read Likewise Server Service configuration settings");

    dwError = SRVSVCSetConfigDefaults();
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SRVSVCGetConfigPath(&pszConfigFilePath);
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SRVSVCParseConfigFile(
                pszConfigFilePath,
                &SRVSVCConfigStartSection,
                &SRVSVCConfigComment,
                &SRVSVCConfigNameValuePair,
                &SRVSVCConfigEndSection);
    BAIL_ON_SRVSVC_ERROR(dwError);

    if (pszConfigFilePath) {
        SRVSVCFreeString(pszConfigFilePath);
        pszConfigFilePath = NULL;
    }

cleanup:

    return dwError;

error:

    if (pszConfigFilePath) {
        SRVSVCFreeString(pszConfigFilePath);
    }

    goto cleanup;

}

static
VOID
SRVSVCInterruptHandler(
    int Signal
    )
{
    if (Signal == SIGINT)
    {
        raise(SIGTERM);
    }
}

static
PVOID
SRVSVCHandleSignals(
    PVOID pArg
    )
{
    DWORD dwError = 0;
    struct sigaction action;
    sigset_t catch_signal_mask;
    int which_signal = 0;
    int sysRet = 0;
    unsigned32 status = 0;

    // After starting up threads, we now want to handle SIGINT async
    // instead of using sigwait() on it.  The reason for this is so
    // that a debugger (such as gdb) can break in properly.
    // See http://sourceware.org/ml/gdb/2007-03/msg00145.html and
    // http://bugzilla.kernel.org/show_bug.cgi?id=9039.

    memset(&action, 0, sizeof(action));
    action.sa_handler = SRVSVCInterruptHandler;

    sysRet = sigaction(SIGINT, &action, NULL);
    dwError = (sysRet != 0) ? errno : 0;
    BAIL_ON_SRVSVC_ERROR(dwError);

    // Unblock SIGINT
    sigemptyset(&catch_signal_mask);
    sigaddset(&catch_signal_mask, SIGINT);

    dwError = pthread_sigmask(SIG_UNBLOCK, &catch_signal_mask, NULL);
    BAIL_ON_SRVSVC_ERROR(dwError);

    // These should already be blocked...
    sigemptyset(&catch_signal_mask);
    sigaddset(&catch_signal_mask, SIGTERM);
    sigaddset(&catch_signal_mask, SIGQUIT);
    sigaddset(&catch_signal_mask, SIGHUP);
    sigaddset(&catch_signal_mask, SIGPIPE);

    while (1)
    {
        /* Wait for a signal to arrive */
        sigwait(&catch_signal_mask, &which_signal);

        switch (which_signal)
        {
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
            {
                rpc_mgmt_stop_server_listening(NULL, &status);
                SRVSVCSetProcessShouldExit(TRUE);

                break;
            }

            case SIGPIPE:
            {
                SRVSVC_LOG_DEBUG("Handled SIGPIPE");

                break;
            }
            case SIGHUP:
            {
                dwError = SRVSVCReadEventLogConfigSettings();
                BAIL_ON_SRVSVC_ERROR(dwError);

                break;
            }
        }
    }

error:
    return NULL;
}

/*
 * Set up the process environment to properly deal with signals.
 * By default, we isolate all threads from receiving asynchronous
 * signals. We create a thread that handles all async signals.
 * The signal handling actions are handled in the handler thread.
 *
 * For AIX, we cant use a thread that sigwaits() on a specific signal,
 * we use a plain old, lame old Unix signal handler.
 *
 */
static
DWORD
SRVSVCStartSignalHandler()
{
    DWORD dwError = 0;

    dwError = pthread_create(&gSignalHandlerThread,
                             NULL,
                             SRVSVCHandleSignals,
                             NULL);
    BAIL_ON_SRVSVC_ERROR(dwError);

    pgSignalHandlerThread = &gSignalHandlerThread;

cleanup:

    return (dwError);

error:

    pgSignalHandlerThread = NULL;

    goto cleanup;
}


int
main(
    int argc,
    char* argv[])
{
    DWORD dwError = 0;
    rpc_binding_vector_p_t pServerBinding = NULL;
    DWORD dwBindAttempts = 0;
    static const DWORD dwMaxBindAttempts = 5;
    static const DWORD dwBindSleepSeconds = 5;

    dwError = SRVSVCSetServerDefaults();
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SRVSVCParseArgs(
                    argc,
                    argv,
                    &gServerInfo
                 );
    BAIL_ON_SRVSVC_ERROR(dwError);

    if (gServerInfo.dwStartAsDaemon) {

        dwError = SRVSVCStartAsDaemon();
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    if (atexit(SRVSVCExitHandler) < 0) {
        dwError = errno;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    SRVSVCCreatePIDFile();

    dwError = SRVSVCInitLogging(get_program_name(argv[0]));
    BAIL_ON_SRVSVC_ERROR(dwError);

    SRVSVCBlockSelectedSignals();

    /* Binding to our RPC endpoint might fail if dcerpcd is not
       yet ready when we start, so attempt it in a loop with
       a small delay between attempts */
    for (dwBindAttempts = 0; dwBindAttempts < dwMaxBindAttempts; dwBindAttempts++)
    {
        dwError = SRVSVCRegisterForRPC("Likewise Server Service",
                                    &pServerBinding);
        if (dwError)
        {
            SRVSVC_LOG_INFO("Failed to bind endpoint; retrying in %i seconds...", (int) dwBindSleepSeconds);
            sleep(dwBindSleepSeconds);
        }
        else
        {
            break;
        }
    }
    /* Bail if we still haven't succeeded after several attempts */
    BAIL_ON_SRVSVC_ERROR(dwError);

    //Read the event log information from srvsvcd.conf
    dwError = SRVSVCReadEventLogConfigSettings();
    if (dwError != 0)
    {
        SRVSVC_LOG_ERROR("Failed to read srvsvcd config file.  Error code: [%u]\n", dwError);
        dwError = 0;
    }

    dwError = SRVSVCStartSignalHandler();
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SRVSVCListenForRPC();
    BAIL_ON_SRVSVC_ERROR(dwError);

    SRVSVC_LOG_INFO("Likewise Server Service exiting...");

 cleanup:

    /*
     * Indicate that the process is exiting
     */
    SRVSVCSetProcessShouldExit(TRUE);

    SRVSVCStopSignalHandler();

    if (pServerBinding) {
        SRVSVCUnregisterForRPC(pServerBinding);
    }

    SRVSVCCloseLog();

    SRVSVCSetConfigDefaults();

    SRVSVCSetProcessExitCode(dwError);

    exit (dwError);

error:

    SRVSVC_LOG_ERROR("Likewise Server Service exiting due to error [code:%d]", dwError);

    goto cleanup;
}
