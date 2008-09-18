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

#ifndef __CT_SERVER_H__
#define __CT_SERVER_H__

#include <ctstatus.h>
#include <sys/types.h>
#include <cttypes.h>
#include <sys/stat.h>


struct _CT_SERVER_HANDLE_DATA;
typedef struct _CT_SERVER_HANDLE_DATA *CT_SERVER_HANDLE;

struct _CT_SERVER_CLIENT_HANDLE_DATA;
typedef struct _CT_SERVER_CLIENT_HANDLE_DATA *CT_SERVER_CLIENT_HANDLE;

typedef bool (*CT_SERVER_DISPATCH)(
    IN CT_SERVER_CLIENT_HANDLE Handle,
    IN void* Context,
    IN uint32_t Version,
    IN uint32_t Type,
    IN size_t Size
    );

#define CT_STATUS_CONNECTION_UNAVAIL _CT_STATUS_SYSTEM(0x127)
#define CT_STATUS_INVALID_MESSAGE    _CT_STATUS_SYSTEM(0x128)

int
CtServerClientGetFd(
    IN CT_SERVER_CLIENT_HANDLE ClientHandle
    );

uid_t
CtServerClientGetUid(
    IN CT_SERVER_CLIENT_HANDLE ClientHandle
    );

CT_SERVER_HANDLE
CtServerClientGetServerHandle(
    IN CT_SERVER_CLIENT_HANDLE ClientHandle
    );

bool
CtServerClientIsAuthenticated(
    IN CT_SERVER_CLIENT_HANDLE ClientHandle
    );

void
CtServerClientSetAuthenticated(
    IN CT_SERVER_CLIENT_HANDLE ClientHandle,
    bool isAuth
    );

CT_STATUS
CtServerCreateClientPath(
    OUT char** Path,
    IN const char* Prefix
    );

CT_STATUS
CtServerConnect(
    OUT int* Fd,
    IN const char* ServerPath,
    IN const char* ClientPath
    );

CT_STATUS
CtGenerateRandomNumber(
    OUT int *num
    );

CT_STATUS
CtSecSocketCreate(
    OUT int *Fd,
    OUT char **SocketPath,
    IN char *Directory
    );

CT_STATUS
CtSecSocketConnect(
    OUT int *Fd,
    IN char *socketPath
    );

CT_STATUS
CtSocketWaitForConnection(
    OUT int *connFd,
    IN int Fd
    );

CT_STATUS
CtSecSocketCleanup(
    IN int Fd,
    IN const char* socketPath
    );

CT_STATUS
CtServerConnectExistingSocket(
    IN int Fd,
    IN const char* ServerPath,
    IN const char* ClientPath
    );

CT_STATUS
CtServerReadMessageData(
    IN int Fd,
    IN uint32_t Size,
    OUT void** Data
    );

CT_STATUS
CtServerReadMessageHeader(
    IN int Fd,
    OUT uint32_t* Version,
    OUT uint32_t* Type,
    OUT uint32_t* Size
    );

CT_STATUS
CtServerReadMessage(
    IN int Fd,
    OUT uint32_t* Version,
    OUT uint32_t* Type,
    OUT uint32_t* Size,
    OUT void** Data
    );

CT_STATUS
CtServerWriteMessage(
    IN int Fd,
    IN uint32_t Version,
    IN uint32_t Type,
    IN size_t Size,
    IN const void* Data
    );

#define CT_SERVER_ROOT_UID 0
#define CT_SERVER_ROOT_GID 0
#define CT_SERVER_ALL_ACCESS_MODE (S_IRWXU|S_IRWXG|S_IRWXO)

CT_STATUS
CtServerCreate(
    OUT CT_SERVER_HANDLE* ServerHandle,
    IN const char* ServerPath,
    IN uid_t Uid,
    IN gid_t Gid,
    IN mode_t Mode,
    IN CT_SERVER_DISPATCH Dispatch,
    IN void* Context
    );

CT_STATUS
CtServerRun(
    IN CT_SERVER_HANDLE ServerHandle
    );

void
CtServerClose(
    IN CT_SERVER_HANDLE ServerHandle
    );

#if 0
/* TODO -- need to tweak interface to allow external proper termination if
   nothing accepted */

CT_STATUS
CtServerRunListener(
    IN const char* ServerPath,
    IN uid_t Uid,
    IN gid_t Gid,
    IN mode_t Mode,
    IN CT_SERVER_DISPATCH Dispatch,
    IN void* Context
    );
#endif

bool
CtServerIsDone(
    IN CT_SERVER_HANDLE ServerHandle
    );

CT_STATUS
CtServerTerminate(
    IN CT_SERVER_HANDLE ServerHandle
    );

void
CtServerDissociate(
    IN CT_SERVER_CLIENT_HANDLE ClientHandle
    );

// TODO - Move to ctsocket.{h,c}
CT_STATUS
CtSocketSetBlocking(
    IN int Fd
    );

CT_STATUS
CtSocketSetNonBlocking(
    IN int Fd
    );

CT_STATUS
CtSocketSetListening(
    IN int Fd
    );

#endif /* __CT_SERVER_*__ */
