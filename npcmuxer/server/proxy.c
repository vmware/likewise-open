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

#ifdef HAVE_CONFIG_H
#    include <config.h>
#endif

#include <ctlogger.h>
#include <ctgoto.h>
#ifdef HAVE_STRING_H
#    include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#    include <strings.h>
#endif
#include <stdlib.h>
#include <ctmemory.h>
#include <ctserver.h>
#include <unistd.h>
#include <ctstring.h>
#include <ctlock.h>
#include <ctlist.h>

#include "proxy.h"
#include "proxycreds.h"

#ifdef __APPLE__
/* Darwin doesn't want to give us strcasecmp */
int strcasecmp(const char *s1, const char *s2);
#endif


typedef struct _PROXY_AUTH_INFO {
    uid_t Uid;
    char* Server;
    NPC_AUTH_FLAGS AuthFlags;
    char* Username;
    char* Password;
    char* CredCache;
    NPC_TOKEN_ID Token;
    CT_LIST_LINKS Links;
} PROXY_AUTH_INFO;

typedef struct _PROXY_STATE {
    pthread_mutex_t* Lock;
    CT_LIST_LINKS AuthList;
    void* Context;
    PROXY_IS_DONE IsDoneFunction;
    // TODO: Duplicate Providers and strings in ProxyCreate and destroy in ProxyDestroy
    PROXY_CONNECTION_PROVIDER* Providers;
    /* Default credentials to use */
    bool UseCredCache;
    const char* Username;
    const char* Password;
} PROXY_STATE;

#define ProxypAcquire(Proxy) CtLockAcquireMutex((Proxy)->Lock)
#define ProxypRelease(Proxy) CtLockReleaseMutex((Proxy)->Lock)

typedef struct _PROXY_CONNECTION_OPTIONAL_AUTH_INFO {
    NPC_AUTH_FLAGS AuthFlags;
    const char* Username;
    const char* Password;
} PROXY_CONNECTION_OPTIONAL_AUTH_INFO;

typedef struct _PROXY_CONNECTION_DATA {
    PROXY_HANDLE Proxy;
    uid_t Uid;
    char* Server;
    PROXY_CONNECTION_PROVIDER* Provider;
    void* InnerState;
    bool HaveAuthInfo;
    NPC_AUTH_FLAGS AuthFlags;
    char* Username;
    char* Password;
    PROXY_KRB5_CRED_HANDLE CredHandle;
    NPC_TOKEN_ID Token;
} _PROXY_CONNECTION_DATA;

/* Must lock Proxy before calling */
PROXY_AUTH_INFO*
ProxypAuthInfoFind(
    IN PROXY_HANDLE Proxy,
    IN uid_t Uid,
    IN const char* Server,
    IN NPC_TOKEN_ID Token
    )
{
    PROXY_AUTH_INFO* found = NULL;
    PROXY_AUTH_INFO* foundDefault = NULL;
    CT_LIST_LINKS* element;
    for (element = Proxy->AuthList.Next; element != &Proxy->AuthList; element = element->Next)
    {
        PROXY_AUTH_INFO* info = CT_FIELD_RECORD(element, PROXY_AUTH_INFO, Links);
        if (Uid == info->Uid && info->Token == Token)
        {
            if (!info->Server[0])
            {
                foundDefault = info;
            }
            else if (!strcasecmp(info->Server, Server))
            {
                found = info;
                break;
            }
        }
    }
    return found ? found : foundDefault;
}

/* Must lock Proxy before calling */
void
ProxypAuthInfoRemoveUser(
    IN PROXY_HANDLE Proxy,
    IN uid_t Uid,
    OUT CT_LIST_LINKS* RemoveList
    )
{
    CT_LIST_LINKS* element;
    CT_LIST_LINKS* next;
    for (element = Proxy->AuthList.Next; element != &Proxy->AuthList; element = next)
    {
        PROXY_AUTH_INFO* info = CT_FIELD_RECORD(element, PROXY_AUTH_INFO, Links);
        if (Uid == info->Uid)
        {
            CtListRemove(element);
        }
        next = element->Next;
        CtListInsertAfter(RemoveList, element);
    }
}

CT_STATUS
ProxypCacheAuthInfo(
    IN PROXY_CONNECTION_HANDLE Connection
    )
{
    CT_STATUS status;
    NPC_AUTH_FLAGS authFlags = 0;
    char* username = NULL;
    char* password = NULL;
    char* credCache = NULL;
    PROXY_AUTH_INFO* info = NULL;
    bool isAcquired = false;
    PROXY_KRB5_CRED_HANDLE credHandle = NULL;

    if (Connection->HaveAuthInfo)
    {
        status = CT_STATUS_SUCCESS;
        GOTO_CLEANUP();
    }

    ProxypAcquire(Connection->Proxy);
    isAcquired = true;

    info = ProxypAuthInfoFind(Connection->Proxy,
                              Connection->Uid,
                              Connection->Server,
                              Connection->Token);
    if (info)
    {
        if (info->Username)
        {
            status = CtAllocateString(&username, info->Username);
            GOTO_CLEANUP_ON_STATUS(status);
        }
        if (info->Password)
        {
            status = CtAllocateString(&password, info->Password);
            GOTO_CLEANUP_ON_STATUS(status);
        }
        if (info->CredCache)
        {
            status = CtAllocateString(&credCache, info->CredCache);
            GOTO_CLEANUP_ON_STATUS(status);
        }
        authFlags = info->AuthFlags;
    }

    ProxypRelease(Connection->Proxy);
    isAcquired = false;

    if (!info)
    {
        if (Connection->Proxy->Username)
        {
            status = CtAllocateString(&username, Connection->Proxy->Username);
            GOTO_CLEANUP_ON_STATUS(status);
        }
        if (Connection->Proxy->Password)
        {
            status = CtAllocateString(&password, Connection->Proxy->Password);
            GOTO_CLEANUP_ON_STATUS(status);
        }
    }

    if (credCache && !Connection->CredHandle)
    {
        status = ProxypCreateKrb5CredHandle(&credHandle,
                                            Connection->Uid,
                                            Connection->Server,
                                            credCache);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    Connection->AuthFlags = authFlags;
    Connection->Username = username;
    username = NULL;
    Connection->Password = password;
    password = NULL;
    if (!Connection->CredHandle)
    {
        Connection->CredHandle = credHandle;
        credHandle = NULL;
    }
    Connection->HaveAuthInfo = true;

    status = CT_STATUS_SUCCESS;

cleanup:
    CT_SAFE_FREE(username);
    CT_SAFE_FREE(password);
    if (credHandle)
    {
        ProxypDestroyKrb5CredHandle(credHandle);
    }
    return status;
}

CT_STATUS
ProxyConnectionGetAuthInfo(
    IN PROXY_CONNECTION_HANDLE Connection,
    OUT OPTIONAL NPC_AUTH_FLAGS* AuthFlags,
    OUT OPTIONAL const char** Username,
    OUT OPTIONAL const char** Password,
    OUT OPTIONAL const char** CredCache
    )
{
    CT_STATUS status;

    status = ProxypCacheAuthInfo(Connection);

    if (AuthFlags)
    {
        *AuthFlags = Connection->AuthFlags;
    }
    if (Username)
    {
        *Username = Connection->Username;
    }
    if (Password)
    {
        *Password = Connection->Password;
    }
    if (CredCache)
    {
        *CredCache = (Connection->CredHandle ?
                      ProxypGetKrb5CredHandleCacheName(Connection->CredHandle) :
                      (Connection->Proxy->UseCredCache ? NULL : "MEMORY:none"));
    }

    return status;
}

bool
ProxyConnectionIsDone(
    IN PROXY_CONNECTION_HANDLE Connection
    )
{
    return Connection->Proxy->IsDoneFunction(Connection->Proxy->Context);
}


CT_STATUS
ProxyCreate(
    OUT PROXY_HANDLE* Proxy,
    IN void* Context,
    IN PROXY_IS_DONE IsDoneFunction,
    IN PROXY_CONNECTION_PROVIDER* Providers,
    IN bool UseCredCache,
    IN OPTIONAL const char* Username,
    IN OPTIONAL const char* Password
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    PROXY_HANDLE proxy = NULL;

    status = CtAllocateMemory((void**)&proxy, sizeof(*proxy));
    GOTO_CLEANUP_ON_STATUS(status);

    status = CtLockCreateMutex(&proxy->Lock);
    GOTO_CLEANUP_ON_STATUS(status);

    CtListInit(&proxy->AuthList);

    proxy->Context = Context;
    proxy->IsDoneFunction = IsDoneFunction;
    proxy->Providers = Providers;
    proxy->UseCredCache = UseCredCache;
    proxy->Username = Username;
    proxy->Password = Password;

cleanup:
    if (status)
    {
        ProxyDestroy(proxy);
        proxy = NULL;
    }

    *Proxy = proxy;

    return status;
}

void
ProxyDestroy(
    IN PROXY_HANDLE Proxy
    )
{
    if (Proxy)
    {
        CtLockFreeMutex(Proxy->Lock);
        CtFreeMemory(Proxy);
    }
}

static
CT_STATUS
ProxypFindProvider(
    OUT PROXY_CONNECTION_PROVIDER** Provider,
    IN PROXY_HANDLE Proxy,
    IN const char* Protocol
    )
{
    CT_STATUS status;
    int EE = 0;
    PROXY_CONNECTION_PROVIDER* provider = NULL;
    int i;

    for (i = 0; Proxy->Providers[i].Name; i++)
    {
        if (!strcmp(Protocol, Proxy->Providers[i].Name))
        {
            provider = &Proxy->Providers[i];
            break;
        }
    }

    if (!provider)
    {
        CT_LOG_DEBUG("Unknown protocol '%s'", CT_LOG_WRAP_STRING(Protocol));
        status = CT_STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    status = CT_STATUS_SUCCESS;

cleanup:
    *Provider = provider;
    return status;
}

static
CT_STATUS
ProxypConnectionOpenInternal(
    OUT PROXY_CONNECTION_HANDLE* Connection,
    IN PROXY_HANDLE Proxy,
    IN int Fd,
    IN uid_t Uid,
    IN const char* Protocol,
    IN const char* Address,
    IN const char* Endpoint,
    IN const char* Options,
    IN const char* CredCache,
    IN NPC_TOKEN_ID Token,
    IN OPTIONAL PROXY_CONNECTION_OPTIONAL_AUTH_INFO* AuthInfo,
    OUT size_t* SessionKeyLen,
    OUT unsigned char** SessionKey
    )
{
    CT_STATUS status;
    int EE = 0;
    PROXY_CONNECTION_PROVIDER* provider = NULL;
    PROXY_CONNECTION_HANDLE connection = NULL;
    size_t SessKeyLen = 0;
    unsigned char* SessKey = NULL;

    status = ProxypFindProvider(&provider, Proxy, Protocol);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtAllocateMemory((void**)&connection, sizeof(*connection));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtAllocateString(&connection->Server, Address);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    connection->Proxy = Proxy;
    connection->Uid = Uid;
    connection->Provider = provider;
    connection->Token = Token;

    if (CredCache)
    {
        status = ProxypCreateKrb5CredHandle(&connection->CredHandle,
                                            connection->Uid,
                                            connection->Server,
                                            CredCache);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (AuthInfo)
    {
        connection->HaveAuthInfo = true;
        connection->AuthFlags = AuthInfo->AuthFlags;

        if (AuthInfo->Username)
        {
            status = CtAllocateString(&connection->Username, AuthInfo->Username);
            GOTO_CLEANUP_ON_STATUS(status);
        }
        
        if (AuthInfo->Password)
        {
            status = CtAllocateString(&connection->Password, AuthInfo->Password);
            GOTO_CLEANUP_ON_STATUS(status);
        }
    }

    status = provider->Open(&connection->InnerState, connection, Fd, Address, Endpoint, Options,
			    &SessKeyLen, &SessKey);

cleanup:
    if (status)
    {
        ProxyConnectionClose(connection);
        connection = NULL;
    }

    *Connection    = connection;
    *SessionKeyLen = SessKeyLen;
    *SessionKey    = SessKey;

    if (status)
    {
        CT_LOG_TRACE("status = 0x%08x (EE = %d)", status, EE);
    }
    return status;
}

CT_STATUS
ProxyConnectionCheckCreds(
    IN PROXY_HANDLE Proxy,
    IN uid_t Uid,
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
    CT_STATUS status;
    PROXY_CONNECTION_HANDLE connection = NULL;
    PROXY_CONNECTION_OPTIONAL_AUTH_INFO info = { 0 };
    size_t SessKeyLen = 0;
    unsigned char* SessKey = NULL;

    info.AuthFlags = AuthFlags;
    info.Username = Username;
    info.Password = Password;
       
    status = ProxypConnectionOpenInternal(&connection, Proxy, -1, Uid,
                                          Protocol, Address, Endpoint,
                                          Options, CredCache, 0, &info,
					  &SessKeyLen, &SessKey);
    ProxyConnectionClose(connection);

    if (status)
    {
        CT_LOG_TRACE("status = 0x%08x", status);
    }

    return status;
}

CT_STATUS
ProxyConnectionOpen(
    OUT PROXY_CONNECTION_HANDLE* Connection,
    IN PROXY_HANDLE Proxy,
    IN int Fd,
    IN uid_t Uid,
    IN const char* Protocol,
    IN const char* Address,
    IN const char* Endpoint,
    IN const char* Options,
    IN const char* CredCache,
    IN NPC_TOKEN_ID Token,
    OUT size_t* SessionKeyLen,
    OUT unsigned char** SessionKey
    )
{
    return ProxypConnectionOpenInternal(Connection, Proxy, Fd, Uid, Protocol,
                                        Address, Endpoint, Options, CredCache,
                                        Token,
                                        NULL, SessionKeyLen, SessionKey);
}

void
ProxyConnectionRun(
    IN PROXY_CONNECTION_HANDLE Connection
    )
{
    Connection->Provider->Run(Connection->InnerState);
}

void
ProxyConnectionClose(
    IN PROXY_CONNECTION_HANDLE Connection
    )
{
    if (Connection)
    {
        if (Connection->InnerState)
        {
            Connection->Provider->Close(Connection->InnerState);
        }
        ProxypDestroyKrb5CredHandle(Connection->CredHandle);
        CT_SAFE_FREE(Connection->Username);
        CT_SAFE_FREE(Connection->Password);
        CtFreeMemory(Connection);
    }
}

static
void
ProxypAuthInfoFree(
    PROXY_AUTH_INFO* Info
    )
{
    if (Info)
    {
        CT_SAFE_FREE(Info->Server);
        CT_SAFE_FREE(Info->Username);
        CT_SAFE_FREE(Info->Password);
        CT_SAFE_FREE(Info->CredCache);
        CtFreeMemory(Info);
    }
}

CT_STATUS
ProxySetAuthInfo(
    IN PROXY_HANDLE Proxy,
    IN uid_t Uid,
    IN const char* Server,
    IN NPC_AUTH_FLAGS AuthFlags,
    IN const char* Username,
    IN const char* Password,
    IN const char* CredCache,
    IN NPC_TOKEN_ID Token
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    PROXY_AUTH_INFO* oldInfo = NULL;
    PROXY_AUTH_INFO* newInfo = NULL;

    status = CtAllocateMemory((void**)&newInfo, sizeof(*newInfo));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtAllocateString(&newInfo->Server, Server);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (Username)
    {
        status = CtAllocateString(&newInfo->Username, Username);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    if (Password)
    {
        status = CtAllocateString(&newInfo->Password, Password);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    if (CredCache)
    {
        status = CtAllocateString(&newInfo->CredCache, CredCache);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    newInfo->AuthFlags = AuthFlags;
    newInfo->Uid = Uid;
    newInfo->Token = Token;

    ProxypAcquire(Proxy);
    oldInfo = ProxypAuthInfoFind(Proxy, Uid, Server, Token);
    if (oldInfo)
    {
        CtListRemove(&oldInfo->Links);
    }
    CtListInsertAfter(&Proxy->AuthList, &newInfo->Links);
    ProxypRelease(Proxy);

    status = CT_STATUS_SUCCESS;

cleanup:
    ProxypAuthInfoFree(oldInfo);
    if (status)
    {
        ProxypAuthInfoFree(newInfo);
    }

    if (status)
    {
        CT_LOG_TRACE("status = 0x%08x (EE = %d)", status, EE);
    }
    return status;
}

CT_STATUS
ProxyClearAuthInfo(
    IN PROXY_HANDLE Proxy,
    IN uid_t Uid,
    IN OPTIONAL const char* Server,
    IN NPC_TOKEN_ID Token
    )
{
    CT_STATUS status = CT_STATUS_NOT_FOUND;
    PROXY_AUTH_INFO* oldInfo = NULL;
    CT_LIST_LINKS removeList = { 0 };

    ProxypAcquire(Proxy);
    if (Server)
    {
        oldInfo = ProxypAuthInfoFind(Proxy, Uid, Server, Token);
        if (oldInfo)
        {
            CtListRemove(&oldInfo->Links);
        }
    }
    else
    {
        CtListInit(&removeList);
        ProxypAuthInfoRemoveUser(Proxy, Uid, &removeList);
    }
    ProxypRelease(Proxy);

    /* Manipulate the heap outside the lock */
    if (Server)
    {
        if (oldInfo)
        {
            ProxypAuthInfoFree(oldInfo);
            status = CT_STATUS_SUCCESS;
        }
    }
    else if (!CtListIsEmpty(&removeList))
    {
        CT_LIST_LINKS* element;
        CT_LIST_LINKS* next;
        for (element = removeList.Next; element != &removeList; element = next)
        {
            PROXY_AUTH_INFO* info = CT_FIELD_RECORD(element, PROXY_AUTH_INFO, Links);
            CtListRemove(element);
            next = element->Next;
            ProxypAuthInfoFree(info);
        }
        status = CT_STATUS_SUCCESS;
    }

    return status;
}

CT_STATUS
ProxyDestroyImpersonationToken(
    IN PROXY_HANDLE Proxy,
    IN uid_t Uid,
    IN NPC_TOKEN_ID Token
    )
{
    return ProxyClearAuthInfo(Proxy, Uid, NULL, Token);
}
