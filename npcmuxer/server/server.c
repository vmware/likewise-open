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

/* -*- mode: c; c-basic-offset: 4 -*- */

#ifdef HAVE_CONFIG_H
#    include <config.h>
#endif

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
#include <termios.h>
#include <signal.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_SELECT_H
#include <select.h>
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <npcmsg.h>
#include "proxy.h"
#include "proxy_tcp.h"
#include "proxy_np.h"
#include <lwnet.h>

#define PARSE_ARGS_LOGLEVEL CT_LOG_LEVEL_VERBOSE
#define DEFAULT_LOGLEVEL CT_LOG_LEVEL_WARN

static
void
Usage(
    IN const char *ProgramName
    )
{
    printf("Usage: %s [options]\n"
           "\n"
           "  options:\n"
           "\n"
           "    --start-as-daemon -- start as a daemon\n"
           "    --logfile <path> -- log to a file instead of stdout/stderr\n"
           "    --loglevel <number> -- from %d to %d inclusive, default is %d\n"
           "    -k -- use Kerberos credentials for authentication\n"
           "    -u <username> -- username for authentication\n"
           "    -p <password> -- password for authentication\n"
           "        (will prompt if username provided w/o password)\n"
           "\n"
           "  examples:\n"
           "\n"
           ""
           "    %s --loglevel %d -u DOMAIN\\username\n"
           "\n"
           "",
           ProgramName,
           1,
           CT_LOG_LEVEL_MAX,
           DEFAULT_LOGLEVEL,
           ProgramName,
           CT_LOG_LEVEL_MAX);
}

typedef struct {
    bool IsHelp;
    bool IsUsageError;
    bool IsDaemon;
    CT_LOG_LEVEL LogLevel;
    const char* LogPath;
    const char* Username;
    const char* Password;
    bool UseCredCache;
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
    CT_LOG_LEVEL logLevel = DEFAULT_LOGLEVEL;
    const char* logPath = NULL;
    const char* username = NULL;
    const char* password = NULL;
    bool useCredCache = false;

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
        else if (!strcmp(arg, "-u") ||
                 !strcmp(arg, "--user") ||
                 !strcmp(arg, "--username"))
        {
            if (i >= (argc-1))
            {
                fprintf(stderr, "Missing argument to %s\n", arg);
                isUsageError = true;
                break;
            }
            username = argv[++i];
        }
        else if (!strcmp(arg, "-p") ||
                 !strcmp(arg, "--pass") ||
                 !strcmp(arg, "--password"))
        {
            if (i >= (argc-1))
            {
                fprintf(stderr, "Missing argument to %s\n", arg);
                isUsageError = true;
                break;
            }
            password = argv[++i];
        }
        else if (!strcmp(arg, "-k") ||
                 !strcmp(arg, "--kerberos"))
        {
            useCredCache = true;
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
        ParsedArgs->IsDaemon  = isDaemon;
        ParsedArgs->LogPath   = logPath;
        ParsedArgs->LogLevel  = logLevel;
        ParsedArgs->Username  = username;
        ParsedArgs->Password  = password;
        ParsedArgs->UseCredCache = useCredCache;
        status = CT_STATUS_SUCCESS;
    }

    return status;
}

static
CT_STATUS
SetupEnvironment()
{
    DWORD dwError = LWNetExtendEnvironmentForKrb5Affinity(TRUE);
    return dwError ? CT_STATUS_OUT_OF_MEMORY : CT_STATUS_SUCCESS;
}

#define GET_STRING(StringPointer, Size, Location, status) \
    do { \
        (status) = CT_STATUS_SUCCESS; \
        if ((Size) > 0) \
        { \
            *(StringPointer) = (Location); \
            if ((*(StringPointer))[(Size)-1]) \
            { \
                (status) = CT_STATUS_INVALID_MESSAGE; \
            } \
        } \
        else \
        { \
            *(StringPointer) = NULL; \
        } \
    } while (0)

#define GET_NEXT_STRING(StringPointer, Size, Message, OffsetPtr, status) \
    do { \
        GET_STRING(StringPointer, Size, CT_PTR_ADD(Message , *(OffsetPtr)), status); \
        *(OffsetPtr) += (Size); \
    } while (0)

static
CT_STATUS
ReplyStatus(
    IN int Fd,
    IN uint32_t Version,
    IN CT_STATUS ReplyStatus
    )
{
    NPC_MSG_PAYLOAD_STATUS reply = { 0 };
    reply.Status = ReplyStatus;
    return CtServerWriteMessage(Fd, Version, NPC_MSG_TYPE_STATUS, sizeof(reply), &reply);
}

typedef struct _CTX {
    PROXY_HANDLE Proxy;
    pthread_mutex_t* Lock;
    CT_SERVER_HANDLE ServerHandle;
    NPC_TOKEN_ID NextToken;
    bool IsDone;
} CTX;

static
void
CleanupChild(void* _pid)
{
    pid_t pid = (pid_t) (size_t) _pid;

    CT_LOG_VERBOSE("Cleaning up child process %i...", (int) pid);
    kill(pid, SIGTERM);
    while (waitpid(pid, NULL, 0) != pid);
    CT_LOG_VERBOSE("Child process %i done", (int) pid);

}

static
void
ChildTermHandler(int sig)
{
    _exit(0);
}

static
void
ChildSetup(CT_SERVER_CLIENT_HANDLE handle)
{
    sigset_t set;
    int i;
    int fd;
    
    /* Unblock signals now that we are a different process instead of just a different thread */
    sigemptyset(&set);
    sigprocmask(SIG_SETMASK, &set, NULL);
    /* Create a handler for the term signal that exits without triggering atexit() callbacks */
    signal(SIGTERM, ChildTermHandler);

    /* Close fds we don't need */
    fd = CtServerClientGetFd(handle);

    for (i = 3; i < 1000; i++)
    {
        if (i != fd)
        {
            if (close(i) == 0)
            {
                CT_LOG_INFO("Successfully closed unneeded fd %i", i);
            }
        }
    }
}

static
CT_STATUS
HandleConnectCheckCreds(
    IN CT_SERVER_CLIENT_HANDLE Handle,
    IN CTX* Context,
    IN uint32_t Version,
    IN uint32_t Type,
    IN size_t Size
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    uid_t uid = CtServerClientGetUid(Handle);
    int fd = CtServerClientGetFd(Handle);
    NPC_MSG_PAYLOAD_CONNECT_CHECK_CREDS* message = NULL;
    size_t offset;
    const char* protocol = NULL;
    const char* address = NULL;
    const char* endpoint = NULL;
    const char* options = NULL;
    const char* username = NULL;
    const char* password = NULL;
    const char* credCache = NULL;
    CT_STATUS localStatus;
    /* Initialize child pid to our own pid rather than 0 since 0 indicates
       being in the child process after the fork */
    pid_t child = getpid();

    if (!CtServerClientIsAuthenticated(Handle))
    {
        localStatus = CT_STATUS_ACCESS_DENIED;
	goto reply;
    }

    status = CtServerReadMessageData(fd, Size, (void**)&message);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (!NPC_IS_SIZE_OK_MSG_PAYLOAD_CONNECT_CHECK_CREDS(message, Size))
    {
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP_EE(EE);
    }

    offset = CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_CONNECT_CHECK_CREDS, Data);

    GET_NEXT_STRING(&protocol, message->ProtocolSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    GET_NEXT_STRING(&address, message->AddressSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    GET_NEXT_STRING(&endpoint, message->EndpointSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    GET_NEXT_STRING(&options, message->OptionsSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    GET_NEXT_STRING(&username, message->UsernameSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    GET_NEXT_STRING(&password, message->PasswordSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    GET_NEXT_STRING(&credCache, message->CredCacheSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    CT_LOG_DEBUG("Protocol = '%s', Address = '%s', Endpoint = '%s', "
                 "Options = '%s', AuthFlags = 0x%08X, Username = '%s', "
                 "Password = '%s', CredCache = '%s'",
                 CT_LOG_WRAP_STRING(protocol),
                 CT_LOG_WRAP_STRING(address),
                 CT_LOG_WRAP_STRING(endpoint),
                 CT_LOG_WRAP_STRING(options),
                 message->AuthFlags,
                 CT_LOG_WRAP_STRING(username),
                 password ? ((*password) ? "*" : "") : "(null)",
                 CT_LOG_WRAP_STRING(credCache));

    /* Because libsmbclient is not thread-safe, fork into a child process before
       setting up the connection */
    if ((child = fork()) != 0)
    {
        /* In parent.  Wait for child to finish and then get out of here */
        pthread_cleanup_push(CleanupChild, (void*) (size_t) child);
        CT_LOG_VERBOSE("Spawned child process %i", (int) child);
        while (waitpid(child, NULL, 0) != child);
        CT_LOG_VERBOSE("Child process %i done", (int) child);
        pthread_cleanup_pop(0);
        goto cleanup;
    }
    else
    {
        /* In child */
        ChildSetup(Handle);

        localStatus = ProxyConnectionCheckCreds(Context->Proxy, uid, protocol,
                                                address, endpoint, options,
                                                message->AuthFlags,
                                                username, password, credCache);
    }

reply:
    CT_LOG_TRACE("replying 0x%08x", localStatus);
    status = ReplyStatus(fd, Version, localStatus);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    CT_SAFE_FREE(message);
    CT_LOG_TRACE("status = 0x%08x (EE = %d)", status, EE);

    if (child == 0)
    {
        _exit(0);
    }

    return status;
}


#define SET_NEXT_STRING(Message, OffsetPtr, String, StringSize) \
    do { \
        memcpy(CT_PTR_ADD(Message , *(OffsetPtr)), String, StringSize); \
        *(OffsetPtr) += (StringSize); \
    } while (0)

static
CT_STATUS
HandleConnect(
    IN CT_SERVER_CLIENT_HANDLE Handle,
    IN CTX* Context,
    IN uint32_t Version,
    IN uint32_t Type,
    IN size_t Size
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    uid_t uid = CtServerClientGetUid(Handle);
    int fd = CtServerClientGetFd(Handle);
    NPC_MSG_PAYLOAD_CONNECT* message = NULL;
    NPC_MSG_PAYLOAD_SESSION_KEY *reply = NULL;
    size_t replySize = 0;
    size_t offset;
    const char* protocol = NULL;
    const char* address = NULL;
    const char* endpoint = NULL;
    const char* options = NULL;
    const char* credCache = NULL;
    CT_STATUS localStatus;
    PROXY_CONNECTION_HANDLE connection = NULL;
    size_t SessKeyLen = 0;
    unsigned char* SessKey = NULL;
    /* Initialize child pid to our own pid rather than 0 since 0 indicates
       being in the child process after the fork */
    pid_t child = getpid();

    if (!CtServerClientIsAuthenticated(Handle))
    {
        localStatus = CT_STATUS_ACCESS_DENIED;
	goto reply;
    }

    status = CtServerReadMessageData(fd, Size, (void**)&message);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (!NPC_IS_SIZE_OK_MSG_PAYLOAD_CONNECT(message, Size))
    {
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP_EE(EE);
    }

    offset = CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_CONNECT, Data);

    GET_NEXT_STRING(&protocol, message->ProtocolSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    GET_NEXT_STRING(&address, message->AddressSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    GET_NEXT_STRING(&endpoint, message->EndpointSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    GET_NEXT_STRING(&options, message->OptionsSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    GET_NEXT_STRING(&credCache, message->CredCacheSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    CT_LOG_DEBUG("Protocol = '%s', Address = '%s', Endpoint = '%s', Option = '%s', CredCache = '%s' Impersonation Token = %ld",
                 CT_LOG_WRAP_STRING(protocol),
                 CT_LOG_WRAP_STRING(address),
                 CT_LOG_WRAP_STRING(endpoint),
                 CT_LOG_WRAP_STRING(options),
                 CT_LOG_WRAP_STRING(credCache),
                 message->Token);

    /* Because libsmbclient is not thread-safe, fork into a child process before
       setting up the connection */
    if ((child = fork()) != 0)
    {
        /* In parent.  Wait for child to finish and then get out of here */
        pthread_cleanup_push(CleanupChild, (void*) (size_t) child);
        CT_LOG_VERBOSE("Spawned child process %i", (int) child);
        while (waitpid(child, NULL, 0) != child);
        CT_LOG_VERBOSE("Child process %i done", (int) child);
        pthread_cleanup_pop(0);
        goto cleanup;
    }
    else
    {
        /* In child */
        ChildSetup(Handle);

        /* Open SMB connection */

        localStatus = ProxyConnectionOpen(&connection, Context->Proxy, fd, uid,
                                          protocol, address, endpoint, options, credCache, message->Token,
                                          &SessKeyLen, &SessKey);
    }

reply:
    replySize = sizeof(NPC_MSG_PAYLOAD_SESSION_KEY) + SessKeyLen;
    status = CtAllocateMemory((void**)&reply, replySize);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    offset = CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_SESSION_KEY, Data);

    reply->Status        = localStatus;
    reply->SessionKeyLen = SessKeyLen;
    if (reply->SessionKeyLen)
    {
       SET_NEXT_STRING(reply, &offset, SessKey, SessKeyLen);
    }

    status = CtServerWriteMessage(fd,
				  NPC_MSG_VERSION,
				  NPC_MSG_TYPE_SESSION_KEY,
				  replySize,
				  reply);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (CT_STATUS_IS_OK(status) &&
        CT_STATUS_IS_OK(localStatus))
    {
        ProxyConnectionRun(connection);
    }

cleanup:
    if (connection)
    {
        ProxyConnectionClose(connection);
    }
    CT_SAFE_FREE(message);
    CT_LOG_TRACE("status = 0x%08x (EE = %d)", status, EE);

    if (child == 0)
    {
        /* We are the child, so exit completely */
        _exit(0);
    }

    return status;
}

static
CT_STATUS
HandleSetAuthInfo(
    IN CT_SERVER_CLIENT_HANDLE Handle,
    IN CTX* Context,
    IN uint32_t Version,
    IN uint32_t Type,
    IN size_t Size
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    uid_t uid = CtServerClientGetUid(Handle);
    int fd = CtServerClientGetFd(Handle);
    NPC_MSG_PAYLOAD_AUTH_INFO* message = NULL;
    size_t offset;
    const char* server = NULL;
    const char* username = NULL;
    const char* password = NULL;
    const char* credCache = NULL;
    CT_STATUS localStatus;

    if (!CtServerClientIsAuthenticated(Handle))
    {
        localStatus = CT_STATUS_ACCESS_DENIED;
	goto reply;
    }

    status = CtServerReadMessageData(fd, Size, (void**)&message);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (!NPC_IS_SIZE_OK_MSG_PAYLOAD_AUTH_INFO(message, Size))
    {
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP_EE(EE);
    }

    offset = CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_AUTH_INFO, Data);

    GET_NEXT_STRING(&server, message->ServerSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    GET_NEXT_STRING(&username, message->UsernameSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    GET_NEXT_STRING(&password, message->PasswordSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    GET_NEXT_STRING(&credCache, message->CredCacheSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    CT_LOG_DEBUG("Uid = %ld, Server = '%s', Username = '%s', "
                 "Password = '%s', CredCache = '%s' Token = %ld",
                 uid,
                 CT_LOG_WRAP_STRING(server),
                 CT_LOG_WRAP_STRING(username),
                 password ? ((*password) ? "*" : "") : "(null)",
                 CT_LOG_WRAP_STRING(credCache),
                 message->Token);

    localStatus = ProxySetAuthInfo(Context->Proxy, uid, server, message->AuthFlags,
                                   username, password, credCache,
                                   message->Token);

reply:
    CT_LOG_TRACE("replying 0x%08x", localStatus);
    status = ReplyStatus(fd, Version, localStatus);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    CT_SAFE_FREE(message);
    CT_LOG_TRACE("status = 0x%08x (EE = %d)", status, EE);
    return status;
}

static
CT_STATUS
SendImpersonationToken(
    IN int Fd,
    IN uint32_t Version,
    IN CT_STATUS ReplyStatus,
    IN NPC_TOKEN_ID Token
    )
{
    NPC_MSG_PAYLOAD_IMP_TOKEN_REP reply;
    CT_STATUS status = CT_STATUS_SUCCESS;
  
    reply.Status = ReplyStatus;
    reply.Token  = Token;

    status = CtServerWriteMessage(Fd, Version, NPC_MSG_TYPE_IMP_TOKEN_REP,
				sizeof(reply), &reply);
    return status;
}

static
CT_STATUS
HandleCreateImpersonationToken(
    IN CT_SERVER_CLIENT_HANDLE Handle,
    IN CTX* Context,
    IN uint32_t Version,
    IN uint32_t Type,
    IN size_t Size
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    uid_t uid = CtServerClientGetUid(Handle);
    int fd = CtServerClientGetFd(Handle);
    void *message = NULL;
    CT_STATUS localStatus;
    int selectresult;
    fd_set readFds;

    NPC_TOKEN_ID allocedToken = 0;

    if (!CtServerClientIsAuthenticated(Handle))
    {
        localStatus = CT_STATUS_ACCESS_DENIED;
	goto reply;
    }

    status = CtServerReadMessageData(fd, Size, (void**)&message);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    CtLockAcquireMutex(Context->Lock);
    allocedToken = Context->NextToken++;
    CtLockReleaseMutex(Context->Lock);

reply:
    localStatus = CT_STATUS_SUCCESS;
    CT_LOG_DEBUG("Created impersonation token %ld for uid %ld",
		 allocedToken, uid);

    status = SendImpersonationToken(fd, Version, localStatus, allocedToken);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // Keep the access token valid as long as the client doesn't disconnect
    // or send a new message.
    while(1)
    {
        FD_ZERO(&readFds);
        FD_SET(fd, &readFds);
        selectresult = select(fd + 1, &readFds, NULL, NULL, NULL);
        if (selectresult < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            CT_LOG_ERROR("select failed with %d (%s)", errno, strerror(errno));
            GOTO_CLEANUP();
        }
        break;
    }


cleanup:
    if(allocedToken != 0)
        ProxyDestroyImpersonationToken(Context->Proxy, uid, allocedToken);
    CT_SAFE_FREE(message);
    CT_LOG_TRACE("status = 0x%08x (EE = %d)", status, EE);
    return status;
}

static
CT_STATUS
HandleClearAuthInfo(
    IN CT_SERVER_CLIENT_HANDLE Handle,
    IN CTX* Context,
    IN uint32_t Version,
    IN uint32_t Type,
    IN size_t Size
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    uid_t uid = CtServerClientGetUid(Handle);
    int fd = CtServerClientGetFd(Handle);
    NPC_MSG_PAYLOAD_AUTH_CLEAR* message = NULL;
    size_t offset;
    char* server = NULL;
    CT_STATUS localStatus;

    if (!CtServerClientIsAuthenticated(Handle))
    {
        localStatus = CT_STATUS_ACCESS_DENIED;
	goto reply;
    }

    status = CtServerReadMessageData(fd, Size, (void**)&message);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (!NPC_IS_SIZE_OK_MSG_PAYLOAD_AUTH_CLEAR(message, Size))
    {
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP_EE(EE);
    }

    offset = CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_AUTH_CLEAR, Data);

    GET_NEXT_STRING(&server, message->ServerSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    CT_LOG_DEBUG("Uid = %ld, Server = '%s', Token = %ld",
                 uid,
                 CT_LOG_WRAP_STRING(server),
                 message->Token);

    localStatus = ProxyClearAuthInfo(Context->Proxy, uid, server, message->Token);

reply:
    CT_LOG_TRACE("replying 0x%08x", localStatus);
    status = ReplyStatus(fd, Version, localStatus);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    CT_SAFE_FREE(message);
    CT_LOG_TRACE("status = 0x%08x (EE = %d)", status, EE);
    return status;
}

static
CT_STATUS
SendNonce(
    IN int Fd,
    IN uint32_t Version,
    IN CT_STATUS ReplyStatus,
    IN uint32_t Nonce
    )
{
    NPC_MSG_PAYLOAD_SEC_SOCKET_REP reply;
    CT_STATUS status = CT_STATUS_SUCCESS;
  
    reply.Status = ReplyStatus;
    reply.Nonce  = Nonce;

    status = CtServerWriteMessage(Fd, Version, NPC_MSG_TYPE_SEC_SOCKET_REP,
				sizeof(reply), &reply);
    return status;
}


static
CT_STATUS
VerifyNonce(
    IN int Fd,
    IN int nonce
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    uint32_t Version, Type, Size;
    uint32_t messageSize;
    NPC_MSG_PAYLOAD_SEC_SOCKET_NONCE* message = NULL;    

    status = CtServerReadMessageHeader(Fd, &Version, &Type, &Size);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    messageSize = sizeof(NPC_MSG_PAYLOAD_SEC_SOCKET_NONCE);

    status = CtServerReadMessageData(Fd, messageSize, (void**)&message);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (message->Nonce != nonce)
    {
        status = CT_STATUS_ACCESS_DENIED;
    }

cleanup:
    CT_SAFE_FREE(message);
    
    return status;
}


static
CT_STATUS
HandleSecureSocketInfo(
    IN CT_SERVER_CLIENT_HANDLE Handle,
    IN CTX* Context,
    IN uint32_t Version,
    IN uint32_t Type,
    IN size_t Size
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    uid_t uid = CtServerClientGetUid(Handle);
    int fd = CtServerClientGetFd(Handle);
    NPC_MSG_PAYLOAD_SEC_SOCKET_INFO* message = NULL;
    size_t offset;
    char* socketPath = NULL;
    int nonce;
    int secFd = -1;

    status = CtServerReadMessageData(fd, Size, (void**)&message);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (!NPC_IS_SIZE_OK_MSG_PAYLOAD_SEC_SOCKET_INFO(message, Size))
    {
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP_EE(EE);
    }

    offset = CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_SEC_SOCKET_INFO, Data);

    GET_NEXT_STRING(&socketPath, message->SocketNameSize, message, &offset, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    CT_LOG_DEBUG("Uid = %ld, SocketPath = '%s'",
                 uid,
                 CT_LOG_WRAP_STRING(socketPath));


    status = CtSecSocketConnect(&secFd, socketPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtGenerateRandomNumber(&nonce);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    CT_LOG_TRACE("replying 0x%08x", status);

    status = SendNonce(fd, Version, status, nonce);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = VerifyNonce(secFd, nonce);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    /* The client has been authenticated */
    CtServerClientSetAuthenticated(Handle, true);

    status = ReplyStatus(secFd, Version, status);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    
cleanup:
    CT_SAFE_CLOSE_FD(secFd);
    CT_SAFE_FREE(message);
    CT_LOG_TRACE("status = 0x%08x (EE = %d)", status, EE);
    return status;
}



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
    bool isDone = false;

    CT_LOG_DEBUG("Version = %d, Type = %d, Size = %d", Version, Type, Size);

    if (Version != NPC_MSG_VERSION)
    {
        status = CT_STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    switch (Type)
    {
        case NPC_MSG_TYPE_CONNECT_CHECK_CREDS:
            status = HandleConnectCheckCreds(Handle, (CTX*)Context, Version, Type, Size);
            break;
        case NPC_MSG_TYPE_CONNECT:
            status = HandleConnect(Handle, (CTX*)Context, Version, Type, Size);
            isDone = true;
            break;
        case NPC_MSG_TYPE_AUTH_SET:
            status = HandleSetAuthInfo(Handle, (CTX*)Context, Version, Type, Size);
            break;
        case NPC_MSG_TYPE_AUTH_CLEAR:
            status = HandleClearAuthInfo(Handle, (CTX*)Context, Version, Type, Size);
            break;
        case NPC_MSG_TYPE_SEC_SOCKET_INFO:
            status = HandleSecureSocketInfo(Handle, (CTX*)Context, Version, Type, Size);
	    break;
        case NPC_MSG_TYPE_CREATE_IMP_TOKEN:
            status = HandleCreateImpersonationToken(Handle, (CTX*)Context, Version, Type, Size);
	    break;
        default:
            CT_LOG_ERROR("Unsupported message type %d", Type);
            status = CT_STATUS_INVALID_MESSAGE;
    }

cleanup:
    if (status)
    {
        isDone = true;
    }

    CT_LOG_TRACE("isDone = %c, status = 0x%08x (EE = %d)",
                 CT_LOG_WRAP_BOOL(isDone), status, EE);

    return isDone;
}

static
bool
CheckIsDone(
    IN void* Context
    )
{
    CTX* ctx = (CTX*)Context;
    bool isDone;
    CtLockAcquireMutex(ctx->Lock);
    isDone = ctx->IsDone;
    CtLockReleaseMutex(ctx->Lock);
    return isDone;
}

static
void*
ServerStop(
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
ServerStart(
    IN void* Context
    )
{
    CT_STATUS status;
    CTX* ctx = (CTX*)Context;
    CT_SERVER_HANDLE server = NULL;
    bool isDone = false;

    status = CtServerCreate(&server,
                            SERVER_PATH,
                            getuid(),
                            getgid(),
                            CT_SERVER_ALL_ACCESS_MODE,
                            ServerDispatch,
                            ctx);
    if (status)
    {
        CT_LOG_ERROR("Failed to start daemon: 0x%08x\n", status);
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
        CT_LOG_ERROR("Failed to run daemon: 0x%08x\n", status);
        goto cleanup;
    }

cleanup:
    if (!CT_STATUS_IS_OK(status))
    {
        CtServerTerminate(server);
        // Notify the signal handling thread that it should end
        CtDaemonExit(1);
    }
    CtServerClose(server);

    return NULL;
}

CT_STATUS
PromptPassword(
    OUT char** Password,
    IN const char* Prompt
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
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
            status = CT_ERRNO_TO_STATUS(errno);
            GOTO_CLEANUP();
        }
        if (0 == bytes)
        {
            status = CT_STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP();
        }
        if (ch == '\n')
        {
            break;
        }
        buffer[index++] = ch;
    }

    if (index == sizeof(buffer))
    {
        status = CT_STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (index > 0)
    {
        status = CtAllocateString(&password, buffer);
    }

cleanup:
    if (Prompt)
    {
        printf("\n");
    }
    tcsetattr(0, TCSANOW, &old_termios);
    if (status)
    {
        CT_SAFE_FREE(password);
    }
    *Password = password;
    return status;
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
    char* password = NULL;
    char* prompt = NULL;
    PROXY_CONNECTION_PROVIDER providers[] =
    {
        {
            "tcp",
            TcpProxyConnect,
            TcpProxyRun,
            TcpProxyClose
        },
        {
            "np",
            NpProxyConnect,
            NpProxyRun,
            NpProxyClose
        },
        {
            0
        }
    };

    status = CtDaemonStartLogger(PARSE_ARGS_LOGLEVEL, false, NULL, NULL);
    GOTO_CLEANUP_ON_STATUS(status);

    status = ParseArgs(&args, argc, argv);
    GOTO_CLEANUP_ON_STATUS(status);

    if (args.IsHelp)
    {
        GOTO_CLEANUP();
    }

    status = SetupEnvironment();
    GOTO_CLEANUP_ON_STATUS(status);

    if (args.Username && !args.Password)
    {
        status = CtAllocateStringPrintf(&prompt, "Password for '%s': ", args.Username);
        GOTO_CLEANUP_ON_STATUS(status);

        status = PromptPassword(&password, prompt);
        GOTO_CLEANUP_ON_STATUS(status);

        args.Password = password;
    }

    status = CtDaemonStartLogger(args.LogLevel, args.IsDaemon, SYSLOG_ID, args.LogPath);
    GOTO_CLEANUP_ON_STATUS(status);

    status = ProxyCreate(&ctx.Proxy, &ctx, CheckIsDone, providers,
                         args.UseCredCache, args.Username, args.Password);
    GOTO_CLEANUP_ON_STATUS(status);

    status = CtLockCreateMutex(&ctx.Lock);
    GOTO_CLEANUP_ON_STATUS(status);

    ctx.NextToken = 1;

    status = CtDaemonRun(programName, PID_FILE, args.IsDaemon, ServerStart, ServerStop, &ctx, &exitCode);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    CT_SAFE_FREE(password);
    CT_SAFE_FREE(prompt);

    ProxyDestroy(ctx.Proxy);

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
