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

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>

static inline
LWMsgStatus
lwmsg_connection_state_start(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_begin_connect(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_finish_connect(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_begin_send_handshake(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_finish_send_handshake(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_begin_recv_handshake(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_finish_recv_handshake(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_idle(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_begin_send_message(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_finish_send_message(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_begin_recv_message(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_finish_recv_message(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_begin_close(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_finish_close(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_begin_reset(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_finish_reset(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_closed(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static inline
LWMsgStatus
lwmsg_connection_state_error(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    );

static
LWMsgStatus
lwmsg_connection_finish_close(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_run(
    LWMsgAssoc* assoc,
    ConnectionEvent event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    ConnectionState state = priv->state;

    /* Run state machine until there is nothing to do */
    while (event != CONNECTION_EVENT_NONE)
    {
        switch (state)
        {
        case CONNECTION_STATE_START:
            BAIL_ON_ERROR(status = lwmsg_connection_state_start(assoc, &state, &event));
            break;
        case CONNECTION_STATE_BEGIN_CONNECT:
            BAIL_ON_ERROR(status = lwmsg_connection_state_begin_connect(assoc, &state, &event));
            break;
        case CONNECTION_STATE_FINISH_CONNECT:
            BAIL_ON_ERROR(status = lwmsg_connection_state_finish_connect(assoc, &state, &event));
            break;
        case CONNECTION_STATE_BEGIN_SEND_HANDSHAKE:
            BAIL_ON_ERROR(status = lwmsg_connection_state_begin_send_handshake(assoc, &state, &event));
            break;
        case CONNECTION_STATE_FINISH_SEND_HANDSHAKE:
            BAIL_ON_ERROR(status = lwmsg_connection_state_finish_send_handshake(assoc, &state, &event));
            break;
        case CONNECTION_STATE_BEGIN_RECV_HANDSHAKE:
            BAIL_ON_ERROR(status = lwmsg_connection_state_begin_recv_handshake(assoc, &state, &event));
            break;
        case CONNECTION_STATE_FINISH_RECV_HANDSHAKE:
            BAIL_ON_ERROR(status = lwmsg_connection_state_finish_recv_handshake(assoc, &state, &event));
            break;
        case CONNECTION_STATE_IDLE:
            BAIL_ON_ERROR(status = lwmsg_connection_state_idle(assoc, &state, &event));
            break;
        case CONNECTION_STATE_BEGIN_SEND_MESSAGE:
            BAIL_ON_ERROR(status = lwmsg_connection_state_begin_send_message(assoc, &state, &event));
            break;
        case CONNECTION_STATE_FINISH_SEND_MESSAGE:
            BAIL_ON_ERROR(status = lwmsg_connection_state_finish_send_message(assoc, &state, &event));
            break;
        case CONNECTION_STATE_BEGIN_RECV_MESSAGE:
            BAIL_ON_ERROR(status = lwmsg_connection_state_begin_recv_message(assoc, &state, &event));
            break;
        case CONNECTION_STATE_FINISH_RECV_MESSAGE:
            BAIL_ON_ERROR(status = lwmsg_connection_state_finish_recv_message(assoc, &state, &event));
            break;
        case CONNECTION_STATE_BEGIN_CLOSE:
            BAIL_ON_ERROR(status = lwmsg_connection_state_begin_close(assoc, &state, &event));
            break;
        case CONNECTION_STATE_FINISH_CLOSE:
            BAIL_ON_ERROR(status = lwmsg_connection_state_finish_close(assoc, &state, &event));
            break;
        case CONNECTION_STATE_BEGIN_RESET:
            BAIL_ON_ERROR(status = lwmsg_connection_state_begin_reset(assoc, &state, &event));
            break;
        case CONNECTION_STATE_FINISH_RESET:
            BAIL_ON_ERROR(status = lwmsg_connection_state_finish_reset(assoc, &state, &event));
            break;
        case CONNECTION_STATE_CLOSED:
            BAIL_ON_ERROR(status = lwmsg_connection_state_closed(assoc, &state, &event));
            break;
        case CONNECTION_STATE_ERROR:
            BAIL_ON_ERROR(status = lwmsg_connection_state_error(assoc, &state, &event));
            break;
        case CONNECTION_STATE_NONE:
            BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
        }
    }

done:

    priv->state = state;

    return status;

error:

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
    case LWMSG_STATUS_PENDING:
    case LWMSG_STATUS_BUSY:
        break;
    default:
        state = CONNECTION_STATE_ERROR;
        break;
    }

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_start(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    switch (*event)
    {
    case CONNECTION_EVENT_ESTABLISH:
        if (priv->fd != -1)
        {
            /* Set up pre-connected fd */
            BAIL_ON_ERROR(status = lwmsg_connection_connect_existing(assoc));

            /* Go straight to the handshake */
            *state = CONNECTION_STATE_BEGIN_SEND_HANDSHAKE;
            *event = CONNECTION_EVENT_FINISH;
        }
        else if (priv->endpoint != NULL)
        {
            /* If we were given an endpoint, we need to establish a connection */
            *state = CONNECTION_STATE_BEGIN_CONNECT;
            *event = CONNECTION_EVENT_FINISH;
        }
        else
        {
            /* Otherwise, we have a problem */
            ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_STATE,
                              "Cannot initialize connection: no file descriptor or endpoint specified");
        }
        break;
    default:
        /* We can't do anything else until we have established the connection */
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
        break;
    }

done:

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_begin_connect(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_connection_begin_connect(assoc));

done:

    if (status == LWMSG_STATUS_SUCCESS || status == LWMSG_STATUS_PENDING)
    {
        *state = CONNECTION_STATE_FINISH_CONNECT;
        *event = CONNECTION_EVENT_FINISH;
    }

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_finish_connect(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    switch (*event)
    {
    case CONNECTION_EVENT_FINISH:
        BAIL_ON_ERROR(status = lwmsg_connection_finish_connect(assoc));
        *state = CONNECTION_STATE_BEGIN_SEND_HANDSHAKE;
        break;
    case CONNECTION_EVENT_CLOSE:
        *state = CONNECTION_STATE_BEGIN_CLOSE;
        break;
    case CONNECTION_EVENT_RESET:
        *state = CONNECTION_STATE_BEGIN_RESET;
        break;
    case CONNECTION_EVENT_ABORT:
        *state = CONNECTION_STATE_BEGIN_CLOSE;
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_BUSY);
        break;
    }

done:

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_begin_send_handshake(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_connection_begin_send_handshake(assoc));

done:

    if (status == LWMSG_STATUS_SUCCESS || status == LWMSG_STATUS_PENDING)
    {
        *state = CONNECTION_STATE_FINISH_SEND_HANDSHAKE;
        *event = CONNECTION_EVENT_FINISH;
    }

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_finish_send_handshake(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    switch (*event)
    {
    case CONNECTION_EVENT_FINISH:
        BAIL_ON_ERROR(status = lwmsg_connection_finish_send_handshake(assoc));
        *state = CONNECTION_STATE_BEGIN_RECV_HANDSHAKE;
        break;
    case CONNECTION_EVENT_CLOSE:
        *state = CONNECTION_STATE_BEGIN_CLOSE;
        break;
    case CONNECTION_EVENT_RESET:
        *state = CONNECTION_STATE_BEGIN_RESET;
        break;
    case CONNECTION_EVENT_ABORT:
        *state = CONNECTION_STATE_BEGIN_CLOSE;
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_BUSY);
        break;
    }

done:

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_begin_recv_handshake(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_connection_begin_recv_handshake(assoc));

done:

    if (status == LWMSG_STATUS_SUCCESS || status == LWMSG_STATUS_PENDING)
    {
        *state = CONNECTION_STATE_FINISH_RECV_HANDSHAKE;
        *event = CONNECTION_EVENT_FINISH;
    }

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_finish_recv_handshake(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    switch (*event)
    {
    case CONNECTION_EVENT_FINISH:
        BAIL_ON_ERROR(status = lwmsg_connection_finish_recv_handshake(assoc));
        *state = CONNECTION_STATE_IDLE;
        *event = CONNECTION_EVENT_NONE;
        break;
    case CONNECTION_EVENT_CLOSE:
        *state = CONNECTION_STATE_BEGIN_CLOSE;
        break;
    case CONNECTION_EVENT_RESET:
        *state = CONNECTION_STATE_BEGIN_RESET;
        break;
    case CONNECTION_EVENT_ABORT:
        *state = CONNECTION_STATE_BEGIN_CLOSE;
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_BUSY);
    }

done:

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_idle(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    switch (*event)
    {
    case CONNECTION_EVENT_NONE:
        BAIL_ON_ERROR(status = LWMSG_STATUS_INTERNAL);
        break;
    case CONNECTION_EVENT_ESTABLISH:
        *event = CONNECTION_EVENT_NONE;
        break;
    case CONNECTION_EVENT_FINISH:
        /* Check for an urgent packet */
        BAIL_ON_ERROR(status = lwmsg_connection_check(assoc));
        *event = CONNECTION_EVENT_NONE;
        break;
    case CONNECTION_EVENT_SEND:
        *state = CONNECTION_STATE_BEGIN_SEND_MESSAGE;
        break;
    case CONNECTION_EVENT_RECV:
        *state = CONNECTION_STATE_BEGIN_RECV_MESSAGE;
        break;
    case CONNECTION_EVENT_CLOSE:
        *state = CONNECTION_STATE_BEGIN_CLOSE;
        break;
    case CONNECTION_EVENT_RESET:
        *state = CONNECTION_STATE_BEGIN_RESET;
        break;
    case CONNECTION_EVENT_ABORT:
        *state = CONNECTION_STATE_BEGIN_CLOSE;
        break;
    }

done:

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_begin_send_message(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_connection_begin_send_message(assoc));

done:

    if (status == LWMSG_STATUS_SUCCESS || status == LWMSG_STATUS_PENDING)
    {
        *state = CONNECTION_STATE_FINISH_SEND_MESSAGE;
        *event = CONNECTION_EVENT_FINISH;
    }

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_finish_send_message(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    switch (*event)
    {
    case CONNECTION_EVENT_FINISH:
        BAIL_ON_ERROR(status = lwmsg_connection_finish_send_message(assoc));
        *state = CONNECTION_STATE_IDLE;
        *event = CONNECTION_EVENT_NONE;
        break;
    case CONNECTION_EVENT_CLOSE:
        *state = CONNECTION_STATE_BEGIN_CLOSE;
        break;
    case CONNECTION_EVENT_RESET:
        *state = CONNECTION_STATE_BEGIN_RESET;
        break;
    case CONNECTION_EVENT_ABORT:
        *state = CONNECTION_STATE_BEGIN_CLOSE;
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_BUSY);
        break;
    }

done:

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_begin_recv_message(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_connection_begin_recv_message(assoc));

done:

    if (status == LWMSG_STATUS_SUCCESS || status == LWMSG_STATUS_PENDING)
    {
        *state = CONNECTION_STATE_FINISH_RECV_MESSAGE;
        *event = CONNECTION_EVENT_FINISH;
    }

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_finish_recv_message(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    switch (*event)
    {
    case CONNECTION_EVENT_FINISH:
        BAIL_ON_ERROR(status = lwmsg_connection_finish_recv_message(assoc));
        *state = CONNECTION_STATE_IDLE;
        *event = CONNECTION_EVENT_NONE;
        break;
    case CONNECTION_EVENT_CLOSE:
        *state = CONNECTION_STATE_BEGIN_CLOSE;
        break;
    case CONNECTION_EVENT_RESET:
        *state = CONNECTION_STATE_BEGIN_RESET;
        break;
    case CONNECTION_EVENT_ABORT:
        *state = CONNECTION_STATE_BEGIN_CLOSE;
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_BUSY);
        break;
    }

done:

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_begin_close(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    if (priv->fd != -1)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_begin_send_shutdown(assoc, LWMSG_STATUS_PEER_CLOSE));
    }

done:

    if (status == LWMSG_STATUS_SUCCESS || status == LWMSG_STATUS_PENDING)
    {
        *state = CONNECTION_STATE_FINISH_CLOSE;
        *event = CONNECTION_EVENT_FINISH;
    }

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_finish_close(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    switch (*event)
    {
    case CONNECTION_EVENT_FINISH:
        if (priv->fd != -1)
        {
            status = lwmsg_connection_finish_send_shutdown(assoc);

            switch (status)
            {
            case LWMSG_STATUS_PEER_CLOSE:
            case LWMSG_STATUS_PEER_ABORT:
            case LWMSG_STATUS_PEER_RESET:
                /* Don't raise an error if the peer beat us to closing the connection */
                status = LWMSG_STATUS_SUCCESS;
                break;
            default:
                BAIL_ON_ERROR(status);
                break;
            }
        }

        BAIL_ON_ERROR(status = lwmsg_connection_finish_close(assoc));

        *state = CONNECTION_STATE_CLOSED;
        *event = CONNECTION_EVENT_NONE;
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_BUSY);
    }

done:

    return status;

error:

    goto done;
}


static inline
LWMsgStatus
lwmsg_connection_state_begin_reset(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = lwmsg_connection_begin_send_shutdown(assoc, LWMSG_STATUS_PEER_RESET));

done:

    if (status == LWMSG_STATUS_SUCCESS || status == LWMSG_STATUS_PENDING)
    {
        *state = CONNECTION_STATE_FINISH_RESET;
        *event = CONNECTION_EVENT_FINISH;
    }

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_finish_reset(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    switch (*event)
    {
    case CONNECTION_EVENT_FINISH:
        status = lwmsg_connection_finish_send_shutdown(assoc);

        switch (status)
        {
        case LWMSG_STATUS_PEER_CLOSE:
        case LWMSG_STATUS_PEER_ABORT:
        case LWMSG_STATUS_PEER_RESET:
            /* Don't raise an error if the peer beat us to closing the connection */
            status = LWMSG_STATUS_SUCCESS;
            break;
        default:
            BAIL_ON_ERROR(status);
            break;
        }

        BAIL_ON_ERROR(status = lwmsg_connection_finish_close(assoc));

        *state = CONNECTION_STATE_START;
        *event = CONNECTION_EVENT_NONE;
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_BUSY);
    }

done:

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_closed(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    switch (*event)
    {
    case CONNECTION_EVENT_FINISH:
        *event = CONNECTION_EVENT_NONE;
        break;
    case CONNECTION_EVENT_RESET:
        *state = CONNECTION_STATE_START;
        *event = CONNECTION_EVENT_NONE;
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

done:

    return status;

error:

    goto done;
}

static inline
LWMsgStatus
lwmsg_connection_state_error(
    LWMsgAssoc* assoc,
    ConnectionState* state,
    ConnectionEvent* event
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    switch(*event)
    {
    case CONNECTION_EVENT_CLOSE:
        *state = CONNECTION_STATE_BEGIN_CLOSE;
        break;
    case CONNECTION_EVENT_RESET:
        *state = CONNECTION_STATE_BEGIN_RESET;
        break;
    case CONNECTION_EVENT_ABORT:
        *state = CONNECTION_STATE_BEGIN_CLOSE;
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
lwmsg_connection_finish_close(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    LWMsgSessionManager* manager = NULL;

    if (priv->fd != -1)
    {
        close(priv->fd);
        priv->fd = -1;
    }

    if (priv->sec_token)
    {
        lwmsg_security_token_delete(priv->sec_token);
        priv->sec_token = NULL;
    }

    if (priv->session)
    {
        BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));

        BAIL_ON_ERROR(status = lwmsg_session_manager_leave_session(manager, priv->session));

        priv->session = NULL;
    }

    lwmsg_connection_buffer_destruct(&priv->recvbuffer);
    BAIL_ON_ERROR(status = lwmsg_connection_buffer_construct(&priv->recvbuffer));
    lwmsg_connection_buffer_destruct(&priv->sendbuffer);
    BAIL_ON_ERROR(status = lwmsg_connection_buffer_construct(&priv->sendbuffer));

error:

    return status;
}
