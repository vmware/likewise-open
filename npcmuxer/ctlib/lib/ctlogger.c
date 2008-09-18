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

#include <ctlogger.h>
#include <cttypes.h>
#include <syslog.h>
#include <stdio.h>
#include <pthread.h>
#include <ctmemory.h>
#include <ctgoto.h>
#include <ctstring.h>
#include <string.h>
#include <ctlock.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(__APPLE__) && (defined(_POSIX_C_SOURCE) || defined(_ANSI_SOURCE))
/* Darwin doesn't like to give us localtime_r */
struct tm *localtime_r(const time_t *clock, struct tm *result);
#endif

#define CT_LOG_TAG_CRITICAL "CRITICAL"
#define CT_LOG_TAG_ERROR    "ERROR"
#define CT_LOG_TAG_WARN     "WARNING"
#define CT_LOG_TAG_NOTICE   "NOTICE"
#define CT_LOG_TAG_INFO     "INFO"
#define CT_LOG_TAG_VERBOSE  "VERBOSE"
#define CT_LOG_TAG_DEBUG    "DEBUG"
#define CT_LOG_TAG_TRACE    "TRACE"

#define CT_LOG_TAG_UNKNOWN  "N/A"

#define CT_LOG_TIME_FORMAT "%Y/%m/%d-%H:%M:%S"
#define CT_LOG_TIME_MAX_SIZE (sizeof(CT_LOG_TIME_FORMAT) + 20)

typedef struct _CT_LOG_LEVEL_ENTRY {
    CT_LOG_LEVEL Level;
    const char* Tag;
    bool UseErrorStream;
    int SyslogPriotity;
} CT_LOG_LEVEL_ENTRY;

CT_LOG_LEVEL_ENTRY gCtLogLevelInfo[CT_LOG_LEVEL_MAX+1] =
{
    { CT_LOG_LEVEL_ALWAYS,   CT_LOG_TAG_INFO,     false, LOG_INFO },
    { CT_LOG_LEVEL_CRITICAL, CT_LOG_TAG_CRITICAL, true,  LOG_CRIT },
    { CT_LOG_LEVEL_ERROR,    CT_LOG_TAG_ERROR,    true,  LOG_ERR },
    { CT_LOG_LEVEL_WARN,     CT_LOG_TAG_WARN,     true,  LOG_WARNING },
    { CT_LOG_LEVEL_NOTICE,   CT_LOG_TAG_NOTICE,   false, LOG_NOTICE },
    { CT_LOG_LEVEL_INFO,     CT_LOG_TAG_INFO,     false, LOG_INFO },
    { CT_LOG_LEVEL_VERBOSE,  CT_LOG_TAG_VERBOSE,  false, LOG_INFO },
    { CT_LOG_LEVEL_DEBUG,    CT_LOG_TAG_DEBUG,    false, LOG_DEBUG },
    { CT_LOG_LEVEL_TRACE,    CT_LOG_TAG_TRACE,    false, LOG_DEBUG },
};

/*
 * Logging targets
 */

typedef uint8_t CT_LOG_TARGET;

#define CT_LOG_TARGET_NONE    0
#define CT_LOG_TARGET_SYSLOG  1
#define CT_LOG_TARGET_FILE    2

typedef struct _CT_LOG_INFO_FILE {
    char* Path;
    FILE* Stream;
} CT_LOG_INFO_FILE, *PCT_LOG_INFO_FILE;

typedef struct _CT_LOG_INFO_SYSLOG {
    char* Identifier;
    int Option;
    int Facility;
} CT_LOG_INFO_SYSLOG, *PCT_LOG_INFO_SYSLOG;

typedef struct _CT_LOG_HANDLE_DATA {
    CT_LOG_LEVEL LogLevel;
    pthread_mutex_t Lock;
    CT_LOG_TARGET Target;
    union {
        CT_LOG_INFO_FILE File;
        CT_LOG_INFO_SYSLOG Syslog;
    } Data;
} CT_LOG_HANDLE_DATA;

typedef struct _CT_LOGGER_STATE {
    pthread_mutex_t Lock;
    CT_LOG_HANDLE LogHandle;
} CT_LOGGER_STATE;

/* These are the values for the global logger */
CT_LOG_LEVEL _gCtLoggerLogLevel = CT_LOG_LEVEL_ERROR;
CT_LOGGER_STATE gCtpLoggerState = { CT_LOCK_INITIALIZER_MUTEX, NULL };

#if defined(sun) || defined(_AIX) || defined (_HPUX)
pthread_once_t gCtpLoggerOnce = { PTHREAD_ONCE_INIT };
#else
pthread_once_t gCtpLoggerOnce = PTHREAD_ONCE_INIT;
#endif

CT_STATUS gCtpLoggerStatus;

#define CT_LOGGER_ACQUIRE() CtLockAcquireMutex(&gCtpLoggerState.Lock)
#define CT_LOGGER_RELEASE() CtLockReleaseMutex(&gCtpLoggerState.Lock)

static
int
__CtpLoggerInitMutex()
{
    int error;
    pthread_mutexattr_t attr;

    error = pthread_mutexattr_init(&attr);
    GOTO_CLEANUP_ON_ERRNO(error);

    error = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    GOTO_CLEANUP_ON_ERRNO(error);

    error = pthread_mutex_init(&gCtpLoggerState.Lock, &attr);
    GOTO_CLEANUP_ON_ERRNO(error);

cleanup:

    return error;
}

static
void
__CtpLoggerPreFork()
{
    /* Acquire logger mutex right before a fork so that
       the forking thread holds it */
    CT_LOGGER_ACQUIRE();
}

static
void
__CtpLoggerParentFork()
{
    /* Release logger mutex after forking */
    CT_LOGGER_RELEASE();
}

static
void
__CtpLoggerChildFork()
{
    /* Attempt to unlock.  This may fail since our thread ID has changed */
    pthread_mutex_unlock(&gCtpLoggerState.Lock);
    /* Re-initialize mutex in case unlocking did not work */
    pthread_mutex_destroy(&gCtpLoggerState.Lock);
    __CtpLoggerInitMutex();
}

static
void
__CtpLoggerInit()
{
    int error;

    error = __CtpLoggerInitMutex();
    GOTO_CLEANUP_ON_ERRNO(error);

    error = pthread_atfork(__CtpLoggerPreFork, __CtpLoggerParentFork, __CtpLoggerChildFork);
    GOTO_CLEANUP_ON_ERRNO(error);

cleanup:

    gCtpLoggerStatus = CtErrnoToStatus(error);
}

CT_STATUS
CtpLoggerInit()
{
    pthread_once(&gCtpLoggerOnce, __CtpLoggerInit);

    return gCtpLoggerStatus;
}

#define _CT_LOGGER_LOG_INIT_ERROR_FORMAT "Error at %s:%d. Error code [0x%8x]"
#define CT_LOGGER_LOG_INIT_ERROR(status, EE) \
    do { \
        if (status) \
        { \
            if (CT_STATUS_IS_OK(CtpLoggerInit())) \
            { \
                CT_LOGGER_ACQUIRE(); \
                if (gCtpLoggerState.LogHandle && (gCtpLoggerState.LogHandle->Target != CT_LOG_TARGET_NONE)) \
                { \
                    CT_LOG_ALWAYS(_CT_LOGGER_LOG_INIT_ERROR_FORMAT, __FILE__, EE, status); \
                } \
                else \
                { \
                    fprintf(stderr, _CT_LOGGER_LOG_INIT_ERROR_FORMAT "\n", __FILE__, EE, status); \
                } \
                CT_LOGGER_RELEASE(); \
            } \
            else \
            { \
                fprintf(stderr, _CT_LOGGER_LOG_INIT_ERROR_FORMAT "\n", __FILE__, EE, status); \
            } \
        } \
    } while (0)

/****************************************************************************/
/* Handle-Based Log Functions */
/****************************************************************************/

/* These allow us to use non-global loggers.  Note that open and close
   can only be used in race-free contexts. */

#define CT_LOG_ACQUIRE(Handle) pthread_mutex_lock(&(Handle)->Lock)
#define CT_LOG_RELEASE(Handle) pthread_mutex_unlock(&(Handle)->Lock)


static
CT_STATUS
CtpLoggerValidateLogLevel(
    CT_LOG_LEVEL LogLevel
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;

    if (LogLevel < 1)
    {
        status = CT_STATUS_INVALID_PARAMETER;
    }

    return status;
}


static
CT_LOG_LEVEL_ENTRY*
CtpLoggerGetLevelEntry(
    CT_LOG_LEVEL LogLevel
    )
{
    CT_LOG_LEVEL_ENTRY* entry = NULL;
    if (LogLevel >= 0 && LogLevel < sizeof(gCtLogLevelInfo)/sizeof(gCtLogLevelInfo[0]))
    {
        entry = &gCtLogLevelInfo[LogLevel];
    }
    /* TODO: Assert */
    return entry;
}

static
int
CtpLoggerGetSyslogPriority(
    CT_LOG_LEVEL LogLevel
    )
{
    CT_LOG_LEVEL_ENTRY* entry = CtpLoggerGetLevelEntry(LogLevel);
    return entry ? entry->SyslogPriotity : LOG_DEBUG;
}

static
const char*
CtpLoggerGetTag(
    CT_LOG_LEVEL LogLevel
    )
{
    CT_LOG_LEVEL_ENTRY* entry = CtpLoggerGetLevelEntry(LogLevel);
    return entry ? entry->Tag : CT_LOG_TAG_UNKNOWN;
}

static
bool
CtpLoggerGetUseErrorStream(
    CT_LOG_LEVEL LogLevel
    )
{
    CT_LOG_LEVEL_ENTRY* entry = CtpLoggerGetLevelEntry(LogLevel);
    return entry ? entry->UseErrorStream : false;
}

#if 0
static
void
CtLoggerSyslogSetMask(
    CT_LOG_LEVEL LogLevel
    )
{
    int mask;

    mask = LOG_UPTO(CTLogGetSyslogMask(LogLevel);
    setlogmask(mask);
}
#endif

#if 0
#define _CT_LOG_ALLOCATE(Status, Pointer, Type, Size, Ok, Error) \
    do { \
        (Pointer) = (Type)(malloc(Size)); \
        (Status) = (Pointer) ? (Ok) : (Error);
    } while (0)
#endif

void
CtLoggerCloseHandle(
    IN OUT CT_LOG_HANDLE Handle
    )
{
    if (Handle)
    {
        switch (Handle->Target)
        {
        case CT_LOG_TARGET_SYSLOG:
        {
            /* close connection to syslog */
            closelog();
            CT_SAFE_FREE(Handle->Data.Syslog.Identifier);
            break;
        }
        case CT_LOG_TARGET_FILE:
        {
            if (Handle->Data.File.Stream)
            {
                fclose(Handle->Data.File.Stream);
                Handle->Data.File.Stream = NULL;
            }
            CT_SAFE_FREE(Handle->Data.File.Path);
            break;
        }
        case CT_LOG_TARGET_NONE:
            break;
        default:
            /* ASSERT */
            break;
        }

        CtFreeMemory(Handle);
    }
}

static
CT_STATUS
CtpLoggerCommonOpenHandle(
    OUT PCT_LOG_HANDLE Handle,
    IN CT_LOG_LEVEL LogLevel,
    IN CT_LOG_TARGET LogTarget
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    CT_LOG_HANDLE handle = NULL;
    int error;

    status = CtpLoggerValidateLogLevel(LogLevel);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtAllocateMemory((void**)&handle, sizeof(*handle));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    error = pthread_mutex_init(&handle->Lock, NULL);
    status = CtErrnoToStatus(error);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    handle->LogLevel = LogLevel;
    handle->Target = LogTarget;

cleanup:
    if (status)
    {
        CtLoggerCloseHandle(handle);
        handle = NULL;
    }

    *Handle = handle;

    CT_LOGGER_LOG_INIT_ERROR(status, EE);
    return status;
}

static
CT_STATUS
CtpLoggerSyslogOpenHandle(
    OUT PCT_LOG_HANDLE Handle,
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Identifier,
    IN int Option,
    IN int Facility
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    CT_LOG_HANDLE handle = NULL;

    status = CtpLoggerCommonOpenHandle(&handle, LogLevel, CT_LOG_TARGET_SYSLOG);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtAllocateString(&handle->Data.Syslog.Identifier, Identifier);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    handle->Data.Syslog.Option = Option;
    handle->Data.Syslog.Facility = Facility;

    openlog(Identifier, Option, Facility);

    /* By default, syslog logging is enabled for all priorities.  We do our own masking
       before calling into syslog, so we do not need to set the syslog mask. */
#if 0
    CtLoggerSyslogSetMask(dwLogLevel);
#endif

cleanup:
    if (status)
    {
        CtLoggerCloseHandle(handle);
        handle = NULL;
    }

    *Handle = handle;

    CT_LOGGER_LOG_INIT_ERROR(status, EE);
    return status;
}

CT_STATUS
CtLoggerOpenHandle(
    OUT PCT_LOG_HANDLE Handle,
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Path
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    CT_LOG_HANDLE handle = NULL;

    status = CtpLoggerCommonOpenHandle(&handle, LogLevel, CT_LOG_TARGET_FILE);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (Path && *Path)
    {
        status = CtAllocateString(&handle->Data.File.Path, Path);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        handle->Data.File.Stream = fopen(Path, "w");
        if (!handle->Data.File.Stream)
        {
            status = CT_ERRNO_TO_STATUS(errno);
            GOTO_CLEANUP_EE(EE);
        }
    }

cleanup:
    if (status)
    {
        CtLoggerCloseHandle(handle);
        handle = NULL;
    }

    *Handle = handle;

    CT_LOGGER_LOG_INIT_ERROR(status, EE);
    return status;
}

static
void
CtpLoggerFileLogMessageUnsafe(
    PCT_LOG_INFO_FILE Info,
    CT_LOG_LEVEL LogLevel,
    const char* Format,
    va_list Args
    )
{
    const char* tag = NULL;
    bool useErrorStream = false;
    time_t currentTime;
    struct tm currentTimeValues;
    char buffer[CT_LOG_TIME_MAX_SIZE];
    FILE* stream;
    size_t last;

    tag = CtpLoggerGetTag(LogLevel);
    useErrorStream = CtpLoggerGetUseErrorStream(LogLevel);

    if (Info->Stream)
    {
        stream = Info->Stream;
    }
    else
    {
        stream = useErrorStream ? stderr : stdout;
    }

    last = strlen(Format);

    currentTime = time(NULL);
    if (!localtime_r(&currentTime, &currentTimeValues) ||
        !strftime(buffer, sizeof(buffer), CT_LOG_TIME_FORMAT, &currentTimeValues))
    {
        strncpy(buffer, "N/A", sizeof(buffer));
        buffer[sizeof(buffer)-1] = 0;
    }

    fprintf(stream, "[%s-%u-0x%lx-%s] ", buffer, (unsigned int) getpid(), (unsigned long)pthread_self(), tag);
    vfprintf(stream, Format, Args);
    if (Format[last-1] != '\n')
    {
        fprintf(stream, "\n");
    }
    fflush(stream);
}

void
sys_vsyslog(
    int priority,
    const char *format,
    va_list ap
    )
{
#if defined(HAVE_VSYSLOG)
    vsyslog(priority, format, ap);
#else
    CT_STATUS status;
    char* buffer = NULL;

    status = CtAllocateStringPrintfV(&buffer, format, ap);
    if (CT_STATUS_IS_OK(status))
    {
        syslog(priority, "%s", buffer);
    }

    CT_SAFE_FREE(buffer);
#endif /* ! HAVE_VSYSLOG */
}

static
void
CtpLoggerSyslogLogMessageUnsafe(
    CT_LOG_LEVEL LogLevel,
    const char* Format,
    va_list Args
    )
{
    int priority = CtpLoggerGetSyslogPriority(LogLevel);
    sys_vsyslog(priority, Format, Args);
}

void
CtLoggerLogMessageHandleV(
    IN CT_LOG_HANDLE Handle,
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Format,
    IN va_list Args
    )
{
    bool isAcquired = false;

    if (LogLevel > Handle->LogLevel)
    {
        goto cleanup;
    }

    CT_LOG_ACQUIRE(Handle);
    isAcquired = true;

    switch (Handle->Target)
    {
    case CT_LOG_TARGET_SYSLOG:
    {
        CtpLoggerSyslogLogMessageUnsafe(LogLevel, Format, Args);
        break;
    }
    case CT_LOG_TARGET_FILE:
    {
        CtpLoggerFileLogMessageUnsafe(&Handle->Data.File, LogLevel, Format, Args);
        break;
    }
    }

cleanup:
    if (isAcquired)
    {
        CT_LOG_RELEASE(Handle);
    }
}

void
CtLoggerLogMessageHandle(
    IN CT_LOG_HANDLE Handle,
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Format,
    IN ...
    )
{
    va_list args;
    va_start(args, Format);
    CtLoggerLogMessageHandleV(Handle, LogLevel, Format, args);
    va_end(args);
}


CT_STATUS
CtLoggerSetLogLevelHandle(
    IN CT_LOG_HANDLE Handle,
    IN CT_LOG_LEVEL LogLevel
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;

    status = CtpLoggerValidateLogLevel(LogLevel);
    GOTO_CLEANUP_ON_STATUS(status);

    /* We assume that replacing this value is atomic */
    Handle->LogLevel = LogLevel;

cleanup:
    return status;
}

/****************************************************************************/
/* Global Log Functions */
/****************************************************************************/

CT_STATUS
CtLoggerSyslogOpen(
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Identifier,
    IN int Option,
    IN int Facility
    )
{
    CT_STATUS status;

    status = CtpLoggerInit();
    GOTO_CLEANUP_ON_STATUS(status);

    CT_LOGGER_ACQUIRE();
    CtLoggerClose();
    status = CtpLoggerSyslogOpenHandle(&gCtpLoggerState.LogHandle, LogLevel, Identifier, Option, Facility);
    if (CT_STATUS_IS_OK(status))
    {
        _gCtLoggerLogLevel = LogLevel;
    }
    CT_LOGGER_RELEASE();

cleanup:
    return status;
}

CT_STATUS
CtLoggerFileOpen(
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Path
    )
{
    CT_STATUS status;

    status = CtpLoggerInit();
    GOTO_CLEANUP_ON_STATUS(status);

    CT_LOGGER_ACQUIRE();
    CtLoggerClose();
    status = CtLoggerOpenHandle(&gCtpLoggerState.LogHandle, LogLevel, Path);
    if (CT_STATUS_IS_OK(status))
    {
        _gCtLoggerLogLevel = LogLevel;
    }
    CT_LOGGER_RELEASE();

cleanup:
    return status;
}

CT_STATUS
CtLoggerSetLogLevel(
    IN CT_LOG_LEVEL LogLevel
    )
{
    CT_STATUS status;
    bool isAcquired = false;

    status = CtpLoggerInit();
    GOTO_CLEANUP_ON_STATUS(status);

    CT_LOGGER_ACQUIRE();
    isAcquired = true;

    if (!gCtpLoggerState.LogHandle)
    {
        GOTO_CLEANUP();
    }

    status = CtLoggerSetLogLevelHandle(gCtpLoggerState.LogHandle, LogLevel);
    GOTO_CLEANUP_ON_STATUS(status);

    /* We assume that replacing this value is atomic */
    _gCtLoggerLogLevel = LogLevel;

cleanup:
    if (isAcquired)
    {
        CT_LOGGER_RELEASE();
    }

    return status;
}

void
CtLoggerLogMessageV(
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Format,
    IN va_list Args
    )
{
    if (CT_STATUS_IS_OK(CtpLoggerInit()))
    {
        CT_LOGGER_ACQUIRE();
        if (gCtpLoggerState.LogHandle)
        {
            CtLoggerLogMessageHandleV(gCtpLoggerState.LogHandle, LogLevel, Format, Args);
        }
        CT_LOGGER_RELEASE();
    }
}

void
CtLoggerLogMessage(
    IN CT_LOG_LEVEL LogLevel,
    IN const char* Format,
    IN ...
    )
{
    va_list args;
    va_start(args, Format);
    CtLoggerLogMessageV(LogLevel, Format, args);
    va_end(args);
}

void
CtLoggerClose(
    )
{
    if (CT_STATUS_IS_OK(CtpLoggerInit()))
    {
        CT_LOGGER_ACQUIRE();
        CtLoggerCloseHandle(gCtpLoggerState.LogHandle);
        gCtpLoggerState.LogHandle = NULL;
        _gCtLoggerLogLevel = CT_LOG_LEVEL_ERROR;
        CT_LOGGER_RELEASE();
    }
}

