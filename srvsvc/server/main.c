/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

#include "includes.h"

DWORD
SrvSvcSetServerDefaults(
    VOID
    );

static
DWORD
SrvSvcParseArgs(
    int                     argc,
    PSTR                    argv[],
    PSRVSVC_RUNTIME_GLOBALS pServerInfo
    );

static
void
ShowUsage(
    PCSTR pszProgramName
    );

static
PSTR
get_program_name(
    PSTR pszFullProgramPath
    );

static
VOID
SrvSvcExitHandler(
    VOID
    );

static
DWORD
SrvSvcGetProcessExitCode(
    PDWORD pdwExitCode
    );

static
VOID
SrvSvcSetProcessExitCode(
    DWORD dwExitCode
    );

static
DWORD
SrvSvcStartAsDaemon(
    VOID
    );

DWORD
SrvSvcRpcInitialize(
    VOID
    );

VOID
SrvSvcRpcShutdown(
    VOID
    );

DWORD
SrvSvcDSNotify(
    VOID
    );

VOID
SrvSvcDSShutdown(
    VOID
    );

DWORD
SrvSvcSMNotify(
    VOID
    );

int
main(
    int argc,
    char* argv[])
{
    DWORD dwError = 0;

    dwError = SrvSvcSetServerDefaults();
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcParseArgs(argc, argv, &gServerInfo);
    BAIL_ON_SRVSVC_ERROR(dwError);

    if (atexit(SrvSvcExitHandler) < 0)
    {
        dwError = errno;
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    if (gServerInfo.dwStartAsDaemon)
    {
        dwError = SrvSvcStartAsDaemon();
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    dwError = SrvSvcReadConfigSettings();
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcInitSecurity();
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcRpcInitialize();
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcDSNotify();
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = SrvSvcSMNotify();
    BAIL_ON_SRVSVC_ERROR(dwError);

 cleanup:
    SRVSVC_LOG_INFO("Likewise Server Service exiting...");

    SrvSvcSetProcessShouldExit(TRUE);

    SrvSvcDSShutdown();

    SrvSvcRpcShutdown();

    SrvSvcSetProcessExitCode(dwError);

    exit (dwError);

error:

    SRVSVC_LOG_ERROR("Likewise Server Service exiting due to error [code:%d]",
                        dwError);

    goto cleanup;
}


DWORD
SrvSvcSetServerDefaults(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = SrvSvcReadConfigSettings();

    return dwError;
}

static
DWORD
SrvSvcParseArgs(
    int                     argc,
    PSTR                    argv[],
    PSRVSVC_RUNTIME_GLOBALS pServerInfo
    )
{
    DWORD dwError = 0;

    typedef enum {
        PARSE_MODE_OPEN = 0,
    } ParseMode;

    ParseMode parseMode = PARSE_MODE_OPEN;
    int iArg = 1;
    PSTR pArg = NULL;

    do
    {
        pArg = argv[iArg++];
        if (pArg == NULL || *pArg == '\0') {
            break;
        }

        switch(parseMode)
        {
            case PARSE_MODE_OPEN:

                if ((strcmp(pArg, "--help") == 0) ||
                    (strcmp(pArg, "-h") == 0))
                {
                    ShowUsage(get_program_name(argv[0]));
                    exit(0);
                }
                else if (strcmp(pArg, "--start-as-daemon") == 0)
                {
                    pServerInfo->dwStartAsDaemon = 1;
                }
                else
                {
                    SRVSVC_LOG_ERROR("Unrecognized command line option [%s]",
                                    pArg);
                    ShowUsage(get_program_name(argv[0]));
                    exit(1);
                }

                break;
        }
    } while (iArg < argc);

    return dwError;
}

static
void
ShowUsage(
    PCSTR pszProgramName
    )
{
    printf("Usage: %s [--start-as-daemon]\n"
            "          [--syslog]\n"
            "          [--logfile logFilePath]\n"
            "          [--loglevel {error, warning, info, verbose, debug}]\n",
            pszProgramName);
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

static
VOID
SrvSvcExitHandler(
    VOID
    )
{
    DWORD dwError = 0;
    DWORD dwExitCode = 0;
    CHAR  szErrCodeFilePath[PATH_MAX+1];
    CHAR  szCachePath[] = CACHEDIR;
    BOOLEAN  bFileExists = 0;
    FILE* fp = NULL;

    sprintf(szErrCodeFilePath, "%s/srvsvcd.err", szCachePath);

    dwError = SrvSvcCheckFileExists(szErrCodeFilePath, &bFileExists);
    BAIL_ON_SRVSVC_ERROR(dwError);

    if (bFileExists) {
        dwError = SrvSvcRemoveFile(szErrCodeFilePath);
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    dwError = SrvSvcGetProcessExitCode(&dwExitCode);
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

    if (fp != NULL) {
        fclose(fp);
    }
}

VOID
SrvSvcSetProcessShouldExit(
    BOOLEAN val
    )
{
    BOOLEAN bInLock = FALSE;

    SRVSVC_LOCK_MUTEX(bInLock, &gServerInfo.mutex);

    gServerInfo.bProcessShouldExit = val;

    SRVSVC_UNLOCK_MUTEX(bInLock, &gServerInfo.mutex);
}

static
DWORD
SrvSvcGetProcessExitCode(
    PDWORD pdwExitCode
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SRVSVC_LOCK_MUTEX(bInLock, &gServerInfo.mutex);

    *pdwExitCode = gServerInfo.dwExitCode;

    SRVSVC_UNLOCK_MUTEX(bInLock, &gServerInfo.mutex);

    return (dwError);
}

static
VOID
SrvSvcSetProcessExitCode(
    DWORD dwExitCode
    )
{
    BOOLEAN bInLock = FALSE;

    SRVSVC_LOCK_MUTEX(bInLock, &gServerInfo.mutex);

    gServerInfo.dwExitCode = dwExitCode;

    SRVSVC_UNLOCK_MUTEX(bInLock, &gServerInfo.mutex);
}

static
DWORD
SrvSvcStartAsDaemon(
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


DWORD
SrvSvcRpcInitialize(
    VOID
    )
{
    DWORD    dwError        = 0;
    DWORD    dwBindAttempts = 0;
    BOOLEAN  bInLock        = FALSE;
    static const DWORD dwMaxBindAttempts  = 5;
    static const DWORD dwBindSleepSeconds = 5;

    SRVSVC_LOCK_MUTEX(bInLock, &gServerInfo.mutex);

    /* Binding to our RPC end-point might fail if dcerpcd is not
       yet ready when we start, so attempt it in a loop with
       a small delay between attempts */
    for (dwBindAttempts = 0; dwBindAttempts < dwMaxBindAttempts; dwBindAttempts++)
    {
        dwError = SrvSvcRegisterForRPC(
                        "Likewise Server Service",
                        &gServerInfo.pServerBinding);
        if (dwError)
        {
            SRVSVC_LOG_INFO("Failed to bind srvsvc endpoint; retrying in "
                            "%i seconds...",
                            (int) dwBindSleepSeconds);
            sleep(dwBindSleepSeconds);
        }
        else
        {
            break;
        }
    }
    /* Bail if we still haven't succeeded after several attempts */
    BAIL_ON_SRVSVC_ERROR(dwError);

    /* Now register the winreg pipe */

    for (dwBindAttempts = 0; dwBindAttempts < dwMaxBindAttempts; dwBindAttempts++)
    {
        dwError = WinRegRegisterForRPC(
                        "Likewise Registry Service",
                        &gServerInfo.pRegistryBinding);
        if (dwError)
        {
            SRVSVC_LOG_INFO("Failed to bind wkssvc endpoint; retrying in "
                            "%i seconds...",
                            (int) dwBindSleepSeconds);
            sleep(dwBindSleepSeconds);
        }
        else
        {
            break;
        }
    }
    /* Bail if we still haven't succeeded after several attempts */
    BAIL_ON_SRVSVC_ERROR(dwError);

    dwError = LwMapErrnoToLwError(dcethread_create(
                                        &gServerInfo.pRpcListenerThread,
                                        NULL,
                                        &SrvSvcListenForRPC,
                                        NULL));
    BAIL_ON_SRVSVC_ERROR(dwError);

    while (!SrvSvcRpcIsListening())
    {
    }

cleanup:

    SRVSVC_UNLOCK_MUTEX(bInLock, &gServerInfo.mutex);

    return dwError;

error:

    goto cleanup;
}


VOID
SrvSvcRpcShutdown(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SRVSVC_LOCK_MUTEX(bInLock, &gServerInfo.mutex);

    if (gServerInfo.pServerBinding)
    {
        dwError = SrvSvcUnregisterForRPC(gServerInfo.pServerBinding);
        BAIL_ON_SRVSVC_ERROR(dwError);

        gServerInfo.pServerBinding = NULL;
    }

    if (gServerInfo.pRegistryBinding)
    {
        dwError = WinRegUnregisterForRPC(gServerInfo.pRegistryBinding);
        BAIL_ON_SRVSVC_ERROR(dwError);

        gServerInfo.pRegistryBinding = NULL;
    }

    SRVSVC_UNLOCK_MUTEX(bInLock, &gServerInfo.mutex);

    dwError = SrvSvcRpcStopListening();
    BAIL_ON_SRVSVC_ERROR(dwError);

    SRVSVC_LOCK_MUTEX(bInLock, &gServerInfo.mutex);

    if (gServerInfo.pRpcListenerThread)
    {
        dwError = LwMapErrnoToLwError(dcethread_interrupt(gServerInfo.pRpcListenerThread));
        BAIL_ON_SRVSVC_ERROR(dwError);

        dwError = LwMapErrnoToLwError(dcethread_join(gServerInfo.pRpcListenerThread, NULL));
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

    if (gServerInfo.pSessionSecDesc)
    {
        SrvSvcSrvDestroyServerSecurityDescriptor(gServerInfo.pSessionSecDesc);
        gServerInfo.pSessionSecDesc = NULL;
    }

cleanup:

    SRVSVC_UNLOCK_MUTEX(bInLock, &gServerInfo.mutex);

    return;

error:

    goto cleanup;
}


DWORD
SrvSvcDSNotify(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = LwDsCacheAddPidException(getpid());
    if (dwError == LW_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK)
    {
        SRVSVC_LOG_ERROR(   "Could not register process pid (%d) "
                            "with Mac DirectoryService Cache plugin",
                            (int) getpid());
        BAIL_ON_SRVSVC_ERROR(dwError);
    }

error:

    return dwError;
}


VOID
SrvSvcDSShutdown(
    VOID
    )
{
    LwDsCacheRemovePidException(getpid());
}


DWORD
SrvSvcSMNotify(
    VOID
    )
{
    DWORD dwError     = 0;
    PCSTR pszSmNotify = NULL;
    int   notifyFd    = -1;

    if ((pszSmNotify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        int  ret        = 0;
        char notifyCode = 0;

        notifyFd = atoi(pszSmNotify);

        do
        {
            ret = write(notifyFd, &notifyCode, sizeof(notifyCode));
        } while(ret != sizeof(notifyCode) && errno == EINTR);

        if (ret < 0)
        {
            SRVSVC_LOG_ERROR("Could not notify service manager: %s (%i)",
                                strerror(errno),
                                errno);

            dwError = LwMapErrnoToLwError(errno);
            BAIL_ON_SRVSVC_ERROR(dwError);
        }
    }

cleanup:

    if (notifyFd >= 0)
    {
        close(notifyFd);
    }

    return dwError;

error:

    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
