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

#include <ctdaemon.h>
#include <stdio.h>
#include <ctgoto.h>
#include <string.h>
#include <stdlib.h>
#include <ctmemory.h>
#include <ctserver.h>
#include <unistd.h>
#include <ctstring.h>
#include <ctlock.h>

#include "test.h"

static
void
Usage(
    IN const char *ProgramName
    )
{
    printf("Usage: %s [--start-as-daemon]\n"
           "          [--logfile logFilePath]\n"
           "          [--loglevel {%d - %d}]\n"
           "          [--count <NUMBER>]\n"
           "",
           ProgramName,
           1,
           CT_LOG_LEVEL_MAX);
}

typedef struct {
    bool IsUsageError;
    bool IsHelp;
    bool IsDaemon;
    CT_LOG_LEVEL LogLevel;
    const char* LogPath;
    size_t Count;
} ARGS;

static
CT_STATUS
ParseArgs(
    OUT ARGS* ParsedArgs,
    IN int argc,
    IN const char* argv[]
    )
{
    CT_STATUS status;
    int i;
    const char* arg;
    bool isUsageError = false;
    bool isHelp = false;
    const char* programName = CtGetProgramName(argv[0]);
    bool isDaemon = false;
    CT_LOG_LEVEL logLevel = CT_LOG_LEVEL_TRACE;
    const char* logPath = NULL;
    size_t count = 0;

    memset(ParsedArgs, 0, sizeof(*ParsedArgs));

    for (i = 1; i < argc; i++)
    {
        arg = argv[i];

        if (!strcmp(arg, "-h") ||
            !strcmp(arg, "--help"))
        {
            isHelp = true;
            break;
        }
        else if (!strcmp(arg, "--start-as-daemon"))
        {
            isDaemon = true;
        }
        else if (!strcmp(arg, "--logfile"))
        {
            if (i >= (argc-1))
            {
                fprintf(stderr, "Missing argument to %s\n", arg);
                isUsageError = true;
                break;
            }
            logPath = argv[++i];
        }
        else if (!strcmp(arg, "--loglevel"))
        {
            if (i >= (argc-1))
            {
                fprintf(stderr, "Missing argument to %s\n", arg);
                isUsageError = true;
                break;
            }
            logLevel = atoi(argv[++i]);
            if (logLevel < 1 || logLevel > CT_LOG_LEVEL_MAX)
            {
                fprintf(stderr, "Argument to %s is out of range\n", arg);
                isUsageError = true;
                break;
            }
        }
        else if (!strcmp(arg, "--count"))
        {
            if (i >= (argc-1))
            {
                fprintf(stderr, "Missing argument to %s\n", arg);
                isUsageError = true;
                break;
            }
            count = atoi(argv[++i]);
            if (count < 0)
            {
                fprintf(stderr, "Argument to %s is out of range\n", arg);
                isUsageError = true;
                break;
            }
        }
        else
        {
            fprintf(stderr, "Error: Unrecognized parameter: '%s'\n", arg);
            isUsageError = true;
            break;
        }
    }

    if (isUsageError || isHelp)
    {
        Usage(programName);
    }

    if (isUsageError)
    {
        ParsedArgs->IsUsageError = true;
        status = CT_STATUS_INVALID_PARAMETER;
    }
    else
    {
        ParsedArgs->IsHelp = isHelp;
        ParsedArgs->IsDaemon = isDaemon;
        ParsedArgs->LogPath = logPath;
        ParsedArgs->LogLevel = logLevel;
        ParsedArgs->Count = count;
        status = CT_STATUS_SUCCESS;
    }

    return status;
}

typedef struct _CTX {
    ARGS* Args;
    size_t Count;
    pthread_mutex_t* Lock;
    CT_SERVER_HANDLE ServerHandle;
    bool IsDone;
} CTX;

static
bool
ServerDispatch(
    IN CT_SERVER_CLIENT_HANDLE Handle,
    IN void* Context,
    IN uint32_t Version,
    IN uint32_t Type,
    IN size_t Size
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    CTX* ctx = (CTX*)Context;
    bool isDone = false;
    TEST_MSG_HELLO* message = NULL;
    TEST_MSG_HELLO_REPLY* reply = NULL;
    char* greeting = NULL;

    CT_LOG_ALWAYS("Version = %u, Type = %u, Size = %lu\n",
                  Version, Type, (long unsigned int)Size);

    if (ctx->Count >= ctx->Args->Count)
    {
        isDone = true;
    }
    else
    {
        ctx->Count++;
    }

    if (Version == TEST_MSG_VERSION && Type == TEST_MSG_TYPE_HELLO)
    {
        int fd = CtServerClientGetFd(Handle);
        uint32_t greetingSize;
        uint32_t replySize;

        status = CtServerReadMessageData(fd, Size, (void**)&message);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        // TODO - Add message size validation here

        status = CtAllocateStringPrintf(&greeting, "Greeting %s from %s",
                                        message->Name,
                                        CT_PTR_ADD(message->Name, message->NameSize));

        greetingSize = strlen(greeting) + 1;
        replySize = CT_FIELD_OFFSET(TEST_MSG_HELLO_REPLY, Greeting) + greetingSize;

        status = CtAllocateMemory((void**)&reply, replySize);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        reply->GreetingSize = greetingSize;
        memcpy(reply->Greeting, greeting, greetingSize);

        status = CtServerWriteMessage(fd,
                                      Version,
                                      TEST_MSG_TYPE_HELLO_REPLY,
                                      replySize,
                                      reply);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    CT_SAFE_FREE(message);
    CT_SAFE_FREE(reply);
    CT_SAFE_FREE(greeting);

    CT_LOG_ALWAYS("Dispatch isDone = %c, status = 0x%08x, EE = %d\n",
                  CT_LOG_WRAP_BOOL(isDone), status, EE);

    return status ? true : isDone;
}

static
void*
ThreadStop(
    IN void* Context
    )
{
    CTX* ctx = (CTX*)Context;

    CT_LOG_TRACE("STOP start");

    CtLockAcquireMutex(ctx->Lock);
    if (ctx->ServerHandle)
    {
        CT_LOG_TRACE("STOP middle");
        CtServerTerminate(ctx->ServerHandle);
    }
    ctx->IsDone = true;
    CtLockReleaseMutex(ctx->Lock);

    CT_LOG_TRACE("STOP done");

    return NULL;
}

static
void*
ThreadMain(
    IN void* Context
    )
{
    CT_STATUS status;
    CTX* ctx = (CTX*)Context;
    CT_SERVER_HANDLE server = NULL;
    bool isDone;

    printf("hello\n");

    //CtDaemonExit(27);

    status = CtServerCreate(&server,
                            SERVER_PATH,
                            getuid(),
                            getgid(),
                            CT_SERVER_ALL_ACCESS_MODE,
                            ServerDispatch,
                            ctx);
    if (status)
    {
        printf("Failed to start daemon: 0x%08x\n", status);
        goto cleanup;
    }

    CtLockAcquireMutex(ctx->Lock);
    ctx->ServerHandle = server;
    isDone = ctx->IsDone;
    CtLockReleaseMutex(ctx->Lock);

    if (isDone)
    {
        CT_LOG_INFO("Done\n");
        goto cleanup;
    }

    status = CtServerRun(server);
    if (status)
    {
        printf("Failed to run daemon: 0x%08x\n", status);
        goto cleanup;
    }

cleanup:
    CtServerClose(server);

    return NULL;
}

int
main(
    int argc,
    const char* argv[]
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    const char* programName = CtGetProgramName(argv[0]);
    ARGS args = { 0 };
    int exitCode = 0;
    CTX ctx = { 0 };

    ctx.Args = &args;

    status = CtDaemonStartLogger(CT_LOG_LEVEL_VERBOSE, false, NULL, NULL);
    GOTO_CLEANUP_ON_STATUS(status);

    status = ParseArgs(&args, argc, argv);
    GOTO_CLEANUP_ON_STATUS(status);

    if (args.IsHelp)
    {
        GOTO_CLEANUP();
    }

    status = CtDaemonStartLogger(args.LogLevel, args.IsDaemon, SYSLOG_ID, args.LogPath);
    GOTO_CLEANUP_ON_STATUS(status);

    status = CtLockCreateMutex(&ctx.Lock);
    GOTO_CLEANUP_ON_STATUS(status);

    status = CtDaemonRun(programName, PID_FILE, args.IsDaemon, ThreadMain, ThreadStop, &ctx, &exitCode);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (status)
    {
        exitCode = 1;
    }

    if (!(args.IsHelp || args.IsUsageError))
    {
        printf("Returning %d\n", exitCode);
    }

    return exitCode;
}
