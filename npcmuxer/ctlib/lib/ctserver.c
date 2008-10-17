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

#ifdef HAVE_CONFIG_H
#    include <config.h>
#endif

#include <ctserver.h>
#include <ctlock.h>
#include <unistd.h>
#include <ctgoto.h>
#include <ctmemory.h>
#include <ctstring.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <string.h>
#include <ctfileutils.h>
#include <sys/un.h>
#include <ctlogger.h>
#include <ctlist.h>

/* sys/select.h is busted on Darwin */
#if defined(HAVE_SYS_SELECT_H) && !defined(__APPLE__)
#include <sys/select.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
/* Work around broken sys/socket.h */
#define AF_LOCAL AF_UNIX
#endif

#ifndef PF_UNIX
#define PF_UNIX AF_UNIX
#endif

typedef struct _CT_SERVER_HANDLE_DATA {
    pthread_mutex_t* Lock;
    pthread_cond_t* Notify;
    char* ServerPath;
    size_t ClientCount;
    bool IsDone;
    int Fd;
    CT_SERVER_DISPATCH Dispatch;
    void* Context;
    CT_LIST_LINKS ActiveClients;
} CT_SERVER_HANDLE_DATA;

typedef struct _CT_SERVER_CLIENT_HANDLE_DATA {
    int Fd;
    uid_t Uid;
    gid_t Gid;
    CT_SERVER_HANDLE ServerHandle;
    bool IsDissociated;
    pthread_t Thread;
    CT_LIST_LINKS links;
} _CT_SERVER_CLIENT_HANDLE_DATA;

#define CT_SERVER_ACQUIRE(Handle) \
    CtLockAcquireMutex((Handle)->Lock)

#define CT_SERVER_RELEASE(Handle) \
    CtLockReleaseMutex((Handle)->Lock)

#define CT_SERVER_WAIT(Handle) \
    CtLockWaitCond((Handle)->Notify, (Handle)->Lock)

#define CT_SERVER_WAIT_TIMEOUT(Handle, Timeout)                         \
    CtLockWaitCondTimeout((Handle)->Notify, (Handle)->Lock, Timeout)

#define _CT_SERVER_MESSAGE_HEADER_MAGIC   0x8a3ff800
#define _CT_SERVER_MESSAGE_HEADER_VERSION 1

#define CT_SERVER_MESSAGE_HEADER_MAGIC \
    (_CT_SERVER_MESSAGE_HEADER_MAGIC | _CT_SERVER_MESSAGE_HEADER_VERSION)

#define CT_SERVER_MAX_LISTEN_QUEUE 20
#define CT_SERVER_MAX_SOCKET_STALE_TIME_SECONDS 30

typedef struct _CT_SERVER_MESSAGE_HEADER {
    uint32_t Magic;
    uint32_t Version;
    uint32_t Type;
    uint32_t Size;
} CT_SERVER_MESSAGE_HEADER;

static
CT_STATUS
CtpWriteData(
    int Fd,
    const void* Buffer,
    size_t Size
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    size_t bytesRemaining = Size;
    const void* data  = Buffer;

    while (bytesRemaining > 0)
    {
        ssize_t bytes = write(Fd, data, bytesRemaining);
        if (bytes < 0)
        {
            if (errno != EAGAIN && errno != EINTR)
            {
                status = CT_ERRNO_TO_STATUS(errno);
                GOTO_CLEANUP_ON_STATUS_EE(status, EE);
            }
        }
        else
        {
            bytesRemaining -= bytes;
            data = CT_PTR_ADD(data, bytes);
        }
    }

cleanup:
    return status;
}

static
CT_STATUS
CtpReadData(
    int Fd,
    void* Buffer,
    size_t BytesToRead,
    OUT size_t* BytesTransferred
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    ssize_t bytes = 0;
    size_t bytesRemaining = BytesToRead;
    int maxFd;
    fd_set masterReadFdSet;
    fd_set readFdSet;
    void* current = Buffer;
    struct timeval timeout;
    int selectResult;

    FD_ZERO(&masterReadFdSet);
    FD_SET(Fd, &masterReadFdSet);
    maxFd = Fd + 1;

    while (bytesRemaining > 0)
    {
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
        readFdSet = masterReadFdSet;
        selectResult = select(maxFd,
                              &readFdSet,
                               NULL /* write_fds */,
                               NULL /* except_fds */,
                               &timeout);
        if (selectResult < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            status = CT_ERRNO_TO_STATUS(errno);
            GOTO_CLEANUP_ON_STATUS_EE(status, EE);
        }
        else if (selectResult == 0)
        {
            /* timed out */
        }
        else
        {
            if (FD_ISSET(Fd, &readFdSet))
            {
                bytes = read(Fd, current, bytesRemaining);
                if (bytes < 0)
                {
                    if (errno != EAGAIN && errno != EINTR)
                    {
                        status = CT_ERRNO_TO_STATUS(errno);
                        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
                    }
                }
                else if (bytes == 0)
                {
                    status = CT_STATUS_CONNECTION_UNAVAIL;
                    GOTO_CLEANUP_EE(EE);
                }
                else
                {
                    current = CT_PTR_ADD(current, bytes);
                    bytesRemaining -= bytes;
                }
            }
            else
            {
                status = CT_STATUS_CONNECTION_UNAVAIL;
                GOTO_CLEANUP_EE(EE);
            }
        }
    }

cleanup:
    *BytesTransferred = (BytesToRead - bytesRemaining);

    if (status || EE)
    {
        CT_LOG_TRACE("status = 0x%08x (EE = %d)", status, EE);
    }

    return status;
}

#if defined(HAVE_GETPEEREID)

#if defined(HAVE_DECL_GETPEEREID) && !HAVE_DECL_GETPEEREID
int getpeereid(int fd, uid_t* uid, gid_t* gid);
#endif

CT_STATUS
CtpRecvCreds(
    int fd,
    uid_t* pUid,
    gid_t* pGid
    )
{
    CT_STATUS status = 0;
    if (getpeereid(fd, pUid, pGid) != 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP_ON_STATUS(status);
    }

 cleanup:

    return status;
}

CT_STATUS
CtpSendCreds(int fd)
{
    return 0;
}

#else

#ifndef CMSG_ALIGN
#    if defined(_CMSG_DATA_ALIGN)
#        define CMSG_ALIGN _CMSG_DATA_ALIGN
#    elif defined(_CMSG_ALIGN)
#        define CMSG_ALIGN _CMSG_ALIGN
#    endif
#endif

#ifndef CMSG_SPACE
#    define CMSG_SPACE(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + CMSG_ALIGN(len))
#endif

#ifndef CMSG_LEN
#    define CMSG_LEN(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#endif


CT_STATUS
CtpSendCreds(
    int fd
    )
{
    CT_STATUS status = 0;
    char payload = 0xff;
    int credFd[2] = {-1, -1};
    struct iovec payload_vec = {0};
    struct msghdr msg = {0};
    int ret = 0;

#ifdef HAVE_STRUCT_MSGHDR_MSG_CONTROL
    union
    {
        struct cmsghdr cm;
        char buf[CMSG_SPACE(sizeof(*credFd))];
    } buf_un;
    struct cmsghdr *cmsg = NULL;
#endif

    /* Create pipe to use as a credential fd */
    if (pipe(credFd))
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    /* Set up dummy payload */
    payload_vec.iov_base = &payload;
    payload_vec.iov_len = sizeof(payload);
    msg.msg_iov = &payload_vec;
    msg.msg_iovlen = 1;

#ifdef HAVE_STRUCT_MSGHDR_MSG_CONTROL
    /* Set up ancillary data */
    msg.msg_control = buf_un.buf;
    msg.msg_controllen = sizeof(buf_un.buf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(*credFd));

    memcpy(CMSG_DATA(cmsg), credFd, sizeof(*credFd));
#else
    msg.msg_accrights = (char*) credFd;
    msg.msg_accrightslen = sizeof(*credFd);
#endif

    /* Send message */
    do
    {
        ret = sendmsg(fd, &msg, 0);
    } while (ret < 0 && (errno == EAGAIN || errno == EINTR));

    if (ret < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP_ON_STATUS(status);
    }

cleanup:

    if (credFd[0] != -1)
        close(credFd[0]);
    if (credFd[1] != -1)
        close(credFd[1]);

    return status;
}

static int
CtpVerifyStat(
    struct stat* statbuf
    )
{
    /* fd must be a fifo (pipe) */
    if (!S_ISFIFO(statbuf->st_mode))
    {
        return 0;
    }
    /* if fd is only accessible by the owner, it is good */
    if ((statbuf->st_mode & (S_IRWXG | S_IRWXO)) == 0)
    {
        return 1;
    }
    /* if fd is only accessible by the owner and the group, and it
       has no inode (doesn't exist on the filesystem), it is good
       (this is the case on Darwin systems) */
    if ((statbuf->st_mode & S_IRWXO) == 0 && statbuf->st_ino == 0)
    {
        return 1;
    }
    /* in all other cases, the fd is bad */
    return 0;
}

CT_STATUS
CtpRecvCreds(
    int fd,
    uid_t* pUid,
    gid_t* pGid
    )
{
    CT_STATUS status = 0;
    int ret = 0;
    char payload = 0;
    int credFd = -1;
    struct iovec payload_vec = {0};
    struct msghdr msg = {0};
#ifdef HAVE_STRUCT_MSGHDR_MSG_CONTROL
    union
    {
        struct cmsghdr cm;
        char buf[CMSG_SPACE(sizeof(credFd))];
    } buf_un;
    struct cmsghdr *cmsg = NULL;
#endif
    struct stat statbuf = {0};

    /* Set up area to receive dummy payload */
    payload_vec.iov_base = &payload;
    payload_vec.iov_len = sizeof(payload);
    msg.msg_iov = &payload_vec;
    msg.msg_iovlen = 1;

#ifdef HAVE_STRUCT_MSGHDR_MSG_CONTROL
    /* Set up area to receive ancillary data */
    msg.msg_control = buf_un.buf;
    msg.msg_controllen = sizeof(buf_un.buf);
#else
    msg.msg_accrights = (char*) &credFd;
    msg.msg_accrightslen = sizeof(credFd);
#endif

    do
    {
        ret = recvmsg(fd, &msg, 0);
    } while (ret < 0 && (errno == EAGAIN || errno == EINTR));

    if (ret < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    /* Extract credential fd */
    
#ifdef HAVE_STRUCT_MSGHDR_MSG_CONTROL
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg))
    {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
        {
            memcpy(&credFd, CMSG_DATA(cmsg), sizeof(credFd));
            break;
        }
    }
#endif

    /* Fail if we couldn't extract a valid fd from message */
    if (credFd == -1)
    {
        status = CT_ERRNO_TO_STATUS(EBADF);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    /* Stat the fd to find the uid/gid of the peer socket */
    if (fstat(credFd, &statbuf) != 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP_ON_STATUS(status);
    }
    
    /* Check that the fd sent must have been created by the peer */
    if (!CtpVerifyStat(&statbuf))
    {
        status = CT_ERRNO_TO_STATUS(EPERM);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    *pUid = statbuf.st_uid;
    *pGid = statbuf.st_gid;

cleanup:

    if (credFd != -1)
    {
        close(credFd);
    }

    return status;
}

#endif

CT_STATUS
CtServerReadMessageData(
    IN int Fd,
    IN uint32_t Size,
    OUT void** Data
    )
{
    CT_STATUS status;
    int EE = 0;
    void* buffer = NULL;
    size_t bytesRead;

    if (Size <= 0)
    {
        status = CT_STATUS_SUCCESS;
        GOTO_CLEANUP_EE(EE);
    }

    status = CtAllocateMemory((void**)&buffer, Size);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtpReadData(Fd, buffer, Size, &bytesRead);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (Size != bytesRead)
    {
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    if (status)
    {
        CT_SAFE_FREE(buffer);
    }

    if (status || EE)
    {
        CT_LOG_TRACE("status = 0x%08x (EE = %d)", status, EE);
    }

    *Data = buffer;
    return status;
}

CT_STATUS
CtServerReadMessageHeader(
    IN int Fd,
    OUT uint32_t* Version,
    OUT uint32_t* Type,
    OUT uint32_t* Size
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    CT_SERVER_MESSAGE_HEADER header;
    size_t bytesRead;

    status = CtpReadData(Fd, &header, sizeof(header), &bytesRead);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (bytesRead != sizeof(header))
    {
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP_EE(EE);
    }

    if (CT_SERVER_MESSAGE_HEADER_MAGIC != header.Magic)
    {
        status = CT_STATUS_INVALID_MESSAGE;
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (!CT_STATUS_IS_OK(status))
    {
        *Version = 0;
        *Type = 0;
        *Size = 0;
    }
    else
    {
        *Version = header.Version;
        *Type = header.Type;
        *Size = header.Size;
    }

    if (status || EE)
    {
        CT_LOG_TRACE("status = 0x%08x (EE = %d)", status, EE);
    }

    return status;
}

CT_STATUS
CtServerReadMessage(
    IN int Fd,
    OUT uint32_t* Version,
    OUT uint32_t* Type,
    OUT uint32_t* Size,
    OUT void** Data
    )
{
    CT_STATUS status;
    uint32_t size;
    void* buffer = NULL;

    status = CtServerReadMessageHeader(Fd, Version, Type, &size);
    GOTO_CLEANUP_ON_STATUS(status);

    status = CtServerReadMessageData(Fd, size, &buffer);
cleanup:
    *Size = size;
    *Data = buffer;
    return status;
}

CT_STATUS
CtServerWriteMessage(
    IN int Fd,
    IN uint32_t Version,
    IN uint32_t Type,
    IN size_t Size,
    IN const void* Data
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    CT_SERVER_MESSAGE_HEADER header = { CT_SERVER_MESSAGE_HEADER_MAGIC, Version, Type, Size };

    status = CtpWriteData(Fd, &header, sizeof(header));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtpWriteData(Fd, Data, Size);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    return status;
}

CT_STATUS
CtSocketSetBlocking(
    IN int Fd
    )
{
    CT_STATUS status;
    int EE = 0;

    int flags = fcntl(Fd, F_GETFL, 0);
    if (flags < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP_EE(EE);
    }

    flags &= ~O_NONBLOCK;

    flags = fcntl(Fd, F_SETFL, flags);
    if (flags < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP_EE(EE);
    }

    status = CT_STATUS_SUCCESS;

cleanup:
    return status;
}

CT_STATUS
CtSocketSetNonBlocking(
    IN int Fd
    )
{
    CT_STATUS status;
    int EE = 0;

    int flags = fcntl(Fd, F_GETFL, 0);
    if (flags < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP_EE(EE);
    }

    flags |= O_NONBLOCK;

    flags = fcntl(Fd, F_SETFL, flags);
    if (flags < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP_EE(EE);
    }

    status = CT_STATUS_SUCCESS;

cleanup:
    return status;
}


CT_STATUS
CtSocketSetListening(
    IN int Fd
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    int ret;

    ret = listen(Fd, 0);
    if (ret < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
	GOTO_CLEANUP_EE(EE);
    }

cleanup:
    return status;
}


static
void
CtpServerClientThreadCleanup(void* Context)
{
    CT_SERVER_CLIENT_HANDLE client = (CT_SERVER_CLIENT_HANDLE) Context;
    int count;

    if (!client->IsDissociated)
    {
        CT_SAFE_CLOSE_FD(client->Fd);
    }

    CT_SERVER_ACQUIRE(client->ServerHandle);
    count = client->ServerHandle->ClientCount;
    client->ServerHandle->ClientCount--;
    CtListRemove(&client->links);
    if (count <= 1)
    {
        CtLockSignalCond(client->ServerHandle->Notify);
    }
    CT_SERVER_RELEASE(client->ServerHandle);
    CT_SAFE_FREE(client);   
}

static
void*
CtpServerClientThread(
    void* Context
    )
{
    CT_STATUS status;
    bool isDone = false;
    CT_SERVER_CLIENT_HANDLE client = (CT_SERVER_CLIENT_HANDLE) Context;

    pthread_cleanup_push(CtpServerClientThreadCleanup, Context);

    CT_SERVER_ACQUIRE(client->ServerHandle);
    client->ServerHandle->ClientCount++;
    CT_SERVER_RELEASE(client->ServerHandle);
    
    status = CtpRecvCreds(client->Fd, &client->Uid, &client->Gid);
    if (status) pthread_exit(NULL);

    CT_LOG_VERBOSE("Received creds uid=%u gid=%u\n", (unsigned int) client->Gid, (unsigned int) client->Gid);

    status = CtSocketSetNonBlocking(client->Fd);
    if (status) pthread_exit(NULL);

    while (!isDone && !client->IsDissociated)
    {
        uint32_t version;
        uint32_t type;
        uint32_t size;

        status = CtServerReadMessageHeader(client->Fd,
                                           &version,
                                           &type,
                                           &size);

        if (status) pthread_exit(NULL);

        isDone = client->ServerHandle->Dispatch(client,
                                                client->ServerHandle->Context,
                                                version,
                                                type,
                                                size);
        if (!isDone)
        {
            isDone = CtServerIsDone(client->ServerHandle);
        }
    }

    pthread_cleanup_pop(true);

    return NULL;
}


static
CT_STATUS
CtpServerInitUnixAddress(
    OUT struct sockaddr_un* Address,
    IN const char* Path
    )
{
    CT_STATUS status;
    memset(Address, 0, sizeof(*Address));
    Address->sun_family = AF_UNIX;
    status = CtCopyString(Address->sun_path, sizeof(Address->sun_path), Path);
    return status;
}

static
CT_STATUS
CtpServerTerminateAccept(
    IN const char* ServerPath
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int fd = -1;

    status = CtServerConnect(&fd, ServerPath);

    CT_SAFE_CLOSE_FD(fd);

    return status;
}

CT_STATUS
CtSocketWaitForConnection(
    OUT int *connFd,
    IN int Fd)
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    int authFd = -1;
    fd_set readFdSet;
    struct timeval timeout;
    int ret;
    socklen_t serverAddressLength = 0;
    struct sockaddr_un serverAddress = {0};

    FD_ZERO(&readFdSet);
    FD_SET(Fd, &readFdSet);

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    ret = select(Fd + 1, &readFdSet, NULL, NULL, &timeout);
    if (ret < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }
    else if (ret == 0)
    {
        status = CT_STATUS_TIMEOUT;
    }
    else
    {
        if (FD_ISSET(Fd, &readFdSet))
        {
            authFd = accept(Fd,
                            (struct sockaddr*)&serverAddress,
                            &serverAddressLength);
            if (authFd < 0)
            {
                status = CT_ERRNO_TO_STATUS(errno);
                GOTO_CLEANUP_EE(EE);
            }
            
            *connFd = authFd;
        }
        else
        {
            /* TODO: perhaps this should be connection unavailable ? */
            status = CT_STATUS_ACCESS_DENIED;
        }
    }
    
cleanup:
    return status;
}

CT_STATUS
CtServerTerminate(
    IN CT_SERVER_HANDLE ServerHandle
    )
{
    CT_STATUS status;

    CT_SERVER_ACQUIRE(ServerHandle);
    ServerHandle->IsDone = true;
    status = CtpServerTerminateAccept(ServerHandle->ServerPath);
    CT_SERVER_RELEASE(ServerHandle);

    return status;
}

bool
CtServerIsDone(
    IN CT_SERVER_HANDLE ServerHandle
    )
{
    bool isDone;

    CT_SERVER_ACQUIRE(ServerHandle);
    isDone = ServerHandle->IsDone;
    CT_SERVER_RELEASE(ServerHandle);

    return isDone;
}

static
void
CtpServerEnsureNoClients(
    IN CT_SERVER_HANDLE ServerHandle,
    unsigned long timeout /* MS */
    )
{
    unsigned long remaining = timeout;
    if (ServerHandle)
    {
        CT_SERVER_ACQUIRE(ServerHandle);
            
        while (ServerHandle->ClientCount > 0)
        {
            if (timeout)
            {
                remaining = CT_SERVER_WAIT_TIMEOUT(ServerHandle, remaining);
                if (remaining == 0)
                    break;
            }
            else
                CT_SERVER_WAIT(ServerHandle);
        }
        CT_SERVER_RELEASE(ServerHandle);
    }
}

CT_STATUS
CtServerCreate(
    OUT CT_SERVER_HANDLE* ServerHandle,
    IN const char* ServerPath,
    IN uid_t Uid,
    IN gid_t Gid,
    IN mode_t Mode,
    IN CT_SERVER_DISPATCH Dispatch,
    IN void* Context
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    struct sockaddr_un serverAddress;
    int serverFd = -1;
    int retval;
    CT_SERVER_HANDLE serverHandle = NULL;

    /* Ignore error */
    CtFileUnlink(ServerPath);

    serverFd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (serverFd < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP_EE(EE);
    }

    status = CtpServerInitUnixAddress(&serverAddress, ServerPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    retval = bind(serverFd,
                  (struct sockaddr*)&serverAddress,
                  sizeof(serverAddress));
    if (retval < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP_EE(EE);
    }

    /*
     * Allow anyone to write to the socket.
     * We check the uids against the messages.
     */
    status = CtFileSetOwnerMode(ServerPath, Uid, Gid, Mode);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtAllocateMemory((void**)&serverHandle, sizeof(*serverHandle));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtLockCreateMutex(&serverHandle->Lock);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtLockCreateCond(&serverHandle->Notify);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = CtAllocateString(&serverHandle->ServerPath, ServerPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    CtListInit(&serverHandle->ActiveClients);

    serverHandle->Dispatch = Dispatch;
    serverHandle->Context = Context;
    serverHandle->Fd = serverFd;
    serverFd = -1;

    status = CT_STATUS_SUCCESS;

cleanup:
    CT_SAFE_CLOSE_FD(serverFd);

    if (status)
    {
        CtServerClose(serverHandle);
        serverHandle = NULL;
    }

    *ServerHandle = serverHandle;

    if (status || EE)
    {
        CT_LOG_ERROR("0x%08x (EE = %d)", status, EE);
    }

    return status;
}

CT_STATUS
CtServerRun(
    IN CT_SERVER_HANDLE ServerHandle
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int EE = 0;
    int retval;
    int clientFd = -1;
    CT_SERVER_CLIENT_HANDLE clientHandle = NULL;
    CT_LIST_LINKS *client;

    retval = listen(ServerHandle->Fd, CT_SERVER_MAX_LISTEN_QUEUE);
    if (retval < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP_EE(EE);
    }

    for (;;)
    {
        struct sockaddr_un clientAddress;
        socklen_t clientAddressLength;
        pthread_t threadId;
        int error;

        if (CtServerIsDone(ServerHandle))
        {
            status = CT_STATUS_SUCCESS;
            break;
        }

        CT_SAFE_CLOSE_FD(clientFd);
        CT_SAFE_FREE(clientHandle);

        clientAddressLength = sizeof(clientAddress);
        memset(&clientAddress, 0, sizeof(clientAddress));

        clientFd = accept(ServerHandle->Fd,
                          (struct sockaddr*)&clientAddress,
                          &clientAddressLength);
        if (clientFd < 0)
        {
            if (errno == EPROTO || errno == ECONNABORTED || errno == EINTR)
            {
                continue;
            }
            else
            {
                status = CT_ERRNO_TO_STATUS(errno);
                GOTO_CLEANUP_EE(EE);
            }
        }

        if (CtServerIsDone(ServerHandle))
        {
            status = CT_STATUS_SUCCESS;
            break;
        }

        status = CtAllocateMemory((void**)&clientHandle, sizeof(*clientHandle));
        if (status)
        {
            CT_LOG_ERROR("Failed to allocate memory for client context");
            continue;
        }

        clientHandle->Fd = clientFd;
        clientHandle->ServerHandle = ServerHandle;

        CT_SERVER_ACQUIRE(ServerHandle);
        CtListInsertAfter(&ServerHandle->ActiveClients, &clientHandle->links);
        CT_SERVER_RELEASE(ServerHandle);

        error = pthread_create(&threadId,
                               NULL,
                               CtpServerClientThread,
                               clientHandle);
        if (error)
        {
            CT_LOG_ERROR("Failed to create client thread");
            continue;
        }

        clientHandle->Thread = threadId;

        clientFd = -1;
        clientHandle = NULL;
        pthread_detach(threadId);
    }

cleanup:
    CtpServerEnsureNoClients(ServerHandle, 1000 * 5);

    CT_SAFE_FREE(clientHandle);
    CT_SAFE_CLOSE_FD(clientFd);

    CT_SERVER_ACQUIRE(ServerHandle);
    for (client = ServerHandle->ActiveClients.Next; client != &ServerHandle->ActiveClients; client = client->Next)
    {
        CT_SERVER_CLIENT_HANDLE handle = CT_FIELD_RECORD(client, _CT_SERVER_CLIENT_HANDLE_DATA, links);

        pthread_cancel(handle->Thread);
    }
    CT_SERVER_RELEASE(ServerHandle);

    CtpServerEnsureNoClients(ServerHandle, 0);

    if (ServerHandle->Fd)
    {
        shutdown(ServerHandle->Fd, SHUT_RDWR);
        CT_SAFE_CLOSE_FD(ServerHandle->Fd);
    }

    if (status || EE)
    {
        CT_LOG_ERROR("0x%08x (EE = %d)", status, EE);
    }

    return status;
}

void
CtServerClose(
    IN CT_SERVER_HANDLE ServerHandle
    )
{
    if (ServerHandle)
    {
        CtpServerEnsureNoClients(ServerHandle, 0);
        CT_SAFE_FREE(ServerHandle->ServerPath);
        CT_SAFE_CLOSE_FD(ServerHandle->Fd);
        CT_SAFE_FREE_MUTEX(ServerHandle->Lock);
        CT_SAFE_FREE_COND(ServerHandle->Notify);
    }
}

CT_STATUS
CtServerConnectExistingSocket(
    IN int Fd,
    IN const char* ServerPath
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    struct sockaddr_un serverAddress;
    int retval;

    status = CtpServerInitUnixAddress(&serverAddress, ServerPath);
    GOTO_CLEANUP_ON_STATUS(status);

    retval = connect(Fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (retval < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP();
    }

    status = CtpSendCreds(Fd);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    return status;
}

CT_STATUS
CtServerConnect(
    OUT int* Fd,
    IN const char* ServerPath
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    int fd = -1;

    fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP();
    }

    status = CtServerConnectExistingSocket(fd, ServerPath);

cleanup:
    if (status)
    {
        CT_SAFE_CLOSE_FD(fd);
    }
    *Fd = fd;
    return status;
}

void
CtServerDissociate(
    IN CT_SERVER_CLIENT_HANDLE ClientHandle
    )
{
    ClientHandle->IsDissociated = true;
}

int
CtServerClientGetFd(
    IN CT_SERVER_CLIENT_HANDLE ClientHandle
    )
{
    return ClientHandle->Fd;
}

uid_t
CtServerClientGetUid(
    IN CT_SERVER_CLIENT_HANDLE ClientHandle
    )
{
    return ClientHandle->Uid;
}

CT_SERVER_HANDLE
CtServerClientGetServerHandle(
    IN CT_SERVER_CLIENT_HANDLE ClientHandle
    )
{
    return ClientHandle->ServerHandle;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
