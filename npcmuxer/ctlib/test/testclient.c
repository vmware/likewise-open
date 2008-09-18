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
#include <signal.h>

#include "test.h"

static
void
Usage(
    IN const char *ProgramName
    )
{
    printf("Usage: %s [--logfile logFilePath]\n"
           "          [--loglevel {%d - %d}]\n"
           "          [--name <STRING>]\n"
           "          [--from <STRING>]\n"
           "",
           ProgramName,
           1,
           CT_LOG_LEVEL_MAX);
}

typedef struct {
    CT_LOG_LEVEL LogLevel;
    const char* LogPath;
    const char* Name;
    const char* From;
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
    CT_LOG_LEVEL logLevel = CT_LOG_LEVEL_ERROR;
    const char* logPath = NULL;
    const char* name = "Joe";
    const char* from = programName;

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
        else if (!strcmp(arg, "--name"))
        {
            if (i >= (argc-1))
            {
                fprintf(stderr, "Missing argument to %s\n", arg);
                isUsageError = true;
                break;
            }
            name = argv[++i];
        }
        else if (!strcmp(arg, "--from"))
        {
            if (i >= (argc-1))
            {
                fprintf(stderr, "Missing argument to %s\n", arg);
                isUsageError = true;
                break;
            }
            from = argv[++i];
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
        status = CT_STATUS_INVALID_PARAMETER;
    }
    else
    {
        ParsedArgs->LogPath = logPath;
        ParsedArgs->LogLevel = logLevel;
        ParsedArgs->Name = name;
        ParsedArgs->From = from;
        status = CT_STATUS_SUCCESS;
    }

    return status;
}


int
main(
    int argc,
    const char* argv[]
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    ARGS args = { 0 };
    int exitCode = 0;
    int fd = -1;
    char* clientPath = NULL;
    TEST_MSG_HELLO* message = NULL;
    TEST_MSG_HELLO_REPLY* messageReply = NULL;
    uint32_t messageSize;
    uint32_t version;
    uint32_t type;
    size_t fromSize;
    size_t nameSize;

    /* Ignore pipe signal in favor of EPIPE */
    signal(SIGPIPE, SIG_IGN);

    status = CtLoggerFileOpen(CT_LOG_LEVEL_VERBOSE, NULL);
    GOTO_CLEANUP_ON_STATUS(status);

    status = ParseArgs(&args, argc, argv);
    GOTO_CLEANUP_ON_STATUS(status);

    status = CtLoggerFileOpen(args.LogLevel, args.LogPath);
    GOTO_CLEANUP_ON_STATUS(status);

    status = CtServerCreateClientPath(&clientPath, CLIENT_PREFIX);
    GOTO_CLEANUP_ON_STATUS(status);

    status = CtServerConnect(&fd, SERVER_PATH, clientPath);
    GOTO_CLEANUP_ON_STATUS(status);

    nameSize = strlen(args.Name) + 1;
    fromSize = strlen(args.From) + 1;

    messageSize = CT_FIELD_OFFSET(TEST_MSG_HELLO, Name) + nameSize + fromSize;

    status = CtAllocateMemory((void**)&message, messageSize);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    message->NameSize = nameSize;
    message->FromSize = fromSize;

    memcpy(message->Name, args.Name, nameSize);
    memcpy(CT_PTR_ADD(message->Name, nameSize), args.From, fromSize);

    status = CtServerWriteMessage(fd, TEST_MSG_VERSION, TEST_MSG_TYPE_HELLO, messageSize, message);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    CT_SAFE_FREE(message);

    status = CtServerReadMessage(fd, &version, &type, &messageSize, (void**)&messageReply);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    printf("Version = %d, Type = %d, Size = %d\n", version, type, messageSize);
    if (type != TEST_MSG_TYPE_HELLO_REPLY)
    {
        printf("Bad reply type\n");
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP();
    }
    if (messageSize < CT_FIELD_OFFSET(TEST_MSG_HELLO_REPLY, Greeting) ||
        messageSize != (CT_FIELD_OFFSET(TEST_MSG_HELLO_REPLY, Greeting) + messageReply->GreetingSize))
    {
        printf("Bad reply size\n");
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP();
    }
    if (messageReply->Greeting[messageReply->GreetingSize-1])
    {
        printf("Bad string in reply\n");
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP();
    }

    printf("Reply = %s\n", messageReply->Greeting);

cleanup:
    CT_SAFE_CLOSE_FD(fd);
    CT_SAFE_FREE(clientPath);
    CT_SAFE_FREE(message);

    if (status)
    {
        exitCode = 1;
    }

    printf("Returning %d (status = 0x%08x, EE = %d)\n", exitCode, status, EE);

    return exitCode;
}
