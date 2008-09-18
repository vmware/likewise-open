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

#include "proxy_tcp.h"

#include <sys/socket.h>
#include <netdb.h>
#include <ctmemory.h>
#include <ctgoto.h>
#include <unistd.h>
#include <string.h>
#include <ctlogger.h>
#include <ctserver.h>

/* sys/select.h is busted on Darwin */
#if defined(HAVE_SYS_SELECT_H) && !defined(__APPLE__)
#include <sys/select.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
/* Work around broken sys/socket.h */
#define AF_LOCAL AF_UNIX
#endif

#ifndef PF_UNIX
#    define PF_UNIX AF_UNIX
#endif

#ifndef PF_INET
#    define PF_INET AF_INET
#endif


typedef struct _TCP_CONTEXT {
    int ClientFd;
    int ServerFd;
} TCP_CONTEXT;

CT_STATUS
TcpProxyConnect(
    OUT void** Context,
    IN PROXY_CONNECTION_HANDLE Connection,
    IN int Fd,
    IN const char* Address,
    IN const char* Endpoint,
    IN const char* Options,
    OUT size_t* SessKeyLen,
    OUT unsigned char** SessKey
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int fd = -1;
    int retval;
    int gaiError;
    struct addrinfo* info = NULL;
    struct addrinfo hints = { 0 };
    TCP_CONTEXT* context = NULL;

    status = CtAllocateMemory((void**)&context, sizeof(*context));
    GOTO_CLEANUP_ON_STATUS(status);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
#ifdef IPPROTO_TCP
    hints.ai_protocol = IPPROTO_TCP;
#endif

    gaiError = getaddrinfo(Address, Endpoint, NULL, &info);
    if (gaiError)
    {
        // TODO--Add error code translation
        status = CT_STATUS_ERRNO_UNEXPECTED;
        GOTO_CLEANUP();
    }

#ifdef IPPROTO_TCP
    fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
    fd = socket(PF_INET, SOCK_STREAM, 0);
#endif
    if (fd < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP();
    }

    retval = connect(fd, info->ai_addr, info->ai_addrlen);
    if (retval < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP();
    }

    status = CtSocketSetNonBlocking(fd);
    GOTO_CLEANUP_ON_STATUS(status);

    context->ClientFd = Fd;
    context->ServerFd = fd;

    status = CT_STATUS_SUCCESS;

cleanup:
    if (info)
    {
        freeaddrinfo(info);
    }
    if (status)
    {
        CT_SAFE_CLOSE_FD(fd);
        CT_SAFE_FREE(context);
    }

    *Context = context;

    return status;
}

typedef struct _PROXY_BUFFER {
    bool IsDone;
    size_t Offset;
    size_t Length;
    char Data[8 * 1024];
} PROXY_BUFFER;

static
CT_STATUS
TcpProxyWrite(
    IN fd_set* FdSet,
    IN int Fd,
    IN OUT PROXY_BUFFER* Buffer,
    IN const char* Label
    )
{
    CT_STATUS status;
    ssize_t bytes;

    if (FD_ISSET(Fd, FdSet))
    {
        for (;;)
        {
            bytes = write(Fd, CT_PTR_ADD(Buffer->Data, Buffer->Offset), Buffer->Length);
            if (bytes < 0)
            {
                int error = errno;
                if (error == EINTR)
                {
                    continue;
                }
                CT_LOG_ERROR("write of %s failed with %d (%s)", Label, error, strerror(error));
                status = CT_ERRNO_TO_STATUS(error);
                GOTO_CLEANUP();
            }
            else if (bytes == 0)
            {
                /* TODO -- What does this mean? */
                CT_LOG_ERROR("write 0 bytes to %s", Label);
            }
            else if (bytes == Buffer->Length)
            {
                Buffer->Length = 0;
                Buffer->Offset = 0;
            }
            else
            {
                CT_ASSERT(bytes < Buffer->Length);
                Buffer->Offset += bytes;
                Buffer->Length -= bytes;
            }
            break;
        }
    }

    status = CT_STATUS_SUCCESS;

cleanup:
    return status;
}

static
CT_STATUS
TcpProxyRead(
    IN fd_set* FdSet,
    IN int Fd,
    IN OUT PROXY_BUFFER* Buffer,
    IN const char* Label
    )
{
    CT_STATUS status;
    ssize_t bytes;

    if (FD_ISSET(Fd, FdSet))
    {
        for (;;)
        {
            bytes = read(Fd, Buffer->Data, sizeof(Buffer->Data));
            if (bytes < 0)
            {
                int error = errno;
                if (error == EINTR)
                {
                    continue;
                }
                CT_LOG_ERROR("read of %s failed with %d (%s)", Label, error, strerror(error));
                status = CT_ERRNO_TO_STATUS(error);
                GOTO_CLEANUP();
            }
            else if (bytes == 0)
            {
                /* EOF */
                CT_LOG_DEBUG("read 0 bytes from %s", Label);
                Buffer->IsDone = true;
                Buffer->Length = 0;
                Buffer->Offset = 0;
            }
            else
            {
                Buffer->Length = bytes;
                Buffer->Offset = 0;
            }
            break;
        }
    }

    status = CT_STATUS_SUCCESS;

cleanup:
    return status;
}

void
TcpSetFdSet(
    IN OUT int* MaxFd,
    IN OUT fd_set* ReadFdSet,
    IN OUT fd_set* WriteFdSet,
    IN PROXY_BUFFER* Buffer,
    IN int ReadFd,
    IN int WriteFd
    )
{
    fd_set* fdSet = NULL;
    int* fd = NULL;

    if (Buffer->Length != 0)
    {
        fd = &WriteFd;
        fdSet = WriteFdSet;
    }
    else if (!Buffer->IsDone)
    {
        fd = &ReadFd;
        fdSet = ReadFdSet;
    }

    if (fd)
    {
        FD_SET(*fd, fdSet);
        *MaxFd = CT_MAX(*MaxFd, *fd);
    }
}

inline
void
ProxyBufferInit(
    OUT PROXY_BUFFER* Buffer
    )
{
    Buffer->IsDone = false;
    Buffer->Length = 0;
}

inline
bool
ProxyBufferIsDone(
    OUT PROXY_BUFFER* Buffer
    )
{
    return (Buffer->IsDone && (0 == Buffer->Length)) ? true : false;
}

void
TcpProxyRun(
    IN void* Context
    )
{
    TCP_CONTEXT* context = (TCP_CONTEXT*) Context;
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    fd_set readFds;
    fd_set writeFds;
    int maxFd;
    int retval;
    PROXY_BUFFER clientToServerBuffer;
    PROXY_BUFFER serverToClientBuffer;

    ProxyBufferInit(&clientToServerBuffer);
    ProxyBufferInit(&serverToClientBuffer);

    while (!ProxyBufferIsDone(&clientToServerBuffer) ||
           !ProxyBufferIsDone(&serverToClientBuffer))
    {
        FD_ZERO(&readFds);
        FD_ZERO(&writeFds);
        maxFd = -1;

        TcpSetFdSet(&maxFd,
                    &readFds,
                    &writeFds,
                    &clientToServerBuffer,
                    context->ClientFd,
                    context->ServerFd);

        TcpSetFdSet(&maxFd,
                    &readFds,
                    &writeFds,
                    &serverToClientBuffer,
                    context->ServerFd,
                    context->ClientFd);

        retval = select(maxFd + 1, &readFds, &writeFds, NULL, NULL);
        if (retval < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            CT_LOG_ERROR("select failed with %d (%s)", errno, strerror(errno));
            GOTO_CLEANUP();
        }
        else if (retval == 0)
        {
            /* timed out */
            continue;
        }

        status = TcpProxyRead(&readFds, context->ClientFd, &clientToServerBuffer, "client");
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        status = TcpProxyRead(&readFds, context->ServerFd, &serverToClientBuffer, "server");
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        status = TcpProxyWrite(&writeFds, context->ClientFd, &clientToServerBuffer, "client");
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        status = TcpProxyWrite(&writeFds, context->ServerFd, &serverToClientBuffer, "server");
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    if (status || EE)
    {
        CT_LOG_ERROR("%s exiting with status = 0x%08x, EE = %d", __FUNCTION__, status, EE);
    }
    return;
}

void
TcpProxyClose(
    IN void* Context
    )
{
    TCP_CONTEXT* context = (TCP_CONTEXT*) Context;

    if (context)
    {
        CT_SAFE_CLOSE_FD(context->ServerFd);
        CtFreeMemory(context);
    }
}
