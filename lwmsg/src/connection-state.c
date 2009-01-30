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
 *        connection-state.c
 *
 * Abstract:
 *
 *        Connection API
 *        State machine
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "assoc-private.h"
#include "connection-private.h"
#include "util-private.h"
#include <lwmsg/buffer.h>
#include <lwmsg/unmarshal.h>
#include <lwmsg/marshal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>

static unsigned char*
lwmsg_connection_packet_payload(ConnectionPacket* packet)
{
    switch (packet->type)
    {
    case CONNECTION_PACKET_MESSAGE:
    case CONNECTION_PACKET_REPLY:
        return (unsigned char*) packet + CONNECTION_PACKET_SIZE(ConnectionPacketMsg);
    case CONNECTION_PACKET_FRAGMENT:
        return (unsigned char*) packet + CONNECTION_PACKET_SIZE(ConnectionPacketBase);
    }

    return NULL;
}

static void
lwmsg_connection_load_packet(LWMsgBuffer* buffer, ConnectionPacket* packet)
{
    buffer->base = (unsigned char*) packet;
    buffer->cursor = lwmsg_connection_packet_payload(packet);
    buffer->end = buffer->base + packet->length;
}

static LWMsgStatus
lwmsg_connection_handle_urgent(LWMsgAssoc* assoc, ConnectionPacket* packet)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    
    switch (packet->type)
    {
    case CONNECTION_PACKET_SHUTDOWN:
        priv->state = CONNECTION_STATE_CLOSED;

        if (priv->fd != -1)
        {
            close(priv->fd);
            priv->fd = -1;
        }

        switch (packet->contents.shutdown.type)
        {
        case CONNECTION_SHUTDOWN_NORMAL:
            status = LWMSG_STATUS_PEER_CLOSE;
            break;
        case CONNECTION_SHUTDOWN_RESET:
            status = LWMSG_STATUS_PEER_RESET;
            break;
        case CONNECTION_SHUTDOWN_ABORT:
            status = LWMSG_STATUS_PEER_ABORT;
            break;
        }
        
        BAIL_ON_ERROR(status);
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        break;
    }

error:

    lwmsg_connection_discard_recv_packet(assoc, packet);

    return status;
}

static LWMsgStatus
lwmsg_connection_recvfull(LWMsgBuffer* buffer, size_t needed)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = (LWMsgAssoc*) buffer->data;
    ConnectionPacket* packet = (ConnectionPacket*) buffer->base;
    
    /* Discard packet */
    lwmsg_connection_discard_recv_packet(assoc, packet);

    if (needed)
    {
    retry:
        BAIL_ON_ERROR(status = lwmsg_connection_recv_packet(assoc, &packet));
        
        /* If the packet is urgent, handle it now and try again */
        if (lwmsg_connection_packet_is_urgent(packet))
        {
            BAIL_ON_ERROR(status = lwmsg_connection_handle_urgent(assoc, packet));
            goto retry;
        }

        /* If we are in the middle of decoding a message and we had to read 
           another packet, it better be a fragment packet or something is amiss */
        if (packet->type != CONNECTION_PACKET_FRAGMENT)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }
        
        lwmsg_connection_load_packet(buffer, packet);
    }
    else
    {
        BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_STATE_NONE, CONNECTION_EVENT_DONE));
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_connection_sendfull(LWMsgBuffer* buffer, size_t needed)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = (LWMsgAssoc*) buffer->data;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionPacket* packet = (ConnectionPacket*) buffer->base;
    ConnectionPacket* urgent = NULL;

    /* Update size of packet based on amount of buffer that was used */
    packet->length = buffer->cursor - buffer->base;

retry:
    /* Now that the packet is complete, send it */
    BAIL_ON_ERROR(status = lwmsg_connection_send_packet(assoc, packet, &urgent));

    /* If we received an urgent packet, deal with it and try again */
    if (urgent)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_handle_urgent(assoc, urgent));
        goto retry;
    }

    /* Discard the sent packet */
    BAIL_ON_ERROR(status = lwmsg_connection_discard_send_packet(assoc, packet));

    /* If we have more fragments to send, allocate a new packet */
    if (needed)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_queue_packet(
                          assoc,
                          CONNECTION_PACKET_FRAGMENT,
                          priv->packet_size,
                          &packet));
        
        lwmsg_connection_load_packet(buffer, packet);
    }
    else
    {
        /* We are done sending, so insert a done event into the state machine */
        BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_STATE_NONE, CONNECTION_EVENT_DONE));
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_connection_do_recv(LWMsgAssoc* assoc, ConnectionPacketType expect)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    LWMsgProtocol* prot = lwmsg_assoc_get_protocol(assoc);
    LWMsgContext* context = &assoc->context;
    LWMsgTypeSpec* type = NULL;
    ConnectionPacket* packet = NULL;
    LWMsgBuffer buffer;

retry:
    BAIL_ON_ERROR(status = lwmsg_connection_recv_packet(assoc, &packet));

    if (lwmsg_connection_packet_is_urgent(packet))
    {
        BAIL_ON_ERROR(status = lwmsg_connection_handle_urgent(assoc, packet));
        goto retry;
    }

    if (packet->type != expect)
    {
        RAISE_ERROR(context, LWMSG_STATUS_MALFORMED, "Did not receive message packet as expected");
    }
    
    priv->message->tag = packet->contents.msg.type;

    status = lwmsg_protocol_get_message_type(prot, priv->message->tag, &type);

    switch (status)
    {
    case LWMSG_STATUS_NOT_FOUND:
        status = LWMSG_STATUS_MALFORMED;
        break;
    default:
        break;
    }

    BAIL_ON_ERROR(status);

    if (type == NULL)
    {
        /* If message has no payload, just set the payload to NULL */
        priv->message->object = NULL;
        
        /* Discard packet */
        lwmsg_connection_discard_recv_packet(assoc, packet);

        /* We are done receiving */
        BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_STATE_NONE, CONNECTION_EVENT_DONE));
    }
    else
    {
        
        lwmsg_connection_load_packet(&buffer, packet);
        
        buffer.wrap = lwmsg_connection_recvfull;
        buffer.data = assoc;
        
        BAIL_ON_ERROR(status = lwmsg_unmarshal(
                          context,
                          type,
                          &buffer,
                          &priv->message->object));
    }
error:

    return status;
}

static LWMsgStatus
lwmsg_connection_do_send(LWMsgAssoc* assoc, ConnectionPacketType ptype)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    LWMsgProtocol* prot = lwmsg_assoc_get_protocol(assoc);
    LWMsgContext* context = &assoc->context;
    LWMsgTypeSpec* type = NULL;
    ConnectionPacket* packet = NULL;
    ConnectionPacket* urgent = NULL;
    LWMsgMessage* message = priv->message;
    LWMsgBuffer buffer;
    
    status = lwmsg_protocol_get_message_type(prot, message->tag, &type);

    switch (status)
    {
    case LWMSG_STATUS_MALFORMED:
        status = LWMSG_STATUS_MALFORMED;
        break;
    default:
        break;
    }

    BAIL_ON_ERROR(status);

    BAIL_ON_ERROR(status = lwmsg_connection_queue_packet(assoc,
                                                         ptype,
                                                         priv->packet_size,
                                                         &packet));

    packet->contents.msg.type = message->tag;

    /* If the message has no payload, send a zero-length message */
    if (type == NULL)
    {
        packet->length = CONNECTION_PACKET_SIZE(ConnectionPacketMsg);
        /* Now that the packet is complete, send it */

    retry:
        BAIL_ON_ERROR(status = lwmsg_connection_send_packet(assoc, packet, &urgent));
        
        /* If we received an urgent packet, deal with it and try again */
        if (urgent)
        {
            BAIL_ON_ERROR(status = lwmsg_connection_handle_urgent(assoc, urgent));
            goto retry;
        }
        
        /* Discard the sent packet */
        BAIL_ON_ERROR(status = lwmsg_connection_discard_send_packet(assoc, packet));
        
        /* We are done sending, so insert a done event into the state machine */
        BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_STATE_NONE, CONNECTION_EVENT_DONE));        
    }
    else
    {
        lwmsg_connection_load_packet(&buffer, packet);

        buffer.wrap = lwmsg_connection_sendfull;
        buffer.data = assoc;
        
        BAIL_ON_ERROR(status = lwmsg_marshal(
                          context,
                          type,
                          message->object,
                          &buffer));
    }
    
error:

    return status;
}

static LWMsgStatus
lwmsg_connection_do_shutdown(
    LWMsgAssoc* assoc,
    ConnectionShutdownType type,
    ConnectionShutdownReason reason
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    LWMsgSessionManager* manager = NULL;
    LWMsgBool session_lost = LWMSG_FALSE;

    /* Remove ourselves from the session, if present */
    if (priv->session)
    {
        size_t num_handles = 0;
        size_t num_assocs = 0;

        BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
        num_handles = lwmsg_session_manager_get_session_handle_count(manager, priv->session);
        BAIL_ON_ERROR(status = lwmsg_session_manager_leave_session(manager, priv->session, &num_assocs));
        priv->session = NULL;

        if (num_handles > 0 && num_assocs == 0)
        {
            /* If there were handles in the session, but there are no longer any associations
               keeping the session alive, we have lost our session and cannot reset the connection */
            session_lost = LWMSG_TRUE;
        }
    }



    switch (type)
    {
    case CONNECTION_SHUTDOWN_RESET:
        if (session_lost)
        {
            /* We can't reset the connection because we lost session state
               in the process */
            priv->state = CONNECTION_STATE_CLOSED;
            BAIL_ON_ERROR(status = LWMSG_STATUS_SESSION_LOST);
        }
        else
        {
            /* Either our session is still being kept alive by another association,
               or it did not contain any state, so we can safely reset */
            priv->state = CONNECTION_STATE_START;
        }
        break;
    default:
        priv->state = CONNECTION_STATE_CLOSED;
    }

    if (priv->fd != -1)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_send_shutdown(assoc, type, reason));
    }

error:

    /* Regardless of any errors that occur, do some basic cleanup tasks */

    /* Close physical connection */
    if (priv->fd != -1)
    {
        shutdown(priv->fd, SHUT_RDWR);
        close(priv->fd);
        priv->fd = -1;
    }

    /* Reset buffers */
    lwmsg_connection_buffer_reset(&priv->sendbuffer);
    lwmsg_connection_buffer_reset(&priv->recvbuffer);

    if (status == LWMSG_STATUS_EOF)
    {
        /* Absorb EOF errors because we don't really case about them if we are
           shutting things down ourselves */
        status = LWMSG_STATUS_SUCCESS;
    }

    return status;
}    

static
LWMsgStatus
lwmsg_connection_connect_local(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    struct sockaddr_un sockaddr;
    int sock = -1;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    
    if (sock == -1)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_SYSTEM, "%s", strerror(errno));
    }

    sockaddr.sun_family = AF_UNIX;

    if (strlen(priv->endpoint) + 1 > sizeof(sockaddr.sun_path))
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER, "Endpoint is too long for underlying protocol");
    }

    strcpy(sockaddr.sun_path, priv->endpoint);

    if (connect(sock, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) == -1)
    {
        switch (errno)
        {
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
        ASSOC_RAISE_ERROR(assoc, status, "%s", strerror(errno));
    }

    priv->fd = sock;

error:

    return status;
}

static
LWMsgStatus
lwmsg_connection_do_connect(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    
    switch (priv->mode)
    {
    case LWMSG_CONNECTION_MODE_LOCAL:
        BAIL_ON_ERROR(status = lwmsg_connection_connect_local(assoc));
        break;
    case LWMSG_CONNECTION_MODE_REMOTE:
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_UNIMPLEMENTED,
                          "Remote connections not presently supported");
    default:
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER,
                          "Unknown connection mode");
    }

error:

    return status;
}

LWMsgStatus
lwmsg_connection_run(
    LWMsgAssoc* assoc,
    ConnectionState target,
    ConnectionEvent event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    /* Run state machine until we reach our target state */
    do
    {
        /* Some events are always handled the same way regardless of
           state, so handle these here */
        switch (event)
        {
        case CONNECTION_EVENT_CLOSE:
            BAIL_ON_ERROR(status = lwmsg_connection_do_shutdown(assoc, CONNECTION_SHUTDOWN_CLOSE, CONNECTION_SHUTDOWN_NORMAL));
            goto done;
        case CONNECTION_EVENT_RESET:
            BAIL_ON_ERROR(status = lwmsg_connection_do_shutdown(assoc, CONNECTION_SHUTDOWN_RESET, CONNECTION_SHUTDOWN_NORMAL));
            goto done;
        case CONNECTION_EVENT_ABORT:
            BAIL_ON_ERROR(status = lwmsg_connection_do_shutdown(assoc, CONNECTION_SHUTDOWN_ABORT, CONNECTION_SHUTDOWN_NORMAL));
            goto done;
        default:
            break;
        }

        switch (priv->state)
        {
        case CONNECTION_STATE_START:
            /* If we were given a pre-connected fd, go straight to the connected state */
            if (priv->fd != -1)
            {
                priv->state = CONNECTION_STATE_CONNECTED;
            }
            /* If we were given an endpoint, we need to establish a connection */
            else if (priv->endpoint != NULL)
            {
                priv->state = CONNECTION_STATE_ESTABLISH;
            }
            /* Otherwise, we have a problem */
            else
            {
                ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_STATE,
                                  "Cannot initialize connection: no file descriptor or endpoint specified");
            }
            break;
        case CONNECTION_STATE_ESTABLISH:
            /* Attempt to establish connection */
            BAIL_ON_ERROR(status = lwmsg_connection_do_connect(assoc));
            priv->state = CONNECTION_STATE_CONNECTED;
            break;
        case CONNECTION_STATE_CONNECTED:
            /* Now that we are connected, perform handshake */
            priv->state = CONNECTION_STATE_SEND_GREETING;
            break;
        case CONNECTION_STATE_SEND_GREETING:
            /* Send greeting as part of handshake */
            BAIL_ON_ERROR(status = lwmsg_connection_send_greeting(assoc));
            priv->state = CONNECTION_STATE_RECV_GREETING;
            break;
        case CONNECTION_STATE_RECV_GREETING:
            /* Receive greeting as part of handshake */
            BAIL_ON_ERROR(status = lwmsg_connection_recv_greeting(assoc));
            priv->state = CONNECTION_STATE_IDLE;
            break;
        case CONNECTION_STATE_IDLE:
            switch (event)
            {
            case CONNECTION_EVENT_SEND:
                priv->state = CONNECTION_STATE_SEND_MESSAGE;
                BAIL_ON_ERROR(status = lwmsg_connection_do_send(assoc, CONNECTION_PACKET_MESSAGE));
                break;
            case CONNECTION_EVENT_RECV:
                priv->state = CONNECTION_STATE_RECV_MESSAGE;
                BAIL_ON_ERROR(status = lwmsg_connection_do_recv(assoc, CONNECTION_PACKET_MESSAGE));
                break;
            case CONNECTION_EVENT_NONE:
                /* Nothing to do, so return */
                goto done;
            default:
                ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER, "Invalid event: %i", event);
            }
            event = CONNECTION_EVENT_NONE;
            break;
        case CONNECTION_STATE_SEND_MESSAGE:
            switch (event)
            {
            case CONNECTION_EVENT_NONE:
                /* FIXME: to support async, we'll need resume incomplete operation here */
                goto done;
            case CONNECTION_EVENT_DONE:
                priv->state = CONNECTION_STATE_WAIT_RECV_REPLY;
                break;
            default:
                ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER, "Invalid event: %i", event);
            }
            event = CONNECTION_EVENT_NONE;
            break;
        case CONNECTION_STATE_RECV_MESSAGE:
            switch (event)
            {
            case CONNECTION_EVENT_NONE:
                /* FIXME: to support async, we'll need resume incomplete operation here */
                goto done;
            case CONNECTION_EVENT_DONE:
                priv->state = CONNECTION_STATE_WAIT_SEND_REPLY;
                break;
            default:
                ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER, "Invalid event: %i", event);
            }
            event = CONNECTION_EVENT_NONE;
            break;
        case CONNECTION_STATE_SEND_REPLY:
            switch (event)
            {
            case CONNECTION_EVENT_DONE:
                priv->state = CONNECTION_STATE_IDLE;
                break;
            case CONNECTION_EVENT_NONE:
                /* FIXME: to support async, we'll need resume incomplete operation here */
                goto done;
            default:
                ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER, "Invalid event: %i", event);
            }
            event = CONNECTION_EVENT_NONE;
            break;
        case CONNECTION_STATE_RECV_REPLY:
            switch (event)
            {
            case CONNECTION_EVENT_DONE:
                priv->state = CONNECTION_STATE_IDLE;
                break;
            case CONNECTION_EVENT_NONE:
                /* FIXME: to support async, we'll need resume incomplete operation here */
                goto done;
            default:
                ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER, "Invalid event: %i", event);
            }
            event = CONNECTION_EVENT_NONE;
            break;
        case CONNECTION_STATE_WAIT_SEND_REPLY:
            switch (event)
            {
            case CONNECTION_EVENT_SEND:
                priv->state = CONNECTION_STATE_SEND_REPLY;
                BAIL_ON_ERROR(status = lwmsg_connection_do_send(assoc, CONNECTION_PACKET_REPLY));
                break;
            case CONNECTION_EVENT_NONE:
                goto done;
            default:
                ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER, "Invalid event: %i", event);
            }
            event = CONNECTION_EVENT_NONE;
            break;
        case CONNECTION_STATE_WAIT_RECV_REPLY:
            switch (event)
            {
            case CONNECTION_EVENT_RECV:
                priv->state = CONNECTION_STATE_RECV_REPLY;
                BAIL_ON_ERROR(status = lwmsg_connection_do_recv(assoc, CONNECTION_PACKET_REPLY));
                break;
            case CONNECTION_EVENT_NONE:
                goto done;
            default:
                ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER, "Invalid event: %i", event);
            }
            event = CONNECTION_EVENT_NONE;
            break;
        case CONNECTION_STATE_CLOSED:
            switch (event)
            {
                /* If we are already closed, these events are no-ops */
            case CONNECTION_EVENT_CLOSE:
            case CONNECTION_EVENT_ABORT:
            case CONNECTION_EVENT_RESET:
                goto done;
            default:
                BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
            }
            break;
        default:
            ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_STATE, "Invalid state: %i\n", priv->state);
            break;
        }       
    } while (priv->state != target);

done:

    return status;

error:
    
    /* Transform generic EOF errors into PEER_CLOSE */
    if (status == LWMSG_STATUS_EOF)
    {
        status = LWMSG_STATUS_PEER_CLOSE;
    }

    goto done;
}
