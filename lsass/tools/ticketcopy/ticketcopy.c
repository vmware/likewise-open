#include <sys/event.h>
#include <sys/time.h>

#include <errno.h>
#include <fcntl.h>
#include <krb5.h>
#include <limits.h>
#include <popt.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <lw/attrs.h>
#include <lw/types.h>

#include <lsa/lsa.h>
#include <lsalog_r.h>
#include <lsautils.h>

#include <lwerror.h>
#include <lwkrb5.h>

#define BAIL_ON_UNIX_ERROR(_expr)                               \
    do {                                                        \
        if (_expr)                                              \
        {                                                       \
            BAIL_WITH_LSA_ERROR(LwMapErrnoToLwError(errno));    \
        }                                                       \
    } while (0)

#define BAIL_ON_KRB5_ERROR(_ctx, _krb5_err, _error)             \
    do {                                                        \
        if (_krb5_err)                                          \
        {                                                       \
            _error = LwTranslateKrb5Error(                      \
                        (_ctx),                                 \
                        (_krb5_err),                            \
                        __FUNCTION__,                           \
                        __FILE__,                               \
                        __LINE__);                              \
            goto error;                                         \
        }                                                       \
    } while (0)

static struct LogLevelName {
    PCSTR       name;
    LsaLogLevel level;
} LogLevelNames[] = {
    { "error",          LSA_LOG_LEVEL_ERROR, },
    { "warning",        LSA_LOG_LEVEL_WARNING, },
    { "info",           LSA_LOG_LEVEL_INFO, },
    { "verbose",        LSA_LOG_LEVEL_VERBOSE, },
    { "debug",          LSA_LOG_LEVEL_DEBUG, },
    { NULL,             0, }
};

enum LogOptionType {
    LOG_OPTION_SYSLOG = 1,
    LOG_OPTION_LEVEL,
};

static void
LogOptionCallback(
        LW_IN poptContext poptContext,
        LW_IN enum poptCallbackReason reason,
        LW_IN const struct poptOption *option,
        LW_IN const char *arg,
        LW_IN void *data
    );

static LsaLogTarget     logTarget = LSA_LOG_TARGET_CONSOLE;
static LsaLogLevel      logLevel = LSA_LOG_LEVEL_INFO;
static PCSTR            logFile = NULL;
static PCSTR            programName;

struct poptOption Options[] = {
    {
        NULL,
        0,
        POPT_ARG_CALLBACK | POPT_CBFLAG_PRE | POPT_CBFLAG_POST,
        LogOptionCallback,
        0,
        0,
        NULL,
    },
    {
        "logfile",
        0,
        POPT_ARG_STRING,
        &logFile,
        0,
        "Output log messages to a file.",
        "path",
    },
    {
        "syslog",
        0,
        POPT_ARG_NONE,
        NULL,
        LOG_OPTION_SYSLOG,
        "Output log messages to syslog (the default).",
        NULL,
    },
    {
        "loglevel",
        0,
        POPT_ARG_STRING,
        NULL,
        LOG_OPTION_LEVEL,
        "Specify the maximum level of message to log."
            " {error, warning, info, verbose, debug, trace}",
        "level",
    },
    POPT_AUTOHELP
    POPT_TABLEEND
};

static void
LogOptionCallback(
        poptContext poptContext,
        enum poptCallbackReason reason,
        const struct poptOption *option,
        const char *arg, void *data
    )
{
    DWORD dwError = 0;

    switch (reason)
    {
        case POPT_CALLBACK_REASON_PRE:
            if (isatty(0))
            {
                logTarget = LSA_LOG_TARGET_CONSOLE;
            }
            else
            {
                logTarget = LSA_LOG_TARGET_SYSLOG;
            }
            break;

        case POPT_CALLBACK_REASON_POST:
        {
            dwError = LsaInitLogging_r(
                    programName,
                    logTarget,
                    logLevel,
                    logFile);
            BAIL_ON_LSA_ERROR(dwError);
            break;
        }

        case POPT_CALLBACK_REASON_OPTION:
            switch (option->val)
            {
                case LOG_OPTION_SYSLOG:
                    logTarget = LSA_LOG_TARGET_SYSLOG;
                    logFile = NULL;
                    break;

                case LOG_OPTION_LEVEL:
                {
                    int i;

                    for (i = 0; LogLevelNames[i].name != NULL; ++i)
                    {
                        if (!strcmp(LogLevelNames[i].name, arg))
                        {
                            logLevel = LogLevelNames[i].level;
                            break;

                        }
                    }

                    if (LogLevelNames[i].name == NULL)
                    {
                        fprintf(stderr, "unknown level %s\n", arg);
                        exit(1);
                    }
                    break;
                }
            }
            break;
    }

error:
    ;
}

int
main(int argc, const char **argv)
{
    poptContext poptContext;
    int poptResult;
    uid_t uid;
    int kq;
    HANDLE lsaConnection = (HANDLE) NULL;
    PVOID pUserInfo = NULL;
    struct kevent event = { 0 };
    int numChanges = 1;
    krb5_context krb5Context = NULL;
    char krb5FileCachePath[PATH_MAX];
    krb5_ccache krb5FileCache = NULL;
    krb5_ccache krb5MemoryCache = NULL;
    krb5_cc_cursor krb5Cursor = NULL;
    krb5_creds krb5Credentials = { 0 };
    krb5_principal krb5Principal = NULL;
    krb5_error_code krb5Error;
    int exitStatus = 0;
    DWORD dwError = LW_ERROR_SUCCESS;

    if ((programName = strrchr(argv[0], '/')) != NULL)
    {
        ++programName;
    }
    else
    {
        programName = argv[0];
    }

    poptContext = poptGetContext(NULL, argc, argv, Options, 0);
    while ((poptResult = poptGetNextOpt(poptContext)) >= 0)
    {
        /* All options are processed automatically. */
    }

    if (poptResult < -1)
    {
        fprintf(stderr, "%s: %s: %s\n", programName,
                poptBadOption(poptContext, POPT_BADOPTION_NOALIAS),
                poptStrerror(poptResult));
        exitStatus = 1;
        goto error;
    }

    uid = getuid();

    /* Make sure we're running as an AD user. */
    dwError = LsaOpenServer(&lsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindUserById(
                    lsaConnection,
                    uid,
                    0,
                    &pUserInfo);
    if (dwError == LW_ERROR_NO_SUCH_USER)
    {
        /*
         * Running as a non-AD user; exit 0 so launchd doesn't restart
         * the ticketcopy program (see com.likewise.ticketcopy.plist).
         */
        LSA_LOG_DEBUG(
            "uid %lu is not an AD user; exiting",
            (unsigned long) uid);
        dwError = LW_ERROR_SUCCESS;
        goto cleanup;
    }
    BAIL_ON_LSA_ERROR(dwError);

    kq = kqueue();
    BAIL_ON_UNIX_ERROR(kq == -1);

    krb5Error = krb5_init_context(&krb5Context);
    BAIL_ON_KRB5_ERROR(krb5Context, krb5Error, dwError);

    krb5Error = krb5_cc_default(krb5Context, &krb5MemoryCache);
    BAIL_ON_KRB5_ERROR(krb5Context, krb5Error, dwError);

    snprintf(
        krb5FileCachePath,
        sizeof(krb5FileCachePath),
        "FILE:/tmp/krb5cc_%lu",
        (unsigned long) uid);

    while (1) /* Forever (or until an error occurs) */
    {
        while ((event.ident = open(krb5FileCachePath + 5, O_RDONLY)) == -1)
        {
            sleep(5);
        }

        event.filter = EVFILT_VNODE;
        event.flags = EV_ADD | EV_ENABLE | EV_CLEAR;
        event.fflags = NOTE_DELETE | NOTE_WRITE;
        numChanges = 1;

        krb5Error = krb5_cc_resolve(
                        krb5Context,
                        krb5FileCachePath,
                        &krb5FileCache);
        BAIL_ON_KRB5_ERROR(krb5Context, krb5Error, dwError);

        while (1) /* While the file continues to exist. */
        {
            /*
             * Turn off KRB5_TC_OPENCLOSE so the file will be opened once
             * and kept open.  This causes it to actually attempt to open
             * the file, so this is where we check for the file not
             * existing and retry after sleeping a bit.
             */
            krb5Error = krb5_cc_set_flags(krb5Context, krb5FileCache, 0);
            if (krb5Error == KRB5_FCC_NOFILE)
            {
                break;
            }
            BAIL_ON_KRB5_ERROR(krb5Context, krb5Error, dwError);

            /* Copy all credentials from the file to the memory cache. */
            krb5Error = krb5_cc_start_seq_get(
                            krb5Context,
                            krb5FileCache,
                            &krb5Cursor);
            BAIL_ON_KRB5_ERROR(krb5Context, krb5Error, dwError);

            while ((krb5Error = krb5_cc_next_cred(
                                    krb5Context,
                                    krb5FileCache,
                                    &krb5Cursor,
                                    &krb5Credentials)) == 0)
            {
                krb5Error = krb5_cc_store_cred(
                                krb5Context,
                                krb5MemoryCache,
                                &krb5Credentials);
                if (krb5Error == KRB5_FCC_NOFILE)
                {
                    krb5Error = krb5_cc_get_principal(
                                    krb5Context,
                                    krb5FileCache,
                                    &krb5Principal);
                    BAIL_ON_KRB5_ERROR(krb5Context, krb5Error, dwError);

                    /* The memory cache was destroyed; re-create it. */
                    krb5Error = krb5_cc_initialize(
                                    krb5Context,
                                    krb5MemoryCache,
                                    krb5Principal);
                    BAIL_ON_KRB5_ERROR(krb5Context, krb5Error, dwError);

                    krb5_free_principal(krb5Context, krb5Principal);
                    krb5Principal = NULL;

                    krb5Error = krb5_cc_store_cred(
                                    krb5Context,
                                    krb5MemoryCache,
                                    &krb5Credentials);
                }
                BAIL_ON_KRB5_ERROR(krb5Context, krb5Error, dwError);

                krb5_free_cred_contents(krb5Context, &krb5Credentials);
            }

            if (krb5Error != KRB5_CC_END)
            {
                BAIL_ON_KRB5_ERROR(krb5Context, krb5Error, dwError);
            }

            krb5Error = krb5_cc_end_seq_get(
                            krb5Context,
                            krb5FileCache,
                            &krb5Cursor);
            krb5Cursor = NULL;
            BAIL_ON_KRB5_ERROR(krb5Context, krb5Error, dwError);

            /*
             * Turn KRB5_TC_OPENCLOSE back on; this will cause
             * the file to be closed and any locks to be
             * released.
             */
            krb5Error = krb5_cc_set_flags(
                            krb5Context,
                            krb5FileCache,
                            KRB5_TC_OPENCLOSE);
            BAIL_ON_KRB5_ERROR(krb5Context, krb5Error, dwError);

            /*
             * Wait for the file to be modified or deleted.  The first
             * time this is called after the file is opened, numChanges
             * will be 1, which will install the fd into the event
             * list.  After that numChanges is changed to 0, so it will
             * just report events from the existing list.
             */
            if (kevent(kq, &event, numChanges, &event, 1, NULL) != 1)
            {
                fprintf(stderr, "kevent failed\n");
                exitStatus = 1;
                goto cleanup;
            }

            if (event.fflags & NOTE_DELETE)
            {
                break;
            }

            numChanges = 0;
        }

        krb5Error = krb5_cc_close(krb5Context, krb5FileCache);
        BAIL_ON_KRB5_ERROR(krb5Context, krb5Error, dwError);
        krb5FileCache = NULL;

        close(event.ident);
        event.ident = -1;

        /*
         * The cache file is usually removed as part of a
         * rename(2) system call, so only wait a short
         * time before the first attempt to re-open it.
         */
        usleep(100000);
    }

error:
cleanup:
    krb5_free_cred_contents(krb5Context, &krb5Credentials);

    if (krb5Cursor)
    {
        krb5_cc_end_seq_get(krb5Context, krb5FileCache, &krb5Cursor);
    }

    if (krb5FileCache)
    {
        krb5_cc_close(krb5Context, krb5FileCache);
    }

    if (krb5Principal)
    {
        krb5_free_principal(krb5Context, krb5Principal);
    }

    if (krb5Context)
    {
        krb5_free_context(krb5Context);
    }

    if (event.ident != -1)
    {
        close(event.ident);
    }

    if (pUserInfo)
    {
        LsaFreeUserInfo(0, pUserInfo);
    }

    if (lsaConnection != (HANDLE) NULL)
    {
        LsaCloseServer(lsaConnection);
    }

    if (dwError)
    {
        exitStatus = 1;
    }

    return exitStatus;
}
