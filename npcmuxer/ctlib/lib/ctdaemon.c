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

#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS
#endif

#include <config.h>
#include <sys/signal.h>
#include <ctdaemon.h>
#include <ctlock.h>
#include <sys/stat.h>
#include <ctgoto.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctmemory.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctstring.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>

typedef struct {
    const char* Program;
    const char* PidFile;
    bool IsExit;
    int ExitCode;
} CT_DAEMON_STATE, *PCT_DAEMON_STATE;

static pthread_mutex_t gCtpDaemonStateLock = CT_LOCK_INITIALIZER_MUTEX;
static PCT_DAEMON_STATE gCtpDaemonState = NULL;

#define CT_DAEMON_STATE_ACQUIRE() \
    CtLockAcquireMutex(&gCtpDaemonStateLock)

#define CT_DAEMON_STATE_RELEASE() \
    CtLockReleaseMutex(&gCtpDaemonStateLock)

CT_STATUS
CtCheckFileExists(
    IN const char* Path,
    OUT bool* Exists
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    struct stat statbuf = { 0 };
    bool exists = false;

    while (1)
    {
        if (stat(Path, &statbuf) < 0)
        {
            int error = errno;
            if (error == EINTR)
            {
                continue;
            }
            else if (error == ENOENT || error == ENOTDIR)
            {
                break;
            }
            status = CT_ERRNO_TO_STATUS(error);
            GOTO_CLEANUP();
        }
        else
        {
            if ((statbuf.st_mode & S_IFMT) == S_IFREG)
            {
                exists = true;
            }
            break;
        }
    }

cleanup:
    *Exists = exists;
    return status;
}

CT_STATUS
CtCaptureOutput(
    IN const char* Command,
    OUT char** Output
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    unsigned int buffer_size = 1024;
    unsigned int read_size;
    unsigned int write_size;
    int out[2] = { -1, -1 };
    pid_t pid = -1;
    int pidStatus;
    void* buffer = NULL;
    bool needWait = false;

    if (pipe(out))
    {
        status = CtErrnoToStatus(errno);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    pid = fork();
    if (pid < 0)
    {
        status = CtErrnoToStatus(errno);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    else if (pid == 0)
    {
        // Child process
        if (dup2(out[1], STDOUT_FILENO) < 0)
            abort();
        if (close(out[0]))
            abort();
        execl("/bin/sh", "/bin/sh", "-c", Command, (char*) NULL);
    }

    needWait = true;

    CT_SAFE_CLOSE_FD_WITH_STATUS(out[1], status);
    GOTO_CLEANUP_ON_STATUS(status);

    status = CtAllocateMemory(&buffer, buffer_size);
    GOTO_CLEANUP_ON_STATUS(status);

    write_size = 0;
    while ((read_size = read(out[0], buffer + write_size, buffer_size - write_size)) > 0)
    {
        write_size += read_size;
        if (write_size == buffer_size)
        {
            buffer_size *= 2;
            status = CtReallocMemory(&buffer, buffer_size);
            GOTO_CLEANUP_ON_STATUS(status);
        }
    }

    if (read_size < 0)
    {
        status = CtErrnoToStatus(errno);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    CT_SAFE_CLOSE_FD_WITH_STATUS(out[0], status);
    GOTO_CLEANUP_ON_STATUS(status);

    needWait = false;
    if (waitpid(pid, &pidStatus, 0) != pid)
    {
        status = CtErrnoToStatus(errno);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:
    if (status)
    {
        CT_SAFE_FREE(buffer);
    }
    CT_SAFE_CLOSE_FD(out[0]);
    CT_SAFE_CLOSE_FD(out[1]);
    if (needWait)
    {
        waitpid(pid, &pidStatus, 0);
    }

    *Output = buffer;
    return status;
}

CT_STATUS
CtCheckProgramPidRunning(
    IN const char* Program,
    IN pid_t Pid,
    OUT bool* IsRunning
    )
{
    CT_STATUS status;
    char* command = NULL;
    char* output = NULL;
    bool isRunning = false;

#if defined(__MACH__) && defined(__APPLE__)
    const char* format = "ps -p %d -o command= | grep %s";
#else
    const char* format = "UNIX95=1 ps -p %ld -o comm= | grep %s";
#endif
    status = CtAllocateStringPrintf(&command, format, (long)Pid, Program);
    GOTO_CLEANUP_ON_STATUS(status);

    status = CtCaptureOutput(command, &output);
    GOTO_CLEANUP_ON_STATUS(status);

    /* We need to make sure that the output is not empty */
    CtStripWhitespace(output);
    if (!CtIsNullOrEmptyString(output))
    {
        /* Technically, this is not really correct because the program name
            might be a substring.  We should really check for a match. */
        isRunning = true;
    }
    status = CT_STATUS_SUCCESS;

cleanup:
    CT_SAFE_FREE(output);
    CT_SAFE_FREE(command);

    *IsRunning = true;
    return status;
}

static
CT_STATUS
CtpRaiseSignal(
    IN int Signal
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int retval = raise(Signal);

    if (retval != 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
    }

    return status;
}


const char*
CtGetProgramName(
    const char* Path
    )
{
    const char* last = NULL;
    const char* current;

    if (!Path || !*Path)
    {
        return NULL;
    }

    for (current = Path; *current; current++)
    {
        if (*current == '/')
        {
            last = current;
        }
    }

    return last + 1;
}


bool
CtDaemonIsExit(
    )
{
    bool isExit = false;

    CT_DAEMON_STATE_ACQUIRE();
    if (gCtpDaemonState)
    {
        isExit = gCtpDaemonState->IsExit;
    }
    CT_DAEMON_STATE_RELEASE();
    return isExit;
}

CT_STATUS
CtDaemonExit(
    IN int ExitCode
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    bool sendSignal = false;

    CT_DAEMON_STATE_ACQUIRE();
    if (gCtpDaemonState)
    {
        gCtpDaemonState->IsExit = true;
        gCtpDaemonState->ExitCode = ExitCode;
        sendSignal = true;
    }
    CT_DAEMON_STATE_RELEASE();

    if (sendSignal)
    {
        status = CtpRaiseSignal(SIGTERM);
    }
    else
    {
        status = CT_STATUS_INVALID_PARAMETER;
    }

    return status;
}

static
CT_STATUS
CtpDaemonStartAsDaemon(
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    pid_t pid;
    int fd = 0;
    int iFd = 0;

    if ((pid = fork()) != 0)
    {
        // Parent terminates
        exit(0);
    }

    // Let the first child be a session leader
    setsid();

    // Ignore SIGHUP, because when the first child terminates
    // it would be a session leader, and thus all processes in
    // its session would receive the SIGHUP signal. By ignoring
    // this signal, we are ensuring that our second child will
    // ignore this signal and will continue execution.
    if (signal(SIGHUP, SIG_IGN) < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP();
    }

    // Spawn a second child
    if ((pid = fork()) != 0)
    {
        // Let the first child terminate
        // This will ensure that the second child cannot be a session leader
        // Therefore, the second child cannot hold a controlling terminal
        exit(0);
    }

    // This is the second child executing
    status = chdir("/");
    GOTO_CLEANUP_ON_STATUS(status);

    // Clear our file mode creation mask
    umask(0);

    for (iFd = 0; iFd < 3; iFd++)
    {
        close(iFd);
    }

    for (iFd = 0; iFd < 3; iFd++)
    {
        fd = open("/dev/null", O_RDWR, 0);
        if (fd < 0) {
            fd = open("/dev/null", O_WRONLY, 0);
        }
        if (fd < 0) {
            CT_LOG_ALWAYS("Failed to open fd %d: %d", iFd, errno);
            exit(1);
        }
        if (fd != iFd) {
            CT_LOG_ALWAYS("Mismatched file descriptors: got %d instead of %d", fd, iFd);
            exit(1);
        }
    }

cleanup:
    return status;
}

#define CT_DAEMON_PID_FILE_CONTENTS_SIZE ((9 * 2) + 2)

static
CT_STATUS
CtpDaemonGetPidFromPidFile(
    OUT pid_t* Pid,
    IN const char* PidFile
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    pid_t pid = 0;
    int fd = -1;
    int result;
    char contents[CT_DAEMON_PID_FILE_CONTENTS_SIZE];

    fd = open(PidFile, O_RDONLY, 0644);
    if (fd < 0)
    {
        int error = errno;
        if (error == ENOTDIR ||
            error == ENOENT)
        {
            status = CT_STATUS_SUCCESS;
            GOTO_CLEANUP();
        }
        status = CT_ERRNO_TO_STATUS(error);
        GOTO_CLEANUP();
    }

    result = read(fd, contents, sizeof(contents)-1);
    if (result < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP();
    }
    else if (result == 0)
    {
        status = CT_STATUS_SUCCESS;
        GOTO_CLEANUP();
    }

    contents[result-1] = 0;

    result = atoi(contents);
    if (result <= 0)
    {
        goto cleanup;
    }

    /* Send the 0 signal to check for the process */
    pid = (pid_t) result;
    result = kill(pid, 0);
    if (result < 0)
    {
        int error = errno;
        if (error == ESRCH)
        {
            pid = 0;
            goto cleanup;
        }
        status = CT_ERRNO_TO_STATUS(error);
        GOTO_CLEANUP();
    }

cleanup:
    CT_SAFE_CLOSE_FD(fd);

    if (CT_STATUS_IS_OK(status) && (pid <= 0))
    {
        unlink(PidFile);
    }

    *Pid = pid;

    return status;
}


static
CT_STATUS
CtpDaemonDeletePidFile(
    IN const char* Program,
    IN const char* PidFile
    )
{
    CT_STATUS status;
    pid_t pid;
    pid_t my_pid;
    bool exists;
    bool isRunning;

    status = CtpDaemonGetPidFromPidFile(&pid, PidFile);
    GOTO_CLEANUP_ON_STATUS(status);

    my_pid = getpid();
    if (pid != 0 && pid != my_pid)
    {
        status = CtCheckProgramPidRunning(Program, pid, &isRunning);
        GOTO_CLEANUP_ON_STATUS(status);

        if (isRunning)
        {
            CT_LOG_ERROR("Daemon already running as %ld", (long) pid);
            status = CT_STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP();
        }
    }

    status = CtCheckFileExists(PidFile, &exists);
    GOTO_CLEANUP_ON_STATUS(status);

    if (exists)
    {
        unlink(PidFile);
    }

cleanup:
    return status;
}

static
CT_STATUS
CtpDaemonCreatePidFile(
    IN const char* PidFile
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int fd = -1;
    pid_t pid;
    char contents[CT_DAEMON_PID_FILE_CONTENTS_SIZE];
    size_t len;
    size_t written;

    fd = open(PidFile, O_CREAT | O_WRONLY | O_EXCL, 0644);
    if (fd < 0)
    {
        int error = errno;
        CT_LOG_ERROR("Could not create pid file: %s", strerror(error));
        status = CT_ERRNO_TO_STATUS(error);
        GOTO_CLEANUP();
    }

    pid = getpid();
    snprintf(contents, sizeof(contents)-1, "%d\n", (int) pid);
    contents[sizeof(contents)-1] = 0;
    len = strlen(contents);

    written = write(fd, contents, len);
    if (written != len)
    {
        int error = errno;
        CT_LOG_ERROR("Could not write to pid file: %s", strerror(error));
        status = CT_ERRNO_TO_STATUS(error);
        GOTO_CLEANUP();
    }

    status = CT_STATUS_SUCCESS;

cleanup:
    if (fd != -1)
    {
        close(fd);
    }

    return status;
}

static
CT_STATUS
CtpDaemonHandleSignal(
    IN int Signal,
    OUT bool* IsExit
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    bool isExit = false;

    switch (Signal)
    {
        case SIGTERM:
        case SIGINT:
        {
            isExit = true;
            break;
        }
        case SIGHUP:
        {
            // Would handle config changes here.
            break;
        }
        default:
        {
            CT_LOG_INFO("Received signal %d", Signal);
        }
        break;
    }

    if (status)
    {
        isExit = true;
    }

    *IsExit = isExit;
    return status;
}

static
CT_STATUS
CtpDaemonStartThread(
    OUT pthread_t* Thread,
    IN CT_DAEMON_FUNCTION ThreadMain,
    IN void* Context
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int retval;

    retval = pthread_create(Thread, NULL, ThreadMain, Context);
    if (retval)
    {
        status = CtErrnoToStatus(errno);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:
    return status;
}

static
void
CtpDaemonStopThread(
    IN pthread_t Thread,
    IN CT_DAEMON_FUNCTION ThreadStop,
    IN void* Context
    )
{
    if (ThreadStop)
    {
        ThreadStop(Context);
    }

    pthread_join(Thread, NULL);
}


static
void
CtpDaemonExitHandler(
    )
{
    /* TODO: Perhaps write exit code out somewhere */

    CT_DAEMON_STATE_ACQUIRE();
    if (gCtpDaemonState)
    {
        CtpDaemonDeletePidFile(gCtpDaemonState->Program, gCtpDaemonState->PidFile);
    }
    CT_DAEMON_STATE_RELEASE();
}


CT_STATUS
CtDaemonStartLogger(
    IN CT_LOG_LEVEL LogLevel,
    IN bool IsDaemon,
    IN const char* SyslogIdentifier,
    IN const char* Path
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;

    if (IsDaemon)
    {
        status = CtLoggerSyslogOpen(LogLevel,
                                    SyslogIdentifier,
                                    LOG_PID,
                                    LOG_DAEMON);
    }
    else
    {
        status = CtLoggerFileOpen(LogLevel, Path);
    }
    CT_LOG_DEBUG("Logging initialized");

    return status;
}

void
CtDaemonStopLogger(
    )
{
    CtLoggerClose();
}

static
CT_STATUS
CtpDaemonBlockSignals(
    OUT sigset_t* SignalSet
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    int sysError = 0;
    sigset_t signalSet;

    // We don't want all our threads to get async signals
    // So, we will handle all the signals in our main thread
    sigemptyset(&signalSet);
    sigaddset(&signalSet, SIGTERM);
    sigaddset(&signalSet, SIGINT);
    sigaddset(&signalSet, SIGHUP);
    sigaddset(&signalSet, SIGPIPE);

    /* Block signals in the initial thread */
    sysError = pthread_sigmask(SIG_BLOCK, &signalSet, NULL);
    status = CtErrnoToStatus(sysError);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    *SignalSet = signalSet;
    return status;
}

static
void
CtpDaemonInterruptHandler(
    IN int Signal
    )
{
    if (Signal == SIGINT)
    {
        CtpRaiseSignal(SIGTERM);
    }
}

static
CT_STATUS
CtpDaemonSetupInterruptHandler(
    IN OUT sigset_t* SignalSet
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    int sysRet = 0;
    int sysError = 0;
    struct sigaction action;
    sigset_t signalSet;

    // After starting up threds, we now want to handle SIGINT async
    // instead of using sigwait() on it.  The reason for this is so
    // that a debugger (such as gdb) can break in properly.
    // See http://sourceware.org/ml/gdb/2007-03/msg00145.html and
    // http://bugzilla.kernel.org/show_bug.cgi?id=9039.

    memset(&action, 0, sizeof(action));
    action.sa_handler = CtpDaemonInterruptHandler;

    sysRet = sigaction(SIGINT, &action, NULL);
    sysError = (sysRet != 0) ? errno : 0;
    status = CtErrnoToStatus(sysError);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // Unblock SIGINT
    sigemptyset(&signalSet);
    sigaddset(&signalSet, SIGINT);

    sysError = pthread_sigmask(SIG_UNBLOCK, &signalSet, NULL);
    status = CtErrnoToStatus(sysError);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // Set up return value of signalSet to no longer include SIGINT.
    signalSet = *SignalSet;
    sigdelset(&signalSet, SIGINT);

cleanup:
    if (!status)
    {
        *SignalSet = signalSet;
    }
    return status;
}


static
CT_STATUS
CtpWaitSignal(
    IN sigset_t* SignalSet,
    OUT int* Signal
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    int sysRet = 0;
    int sysError = 0;
    int signal = 0;
#ifdef HAVE_SIGWAITINFO
    siginfo_t info;
#endif

    // ISSUE-2008/07/21-dalmeida -- Why did we add the sigwaitinfo?
    // It came in as part of a porting change (20666), but there
    // is no info as to the reason.  Originally, we just dig
    // sigwait because of the reason below.

    /* We use sigwait instead of sigtimedwait or sigwaitinfo
       because the former is the only thing available on
       Mac OS X. */
    
#ifdef HAVE_SIGWAITINFO
    sysRet = sigwaitinfo(SignalSet, &info);
    if (sysRet < 0)
        sysError = errno;
    else
        signal = sysRet;
#else
    sysRet = sigwait(SignalSet, &signal);
    sysError = (sysRet != 0) ? errno : 0;
#endif

    if (sysError)
    {
        if (sysError == EINTR)
        {
            sysError = 0;
        }
        status = CtErrnoToStatus(sysError);
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    *Signal = signal;
    return status;
}

CT_STATUS
CtDaemonRun(
    IN const char* Program,
    IN const char* PidFile,
    IN bool IsDaemon,
    IN CT_DAEMON_FUNCTION ThreadMain,
    IN CT_DAEMON_FUNCTION ThreadStop,
    IN void* Context,
    OUT int* ExitCode
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    CT_DAEMON_STATE state = { 0 };
    sigset_t signalSet;
    bool isStarted = false;
    bool isExit;
    pthread_t thread;
    int exitCode = 0;

    if (!ThreadMain)
    {
        status = CT_STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    state.Program = Program;
    state.PidFile = PidFile;

    CT_DAEMON_STATE_ACQUIRE();
    if (gCtpDaemonState)
    {
        status = CT_STATUS_INVALID_PARAMETER;
    }
    else
    {
        gCtpDaemonState = &state;
    }
    CT_DAEMON_STATE_RELEASE();
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    /* Try to delete before daemonizing so we can spit out
       error if another is running before becoming a daemon */
    status = CtpDaemonDeletePidFile(Program, PidFile);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (IsDaemon)
    {
        status = CtpDaemonStartAsDaemon();
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    /* Only run the daemon exit handler from the daemon proces. If this
     * is installed before running CtpDeamonStartAsDaemon, then it will
     * get run in both the parent and child process.
     */
    if (atexit(CtpDaemonExitHandler) < 0)
    {
        status = CtErrnoToStatus(errno);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    /* Create PID file after daemonizing so we are have proper PID */
    status = CtpDaemonCreatePidFile(PidFile);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // Block signals before starting up thread(s) so that we handle
    // signals in the main thread
    status = CtpDaemonBlockSignals(&signalSet);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    
    status = CtpDaemonStartThread(&thread, ThreadMain, Context);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    isStarted = true;

    // Enable handling of SIGINT now that we are started.
    status = CtpDaemonSetupInterruptHandler(&signalSet);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    isExit = false;
    do {
        int signal;

        status = CtpWaitSignal(&signalSet, &signal);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        if (signal)
        {
            status = CtpDaemonHandleSignal(signal, &isExit);
            GOTO_CLEANUP_ON_STATUS_EE(status, EE);
        }
    } while (!isExit);

cleanup:
    /*
     * Indicate that the process is exiting
     */

    // Use locks to ensure memory barriers are used
    CT_DAEMON_STATE_ACQUIRE();
    if (gCtpDaemonState)
    {
        gCtpDaemonState->IsExit = true;
    }
    CT_DAEMON_STATE_RELEASE();

    if (isStarted)
    {
        CtpDaemonStopThread(thread, ThreadStop, Context);
    }

    if (CT_STATUS_IS_OK(status))
    {
        // Use locks to ensure memory barriers are used
        CT_DAEMON_STATE_ACQUIRE();
        if (gCtpDaemonState)
        {
            exitCode = gCtpDaemonState->ExitCode;
            CT_DAEMON_STATE_RELEASE();
            CT_LOG_INFO("Daemon exiting (exit code = %d)", exitCode);
        }
        CT_DAEMON_STATE_RELEASE();
    }

    CT_DAEMON_STATE_ACQUIRE();
    gCtpDaemonState = NULL;
    CT_DAEMON_STATE_RELEASE();

    *ExitCode = exitCode;

    if (status)
    {
        CT_LOG_ERROR("status = 0x%08x (EE = %d)", status, EE);
    }

    return status;
}

