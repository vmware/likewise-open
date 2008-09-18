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

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <signal.h>
#include <npc.h>
#include <cttypes.h>

#define WRAP_STRING(x) ((x) ? (x) : "(null)")
#define WRAP_PASSWORD(x) ((password) ? ((*(password)) ? "*" : "") : "(null)")

const char*
GetProgramName(
    /* IN */ const char* Path
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

int
PromptPassword(
    /* OUT */ char** Password,
    /* IN */ const char* Prompt
    )
{
    int error = 0;
    char buffer[129] = { 0 };
    int index = 0;
    struct termios old_termios;
    struct termios new_termios;
    char ch;
    char* password = NULL;

    if (Prompt)
    {
        printf("%s", Prompt);
        fflush(stdout);
    }

    tcgetattr(0, &old_termios);
    memcpy(&new_termios, &old_termios, sizeof(old_termios));
    new_termios.c_lflag &= ~(ECHO);
    tcsetattr(0, TCSANOW, &new_termios);

    while (index < sizeof(buffer) - 1)
    {
        ssize_t bytes = read(0, &ch, 1);
        if (bytes < 0)
        {
            error = errno;
            goto cleanup;
        }
        if (0 == bytes)
        {
            error = EINVAL;
            goto cleanup;
        }
        if (ch == '\n')
        {
            break;
        }
        buffer[index++] = ch;
    }

    if (index == sizeof(buffer))
    {
        error = EINVAL;
        goto cleanup;
    }

    if (index > 0)
    {
        password = strdup(buffer);
        if (!password)
        {
            error = ENOMEM;
        }
    }

cleanup:
    if (Prompt)
    {
        printf("\n");
    }
    tcsetattr(0, TCSANOW, &old_termios);
    if (error)
    {
        if (password)
        {
            free(password);
        }
    }
    *Password = password;
    return error;
}

typedef struct {
    const char** Elements;
    int Count;
    int Index;
    const char* ProgramName;
} ARGS;

void
ArgsInit(
    /* OUT */ ARGS* Args,
    /* IN */ int argc,
    /* IN */ const char* argv[]
    )
{
    Args->Elements = argv;
    Args->Count = argc;
    Args->Index = 1;
    Args->ProgramName = NULL;
}

int
ArgsGetRemaining(
    /* IN */ ARGS* Args
    )
{
    return Args->Count - Args->Index;
}

bool
ArgsPeekArg(
    /* OUT */ const char** Argument,
    /* IN */ ARGS* Args,
    /* IN */ int Index
    )
{
    bool found = (Index >= 0) && (ArgsGetRemaining(Args) > 0);
    if (Argument)
    {
        *Argument = found ? Args->Elements[Index] : NULL;
    }
    return found;
}

bool
ArgsPeekNextArg(
    /* OUT */ const char** Argument,
    /* IN OUT */ ARGS* Args
    )
{
    return ArgsPeekArg(Argument, Args, Args->Index);
}

bool
ArgsGetNextArg(
    /* OUT */ const char** Argument,
    /* IN OUT */ ARGS* Args
    )
{
    bool found = ArgsPeekNextArg(Argument, Args);
    if (found)
    {
        Args->Index++;
    }
    return found;
}

const char*
ArgsGetProgramName(
    /* IN OUT */ ARGS* Args
    )
{
    if (!Args->ProgramName && (Args->Count > 0))
    {
        Args->ProgramName = GetProgramName(Args->Elements[0]);
    }
    return Args->ProgramName;
}

static
void
Usage(
    const char *ProgramName
    )
{
    printf("Usage: %s COMMAND [OPTIONS...]\n"
           "\n"
           "  commands:\n"
           "\n"
           "    connect [protocol [address [endpoint [options]]]]\n"
           "    setauth [server [authflags [username [password]]]]\n"
           "    clearauth [server]\n"
           "\n",
           ProgramName);
}

static
int
CommandConnect(
    /* IN */ const char* Command,
    /* IN OUT */ ARGS* Args
    )
{
    int error = 0;
    int fd = -1;
    const char* protocol = NULL;
    const char* address = NULL;
    const char* endpoint = NULL;
    const char* options = NULL;
    unsigned char sessKey[64] = {0};
    size_t sessKeyLen = 0;

    ArgsGetNextArg(&protocol, Args);
    ArgsGetNextArg(&address, Args);
    ArgsGetNextArg(&endpoint, Args);
    ArgsGetNextArg(&options, Args);

    if (ArgsGetRemaining(Args) > 0)
    {
        Usage(ArgsGetProgramName(Args));
        return EINVAL;
    }

    printf("%s: Protocol = '%s', Address = '%s', Endpoint = '%s', "
           "Options = '%s'\n",
           Command, WRAP_STRING(protocol), WRAP_STRING(address),
           WRAP_STRING(endpoint), WRAP_STRING(options));

    error = NpcConnect(&fd, protocol, address, endpoint, options,
		       sizeof(sessKey), &sessKeyLen, sessKey);
    printf("error = %d, fd = %d\n", error, fd);

    if (fd >= 0)
    {
        close(fd);
    }

    return error;
}


static
int
CommandConnectCheck(
    /* IN */ const char* Command,
    /* IN OUT */ ARGS* Args
    )
{
    int error = 0;
    const char* protocol = NULL;
    const char* address = NULL;
    const char* endpoint = NULL;
    const char* options = NULL;
    const char* authFlagsString = NULL;
    const char* username = NULL;
    const char* password = NULL;
    char* freePassword = NULL;
    NPC_AUTH_FLAGS authFlags = 0;

    ArgsGetNextArg(&protocol, Args);
    ArgsGetNextArg(&address, Args);
    ArgsGetNextArg(&endpoint, Args);
    ArgsGetNextArg(&options, Args);

    ArgsGetNextArg(&authFlagsString, Args);
    ArgsGetNextArg(&username, Args);
    ArgsGetNextArg(&password, Args);

    if (ArgsGetRemaining(Args) > 0)
    {
        Usage(ArgsGetProgramName(Args));
        return EINVAL;
    }

    if (authFlagsString)
    {
        long temp = strtol(authFlagsString, NULL, 16);
        if (temp < 0 || temp > 0xFFFFFFFF)
        {
            printf("Error parsing hex number '%s' (got 0x%08lx)\n", authFlagsString, temp);
            return EINVAL;
        }
        authFlags = temp;
    }

    if (username && !password)
    {
        printf("Password for '%s': ", username);
        fflush(stdout);
        error = PromptPassword(&freePassword, "");
        if (error)
        {
            return error;
        }
        password = freePassword;
    }

    printf("%s: Protocol = '%s', Address = '%s', Endpoint = '%s', "
           "Options = '%s', AuthFlags = 0x%08x, Username = '%s', "
           "Password = '%s'\n",
           Command, WRAP_STRING(protocol), WRAP_STRING(address),
           WRAP_STRING(endpoint), WRAP_STRING(options), authFlags,
           WRAP_STRING(username), WRAP_PASSWORD(password));

    error = NpcConnectCheckCreds(protocol, address, endpoint, options, authFlags, username, password);
    printf("error = %d\n", error);

    return error;
}

static
int
CommandAuthSet(
    /* IN */ const char* Command,
    /* IN OUT */ ARGS* Args
    )
{
    int error = 0;
    const char* server = NULL;
    const char* authFlagsString = NULL;
    const char* username = NULL;
    const char* password = NULL;
    char* freePassword = NULL;
    NPC_AUTH_FLAGS authFlags = 0;

    ArgsGetNextArg(&server, Args);
    ArgsGetNextArg(&authFlagsString, Args);
    ArgsGetNextArg(&username, Args);
    ArgsGetNextArg(&password, Args);

    if (ArgsGetRemaining(Args) > 0)
    {
        Usage(ArgsGetProgramName(Args));
        return EINVAL;
    }

    if (authFlagsString)
    {
        long temp = strtol(authFlagsString, NULL, 16);
        if (temp < 0 || temp > 0xFFFFFFFF)
        {
            printf("Error parsing hex number '%s' (got 0x%08lx)\n", authFlagsString, temp);
            return EINVAL;
        }
        authFlags = temp;
    }

    if (username && !password)
    {
        printf("Password for '%s': ", username);
        fflush(stdout);
        error = PromptPassword(&freePassword, "");
        if (error)
        {
            return error;
        }
        password = freePassword;
    }

    printf("%s: Server = '%s', AuthFlags = 0x%08x, Username = '%s', "
           "Password = '%s'\n", Command, WRAP_STRING(server),
           authFlags, WRAP_STRING(username), WRAP_PASSWORD(password));

    error = NpcSetAuthInfo(server, authFlags, username, password);
    printf("error = %d\n", error);

    if (freePassword)
    {
        free(freePassword);
    }
    return error;
}

static
int
CommandAuthClear(
    /* IN */ const char* Command,
    /* IN OUT */ ARGS* Args
    )
{
    int error = 0;
    const char* server = NULL;

    ArgsGetNextArg(&server, Args);

    if (ArgsGetRemaining(Args) > 0)
    {
        Usage(ArgsGetProgramName(Args));
        return EINVAL;
    }

    printf("%s: '%s'\n", Command, WRAP_STRING(server));

    error = NpcClearAuthInfo(server);
    printf("error = %d\n", error);

    return error;
}

int
main(
    int argc,
    const char* argv[]
    )
{
    int error = 0;
    const char* programName = GetProgramName(argv[0]);
    const char* command = NULL;
    ARGS args;

    /* Ignore pipe signal in favor of EPIPE */
    signal(SIGPIPE, SIG_IGN);

    ArgsInit(&args, argc, argv);

    for (;;)
    {
        if (!ArgsGetNextArg(&command, &args))
        {
            printf("Missing command\n");
            Usage(programName);
            return EINVAL;
        }

        if (!strcmp(command, "-d"))
        {
            NpcSetDebugLevel(2);
        }
        else if ('-' == command[0])
        {
            printf("Invalid option '%s'\n", command);
            Usage(programName);
            return EINVAL;
        }
        else
        {
            break;
        }
    }

    if (!strcmp(command, "connect"))
    {
        error = CommandConnect(command, &args);
    }
    else if (!strcmp(command, "connectcheck"))
    {
        error = CommandConnectCheck(command, &args);
    }
    else if (!strcmp(command, "setauth"))
    {
        error = CommandAuthSet(command, &args);
    }
    else if (!strcmp(command, "clearauth"))
    {
        error = CommandAuthClear(command, &args);
    }
    else
    {
        printf("Invalid command '%s'\n", command);
        Usage(programName);
        return EINVAL;
    }

    if (error)
    {
        printf("Got error %d\n", error);
    }
    else
    {
        printf("Success!\n");
    }
    return error;
}
