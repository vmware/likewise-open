/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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

/*
 * Module Name:
 *
 *        connection-wire.c
 *
 * Abstract:
 *
 *        Connection API
 *        Wire protocol and network logic
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "connection-private.h"
#include "assoc-private.h"
#include "util-private.h"
#include "convert.h"
#include "xnet-private.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <config.h>

#ifndef CMSG_ALIGN
#    if defined(_CMSG_DATA_ALIGN)
#        define CMSG_ALIGN _CMSG_DATA_ALIGN
#    elif defined(_CMSG_ALIGN)
#        define CMSG_ALIGN _CMSG_ALIGN
#    elif defined(ALIGN)
#        define CMSG_ALIGN ALIGN
#    endif
#endif

#ifndef CMSG_SPACE
#    define CMSG_SPACE(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + CMSG_ALIGN(len))
#endif

#ifndef CMSG_LEN
#    define CMSG_LEN(len) (CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
#endif

static void
lwmsg_connection_packet_ntoh(ConnectionPacket* packet)
{
    packet->length = lwmsg_convert_uint32(packet->length, LWMSG_BIG_ENDIAN, LWMSG_NATIVE_ENDIAN);

    switch (packet->type)
    {
    case CONNECTION_PACKET_MESSAGE:
    case CONNECTION_PACKET_REPLY:
        packet->contents.msg.type = lwmsg_convert_uint16(packet->contents.msg.type, LWMSG_BIG_ENDIAN, LWMSG_NATIVE_ENDIAN);
        break;
    case CONNECTION_PACKET_GREETING:
        packet->contents.greeting.packet_size = lwmsg_convert_uint32(
            packet->contents.greeting.packet_size,
            LWMSG_BIG_ENDIAN,
            LWMSG_NATIVE_ENDIAN);
        break;
    }
}

LWMsgBool
lwmsg_connection_packet_is_urgent(
    ConnectionPacket* packet
    )
{
    switch (packet->type)
    {
    case CONNECTION_PACKET_SHUTDOWN:
        return LWMSG_TRUE;
    default:
        return LWMSG_FALSE;
    }
}

static LWMsgBool
lwmsg_connection_urgent_packet_pending(
    ConnectionBuffer* buffer
    )
{
    return 
        buffer->base_length >= CONNECTION_PACKET_SIZE(ConnectionPacketBase) &&
        lwmsg_connection_packet_is_urgent((ConnectionPacket*) buffer->base);
}

static LWMsgStatus
lwmsg_connection_recvmsg(int fd, struct msghdr* msghdr, int flags, size_t* out_received)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ssize_t received;

    received = recvmsg(fd, msghdr, flags);

    if (received == 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_EOF);
    }
    else if (received < 0)
    {
        switch (errno)
        {
        case EAGAIN:
        case EINTR:
            status = LWMSG_STATUS_AGAIN;
            break;
        case EPIPE:
        case ECONNRESET:
            status = LWMSG_STATUS_EOF;
            break;
        default:
            status = LWMSG_STATUS_ERROR;
            break;
        }
        BAIL_ON_ERROR(status);
    }

    *out_received = (size_t) received;

error:

    return status;
}

static
LWMsgStatus
lwmsg_connection_recvbuffer(LWMsgAssoc* assoc)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionBuffer* buffer = &priv->recvbuffer;
    struct msghdr msghdr = {0};
    struct iovec iovec = {0};
    union
    {
        struct cmsghdr cm;
        char buf[CMSG_SPACE(sizeof(int) * MAX_FD_PAYLOAD)];
    } buf_un;
    struct cmsghdr *cmsg = NULL;
    size_t received = 0;

    iovec.iov_base = buffer->base + buffer->base_length;
    iovec.iov_len = buffer->base_capacity - buffer->base_length;
    
    msghdr.msg_iov = &iovec;
    msghdr.msg_iovlen = 1;

    msghdr.msg_control = buf_un.buf;
    msghdr.msg_controllen = sizeof(buf_un.buf);

    BAIL_ON_ERROR(status = lwmsg_connection_recvmsg(priv->fd, &msghdr, 0, &received));

    buffer->base_length += received;

    if (msghdr.msg_controllen > 0 &&
        /* Work around bizarre behavior of X/Open recvmsg on HP-UX 11.11 */
        msghdr.msg_controllen <= sizeof(buf_un.buf))
    {
        for (cmsg = CMSG_FIRSTHDR(&msghdr); cmsg; cmsg = CMSG_NXTHDR(&msghdr, cmsg))
        {
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
            {
                size_t numfds = (cmsg->cmsg_len - CMSG_ALIGN(sizeof(*cmsg))) / sizeof(int);
                BAIL_ON_ERROR(status = lwmsg_connection_buffer_ensure_fd_capacity(buffer, numfds));
                memcpy(buffer->fd + buffer->fd_length, CMSG_DATA(cmsg), numfds * sizeof(int));
                buffer->fd_length += numfds;
            }
        }
    }

error:

    return status;
}

LWMsgStatus
lwmsg_connection_discard_recv_packet(
    LWMsgAssoc* assoc,
    ConnectionPacket* packet
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionBuffer* buffer = &priv->recvbuffer;

    if (buffer->base_length >= CONNECTION_PACKET_SIZE(ConnectionPacketBase))
    {
        size_t used = packet->length;
        
        memmove(buffer->base, buffer->base + used, buffer->base_length - used);
        buffer->base_length -= used;
    }

    return status;
}

LWMsgStatus
lwmsg_connection_discard_send_packet(
    LWMsgAssoc* assoc,
    ConnectionPacket* packet
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionBuffer* buffer = &priv->sendbuffer;

    if (buffer->base_length >= CONNECTION_PACKET_SIZE(ConnectionPacketBase))
    {
        buffer->cursor = buffer->base;
        buffer->base_length = 0;
    }
    
    return status;
}

static LWMsgStatus
lwmsg_connection_sendmsg(int fd, struct msghdr* msghdr, int flags, size_t* out_sent)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ssize_t sent;

    sent = sendmsg(fd, msghdr, flags);

    if (sent == 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_EOF);
    }
    else if (sent < 0)
    {
        switch (errno)
        {
        case EAGAIN:
        case EINTR:
            status = LWMSG_STATUS_AGAIN;
            break;
        case EPIPE:
        case ECONNRESET:
            status = LWMSG_STATUS_EOF;
            break;
        default:
            status = LWMSG_STATUS_ERROR;
            break;
        }
        BAIL_ON_ERROR(status);
    }

    *out_sent = (size_t) sent;

error:

    return status;
}

static
LWMsgStatus
lwmsg_connection_sendbuffer(LWMsgAssoc* assoc)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionBuffer* buffer = &priv->sendbuffer;
    struct msghdr msghdr = {0};
    struct iovec iovec = {0};
    union
    {
        struct cmsghdr cm;
        char buf[CMSG_SPACE(sizeof(int) * MAX_FD_PAYLOAD)];
    } buf_un;
    struct cmsghdr *cmsg = NULL;
    size_t sent = 0;
    size_t fds_to_send = buffer->fd_length;
    size_t i;

    if (fds_to_send > MAX_FD_PAYLOAD)
    {
        fds_to_send = MAX_FD_PAYLOAD;
    }

    iovec.iov_base = buffer->cursor;
    iovec.iov_len = buffer->base_length - (buffer->cursor - buffer->base);
    
    msghdr.msg_iov = &iovec;
    msghdr.msg_iovlen = 1;

    if (fds_to_send)
    {
        msghdr.msg_control = buf_un.buf;
        msghdr.msg_controllen = CMSG_SPACE(fds_to_send * sizeof(int));
        
        cmsg = CMSG_FIRSTHDR(&msghdr);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(fds_to_send * sizeof(int));
        
        memcpy(CMSG_DATA(cmsg), buffer->fd, sizeof(int) * fds_to_send);
    }

    BAIL_ON_ERROR(status = lwmsg_connection_sendmsg(priv->fd, &msghdr, 0, &sent));

    buffer->cursor += sent;

    /* Close sent fds since they were copies */
    for (i = 0; i < fds_to_send; i++)
    {
        close(buffer->fd[i]);
    }

    /* Remove sent fds from queue */
    memmove(buffer->fd, buffer->fd + fds_to_send, (buffer->fd_length - fds_to_send) * sizeof(int));
    buffer->fd_length -= fds_to_send;

error:

    return status;
}

LWMsgStatus
lwmsg_connection_queue_fd(LWMsgAssoc* assoc, int fd)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionBuffer* buffer = &priv->sendbuffer;

    BAIL_ON_ERROR(status = lwmsg_connection_buffer_ensure_fd_capacity(buffer, 1));
    buffer->fd[buffer->fd_length] = dup(fd);
    buffer->fd_length++;
    
error:

    return status;
}

LWMsgStatus
lwmsg_connection_dequeue_fd(LWMsgAssoc* assoc, int* out_fd)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionBuffer* buffer = &priv->recvbuffer;

    if (buffer->fd_length == 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_AGAIN);
    }

    *out_fd = buffer->fd[0];
    memmove(buffer->fd, buffer->fd + 1, (buffer->fd_length - 1) * sizeof(int));
    buffer->fd_length--;

error:

    return status;
}

static
LWMsgStatus
lwmsg_connection_check_timeout(
    LWMsgAssoc* assoc,
    struct timeval* remaining
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    LWMsgTime now;
    LWMsgTime diff;

    BAIL_ON_ERROR(status = lwmsg_time_now(&now));

    /* Check for time appearing to move backward */
    if (priv->timeout_set && lwmsg_time_compare(&priv->last_time, &now) == LWMSG_TIME_GREATER)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_TIMEOUT);
    }

    priv->last_time = now;

    if (priv->timeout_set)
    {
        lwmsg_time_difference(&now, &priv->end_time, &diff);

        if (diff.seconds < 0 || diff.microseconds < 0)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_TIMEOUT);
        }

        remaining->tv_sec = diff.seconds;
        remaining->tv_usec = diff.microseconds;
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_connection_transceive(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionBuffer* sendbuffer = &priv->sendbuffer;
    ConnectionBuffer* recvbuffer = &priv->recvbuffer;
    int fd = priv->fd;
    fd_set readfds;
    fd_set writefds;
    int nfds = 0;
    int ret = 0;
    struct timeval timeout = {0, 0};

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    nfds = 0;

    if (fd == -1)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_EOF);
    }

    if (sendbuffer->cursor < sendbuffer->base + sendbuffer->base_length)
    {
        FD_SET(fd, &writefds);
        if (nfds < fd + 1)
        {
            nfds = fd + 1;
        }
    }
    
    if (recvbuffer->base_length < recvbuffer->base_capacity)
    {
        FD_SET(fd, &readfds);
        if (nfds < fd + 1)
        {
            nfds = fd + 1;
        }
    }
    
    if (nfds)
    {
        if (priv->interrupt)
        {
            FD_SET(priv->interrupt->fd[0], &readfds);
            if (nfds < priv->interrupt->fd[0] + 1)
            {
                nfds = priv->interrupt->fd[0] + 1;
            }
        }

        BAIL_ON_ERROR(status = lwmsg_connection_check_timeout(assoc, &timeout));

        ret = select(nfds, &readfds, &writefds, NULL, priv->timeout_set ? &timeout : NULL);
        
        if (ret == -1)
        {
            switch (errno)
            {
            case EAGAIN:
            case EINTR:
                BAIL_ON_ERROR(status = LWMSG_STATUS_AGAIN);
            default:
                BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
            }
        }
        else if (ret > 0)
        {
            if (priv->interrupt && FD_ISSET(priv->interrupt->fd[0], &readfds))
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_INTERRUPT);
            }

            if (FD_ISSET(fd, &readfds))
            {
                BAIL_ON_ERROR(status = lwmsg_connection_recvbuffer(assoc));
            }

            if (lwmsg_connection_urgent_packet_pending(&priv->recvbuffer))
            {
                goto error;
            }

            if (FD_ISSET(fd, &writefds))
            {
                BAIL_ON_ERROR(status = lwmsg_connection_sendbuffer(assoc));
            }
        }
        else
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_TIMEOUT);
        }
    }

error:

    return status;
}

LWMsgStatus
lwmsg_connection_recv_packet(
    LWMsgAssoc* assoc,
    ConnectionPacket** out_packet
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionBuffer* buffer = &priv->recvbuffer;
    ConnectionPacket* packet = NULL;
    size_t curlength = 0;
    
    /* If not enough data to decode a packet header is in the buffer, transceive until there is */
    if (buffer->base_length - (buffer->cursor - buffer->base) < CONNECTION_PACKET_SIZE(ConnectionPacketBase))
    {
        while (buffer->base_length - (buffer->cursor - buffer->base) < CONNECTION_PACKET_SIZE(ConnectionPacketBase))
        {
            BAIL_ON_ERROR(status = lwmsg_connection_transceive(assoc));
        }
        
        packet = (ConnectionPacket*) buffer->base;
        curlength = lwmsg_convert_uint32(packet->length, LWMSG_BIG_ENDIAN, LWMSG_NATIVE_ENDIAN);
    }
    else
    {
        packet = (ConnectionPacket*) buffer->base;
    }
    
    /* Transceive until we have the entire packet */
    while (buffer->base_length < curlength)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_transceive(assoc));
    }

    lwmsg_connection_packet_ntoh(packet);

    *out_packet = packet;

error:

    return status;
}

/* Invariants
 * - Only one packet is queued at a time
 * - The queue packet is always at the start of the buffer
 */
LWMsgStatus
lwmsg_connection_queue_packet(
    LWMsgAssoc* assoc,
    ConnectionPacketType type,
    size_t size,
    ConnectionPacket** out_packet
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv =  lwmsg_assoc_get_private(assoc);
    ConnectionPacket* packet = (ConnectionPacket*) priv->sendbuffer.base;

    if (size > priv->sendbuffer.base_capacity)
    {
        // Internal error
        BAIL_ON_ERROR(status = LWMSG_STATUS_ERROR);
    }

    packet->type = type;
    packet->length = size;

    *out_packet = packet;

error:

    return status;
}

LWMsgStatus
lwmsg_connection_send_packet(
    LWMsgAssoc* assoc,
    ConnectionPacket* packet,
    ConnectionPacket** urgent
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionBuffer* buffer = &priv->sendbuffer;

    buffer->cursor = buffer->base;
    buffer->base_length = packet->length;

    lwmsg_connection_packet_ntoh(packet);
    
    /* Transceive until we send the entire packet */
    while (buffer->cursor < buffer->base + buffer->base_length)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_transceive(assoc));
    }

error:

    if (status == LWMSG_STATUS_EOF && urgent)
    {
        /* If we get an EOF while writing, try to read an urgent packet */
        lwmsg_connection_transceive(assoc);

        if (lwmsg_connection_urgent_packet_pending(&priv->recvbuffer))
        {
            *urgent = (ConnectionPacket*) priv->recvbuffer.base;

            lwmsg_connection_packet_ntoh(*urgent);

            status = LWMSG_STATUS_SUCCESS;
        }
    }

    return status;
}

LWMsgStatus
lwmsg_connection_send_greeting(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionPacket* packet = NULL;
    int fds[2] = { -1, -1 };
    LWMsgSessionManager* manager = NULL;
    const LWMsgSessionID* smid = NULL;

    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
    
    smid = lwmsg_session_manager_get_id(manager);

    BAIL_ON_ERROR(status = lwmsg_connection_queue_packet(
                      assoc,
                      CONNECTION_PACKET_GREETING,
                      CONNECTION_PACKET_SIZE(ConnectionPacketGreeting),
                      &packet));

    packet->contents.greeting.packet_size = (uint32_t) priv->packet_size;
    packet->contents.greeting.flags = 0;
    
    memcpy(packet->contents.greeting.smid, smid->bytes, sizeof(smid->bytes));

#ifndef HAVE_PEERID_METHOD
    /* If this system does not have a simple method for getting the identity
     * of a socket peer, improvise
     */
    if (priv->mode == LWMSG_CONNECTION_MODE_LOCAL)
    {
        /* If we are connecting to a local endpoint */
        if (priv->endpoint)
        {
            uid_t uid;
            gid_t gid;

            BAIL_ON_ERROR(status = lwmsg_connection_get_endpoint_owner(
                              assoc,
                              priv->endpoint,
                              &uid,
                              &gid));

            /* Only send a token to root or ourselves since it could
               be used by the peer to impersonate us */
            if (uid == 0 || uid == getuid())
            {
                /* Send an auth fd */
                if (pipe(fds) != 0)
                {
                    ASSOC_RAISE_ERROR(assoc, LWMSG_STATUS_SYSTEM, "%s", strerror(errno));
                }

                BAIL_ON_ERROR(status = lwmsg_connection_queue_fd(assoc, fds[0]));

                packet->contents.greeting.flags |= CONNECTION_GREETING_AUTH_LOCAL;
            }
        }
    }
#endif

    BAIL_ON_ERROR(status = lwmsg_connection_send_packet(assoc, packet, NULL));

    BAIL_ON_ERROR(status = lwmsg_connection_discard_send_packet(assoc, packet));

error:

    if (fds[0] != -1)
    {
        close(fds[0]);
    }

    if (fds[1] != -1)
    {
        close(fds[1]);
    }

    return status;
}


LWMsgStatus
lwmsg_connection_recv_greeting(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionPacket* packet = NULL;
    int fd = -1;
    LWMsgSessionManager* manager = NULL;
    size_t assoc_count = 0;

    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));

    BAIL_ON_ERROR(status = lwmsg_connection_recv_packet(assoc, &packet));
    
    if (packet->type != CONNECTION_PACKET_GREETING ||
        packet->length != CONNECTION_PACKET_SIZE(ConnectionPacketGreeting))
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_MALFORMED, "Received malformed greeting packet");
    }

    if (packet->contents.greeting.packet_size > (uint32_t) priv->packet_size)
    {
        priv->packet_size = packet->contents.greeting.packet_size;
    }

    if (priv->mode == LWMSG_CONNECTION_MODE_LOCAL)
    {
#ifdef HAVE_PEERID_METHOD
        /* If we have a simple way of getting the peer id, just use it */
        BAIL_ON_ERROR(status = lwmsg_local_token_from_socket_peer(priv->fd, &priv->sec_token));
#else
        /* Otherwise, use the explicit auth method */
        if (packet->contents.greeting.flags & CONNECTION_GREETING_AUTH_LOCAL)
        {
            struct stat statbuf;

            BAIL_ON_ERROR(status = lwmsg_connection_dequeue_fd(assoc, &fd));
            
            if (fstat(fd, &statbuf))
            {
                ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_SYSTEM, "%s", strerror(errno));
            }
            
            if (!S_ISFIFO(statbuf.st_mode) ||
                (statbuf.st_mode & (S_IRWXO | S_IRWXG)) != 0)
            {
                ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_SECURITY, "Received invalid security token");
            }
            
            BAIL_ON_ERROR(status = lwmsg_local_token_new(statbuf.st_uid, statbuf.st_gid, &priv->sec_token));
        }
        else if (priv->endpoint)
        {
            uid_t uid;
            gid_t gid;

            /* Attempt to stat endpoint for owner information */
            BAIL_ON_ERROR(status = lwmsg_connection_get_endpoint_owner(
                              assoc,
                              priv->endpoint,
                              &uid,
                              &gid));

            BAIL_ON_ERROR(status = lwmsg_local_token_new(uid, gid, &priv->sec_token));
        }
#endif
    }
    else if (priv->mode == LWMSG_CONNECTION_MODE_PAIR)
    {
        BAIL_ON_ERROR(status = lwmsg_local_token_new(getuid(), getgid(), &priv->sec_token));
    }

    /* Register session with local session manager */
    BAIL_ON_ERROR(status = lwmsg_session_manager_enter_session(
                      manager,
                      (LWMsgSessionID*) packet->contents.greeting.smid,
                      priv->sec_token,
                      &priv->session,
                      &assoc_count));

    /* Record whether we are the session leader (first assoc in a session) */
    priv->is_session_leader = (assoc_count == 1);

    /* Discard packet */
    BAIL_ON_ERROR(status = lwmsg_connection_discard_recv_packet(assoc, packet));

    BAIL_ON_ERROR(status = lwmsg_connection_buffer_resize(&priv->recvbuffer, priv->packet_size));
    BAIL_ON_ERROR(status = lwmsg_connection_buffer_resize(&priv->sendbuffer, priv->packet_size));

error:

    if (fd != -1)
    {
        close(fd);
    }

    return status;
}

LWMsgStatus
lwmsg_connection_send_shutdown(
    LWMsgAssoc* assoc,
    ConnectionShutdownType type,
    ConnectionShutdownReason reason
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPacket* packet = NULL;

    BAIL_ON_ERROR(status = lwmsg_connection_queue_packet(
                      assoc,
                      CONNECTION_PACKET_SHUTDOWN,
                      CONNECTION_PACKET_SIZE(ConnectionPacketShutdown),
                      &packet));

    packet->contents.shutdown.type = type;
    packet->contents.shutdown.reason = reason;

    BAIL_ON_ERROR(status = lwmsg_connection_send_packet(assoc, packet, NULL));

    BAIL_ON_ERROR(status = lwmsg_connection_discard_send_packet(assoc, packet));

error:

    return status;
}
