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

#ifndef __PROXY_H__
#define __PROXY_H__

#include <ctstatus.h>
#include <npctypes.h>
#include <cttypes.h>
#include <sys/types.h>

struct _PROXY_CONNECTION_DATA;
typedef struct _PROXY_CONNECTION_DATA *PROXY_CONNECTION_HANDLE;

struct _PROXY_STATE;
typedef struct _PROXY_STATE *PROXY_HANDLE;

/* For connection providers */
CT_STATUS
ProxyConnectionGetAuthInfo(
    IN PROXY_CONNECTION_HANDLE Connection,
    OUT OPTIONAL NPC_AUTH_FLAGS* AuthFlags,
    OUT OPTIONAL const char** Username,
    OUT OPTIONAL const char** Password,
    OUT OPTIONAL const char** CredCache
    );

bool
ProxyConnectionIsDone(
    IN PROXY_CONNECTION_HANDLE Connection
    );

/* For server */

typedef CT_STATUS (*PROXY_CONNECTION_OPEN)(
    OUT void** Context,
    IN PROXY_CONNECTION_HANDLE Connection,
    IN int Fd,
    IN const char* Address,
    IN const char* Endpoint,
    IN const char* Options,
    OUT size_t* SessKeyLen,
    OUT unsigned char** SessKey
    );

typedef void (*PROXY_CONNECTION_RUN)(
    IN void* Context
    );

typedef void (*PROXY_CONNECTION_CLOSE)(
    IN void* Context
    );

typedef struct _PROXY_CONNECTION_PROVIDER {
    const char* Name;
    PROXY_CONNECTION_OPEN Open;
    PROXY_CONNECTION_RUN Run;
    PROXY_CONNECTION_CLOSE Close;
} PROXY_CONNECTION_PROVIDER;

typedef bool (*PROXY_IS_DONE)(
    IN void* Context
    );

/* TODO -- remove username/password */
CT_STATUS
ProxyCreate(
    OUT PROXY_HANDLE* Proxy,
    IN void* Context,
    IN PROXY_IS_DONE IsDoneFunction,
    IN PROXY_CONNECTION_PROVIDER* Providers,
    IN bool UseCredCache,
    IN OPTIONAL const char* Username,
    IN OPTIONAL const char* Password
    );

void
ProxyDestroy(
    IN PROXY_HANDLE Proxy
    );

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
    );

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
    NPC_TOKEN_ID Token,
    OUT size_t* SessionKeyLen,
    OUT unsigned char** SessionKey
    );

void
ProxyConnectionRun(
    IN PROXY_CONNECTION_HANDLE Connection
    );

void
ProxyConnectionClose(
    IN PROXY_CONNECTION_HANDLE Connection
    );

CT_STATUS
ProxySetAuthInfo(
    IN PROXY_HANDLE Proxy,
    IN uid_t Uid,
    IN const char* Server,
    IN NPC_AUTH_FLAGS AuthFlags,
    IN const char* Username,
    IN const char* Password,
    IN const char* CredCache,
    NPC_TOKEN_ID Token
    );

CT_STATUS
ProxyClearAuthInfo(
    IN PROXY_HANDLE Proxy,
    IN uid_t Uid,
    IN OPTIONAL const char* Server,
    IN NPC_TOKEN_ID Token
    );

CT_STATUS
ProxyDestroyImpersonationToken(
    IN PROXY_HANDLE Proxy,
    IN uid_t Uid,
    IN NPC_TOKEN_ID Token
    );

#endif /* __PROXY_H__ */
