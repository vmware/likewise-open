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

#ifndef __NPC_H__
#define __NPC_H__

#include <npctypes.h>

void
NpcSetDebugLevel(
    /* IN */ uint32_t Level
    );

int
NpcConnectCheckCreds(
    /* IN */ const char* Protocol,
    /* IN */ const char* Address,
    /* IN */ const char* Endpoint,
    /* IN */ const char* Options,
    /* IN */ NPC_AUTH_FLAGS AuthFlags,
    /* IN */ const char* Username,
    /* IN */ const char* Password
    );

int
NpcConnect(
    /* OUT */ int* Fd,
    /* IN */ const char* Protocol,
    /* IN */ const char* Address,
    /* IN */ const char* Endpoint,
    /* IN */ const char* Options,
    /* OUT */ size_t SessKeySize,
    /* OUT */ size_t* SessKeyLen,
    /* OUT */ unsigned char* SessKey
    );

int
NpcConnectExistingSocket(
    /* IN */ int Fd,
    /* IN */ const char* Protocol,
    /* IN */ const char* Address,
    /* IN */ const char* Endpoint,
    /* IN */ const char* Options,
    /* OUT */ size_t SessKeySize,
    /* OUT */ size_t* SessKeyLen,
    /* OUT */ unsigned char* SessKey
    );

int
NpcSetAuthInfo(
    /* IN */ const char* Server,
    /* IN */ NPC_AUTH_FLAGS AuthFlags,
    /* IN */ const char* Username,
    /* IN */ const char* Password
    );

int
NpcClearAuthInfo(
    /* IN OPTIONAL */ const char* Server
    );

#define NPC_TOKEN_HANDLE_NOT_IMPERSONATING	NULL

struct _NPC_TOKEN_HANDLE_DATA;
typedef struct _NPC_TOKEN_HANDLE_DATA NPC_TOKEN_HANDLE_DATA, *NPC_TOKEN_HANDLE;

int
NpcCreateImpersonationToken(
    /* OUT */ NPC_TOKEN_HANDLE* Handle
    );

int
NpcCloseImpersonationToken(
    /* IN */ NPC_TOKEN_HANDLE Handle
    );

int
NpcInit(
    );

void
NpcCleanup(
    );

int
NpcGetImpersonationTokenId(
    /* IN */ NPC_TOKEN_HANDLE Handle,
    /* OUT */ NPC_TOKEN_ID* Id
    );

NPC_TOKEN_HANDLE
NpcGetThreadImpersonationToken(
    );

int
NpcImpersonate(
    /* IN */ NPC_TOKEN_HANDLE Handle
    );

int
NpcRevertToSelf(
    );

#if 0
void
NpcFree(
    /* IN */ void* Memory
    );

int
NpcEnumAuthInfoServers(
    /* OUT */ uint32_t* Count,
    /* OUT */ char*** Servers
    );

/* TODO-Need to think about how we key into sessions... */

typedef struct _NPC_AUTH_INFO {
    char* Server;
    NPC_AUTH_FLAGS Flags;
    char* Username;
    /* Never return password */
} NPC_AUTH_INFO;

int
NpcEnumAuthInfo(
    /* IN OPTIONAL */ uid_t* Uid,
    /* OUT */ uint32_t* Count,
    /* OUT */ NPC_AUTH_INFO** Info
    );

typedef struct _NPC_CONNECTION_INFO {
    char* Server;
    NPC_AUTH_FLAGS Flags;
    char* Username;
    /* Never return password */
} NPC_AUTH_INFO;

int
NpcEnumConnections(
    /* IN OPTIONAL */ uid_t* Uid,
    /* OUT */ uint32_t* Count,
    /* OUT */ NPC_CONNECTION_INFO* Info
    );
#endif

int
NpcGetCredCacheName(
    /* OUT */ char** CredCache
    );

int
NpcSetCredCacheName(
    /* IN */ const char* CredCache
    );

void
NpcFreeCredCacheName(
    /* IN */ char* CredCache
    );

#endif /* __NPC_H__ */
