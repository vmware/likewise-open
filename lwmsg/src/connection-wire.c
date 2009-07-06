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
#include <lwmsg/data.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
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
        packet->contents.msg.type = lwmsg_convert_uint16(packet->contents.msg.type, LWMSG_BIG_ENDIAN, LWMSG_NATIVE_ENDIAN);
        break;
    case CONNECTION_PACKET_GREETING:
        packet->contents.greeting.packet_size = lwmsg_convert_uint32(
            packet->contents.greeting.packet_size,
            LWMSG_BIG_ENDIAN,
            LWMSG_NATIVE_ENDIAN);
        break;
    case CONNECTION_PACKET_SHUTDOWN:
        packet->contents.shutdown.status = lwmsg_convert_uint32(
            packet->contents.shutdown.status,
            LWMSG_BIG_ENDIAN,
            LWMSG_NATIVE_ENDIAN);
        break;
    }
}

static
uint32_t
lwmsg_connection_packet_length(
    ConnectionPacket* packet
    )
{
    return lwmsg_convert_uint32(packet->length, LWMSG_BIG_ENDIAN, LWMSG_NATIVE_ENDIAN);
}

static
LWMsgBool
lwmsg_connection_fragment_is_complete(
    ConnectionFragment* fragment
    )
{
    return !((fragment->cursor - fragment->data) < CONNECTION_PACKET_SIZE(ConnectionPacketBase) ||
             (fragment->cursor - fragment->data) < lwmsg_connection_packet_length((ConnectionPacket*)fragment->data));
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
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
        case EWOULDBLOCK:
#endif
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
lwmsg_connection_recv_fragment(
    LWMsgAssoc* assoc,
    ConnectionFragment* fragment
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionBuffer* buffer = &priv->recvbuffer;
    size_t length = 0;

    /* While we do not have an entire packet, repeat */
    while (!lwmsg_connection_fragment_is_complete(fragment))
    {
        struct msghdr msghdr = {0};
        struct iovec iovec = {0};
        union
        {
            struct cmsghdr cm;
            char buf[CMSG_SPACE(sizeof(int) * MAX_FD_PAYLOAD)];
        } buf_un;
        struct cmsghdr *cmsg = NULL;
        size_t received = 0;

        iovec.iov_base = fragment->cursor;

        if ((fragment->cursor - fragment->data) < CONNECTION_PACKET_SIZE(ConnectionPacketBase))
        {
            /* If we have not received a full packet header, ask for the remainder */
            iovec.iov_len = CONNECTION_PACKET_SIZE(ConnectionPacketBase) - (fragment->cursor - fragment->data);
        }
        else
        {
            /* Otherwise, we know the length of the packet, so ask for the rest of it */
            length = lwmsg_connection_packet_length((ConnectionPacket*)fragment->data);
            if (length > priv->packet_size)
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
            }

            iovec.iov_len = length - (fragment->cursor - fragment->data);
        }

        msghdr.msg_iov = &iovec;
        msghdr.msg_iovlen = 1;

        msghdr.msg_control = buf_un.buf;
        msghdr.msg_controllen = sizeof(buf_un.buf);

        BAIL_ON_ERROR(status = lwmsg_connection_recvmsg(priv->fd, &msghdr, 0, &received));

        fragment->cursor += received;

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
    }

error:

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
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
        case EWOULDBLOCK:
#endif
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
lwmsg_connection_send_fragment(
    LWMsgAssoc* assoc,
    ConnectionFragment* fragment
    )
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
    size_t fds_to_send = 0;
    size_t i;

    while (!lwmsg_connection_fragment_is_complete(fragment))
    {
        fds_to_send = buffer->fd_length;

        if (fds_to_send > MAX_FD_PAYLOAD)
        {
            fds_to_send = MAX_FD_PAYLOAD;
        }

        iovec.iov_base = fragment->cursor;
        iovec.iov_len =
            lwmsg_connection_packet_length((ConnectionPacket*) fragment->data)
            - (fragment->cursor - fragment->data);

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

        fragment->cursor += sent;

        /* Close sent fds since they were copies */
        for (i = 0; i < fds_to_send; i++)
        {
            close(buffer->fd[i]);
        }

        /* Remove sent fds from queue */
        memmove(buffer->fd, buffer->fd + fds_to_send, (buffer->fd_length - fds_to_send) * sizeof(int));
        buffer->fd_length -= fds_to_send;
    }

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

    if (priv->end_time.seconds > 0)
    {
        /* Depending on our available source of time information, we may
           see time move backward if the system clock changes.  Check for this
           situation and restart the timeout */
        if (lwmsg_time_compare(&priv->last_time, &now) == LWMSG_TIME_GREATER)
        {
            /* Recalculate deadline */
            lwmsg_time_sum(&now, &priv->timeout.current, &priv->end_time);
        }

        lwmsg_time_difference(&now, &priv->end_time, &diff);

        if (diff.seconds < 0 || diff.microseconds < 0)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_TIMEOUT);
        }

        remaining->tv_sec = diff.seconds;
        remaining->tv_usec = diff.microseconds;
    }

    priv->last_time = now;

error:

    return status;
}

static
LWMsgStatus
lwmsg_connection_check_full_fragment(
    LWMsgAssoc* assoc,
    ConnectionFragment* fragment
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPacket* packet = (ConnectionPacket*) fragment->data;

    switch (packet->type)
    {
    case CONNECTION_PACKET_SHUTDOWN:
        status = lwmsg_convert_uint32(
                    packet->contents.shutdown.status,
                    LWMSG_BIG_ENDIAN,
                    LWMSG_NATIVE_ENDIAN);

        switch (status)
        {
        case LWMSG_STATUS_PEER_CLOSE:
        case LWMSG_STATUS_PEER_ABORT:
        case LWMSG_STATUS_PEER_RESET:
            BAIL_ON_ERROR(status);
            break;
        default:
            BAIL_ON_ERROR(status = LWMSG_STATUS_PEER_ABORT);
            break;
        }
        break;
    case CONNECTION_PACKET_MESSAGE:
        if (lwmsg_connection_packet_length(packet) < CONNECTION_PACKET_SIZE(ConnectionPacketMsg))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }
        break;
    case CONNECTION_PACKET_GREETING:
        if (lwmsg_connection_packet_length(packet) != CONNECTION_PACKET_SIZE(ConnectionPacketGreeting))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }
        break;
    }

error:

    return status;
}

/*
   Perform one round of sending and receiving:

   - Attempt to send one fragment if any are pending
   - Attempt to receive one fragment if data is available
*/
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
    ConnectionFragment* fragment = NULL;
    int fd = priv->fd;
    fd_set readfds;
    fd_set writefds;
    int nfds = 0;
    int ret = 0;
    struct timeval timeout = {0, 0};
    LWMsgBool do_recv = LWMSG_FALSE;
    LWMsgBool do_send = LWMSG_FALSE;
    LWMsgBool did_recv = LWMSG_FALSE;
    LWMsgBool did_send = LWMSG_FALSE;

    if (!priv->is_nonblock)
    {
        /* If we are in blocking mode, we first need to select() */
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        nfds = 0;

        /* Wait to write if we have any pending fragments in the send buffer */
        if (sendbuffer->num_pending)
        {
            FD_SET(fd, &writefds);
            if (nfds < fd + 1)
            {
                nfds = fd + 1;
            }
        }

        /* Wait to read if we have any incomplete fragments in the receive buffer */
        if (recvbuffer->num_pending &&
            !lwmsg_connection_fragment_is_complete(
                lwmsg_connection_buffer_get_last_fragment(recvbuffer)))
        {
            FD_SET(fd, &readfds);
            if (nfds < fd + 1)
            {
                nfds = fd + 1;
            }
        }

        if (nfds)
        {
            BAIL_ON_ERROR(status = lwmsg_connection_check_timeout(assoc, &timeout));

            do
            {
                ret = select(nfds, &readfds, &writefds, NULL, priv->end_time.seconds >= 0 ? &timeout : NULL);
            } while (ret == -1 && errno == EINTR);

            if (ret == -1)
            {
                ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_SYSTEM, "%s", strerror(errno));
            }
            else if (ret > 0)
            {
                if (FD_ISSET(fd, &readfds))
                {
                    do_recv = LWMSG_TRUE;
                }

                if (FD_ISSET(fd, &writefds))
                {
                    do_send = LWMSG_TRUE;
                }
            }
            else
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_TIMEOUT);
            }
        }
    }
    else
    {
        /* We receive if we have an incomplete fragment in the recv buffer */
        do_recv =
            recvbuffer->num_pending > 0 &&
            !lwmsg_connection_fragment_is_complete(
                lwmsg_connection_buffer_get_last_fragment(recvbuffer));
        /* We send if we have any fragments in the send buffer */
        do_send = sendbuffer->num_pending > 0;
    }

    if (do_recv)
    {
        fragment = lwmsg_connection_buffer_get_last_fragment(recvbuffer);

        status = lwmsg_connection_recv_fragment(assoc, fragment);

        switch (status)
        {
        case LWMSG_STATUS_AGAIN:
            status = LWMSG_STATUS_SUCCESS;
            break;
        case LWMSG_STATUS_EOF:
            BAIL_ON_ERROR(status = LWMSG_STATUS_PEER_CLOSE);
            break;
        default:
            BAIL_ON_ERROR(status);

            did_recv = LWMSG_TRUE;

            if (lwmsg_connection_fragment_is_complete(fragment))
            {
                BAIL_ON_ERROR(status = lwmsg_connection_check_full_fragment(assoc, fragment));
            }
        }
    }

    if (do_send)
    {
        fragment = lwmsg_connection_buffer_get_first_fragment(sendbuffer);

        status = lwmsg_connection_send_fragment(assoc, fragment);
        switch (status)
        {
        case LWMSG_STATUS_AGAIN:
            status = LWMSG_STATUS_SUCCESS;
            break;
        case LWMSG_STATUS_EOF:
            BAIL_ON_ERROR(status = LWMSG_STATUS_PEER_CLOSE);
            break;
        default:
            BAIL_ON_ERROR(status);

            did_send = LWMSG_TRUE;

            /* We sucessfully sent the entire fragment, so remove it */
            lwmsg_connection_buffer_dequeue_fragment(sendbuffer);
            lwmsg_connection_buffer_free_fragment(sendbuffer, fragment);
        }
    }

    if (priv->is_nonblock && (do_recv || do_send) && !(did_recv || did_send))
    {
        /* We are in non-blocking mode.
           We wanted to do work.
           We did not succeed in doing any.

           This means that the current operation cannot complete and needs to be finished later */
        BAIL_ON_ERROR(status = LWMSG_STATUS_PENDING);
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_connection_recv_full_fragment(
    LWMsgAssoc* assoc,
    ConnectionFragment** fragment
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionBuffer* buffer = &priv->recvbuffer;
    ConnectionFragment* frag = NULL;
    ConnectionPacket* packet = NULL;

    if (buffer->num_pending == 0)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_buffer_create_fragment(
                          buffer,
                          priv->packet_size,
                          &frag));
        lwmsg_connection_buffer_queue_fragment(buffer, frag);
    }
    else
    {
        frag = lwmsg_connection_buffer_get_first_fragment(buffer);
    }

    while (!lwmsg_connection_fragment_is_complete(frag))
    {
        BAIL_ON_ERROR(status = lwmsg_connection_transceive(assoc));
    }

    packet = (ConnectionPacket*) frag->data;

    lwmsg_connection_packet_ntoh(packet);

    lwmsg_connection_buffer_dequeue_fragment(buffer);

    *fragment = frag;

error:

    return status;
}

static unsigned char*
lwmsg_connection_packet_payload(ConnectionPacket* packet)
{
    switch (packet->type)
    {
    case CONNECTION_PACKET_MESSAGE:
        return (unsigned char*) packet + CONNECTION_PACKET_SIZE(ConnectionPacketMsg);
    }

    return NULL;
}

static void
lwmsg_connection_load_fragment(
    LWMsgBuffer* buffer,
    ConnectionFragment* fragment,
    size_t space
    )
{
    ConnectionPacket* packet = (ConnectionPacket*) fragment->data;

    buffer->base = (unsigned char*) fragment;
    buffer->cursor = lwmsg_connection_packet_payload(packet);
    buffer->end = fragment->data + space;
}

static LWMsgStatus
lwmsg_connection_recv_wrap(LWMsgBuffer* buffer, size_t needed)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = (LWMsgAssoc*) buffer->data;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionFragment* fragment = (ConnectionFragment*) buffer->base;
    ConnectionPacket* packet = NULL;

    buffer->base = NULL;

    /* Discard packet */
    lwmsg_connection_buffer_free_fragment(&priv->recvbuffer, fragment);
    fragment = NULL;

    if (needed)
    {
        fragment = lwmsg_connection_buffer_get_first_fragment(&priv->recvbuffer);
        if (!fragment)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }
        lwmsg_connection_buffer_dequeue_fragment(&priv->recvbuffer);

        packet = (ConnectionPacket*) fragment->data;

        lwmsg_connection_packet_ntoh(packet);

        lwmsg_connection_load_fragment(buffer, fragment, packet->length);
        fragment = NULL;
    }

error:

    if (fragment)
    {
        lwmsg_connection_buffer_free_fragment(&priv->recvbuffer, fragment);
    }

    return status;
}

static LWMsgStatus
lwmsg_connection_send_wrap(LWMsgBuffer* buffer, size_t needed)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = (LWMsgAssoc*) buffer->data;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionFragment* fragment = (ConnectionFragment*) buffer->base;
    ConnectionPacket* packet = (ConnectionPacket*) fragment->data;

    buffer->base = NULL;

    /* Update size of packet based on amount of buffer that was used */
    packet->length = buffer->cursor - (unsigned char*) packet;

    /* If the marshaller is done, this is the last fragment */
    if (needed == 0)
    {
        packet->flags |= CONNECTION_PACKET_FLAG_LAST_FRAGMENT;
    }

    /* Put the packet in network byte order */
    lwmsg_connection_packet_ntoh(packet);

    /* Now that the packet is complete, send it */
    lwmsg_connection_buffer_queue_fragment(
                      &priv->sendbuffer,
                      fragment);
    fragment = NULL;

    /* Flush the send buffer */
    status = lwmsg_connection_flush(assoc);

    if (needed && status == LWMSG_STATUS_PENDING)
    {
        /* If we have more to marshal but we can't send,
           ignore it for now and continue to queue up packets */
        status = LWMSG_STATUS_SUCCESS;
    }

    BAIL_ON_ERROR(status);

    /* If we have more fragments to send, allocate a new packet */
    if (needed)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_buffer_create_fragment(
                          &priv->sendbuffer,
                          priv->packet_size,
                          &fragment));

        packet = (ConnectionPacket*) fragment->data;
        packet->type = CONNECTION_PACKET_MESSAGE;
        packet->flags = 0;
        packet->contents.msg.type = priv->params.message->tag;

        lwmsg_connection_load_fragment(buffer, fragment, priv->packet_size);
        fragment = NULL;
    }

error:

    if (fragment)
    {
        lwmsg_connection_buffer_free_fragment(&priv->sendbuffer, fragment);
    }

    return status;
}

LWMsgStatus
lwmsg_connection_begin_timeout(
    LWMsgAssoc* assoc,
    LWMsgTime* value
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    if (!priv->is_nonblock)
    {
        /* Record current timeout value */
        priv->timeout.current = *value;
        /* Get current time */
        BAIL_ON_ERROR(status = lwmsg_time_now(&priv->last_time));

        if (value->seconds >= 0)
        {
            /* Calculate absolute deadline */
            lwmsg_time_sum(&priv->last_time, value, &priv->end_time);
        }
        else
        {
            /* Mark deadline as unset */
            memset(&priv->end_time, 0xFF, sizeof(priv->last_time));
        }
    }

error:

    return status;
}

LWMsgStatus
lwmsg_connection_flush(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionFragment* fragment = NULL;

    /* When performing a flush, have a packet ready in the
       recv buffer so that we can catch peer shutdown packets */
    if (priv->recvbuffer.num_pending == 0)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_buffer_create_fragment(
                          &priv->recvbuffer,
                          priv->packet_size,
                          &fragment));

        lwmsg_connection_buffer_queue_fragment(&priv->recvbuffer, fragment);
    }

    while (priv->sendbuffer.num_pending > 0)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_transceive(assoc));
    }

error:

    return status;
}

LWMsgStatus
lwmsg_connection_check(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionFragment* fragment = NULL;

    if (priv->recvbuffer.num_pending == 0)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_buffer_create_fragment(
                          &priv->recvbuffer,
                          priv->packet_size,
                          &fragment));

        lwmsg_connection_buffer_queue_fragment(&priv->recvbuffer, fragment);
    }

    BAIL_ON_ERROR(status = lwmsg_connection_transceive(assoc));

error:

    if (status == LWMSG_STATUS_PENDING)
    {
        status = LWMSG_STATUS_SUCCESS;
    }

    return status;
}

static
LWMsgStatus
lwmsg_connection_begin_connect_local(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    struct sockaddr_un sockaddr;
    long opts = 0;

    priv->fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (priv->fd == -1)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_SYSTEM, "%s", strerror(errno));
    }

    /* Get socket flags */
    if ((opts = fcntl(priv->fd, F_GETFL, 0)) < 0)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_SYSTEM,
                          "Could not get socket flags: %s", strerror(errno));
    }

    /* Set non-blocking flag */
    opts |= O_NONBLOCK;

    /* Set socket flags */
    if (fcntl(priv->fd, F_SETFL, opts) < 0)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_SYSTEM,
                          "Could not set socket flags: %s", strerror(errno));
    }

    sockaddr.sun_family = AF_UNIX;

    if (strlen(priv->endpoint) + 1 > sizeof(sockaddr.sun_path))
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER, "Endpoint is too long for underlying protocol");
    }

    strcpy(sockaddr.sun_path, priv->endpoint);

    if (connect(priv->fd, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) < 0)
    {
        switch (errno)
        {
        case 0:
            status = LWMSG_STATUS_SUCCESS;
            break;
        case EINPROGRESS:
            status = LWMSG_STATUS_PENDING;
            break;
        case ENOENT:
            status = LWMSG_STATUS_FILE_NOT_FOUND;
            break;
        case ECONNREFUSED:
            status = LWMSG_STATUS_CONNECTION_REFUSED;
            break;
        default:
            status = LWMSG_STATUS_SYSTEM;
            break;
        }
        BAIL_ON_ERROR(status);
    }

done:

    return status;

error:

    if (priv->fd >= 0)
    {
        close(priv->fd);
        priv->fd = -1;
    }

    goto done;
}

LWMsgStatus
lwmsg_connection_begin_connect(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    switch (priv->mode)
    {
    case LWMSG_CONNECTION_MODE_LOCAL:
        BAIL_ON_ERROR(status = lwmsg_connection_begin_connect_local(assoc));
        break;
    case LWMSG_CONNECTION_MODE_REMOTE:
        BAIL_ON_ERROR(status = LWMSG_STATUS_UNIMPLEMENTED);
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_INTERNAL);
        break;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_connection_finish_connect(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    long err = 0;
    socklen_t len = 0;
    fd_set readfds;
    fd_set writefds;
    struct timeval timeout = {0, 0};
    int nfds = 0;

    if (!priv->is_nonblock)
    {
        do
        {
            /* Check for a timeout and get the value to pass to select */
            BAIL_ON_ERROR(status = lwmsg_connection_check_timeout(assoc, &timeout));
            FD_ZERO(&writefds);
            FD_ZERO(&readfds);
            FD_SET(priv->fd, &writefds);

            nfds = priv->fd + 1;

            err = select(nfds, &readfds, &writefds, NULL, priv->end_time.seconds >= 0 ? &timeout : NULL);
        } while (err == 0);

        if (err < 0)
        {
            /* Select failed for some reason */
            ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_SYSTEM, "%s", strerror(errno));
        }
    }

    /* If we make it here, our connect operation should be done */
    len = sizeof(err);
    /* Use getsockopt to extract the result of the connect call */
    if (getsockopt(priv->fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
    {
        /* Getsockopt failed for some reason */
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_SYSTEM, "%s", strerror(errno));
    }

    switch (err)
    {
    case 0:
        status = LWMSG_STATUS_SUCCESS;
        break;
    case EINPROGRESS:
        status = LWMSG_STATUS_PENDING;
        break;
    case ENOENT:
        status = LWMSG_STATUS_FILE_NOT_FOUND;
        break;
    case ECONNREFUSED:
        status = LWMSG_STATUS_CONNECTION_REFUSED;
        break;
    default:
        status = LWMSG_STATUS_SYSTEM;
        break;
    }

    BAIL_ON_ERROR(status);

done:

    return status;

error:

    if (status != LWMSG_STATUS_PENDING)
    {
        if (priv->fd != -1)
        {
            close(priv->fd);
            priv->fd = -1;
        }
    }

    goto done;
}

LWMsgStatus
lwmsg_connection_connect_existing(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    long opts = 0;

    BAIL_ON_ERROR(status = lwmsg_connection_begin_timeout(
                      assoc,
                      &priv->timeout.establish));

    /* Get socket flags */
    if ((opts = fcntl(priv->fd, F_GETFL, 0)) < 0)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_SYSTEM,
                          "Could not get socket flags: %s", strerror(errno));
    }

    /* Set non-blocking flag */
    opts |= O_NONBLOCK;

    /* Set socket flags */
    if (fcntl(priv->fd, F_SETFL, opts) < 0)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_SYSTEM,
                          "Could not set socket flags: %s", strerror(errno));
    }

error:

    return status;
}


LWMsgStatus
lwmsg_connection_begin_send_handshake(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionFragment* fragment = NULL;
    ConnectionPacket* packet = NULL;
    int fds[2] = { -1, -1 };
    LWMsgSessionManager* manager = NULL;
    const LWMsgSessionID* smid = NULL;

    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
    
    smid = lwmsg_session_manager_get_id(manager);

    BAIL_ON_ERROR(status = lwmsg_connection_buffer_create_fragment(
                      &priv->sendbuffer,
                      priv->packet_size,
                      &fragment));

    packet = (ConnectionPacket*) fragment->data;

    packet->type = CONNECTION_PACKET_GREETING;
    packet->length = CONNECTION_PACKET_SIZE(ConnectionPacketGreeting);
    packet->flags = CONNECTION_PACKET_FLAG_SINGLE;
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

    lwmsg_connection_packet_ntoh(packet);

    lwmsg_connection_buffer_queue_fragment(&priv->sendbuffer, fragment);
    fragment = NULL;

error:

    if (fds[0] != -1)
    {
        close(fds[0]);
    }

    if (fds[1] != -1)
    {
        close(fds[1]);
    }

    if (fragment)
    {
        lwmsg_connection_buffer_free_fragment(&priv->sendbuffer, fragment);
    }

    return status;
}

LWMsgStatus
lwmsg_connection_finish_send_handshake(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_connection_flush(assoc));

error:

    return status;
}

LWMsgStatus
lwmsg_connection_begin_recv_handshake(
    LWMsgAssoc* assoc
    )
{
    return LWMSG_STATUS_SUCCESS;
}

LWMsgStatus
lwmsg_connection_finish_recv_handshake(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionFragment* fragment = NULL;
    ConnectionPacket* packet = NULL;
    int fd = -1;
    LWMsgSessionManager* manager = NULL;

    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));

    BAIL_ON_ERROR(status = lwmsg_connection_recv_full_fragment(assoc, &fragment));

    packet = (ConnectionPacket*) fragment->data;

    if (packet->type != CONNECTION_PACKET_GREETING)
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
                      priv->params.establish.construct,
                      priv->params.establish.destruct,
                      priv->params.establish.construct_data,
                      &priv->session));

    /* Reconstruct buffers in case our packet size changed */
    lwmsg_connection_buffer_destruct(&priv->recvbuffer);
    BAIL_ON_ERROR(status = lwmsg_connection_buffer_construct(&priv->recvbuffer));
    lwmsg_connection_buffer_destruct(&priv->sendbuffer);
    BAIL_ON_ERROR(status = lwmsg_connection_buffer_construct(&priv->sendbuffer));

    /* Set up marshal token for message exchange (this is where byte order
       and other negotiated format settings should be set) */
    if (priv->marshal_context)
    {
        lwmsg_data_context_delete(priv->marshal_context);
        priv->marshal_context = NULL;
    }

    BAIL_ON_ERROR(status = lwmsg_data_context_new(&assoc->context, &priv->marshal_context));

error:

    if (fragment)
    {
        lwmsg_connection_buffer_free_fragment(&priv->recvbuffer, fragment);
    }

    if (fd != -1)
    {
        close(fd);
    }

    return status;
}

LWMsgStatus
lwmsg_connection_begin_send_message(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    LWMsgProtocol* prot = lwmsg_assoc_get_protocol(assoc);
    LWMsgTypeSpec* type = NULL;
    ConnectionFragment* fragment = NULL;
    ConnectionPacket* packet = NULL;
    LWMsgMessage* message = priv->params.message;
    LWMsgBuffer buffer;

    memset(&buffer, 0, sizeof(buffer));

    status = lwmsg_protocol_get_message_type(prot, message->tag, &type);

    switch (status)
    {
    case LWMSG_STATUS_NOT_FOUND:
        status = LWMSG_STATUS_MALFORMED;
        break;
    default:
        break;
    }

    BAIL_ON_ERROR(status);

    BAIL_ON_ERROR(status = lwmsg_connection_buffer_create_fragment(
                      &priv->sendbuffer,
                      priv->packet_size,
                      &fragment));

    packet = (ConnectionPacket*) fragment->data;

    packet->type = CONNECTION_PACKET_MESSAGE;
    packet->flags = CONNECTION_PACKET_FLAG_FIRST_FRAGMENT;
    packet->contents.msg.type = message->tag;

    /* Set up timeout */
    BAIL_ON_ERROR(status = lwmsg_connection_begin_timeout(
                      assoc,
                      &priv->timeout.message));

    /* If the message has no payload, send a zero-length message */
    if (type == NULL)
    {
        packet->length = CONNECTION_PACKET_SIZE(ConnectionPacketMsg);
        packet->flags |= CONNECTION_PACKET_FLAG_LAST_FRAGMENT;

        lwmsg_connection_packet_ntoh(packet);

        lwmsg_connection_buffer_queue_fragment(&priv->sendbuffer, fragment);
        fragment = NULL;
    }
    else
    {
        lwmsg_connection_load_fragment(&buffer, fragment, priv->packet_size);
        fragment = NULL;

        buffer.wrap = lwmsg_connection_send_wrap;
        buffer.data = assoc;

        BAIL_ON_ERROR(status = lwmsg_data_marshal(
                          priv->marshal_context,
                          type,
                          message->data,
                          &buffer));
    }

error:

    if (fragment)
    {
        lwmsg_connection_buffer_free_fragment(&priv->sendbuffer, fragment);
    }

    if (buffer.base)
    {
        lwmsg_connection_buffer_free_fragment(&priv->sendbuffer, (ConnectionFragment*) buffer.base);
    }


    return status;
}

LWMsgStatus
lwmsg_connection_finish_send_message(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_connection_flush(assoc));

error:

    return status;
}

LWMsgStatus
lwmsg_connection_begin_recv_message(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    /* Set up timeout */
    BAIL_ON_ERROR(status = lwmsg_connection_begin_timeout(
                      assoc,
                      &priv->timeout.message));

error:

    return status;
}

LWMsgStatus
lwmsg_connection_finish_recv_message(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    LWMsgProtocol* prot = lwmsg_assoc_get_protocol(assoc);
    LWMsgContext* context = &assoc->context;
    LWMsgTypeSpec* type = NULL;
    ConnectionFragment* last_frag = NULL; /* Do not free */
    ConnectionFragment* fragment = NULL;
    ConnectionPacket* packet = NULL;
    LWMsgBuffer buffer;
    ConnectionBuffer* recvbuffer = &priv->recvbuffer;

    memset(&buffer, 0, sizeof(buffer));

    /* Keep reading packets until we get the final fragment */
    while (!(last_frag = lwmsg_connection_buffer_get_last_fragment(recvbuffer)) ||
           !lwmsg_connection_fragment_is_complete(last_frag) ||
           !(((ConnectionPacket*) last_frag->data)->flags & CONNECTION_PACKET_FLAG_LAST_FRAGMENT))
    {
        /* If there is no fragment in the recv buffer or the last fragment in the buffer
           is complete add a new fragment to the buffer to recv into */
        if (last_frag == NULL || lwmsg_connection_fragment_is_complete(last_frag))
        {
            BAIL_ON_ERROR(status = lwmsg_connection_buffer_create_fragment(
                          recvbuffer,
                          priv->packet_size,
                          &last_frag));
            lwmsg_connection_buffer_queue_fragment(recvbuffer, last_frag);
        }

        BAIL_ON_ERROR(status = lwmsg_connection_transceive(assoc));

        packet = (ConnectionPacket*) last_frag->data;

        if (lwmsg_connection_fragment_is_complete(last_frag) &&
            packet->type != CONNECTION_PACKET_MESSAGE)
        {
            RAISE_ERROR(context, LWMSG_STATUS_MALFORMED, "Did not receive message packet as expected");
        }
    }

    fragment = lwmsg_connection_buffer_get_first_fragment(recvbuffer);
    lwmsg_connection_buffer_dequeue_fragment(recvbuffer);

    packet = (ConnectionPacket*) fragment->data;

    lwmsg_connection_packet_ntoh(packet);

    status = lwmsg_protocol_get_message_type(prot, packet->contents.msg.type, &type);

    switch (status)
    {
    case LWMSG_STATUS_NOT_FOUND:
        status = LWMSG_STATUS_MALFORMED;
        break;
    default:
        break;
    }

    BAIL_ON_ERROR(status);

    priv->params.message->tag = packet->contents.msg.type;

    if (type == NULL)
    {
        /* If message has no payload, just set the payload to NULL */
        priv->params.message->data = NULL;
    }
    else
    {
        lwmsg_connection_load_fragment(&buffer, fragment, packet->length);
        fragment = NULL;

        buffer.wrap = lwmsg_connection_recv_wrap;
        buffer.data = assoc;

        BAIL_ON_ERROR(status = lwmsg_data_unmarshal(
                          priv->marshal_context,
                          type,
                          &buffer,
                          &priv->params.message->data));
    }

error:

    if (fragment)
    {
        lwmsg_connection_buffer_free_fragment(&priv->recvbuffer, fragment);
    }

    if (buffer.base)
    {
        lwmsg_connection_buffer_free_fragment(&priv->recvbuffer, (ConnectionFragment*) buffer.base);
    }

    return status;
}

LWMsgStatus
lwmsg_connection_begin_send_shutdown(
    LWMsgAssoc* assoc,
    LWMsgStatus reason
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPacket* packet = NULL;
    ConnectionFragment* fragment = NULL;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    /* Set up timeout */
    BAIL_ON_ERROR(status = lwmsg_connection_begin_timeout(
                      assoc,
                      &priv->timeout.establish));

    BAIL_ON_ERROR(status = lwmsg_connection_buffer_create_fragment(
                      &priv->sendbuffer,
                      priv->packet_size,
                      &fragment));

    packet = (ConnectionPacket*) fragment->data;

    packet->type = CONNECTION_PACKET_SHUTDOWN;
    packet->length = CONNECTION_PACKET_SIZE(ConnectionPacketShutdown);
    packet->flags = CONNECTION_PACKET_FLAG_SINGLE;
    packet->contents.shutdown.status = reason;

    lwmsg_connection_packet_ntoh(packet);

    lwmsg_connection_buffer_queue_fragment(&priv->sendbuffer, fragment);

error:

    return status;
}

LWMsgStatus
lwmsg_connection_finish_send_shutdown(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    /* Flush the send buffer */
    BAIL_ON_ERROR(status = lwmsg_connection_flush(assoc));

error:

    return status;
}
