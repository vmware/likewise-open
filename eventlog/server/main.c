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
 * Eventlog Service (Process Utilities)
 *
 */
#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS 1
#endif

#include "includes.h"


EVTSERVERINFO gServerInfo =
{
    PTHREAD_MUTEX_INITIALIZER,  /* Lock              */
    0,                          /* Start as daemon   */
    FALSE,                      /* Log to syslog */
    LOG_LEVEL_ERROR,            /* Max Log Level     */
    "",                         /* Log file path     */
    "",                         /* Config file path  */
    "",                         /* Cache path        */
    "",                         /* Prefix path       */
    0,                          /* Process exit flag */
    0,                          /* Process exit code */
    0,                          /* Replace existing db flag */
    0,                          /* Max log size  */
    0,                          /* Max records */
    0,                          /* Remove records older than*/
    0,                          /* Enable/disable Remove records a boolean value TRUE or FALSE*/
    { NULL, NULL },             /* Who is allowed to read events   */
    { NULL, NULL },             /* Who is allowed to write events  */
    { NULL, NULL }              /* Who is allowed to delete events */
};

#define EVT_LOCK_SERVERINFO   pthread_mutex_lock(&gServerInfo.lock)
#define EVT_UNLOCK_SERVERINFO pthread_mutex_unlock(&gServerInfo.lock)




static
DWORD
EVTGetProcessExitCode(
    PDWORD pdwExitCode
    );

static
void
EVTExitHandler(
    void
    )
{
    DWORD dwError = 0;
    DWORD dwExitCode = 0;
    CHAR  szErrCodeFilePath[PATH_MAX+1];
    PSTR  pszCachePath = NULL;
    BOOLEAN  bFileExists = 0;
    FILE* fp = NULL;

    dwError = EVTGetCachePath(&pszCachePath);
    BAIL_ON_EVT_ERROR(dwError);

    sprintf(szErrCodeFilePath, "%s/eventlogd.err", pszCachePath);

    dwError = EVTCheckFileExists(szErrCodeFilePath, &bFileExists);
    BAIL_ON_EVT_ERROR(dwError);

    if (bFileExists) {
        dwError = EVTRemoveFile(szErrCodeFilePath);
        BAIL_ON_EVT_ERROR(dwError);
    }

    dwError = EVTGetProcessExitCode(&dwExitCode);
    BAIL_ON_EVT_ERROR(dwError);

    if (dwExitCode) {
        fp = fopen(szErrCodeFilePath, "w");
        if (fp == NULL) {
            dwError = errno;
            BAIL_ON_EVT_ERROR(dwError);
        }
        fprintf(fp, "%d\n", dwExitCode);
    }

error:

    if (pszCachePath) {
        EVTFreeString(pszCachePath);
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
EVTProcessShouldExit()
{
    BOOLEAN bResult = 0;

    EVT_LOCK_SERVERINFO;

    bResult = gServerInfo.bProcessShouldExit;

    EVT_UNLOCK_SERVERINFO;

    return bResult;
}

void
EVTSetProcessShouldExit(
    BOOLEAN val
    )
{
    EVT_LOCK_SERVERINFO;

    gServerInfo.bProcessShouldExit = val;

    EVT_UNLOCK_SERVERINFO;
}

DWORD
EVTGetProcessExitCode(
    PDWORD pdwExitCode
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pdwExitCode = gServerInfo.dwExitCode;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

void
EVTSetProcessExitCode(
    DWORD dwExitCode
    )
{
    EVT_LOCK_SERVERINFO;

    gServerInfo.dwExitCode = dwExitCode;

    EVT_UNLOCK_SERVERINFO;
}

DWORD
EVTGetCachePath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    dwError = EVTAllocateString(gServerInfo.szCachePath, ppszPath);

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetConfigPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    dwError = EVTAllocateString(gServerInfo.szConfigFilePath, ppszPath);

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetMaxRecords(
    DWORD* pdwMaxRecords
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pdwMaxRecords = gServerInfo.dwMaxRecords;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetMaxAge(
    DWORD* pdwMaxAge
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pdwMaxAge = gServerInfo.dwMaxAge;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetMaxLogSize(
    DWORD* pdwMaxLogSize
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pdwMaxLogSize = gServerInfo.dwMaxLogSize;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetDBPurgeInterval(
    PDWORD pdwPurgeInterval
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *pdwPurgeInterval = gServerInfo.dwPurgeInterval;

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    dwError = EVTAllocateString(gServerInfo.szPrefixPath, ppszPath);

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

DWORD
EVTGetAllowReadToLocked(
    PEVTALLOWEDDATA * ppAllowReadTo
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *ppAllowReadTo = &gServerInfo.pAllowReadTo;

    return (dwError);
}

DWORD
EVTGetAllowWriteToLocked(
    PEVTALLOWEDDATA * ppAllowWriteTo
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *ppAllowWriteTo = &gServerInfo.pAllowWriteTo;

    return (dwError);
}

DWORD
EVTGetAllowDeleteToLocked(
    PEVTALLOWEDDATA * ppAllowDeleteTo
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    *ppAllowDeleteTo = &gServerInfo.pAllowDeleteTo;

    return (dwError);
}

void
EVTUnlockServerInfo()
{
    EVT_UNLOCK_SERVERINFO;
}

void
EVTFreeAllowData(
    PEVTALLOWEDDATA pAllowData
    )
{
    EVT_SAFE_FREE_STRING(pAllowData->configData);
    EVTAccessFreeData(pAllowData->pAllowedTo);
    pAllowData->configData = NULL;
    pAllowData->pAllowedTo = NULL;
}

DWORD
EVTSetAllowData(
    PCSTR   pszValue,
    PEVTALLOWEDDATA pAllowData
    )
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    EVTFreeAllowData(pAllowData);

    dwError = EVTAllocateString(
                  pszValue,
                  &pAllowData->configData);    

    if ( !dwError )
    {
        dwError = EVTAccessGetData(
                      pszValue,
                      &pAllowData->pAllowedTo);
    }

    EVT_UNLOCK_SERVERINFO;

    return (dwError);
}

static
void
ShowUsage(
    const PSTR pszProgramName
    )
{
    printf("Usage: %s [--start-as-daemon]\n"
            "          [--syslog]\n"
            "          [--logfile logFilePath]\n"
            "          [--replacedb]\n"
            "          [--loglevel {0, 1, 2, 3, 4, 5}]\n"
            "          [--configfile configfilepath]\n", pszProgramName);
}

void
get_server_info_r(
    PEVTSERVERINFO pServerInfo
    )
{
    if (pServerInfo == NULL) {
        return;
    }

    EVT_LOCK_SERVERINFO;

    memcpy(pServerInfo, &gServerInfo, sizeof(EVTSERVERINFO));

    pthread_mutex_init(&pServerInfo->lock, NULL);

    EVT_UNLOCK_SERVERINFO;
}

static
DWORD
EVTParseArgs(
    int argc,
    PSTR argv[],
    PEVTSERVERINFO pEVTServerInfo
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
                    pEVTServerInfo->dwStartAsDaemon = 1;
                }
                else if (strcmp(pArg, "--configfile") == 0) {
                    parseMode = PARSE_MODE_CONFIGFILE;
                }
                else if (strcmp(pArg, "--syslog") == 0)
                {
                    pEVTServerInfo->bLogToSyslog = TRUE;
                }
                else if (strcmp(pArg, "--loglevel") == 0) {
                    parseMode = PARSE_MODE_LOGLEVEL;
                }
                else if (strcmp(pArg, "--replacedb") == 0) {
                    pEVTServerInfo->bReplaceDB = TRUE;
                } else {
                    EVT_LOG_ERROR("Unrecognized command line option [%s]",
                                    pArg);
                    ShowUsage(get_program_name(argv[0]));
                    exit(1);
                }
            }
            break;
            case PARSE_MODE_LOGFILE:
            {
                strncpy(pEVTServerInfo->szLogFilePath, pArg, PATH_MAX);
                *(pEVTServerInfo->szLogFilePath+PATH_MAX) = '\0';
                parseMode = PARSE_MODE_OPEN;
            }
            break;
            case PARSE_MODE_LOGLEVEL:
            {
                dwLogLevel = atoi(pArg);

                if (dwLogLevel < LOG_LEVEL_ALWAYS || dwLogLevel > LOG_LEVEL_DEBUG) {

                    EVT_LOG_ERROR("Error: Invalid log level [%d]", dwLogLevel);
                    ShowUsage(get_program_name(argv[0]));
                    exit(1);
                }

                pEVTServerInfo->dwLogLevel = dwLogLevel;

                parseMode = PARSE_MODE_OPEN;
            }
            break;
            case PARSE_MODE_CONFIGFILE:
            {
                strncpy(pEVTServerInfo->szConfigFilePath, pArg, PATH_MAX);
                *(pEVTServerInfo->szConfigFilePath+PATH_MAX) = '\0';
                parseMode = PARSE_MODE_OPEN;

            }
            break;
        }
    } while (iArg < argc);

    return 0;
}

static
DWORD
EVTStartAsDaemon()
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
        BAIL_ON_EVT_ERROR(dwError);
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
    BAIL_ON_EVT_ERROR(dwError);

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

#define DAEMON_NAME "likewise-eventlogd"
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
EVTCreatePIDFile()
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
EVTSetConfigDefaults()
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    gServerInfo.dwMaxLogSize = EVT_DEFAULT_MAX_LOG_SIZE;
    gServerInfo.dwMaxRecords =  EVT_DEFAULT_MAX_RECORDS;
    gServerInfo.dwMaxAge = EVT_DEFAULT_MAX_AGE;
    gServerInfo.dwPurgeInterval = EVT_DEFAULT_PURGE_INTERVAL;

    EVTFreeAllowData(&gServerInfo.pAllowReadTo);
    EVTFreeAllowData(&gServerInfo.pAllowWriteTo);
    EVTFreeAllowData(&gServerInfo.pAllowDeleteTo);

    EVT_UNLOCK_SERVERINFO;

    return dwError;
}

static
DWORD
EVTSetServerDefaults()
{
    DWORD dwError = 0;

    EVT_LOCK_SERVERINFO;

    gServerInfo.dwLogLevel = LOG_LEVEL_ERROR;

    *(gServerInfo.szLogFilePath) = '\0';

    memset(gServerInfo.szConfigFilePath, 0, PATH_MAX+1);
    strncpy(gServerInfo.szConfigFilePath, DEFAULT_CONFIG_FILE_PATH, PATH_MAX);
    strcpy(gServerInfo.szCachePath, CACHEDIR);
    strcpy(gServerInfo.szPrefixPath, PREFIXDIR);

    EVT_UNLOCK_SERVERINFO;

    dwError = EVTSetConfigDefaults();

    return dwError;
}

static
void
EVTBlockSelectedSignals()
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
EVTInitLogging(
    PSTR pszProgramName
    )
{
    if ((gServerInfo.dwStartAsDaemon &&
            gServerInfo.szLogFilePath[0] == '\0') ||
            gServerInfo.bLogToSyslog)
    {

        return EVTInitLoggingToSyslog(gServerInfo.dwLogLevel,
                                      pszProgramName,
                                      LOG_PID,
                                      LOG_DAEMON);
    }
    else
    {
        return EVTInitLoggingToFile(gServerInfo.dwLogLevel,
                                    gServerInfo.szLogFilePath);
    }
}

static
DWORD
EVTStopSignalHandler()
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
EVTConfigStartSection(
    PCSTR    pszSectionName,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{

    //This callback may not be required,retaining it for future
    EVT_LOG_VERBOSE("EVTConfigStartSection: SECTION Name=%s", pszSectionName);

    *pbSkipSection = FALSE;
    *pbContinue = TRUE;

    return 0;
}

DWORD
EVTConfigComment(
    PCSTR    pszComment,
    PBOOLEAN pbContinue
    )
{
    //This callback may not be required,retaining it for future
    EVT_LOG_VERBOSE("EVTConfigComment: %s",
        (IsNullOrEmptyString(pszComment) ? "" : pszComment));

    *pbContinue = TRUE;

    return 0;
}

DWORD
EVTConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PBOOLEAN pbContinue
    )
{

    //strip the white spaces
    EVTStripWhitespace((PSTR)pszName,1,1);
    EVTStripWhitespace((PSTR)pszValue,1,1);

    EVT_LOG_INFO("EVTConfigNameValuePair: NAME=%s, VALUE=%s",
        (IsNullOrEmptyString(pszName) ? "" : pszName),
        (IsNullOrEmptyString(pszValue) ? "" : pszValue));

    if ( !strcmp(pszName, "max-disk-usage") ) {
        DWORD dwDiskUsage = 0;
        EVTParseDiskUsage((PCSTR)pszValue, &dwDiskUsage);
        EVT_LOCK_SERVERINFO;
        gServerInfo.dwMaxLogSize = dwDiskUsage;
        EVT_UNLOCK_SERVERINFO;
    }
    else if ( !strcmp(pszName, "max-num-events") ) {
        DWORD dwMaxEntries = 0;
        EVTParseMaxEntries((PCSTR)pszValue, &dwMaxEntries);
        EVT_LOCK_SERVERINFO;
        gServerInfo.dwMaxRecords = dwMaxEntries;
        EVT_UNLOCK_SERVERINFO;
    }
    else if ( !strcmp(pszName, "max-event-lifespan") ) {
        DWORD dwMaxLifeSpan = 0;
        EVTParseDays((PCSTR)pszValue, &dwMaxLifeSpan);
        EVT_LOCK_SERVERINFO;
        gServerInfo.dwMaxAge = dwMaxLifeSpan;
        EVT_UNLOCK_SERVERINFO;
    }
    else if ( !strcmp(pszName, "event-db-purge-interval") ) {
		DWORD dwPurgeInterval = 0;
		EVTParseDays((PCSTR)pszValue, &dwPurgeInterval);
        EVT_LOCK_SERVERINFO;
        gServerInfo.dwPurgeInterval = dwPurgeInterval;
        EVT_UNLOCK_SERVERINFO;
    }
    else if ( !strcmp(pszName, "allow-read-to") ) {
        EVTSetAllowData( pszValue, &gServerInfo.pAllowReadTo);
    }
    else if ( !strcmp(pszName, "allow-write-to") ) {
        EVTSetAllowData( pszValue, &gServerInfo.pAllowWriteTo);
    }
    else if ( !strcmp(pszName, "allow-delete-to") ) {
        EVTSetAllowData( pszValue, &gServerInfo.pAllowDeleteTo);
    }

    *pbContinue = TRUE;

    return 0;
}

DWORD
EVTConfigEndSection(
    PCSTR pszSectionName,
    PBOOLEAN pbContinue
    )
{
    //This callback may not be required,retaining it for future
    EVT_LOG_VERBOSE("EVTConfigEndSection: SECTION Name=%s", pszSectionName);

    *pbContinue = TRUE;

    return 0;
}


static
DWORD
EVTReadEventLogConfigSettings()
{
    DWORD dwError = 0;
    PSTR pszConfigFilePath = NULL;

    EVT_LOG_INFO("Read Eventlog configuration settings");

    dwError = EVTSetConfigDefaults();
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTGetConfigPath(&pszConfigFilePath);
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTParseConfigFile(
                pszConfigFilePath,
                &EVTConfigStartSection,
                &EVTConfigComment,
                &EVTConfigNameValuePair,
                &EVTConfigEndSection);
    BAIL_ON_EVT_ERROR(dwError);

    if (pszConfigFilePath) {
        EVTFreeString(pszConfigFilePath);
        pszConfigFilePath = NULL;
    }

cleanup:

    return dwError;

error:

    if (pszConfigFilePath) {
        EVTFreeString(pszConfigFilePath);
    }

    goto cleanup;

}

static
VOID
EVTInterruptHandler(
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
EVTHandleSignals(
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
    action.sa_handler = EVTInterruptHandler;

    sysRet = sigaction(SIGINT, &action, NULL);
    dwError = (sysRet != 0) ? errno : 0;
    BAIL_ON_EVT_ERROR(dwError);

    // Unblock SIGINT
    sigemptyset(&catch_signal_mask);
    sigaddset(&catch_signal_mask, SIGINT);

    dwError = pthread_sigmask(SIG_UNBLOCK, &catch_signal_mask, NULL);
    BAIL_ON_EVT_ERROR(dwError);

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
                EVTSetProcessShouldExit(TRUE);

                break;
            }

            case SIGPIPE:
            {
                EVT_LOG_DEBUG("Handled SIGPIPE");

                break;
            } 
            case SIGHUP:
            {
                dwError = EVTReadEventLogConfigSettings();
                BAIL_ON_EVT_ERROR(dwError);

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
EVTStartSignalHandler()
{
    DWORD dwError = 0;

    dwError = pthread_create(&gSignalHandlerThread,
                             NULL,
                             EVTHandleSignals,
                             NULL);
    BAIL_ON_EVT_ERROR(dwError);

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

    dwError = EVTSetServerDefaults();
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTLoadLsaLibrary();
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTParseArgs(
                    argc,
                    argv,
                    &gServerInfo
                 );
    BAIL_ON_EVT_ERROR(dwError);


    dwError = SrvCreateDB(gServerInfo.bReplaceDB);
    BAIL_ON_EVT_ERROR(dwError);

    if (gServerInfo.bReplaceDB) {
        goto cleanup;
    }

    if (gServerInfo.dwStartAsDaemon) {

        dwError = EVTStartAsDaemon();
        BAIL_ON_EVT_ERROR(dwError);
    }

    if (atexit(EVTExitHandler) < 0) {
        dwError = errno;
        BAIL_ON_EVT_ERROR(dwError);
    }

    EVTCreatePIDFile();

    dwError = EVTInitLogging(get_program_name(argv[0]));
    BAIL_ON_EVT_ERROR(dwError);

    EVTBlockSelectedSignals();

    /* Binding to our RPC endpoint might fail if dcerpcd is not
       yet ready when we start, so attempt it in a loop with
       a small delay between attempts */
    for (dwBindAttempts = 0; dwBindAttempts < dwMaxBindAttempts; dwBindAttempts++)
    {
        dwError = EVTRegisterForRPC("Likewise Eventlog Service",
                                    &pServerBinding);
        if (dwError)
        {
            EVT_LOG_INFO("Failed to bind endpoint; retrying in %i seconds...", (int) dwBindSleepSeconds);
            sleep(dwBindSleepSeconds);
        }
        else
        {
            break;
        }
    }
    /* Bail if we still haven't succeeded after several attempts */
    BAIL_ON_EVT_ERROR(dwError);

    //Read the event log information from eventlog-settings.conf
    dwError = EVTReadEventLogConfigSettings();
    if (dwError != 0)
    {
        EVT_LOG_ERROR("Failed to read eventlog config file.  Error code: [%u]\n", dwError);
        dwError = 0;
    }

    dwError = EVTStartSignalHandler();
    BAIL_ON_EVT_ERROR(dwError);

    dwError = SrvInitEventDatabase();
    BAIL_ON_EVT_ERROR(dwError);

    dwError = EVTListenForRPC();
    BAIL_ON_EVT_ERROR(dwError);

    EVT_LOG_INFO("Eventlog Service exiting...");

 cleanup:

    /*
     * Indicate that the process is exiting
     */
    EVTSetProcessShouldExit(TRUE);

    EVTStopSignalHandler();

    if (pServerBinding) {
        EVTUnregisterForRPC(pServerBinding);
    }

    EVTCloseLog();

    SrvShutdownEventDatabase();

    EVTSetConfigDefaults();
    EVTUnloadLsaLibrary();

    EVTSetProcessExitCode(dwError);

    exit (dwError);

error:

    EVT_LOG_ERROR("Eventlog exiting due to error [code:%d]", dwError);

    goto cleanup;
}
