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

/* ex: set shiftwidth=4 expandtab: */
/* -*- mode: c; c-basic-offset: 4 -*- */

#include <sys/socket.h>
#include <ctmemory.h>
#include <ctserver.h>
#include <ctstring.h>
#include <ctgoto.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

#include <npc.h>
#include <npcmsg.h>
#include <krb5.h>

/* TODO: Hmm...we may need to do something about this... */
#if 0
    /* Ignore pipe signal in favor of EPIPE */
    signal(SIGPIPE, SIG_IGN);
#endif

struct _NPC_TOKEN_HANDLE_DATA
{
    NPC_TOKEN_ID Id;
    int Fd;
    int Referenced;
    pthread_mutex_t Mutex;
};

typedef struct _NPC_STATE
{
    pthread_once_t InitOnce;
    pthread_once_t CleanupOnce;
    CT_STATUS InitStatus;
    pthread_key_t ImpersonationTokenKey;
    pthread_key_t CredCacheKey;
    bool IsInitialized;
} NPC_STATE, *PNPC_STATE;

static NPC_STATE gNpcpState = {
#if defined(sun) || defined(_AIX) || defined (_HPUX)
    .InitOnce = { PTHREAD_ONCE_INIT },
    .CleanupOnce = { PTHREAD_ONCE_INIT },
#else
    .InitOnce = PTHREAD_ONCE_INIT,
    .CleanupOnce = PTHREAD_ONCE_INIT,
#endif
    .InitStatus = CT_STATUS_SUCCESS,
    .IsInitialized = false
};

uint32_t gNpcpDebugLevel = 0;

#define _NPC_DEBUG(Level, Format, ...) \
    do { \
        if (gNpcpDebugLevel >= Level) \
        { \
            fprintf(stderr, "[%d] %s() " Format "\n", Level, __FUNCTION__, ## __VA_ARGS__); \
        } \
    } while (0)

#define NPC_DEBUG(Format, ...) \
    _NPC_DEBUG(1, Format, ## __VA_ARGS__)

#define NPC_DEBUG_TRACE(Format, ...) \
    _NPC_DEBUG(2, Format, ## __VA_ARGS__)

#define NPCP_STRING_SIZE(String) ((String) ? (strlen(String) + 1) : 0)

void
NpcSetDebugLevel(
    IN uint32_t Level
    )
{
    gNpcpDebugLevel = Level;
}


#define SET_NEXT_STRING(Message, OffsetPtr, String, StringSize) \
    do { \
        memcpy(CT_PTR_ADD(Message , *(OffsetPtr)), String, StringSize); \
        *(OffsetPtr) += (StringSize); \
    } while (0)


static
CT_STATUS
NpcpGetStatusReply(
    IN int Fd
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    NPC_MSG_PAYLOAD_STATUS* reply = NULL;
    uint32_t messageSize;
    uint32_t version;
    uint32_t type;

    status = CtServerReadMessage(Fd, &version, &type, &messageSize,
                                 (void**)&reply);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    NPC_DEBUG("Version = %d, Type = %d, Size = %d", version, type, messageSize);

    if (type != NPC_MSG_TYPE_STATUS)
    {
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP_EE(EE);
    }

    if (messageSize != sizeof(NPC_MSG_TYPE_STATUS))
    {
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP_EE(EE);
    }

    status = reply->Status;

cleanup:
    CT_SAFE_FREE(reply);

    return status;
}


static
CT_STATUS
NpcpConnectToServerWithSocket(
    IN int Fd
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;

    status = CtServerConnectExistingSocket(Fd, SERVER_PATH);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:

    NPC_DEBUG_TRACE("status = 0x%08x, EE = %d", status, EE);

    return status;
}


static
CT_STATUS
NpcpConnectToServer(
    OUT int* Fd
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;

    status = CtServerConnect(Fd, SERVER_PATH);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    NPC_DEBUG_TRACE("status = 0x%08x, EE = %d", status, EE);

    return status;
}


static
CT_STATUS
NpcpTransactConnect(
    IN int Fd,
    IN const char* Protocol,
    IN const char* Address,
    IN const char* Endpoint,
    IN const char* Options,
    IN const char* CredCache,
    IN const size_t SessionKeySize,
    OUT size_t* SessionKeyLen,
    OUT unsigned char* SessionKey
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    uint32_t messageSize, replySize;
    NPC_MSG_PAYLOAD_CONNECT* message = NULL;
    size_t offset = CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_CONNECT, Data);
    size_t protocolSize;
    size_t addressSize;
    size_t endpointSize;
    size_t optionsSize;
    size_t credCacheSize;
    NPC_MSG_PAYLOAD_SESSION_KEY *reply = NULL;
    uint32_t version;
    uint32_t type;
    NPC_TOKEN_HANDLE currentHandle;

    protocolSize = NPCP_STRING_SIZE(Protocol);
    addressSize = NPCP_STRING_SIZE(Address);
    endpointSize = NPCP_STRING_SIZE(Endpoint);
    optionsSize = NPCP_STRING_SIZE(Options);
    credCacheSize = NPCP_STRING_SIZE(CredCache);

    messageSize = offset + protocolSize + addressSize + endpointSize + optionsSize + credCacheSize;
    status = CtAllocateMemory((void**)&message, messageSize);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    message->ProtocolSize = protocolSize;
    message->AddressSize = addressSize;
    message->EndpointSize= endpointSize;
    message->OptionsSize = optionsSize;
    message->CredCacheSize = credCacheSize;
    currentHandle = NpcGetThreadImpersonationToken();
    if(currentHandle == NULL)
        message->Token = 0;
    else
        message->Token = currentHandle->Id;

    SET_NEXT_STRING(message, &offset, Protocol, protocolSize);
    SET_NEXT_STRING(message, &offset, Address, addressSize);
    SET_NEXT_STRING(message, &offset, Endpoint, endpointSize);
    SET_NEXT_STRING(message, &offset, Options, optionsSize);
    SET_NEXT_STRING(message, &offset, CredCache, credCacheSize);

    /* send the connection request */
    status = CtServerWriteMessage(Fd,
                                  NPC_MSG_VERSION,
                                  NPC_MSG_TYPE_CONNECT,
                                  messageSize,
                                  message);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    /* read the connection status and session key */
    status = CtServerReadMessage(Fd,
				 &version,
				 &type,
				 &replySize,
				 (void**)&reply);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    NPC_DEBUG("Version = %d, Type = %d, Size = %d", version, type, messageSize);
    
    if (type != NPC_MSG_TYPE_SESSION_KEY)
    {
	status = CT_STATUS_INVALID_MESSAGE;
	GOTO_CLEANUP_EE(EE);
    }

    if (replySize != sizeof(NPC_MSG_PAYLOAD_SESSION_KEY) + reply->SessionKeyLen)
    {
	status = CT_STATUS_INVALID_MESSAGE;
	GOTO_CLEANUP_EE(EE);
    }

    if (reply->SessionKeyLen > SessionKeySize)
    {
	status = CT_STATUS_INVALID_PARAMETER;
	GOTO_CLEANUP_EE(EE);
    }

    status         = reply->Status;
    *SessionKeyLen = reply->SessionKeyLen;
    memcpy((void*)(SessionKey), (void*)(reply->Data), reply->SessionKeyLen);

cleanup:
    CT_SAFE_FREE(message);
    CT_SAFE_FREE(reply);
    NPC_DEBUG_TRACE("status = 0x%08x, EE = %d", status, EE);

    return status;
}

static
CT_STATUS
NpcpTransactConnectCheckCreds(
    IN int Fd,
    IN const char* Protocol,
    IN const char* Address,
    IN const char* Endpoint,
    IN const char* Options,
    IN NPC_AUTH_FLAGS AuthFlags,
    IN const char* Username,
    IN const char* Password,
    IN const char* CredCache
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    uint32_t messageSize;
    NPC_MSG_PAYLOAD_CONNECT_CHECK_CREDS* message = NULL;
    size_t offset = CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_CONNECT_CHECK_CREDS, Data);
    size_t protocolSize;
    size_t addressSize;
    size_t endpointSize;
    size_t optionsSize;
    size_t usernameSize;
    size_t passwordSize;
    size_t credCacheSize;

    protocolSize = NPCP_STRING_SIZE(Protocol);
    addressSize = NPCP_STRING_SIZE(Address);
    endpointSize = NPCP_STRING_SIZE(Endpoint);
    optionsSize = NPCP_STRING_SIZE(Options);
    usernameSize = NPCP_STRING_SIZE(Username);
    passwordSize = NPCP_STRING_SIZE(Password);
    credCacheSize = NPCP_STRING_SIZE(CredCache);;

    messageSize = offset + protocolSize + addressSize + endpointSize + optionsSize + usernameSize + passwordSize + credCacheSize;
    status = CtAllocateMemory((void**)&message, messageSize);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    message->ProtocolSize = protocolSize;
    message->AddressSize = addressSize;
    message->EndpointSize= endpointSize;
    message->OptionsSize = optionsSize;
    message->UsernameSize = usernameSize;
    message->PasswordSize= passwordSize;
    message->CredCacheSize = credCacheSize;
    message->AuthFlags = AuthFlags;

    SET_NEXT_STRING(message, &offset, Protocol, protocolSize);
    SET_NEXT_STRING(message, &offset, Address, addressSize);
    SET_NEXT_STRING(message, &offset, Endpoint, endpointSize);
    SET_NEXT_STRING(message, &offset, Options, optionsSize);
    SET_NEXT_STRING(message, &offset, Username, usernameSize);
    SET_NEXT_STRING(message, &offset, Password, passwordSize);
    SET_NEXT_STRING(message, &offset, CredCache, credCacheSize);

    status = CtServerWriteMessage(Fd,
                                  NPC_MSG_VERSION,
                                  NPC_MSG_TYPE_CONNECT_CHECK_CREDS,
                                  messageSize,
                                  message);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NpcpGetStatusReply(Fd);

cleanup:
    CT_SAFE_FREE(message);

    NPC_DEBUG_TRACE("status = 0x%08x, EE = %d", status, EE);

    return status;
}

static
CT_STATUS
NpcpTransactSetAuthInfo(
    IN int Fd,
    IN const char* Server,
    IN NPC_AUTH_FLAGS AuthFlags,
    IN const char* Username,
    IN const char* Password,
    IN const char* CredCache
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    uint32_t messageSize;
    NPC_MSG_PAYLOAD_AUTH_INFO* message = NULL;
    size_t offset = CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_AUTH_INFO, Data);
    size_t serverSize;
    size_t usernameSize;
    size_t passwordSize;
    size_t credCacheSize;
    NPC_TOKEN_HANDLE currentHandle;

    serverSize = NPCP_STRING_SIZE(Server);
    usernameSize = NPCP_STRING_SIZE(Username);
    passwordSize = NPCP_STRING_SIZE(Password);
    credCacheSize = NPCP_STRING_SIZE(CredCache);;

    messageSize = offset + serverSize + usernameSize + passwordSize + credCacheSize;
    status = CtAllocateMemory((void**)&message, messageSize);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    message->ServerSize = serverSize;
    message->UsernameSize = usernameSize;
    message->PasswordSize= passwordSize;
    message->CredCacheSize = credCacheSize;
    message->AuthFlags = AuthFlags;
    currentHandle = NpcGetThreadImpersonationToken();
    if(currentHandle == NULL)
        message->Token = 0;
    else
        message->Token = currentHandle->Id;

    SET_NEXT_STRING(message, &offset, Server, serverSize);
    SET_NEXT_STRING(message, &offset, Username, usernameSize);
    SET_NEXT_STRING(message, &offset, Password, passwordSize);
    SET_NEXT_STRING(message, &offset, CredCache, credCacheSize);

    status = CtServerWriteMessage(Fd,
                                  NPC_MSG_VERSION,
                                  NPC_MSG_TYPE_AUTH_SET,
                                  messageSize,
                                  message);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NpcpGetStatusReply(Fd);

cleanup:
    CT_SAFE_FREE(message);

    NPC_DEBUG_TRACE("status = 0x%08x, EE = %d", status, EE);

    return status;
}

static
CT_STATUS
NpcpTransactClearAuthInfo(
    IN int Fd,
    IN OPTIONAL const char* Server
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    uint32_t messageSize;
    NPC_MSG_PAYLOAD_AUTH_CLEAR* message = NULL;
    size_t offset = CT_FIELD_OFFSET(NPC_MSG_PAYLOAD_AUTH_CLEAR, Data);
    size_t serverSize;
    NPC_TOKEN_HANDLE currentHandle;

    serverSize = NPCP_STRING_SIZE(Server);

    messageSize = offset + serverSize;
    status = CtAllocateMemory((void**)&message, messageSize);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    message->ServerSize = serverSize;

    SET_NEXT_STRING(message, &offset, Server, serverSize);

    currentHandle = NpcGetThreadImpersonationToken();
    if(currentHandle == NULL)
        message->Token = 0;
    else
        message->Token = currentHandle->Id;

    status = CtServerWriteMessage(Fd,
                                  NPC_MSG_VERSION,
                                  NPC_MSG_TYPE_AUTH_CLEAR,
                                  messageSize,
                                  message);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NpcpGetStatusReply(Fd);

cleanup:
    CT_SAFE_FREE(message);

    NPC_DEBUG_TRACE("status = 0x%08x, EE = %d", status, EE);

    return status;
}

int
NpcCreateImpersonationToken(
    OUT NPC_TOKEN_HANDLE* Handle
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int error;
    int EE = 0;
    int fd = -1;
    NPC_MSG_PAYLOAD_IMP_TOKEN_REP* reply = NULL;
    uint32_t messageSize;
    uint32_t version;
    uint32_t type;
    NPC_TOKEN_HANDLE_DATA* handle = NULL;

    *Handle = NULL;

    status = NpcpConnectToServer(&fd);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtServerWriteMessage(fd,
				  NPC_MSG_VERSION,
				  NPC_MSG_TYPE_CREATE_IMP_TOKEN,
				  0,
				  0);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtServerReadMessage(fd, &version, &type, &messageSize,
                                 (void**)&reply);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    NPC_DEBUG("Version = %d, Type = %d, Size = %d", version, type, messageSize);

    if (type != NPC_MSG_TYPE_IMP_TOKEN_REP)
    {
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP_EE(EE);
    }

    if (messageSize != sizeof(NPC_MSG_PAYLOAD_IMP_TOKEN_REP))
    {
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP_EE(EE);
    }

    status = reply->Status;    

    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    NPC_DEBUG("Token = %ld", (long int)reply->Token);

    status = CtAllocateMemory((void**)&handle, sizeof(*handle));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    handle->Id = reply->Token;
    handle->Fd = fd;
    fd = -1;
    
    error = pthread_mutex_init(&handle->Mutex, NULL);
    status = CtErrnoToStatus(error);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    handle->Referenced = 1;
    *Handle = handle;
    handle = NULL;

cleanup:
    CT_SAFE_CLOSE_FD(fd);
    CT_SAFE_FREE(handle);
    CT_SAFE_FREE(reply);

    NPC_DEBUG_TRACE("status = 0x%08x, EE = %d", status, EE);
    return CtStatusToErrno(status);
}

void
NpcpCloseImpersonationToken(
    IN NPC_TOKEN_HANDLE Handle
    )
{
    NPC_DEBUG("Freeing token id %ld", (unsigned long int)Handle->Id);
    CT_SAFE_CLOSE_FD(Handle->Fd);
    pthread_mutex_destroy(&Handle->Mutex);
    CT_SAFE_FREE(Handle);
}

int
NpcCloseImpersonationToken(
    IN NPC_TOKEN_HANDLE Handle
    )
{
    CT_STATUS status;
    int EE = 0;
    bool locked = false;
    bool freeHandle = false;
    int error;

    if (!Handle)
    {
        return CT_STATUS_INVALID_PARAMETER;
    }

    error = pthread_mutex_lock(&Handle->Mutex);
    status = CtErrnoToStatus(error);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    locked = true;

    Handle->Referenced--;
    if (Handle->Referenced == 0)
    {
        freeHandle = true;
    }

    error = pthread_mutex_unlock(&Handle->Mutex);
    status = CtErrnoToStatus(error);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    locked = false;

    if (freeHandle)
    {
        NpcpCloseImpersonationToken(Handle);
    }

cleanup:
    if(locked)
    {
        pthread_mutex_unlock(&Handle->Mutex);
    }
    return status;
}

int
NpcGetImpersonationTokenId(
    IN NPC_TOKEN_HANDLE Handle,
    OUT NPC_TOKEN_ID* Id
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    NPC_TOKEN_ID id = NPC_TOKEN_ID_INVALID;

    if (!Handle || !Id)
    {
        status = CT_STATUS_INVALID_PARAMETER;
	GOTO_CLEANUP_EE(EE);
    }

    id = Handle->Id;

cleanup:
    if (Id)
    {
	*Id = id;
    }

    return status;
}

static
void
NpcpInitStateOnce(
    )
{
    CT_STATUS status;
    int error;

    error = pthread_key_create(&gNpcpState.ImpersonationTokenKey, NULL);
    status = CtErrnoToStatus(error);
    if (!CT_STATUS_IS_OK(status))
    {
        NPC_DEBUG("Error creating key: status = 0x%08x, error = %d", status, error);
    }

    error = pthread_key_create(&gNpcpState.CredCacheKey, CtFreeMemory);
    status = CtErrnoToStatus(error);

    if (!CT_STATUS_IS_OK(status))
    {
        NPC_DEBUG("Error creating key: status = 0x%08x, error = %d", status, error);
    }

    gNpcpState.InitStatus = status;
    gNpcpState.IsInitialized = true;
}

void
NpcpCleanupStateOnce(
    )
{
    CT_STATUS status;
    int error;

    if (gNpcpState.IsInitialized && CT_STATUS_IS_OK(gNpcpState.InitStatus))
    {
        error = pthread_key_delete(gNpcpState.ImpersonationTokenKey);
        status = CtErrnoToStatus(error);

        if (!CT_STATUS_IS_OK(status))
        {
            NPC_DEBUG("Error deleting key: status = 0x%08x, error = %d", status, error);
        }

        error = pthread_key_delete(gNpcpState.CredCacheKey);
        status = CtErrnoToStatus(error);

        if (!CT_STATUS_IS_OK(status))
        {
            NPC_DEBUG("Error deleting key: status = 0x%08x, error = %d", status, error);
        }

        gNpcpState.IsInitialized = false;
    }
}

static
CT_STATUS
NpcpInitState(
    )
{
    pthread_once(&gNpcpState.InitOnce, NpcpInitStateOnce);
    /* TODO - This is just until we start using some check-initialized function */
    return gNpcpState.IsInitialized ? gNpcpState.InitStatus : CT_STATUS_INVALID_PARAMETER;
}

int
NpcInit(
    )
{
    return NpcpInitState();
}

void
NpcCleanup(
    )
{
    pthread_once(&gNpcpState.CleanupOnce, NpcpCleanupStateOnce);
}

NPC_TOKEN_HANDLE
NpcGetThreadImpersonationToken(
    )
{
    CT_STATUS status;
    int EE = 0;
    NPC_TOKEN_HANDLE handle = NULL;

    status = NpcpInitState();
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    handle = (NPC_TOKEN_HANDLE) pthread_getspecific(gNpcpState.ImpersonationTokenKey);

cleanup:
    return handle;
}

int
NpcImpersonate(
    IN NPC_TOKEN_HANDLE Handle
    )
{
    CT_STATUS status;
    int error;
    int EE = 0;
    NPC_TOKEN_HANDLE oldHandle = NULL;
    bool locked = false;

    if (!Handle)
    {
        status = CT_STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = NpcpInitState();
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    oldHandle = NpcGetThreadImpersonationToken();
    if (oldHandle)
    {
        /* TODO - Need another error code, something about already existing */
        status = CT_STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    error = pthread_mutex_lock(&Handle->Mutex);
    status = CtErrnoToStatus(error);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    locked = true;

    error = pthread_setspecific(gNpcpState.ImpersonationTokenKey, Handle);
    status = CtErrnoToStatus(error);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    Handle->Referenced++;

cleanup:
    if(locked)
    {
        pthread_mutex_unlock(&Handle->Mutex);
    }
    return status;
}

int
NpcRevertToSelf(
    )
{
    CT_STATUS status;
    int EE = 0;
    NPC_TOKEN_HANDLE oldHandle = NULL;
    bool locked = false;
    bool freeHandle = false;
    int error;

    status = NpcpInitState();
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    oldHandle = NpcGetThreadImpersonationToken();
    if (!oldHandle)
    {
        /* TODO - Need another error code, something about already existing */
        status = CT_STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    error = pthread_mutex_lock(&oldHandle->Mutex);
    status = CtErrnoToStatus(error);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    locked = true;

    error = pthread_setspecific(gNpcpState.ImpersonationTokenKey, NULL);
    status = CtErrnoToStatus(error);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    oldHandle->Referenced--;

    if(oldHandle->Referenced == 0)
    {
        freeHandle = true;
    }

    error = pthread_mutex_unlock(&oldHandle->Mutex);
    status = CtErrnoToStatus(error);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    locked = false;

    if(freeHandle)
    {
        NpcpCloseImpersonationToken(oldHandle);
    }

cleanup:
    if(locked)
    {
        pthread_mutex_unlock(&oldHandle->Mutex);
    }
    return status;
}

int
NpcConnectCheckCreds(
    IN const char* Protocol,
    IN const char* Address,
    IN const char* Endpoint,
    IN const char* Options,
    IN NPC_AUTH_FLAGS AuthFlags,
    IN const char* Username,
    IN const char* Password
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    int fd = -1;
    char* credCache = NULL;

    status = NpcGetCredCacheName(&credCache);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NpcpConnectToServer(&fd);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NpcpTransactConnectCheckCreds(fd, Protocol, Address, Endpoint, Options, AuthFlags, Username, Password, credCache);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    CT_SAFE_CLOSE_FD(fd);
    CT_SAFE_FREE(credCache);

    NPC_DEBUG_TRACE("status = 0x%08x, EE = %d", status, EE);
    return CtStatusToErrno(status);
}


int
NpcConnect(
    OUT int* Fd,
    IN const char* Protocol,
    IN const char* Address,
    IN const char* Endpoint,
    IN const char* Options,
    OUT size_t SessKeySize,
    OUT size_t* SessKeyLen,
    OUT unsigned char* SessKey
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    int fd = -1;
    char* credCache = NULL;

    status = NpcGetCredCacheName(&credCache);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NpcpConnectToServer(&fd);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NpcpTransactConnect(fd, Protocol, Address, Endpoint, Options, credCache,
				 SessKeySize, SessKeyLen, SessKey);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    CT_SAFE_FREE(credCache);
    if (status)
    {
        CT_SAFE_CLOSE_FD(fd);
    }
    *Fd = fd;

    NPC_DEBUG_TRACE("status = 0x%08x, EE = %d", status, EE);

    return CtStatusToErrno(status);
}

int
NpcConnectExistingSocket(
    IN int Fd,
    IN const char* Protocol,
    IN const char* Address,
    IN const char* Endpoint,
    IN const char* Options,
    OUT size_t SessKeySize,
    OUT size_t* SessKeyLen,
    OUT unsigned char* SessKey
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    char* credCache = NULL;

    status = NpcGetCredCacheName(&credCache);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NpcpConnectToServerWithSocket(Fd);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NpcpTransactConnect(Fd, Protocol, Address, Endpoint, Options, credCache,
				 SessKeySize, SessKeyLen, SessKey);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    CT_SAFE_FREE(credCache);
    NPC_DEBUG_TRACE("status = 0x%08x, EE = %d", status, EE);
    return CtStatusToErrno(status);
}

int
NpcSetAuthInfo(
    IN const char* Server,
    IN NPC_AUTH_FLAGS AuthFlags,
    IN const char* Username,
    IN const char* Password
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    int fd = -1;
    char* credCache = NULL;

    status = NpcGetCredCacheName(&credCache);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NpcpConnectToServer(&fd);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NpcpTransactSetAuthInfo(fd, Server, AuthFlags, Username, Password, credCache);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    CT_SAFE_CLOSE_FD(fd);
    CT_SAFE_FREE(credCache);

    NPC_DEBUG_TRACE("status = 0x%08x, EE = %d", status, EE);
    return CtStatusToErrno(status);
}

int
NpcClearAuthInfo(
    IN OPTIONAL const char* Server
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    int fd = -1;

    status = NpcpConnectToServer(&fd);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NpcpTransactClearAuthInfo(fd, Server);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    CT_SAFE_CLOSE_FD(fd);

    NPC_DEBUG_TRACE("status = 0x%08x, EE = %d", status, EE);
    return CtStatusToErrno(status);
}

int
NpcGetCredCacheName(
    OUT char** CredCache
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    krb5_context ctx = NULL;
    krb5_error_code error = 0;
    const char* credCache = NULL;
    char* result = NULL;

    credCache = (const char *) pthread_getspecific(gNpcpState.CredCacheKey);

    if (credCache == NULL)
    {
        error = krb5_init_context(&ctx);
        if (error)
        {
            status = CT_STATUS_OUT_OF_MEMORY;
            GOTO_CLEANUP();
        }

        credCache = krb5_cc_default_name(ctx);
        if (!credCache)
        {
            status = CT_STATUS_OUT_OF_MEMORY;
            GOTO_CLEANUP();
        }
    }

    status = CtAllocateString(&result, credCache);

cleanup:
    if (ctx)
    {
        krb5_free_context(ctx);
    }
    if (status)
    {
        CT_SAFE_FREE(result);
    }
    *CredCache = result;
    return CtStatusToErrno(status);
}

int
NpcSetCredCacheName(
    IN const char* CredCache
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    char* credCacheCopy = NULL;
    int EE = 0;
    int error = 0;
    const char *prefix = "";

    if (CredCache != NULL)
    {
        if (!strchr(CredCache, ':'))
        {
            prefix = "FILE:";
        }

        status = CtAllocateStringPrintf(
                &credCacheCopy,
                "%s%s",
                prefix,
                CredCache);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    error = pthread_setspecific(gNpcpState.CredCacheKey, credCacheCopy);
    status = CtErrnoToStatus(error);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    credCacheCopy = NULL;

cleanup:
    CT_SAFE_FREE(credCacheCopy);
    return CtStatusToErrno(status);
}

void
NpcFreeCredCacheName(
    IN char* CredCache
    )
{
    CT_SAFE_FREE(CredCache);
}
