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
 *        connection.c
 *
 * Abstract:
 *
 *        Connection API
 *        Primary entry points
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "assoc-private.h"
#include "connection-private.h"
#include "util-private.h"
#include "convert.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

static LWMsgStatus
lwmsg_connection_construct(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    BAIL_ON_ERROR(status = lwmsg_connection_construct_buffer(&priv->sendbuffer));
    BAIL_ON_ERROR(status = lwmsg_connection_construct_buffer(&priv->recvbuffer));

    priv->packet_size = 2048;
    priv->fd = -1;
    priv->mode = LWMSG_CONNECTION_MODE_NONE;

error:

    return status;
}

static void
lwmsg_connection_destruct(
    LWMsgAssoc* assoc
    )
{
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    LWMsgSessionManager* manager = NULL;

    lwmsg_connection_destruct_buffer(&priv->sendbuffer);
    lwmsg_connection_destruct_buffer(&priv->recvbuffer);

    if (priv->fd != -1)
    {
        close(priv->fd);
        priv->fd = -1;
    }

    if (priv->endpoint)
    {
        free(priv->endpoint);
        priv->endpoint = NULL;
    }

    if (priv->sec_token)
    {
        lwmsg_security_token_delete(priv->sec_token);
        priv->sec_token = NULL;
    }

    if (priv->session)
    {
        if (lwmsg_assoc_get_session_manager(assoc, &manager))
        {
            /* This should never happen, really */
            abort();
        }

        if (lwmsg_session_manager_leave_session(manager, priv->session))
        {
            /* Neither should this */
            abort();
        }

        priv->session = NULL;
    }
}


static LWMsgStatus
lwmsg_connection_set_end_time(
    ConnectionPrivate* priv,
    LWMsgTime* timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTime now;

    if (timeout)
    {
        priv->timeout_set = 1;
        BAIL_ON_ERROR(status = lwmsg_time_now(&now));
        lwmsg_time_sum(&now, timeout, &priv->end_time);
    }
    else
    {
        priv->timeout_set = 0;
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_connection_close(
    LWMsgAssoc* assoc,
    LWMsgTime* timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    lwmsg_connection_set_end_time(priv, timeout);
    BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_STATE_NONE, CONNECTION_EVENT_CLOSE));

error:

    return status;
}

static LWMsgStatus
lwmsg_connection_reset(
    LWMsgAssoc* assoc,
    LWMsgTime* timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    lwmsg_connection_set_end_time(priv, timeout);
    BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_STATE_NONE, CONNECTION_EVENT_RESET));

error:

    return status;
}

static LWMsgStatus
lwmsg_connection_send_msg(
    LWMsgAssoc* assoc,
    LWMsgMessage* message,
    LWMsgTime* timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    
    priv->message = message;
    
    lwmsg_connection_set_end_time(priv, timeout);

    BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_STATE_NONE, CONNECTION_EVENT_SEND));

error:

    priv->message = NULL;

    return status;
}

static LWMsgStatus
lwmsg_connection_recv_msg(
    LWMsgAssoc* assoc,
    LWMsgMessage* message,
    LWMsgTime* timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    
    priv->message = message;

    lwmsg_connection_set_end_time(priv, timeout);

    BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_STATE_NONE, CONNECTION_EVENT_RECV));

error:

    priv->message = NULL;

    return status;
}

static LWMsgStatus
lwmsg_connection_get_peer_security_token(
    LWMsgAssoc* assoc,
    LWMsgSecurityToken** out_token
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    
    if (!priv->sec_token)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }
    
    *out_token = priv->sec_token;

error:

    return status;
}

static LWMsgStatus
lwmsg_connection_get_session(
    LWMsgAssoc* assoc,
    LWMsgSession** session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);
    
    if (!priv->session)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }
    
    *session = priv->session;

error:

    return status;
}
    
static LWMsgAssocState
lwmsg_connection_get_state(
    LWMsgAssoc* assoc
    )
{
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    switch (priv->state)
    {
    case CONNECTION_STATE_START:
        if (priv->fd != -1 || priv->endpoint != NULL)
        {
            return LWMSG_ASSOC_STATE_READY_SEND_RECV;
        }
        else
        {
            return LWMSG_ASSOC_STATE_NOT_READY;
        }
    case CONNECTION_STATE_IDLE:
        return LWMSG_ASSOC_STATE_READY_SEND_RECV;
    case CONNECTION_STATE_WAIT_SEND_REPLY:
        return LWMSG_ASSOC_STATE_READY_SEND;
    case CONNECTION_STATE_WAIT_RECV_REPLY:
        return LWMSG_ASSOC_STATE_READY_RECV;
    case CONNECTION_STATE_LOCAL_CLOSED:
        return LWMSG_ASSOC_STATE_LOCAL_CLOSED;
    case CONNECTION_STATE_LOCAL_ABORTED:
        return LWMSG_ASSOC_STATE_LOCAL_ABORTED;
    case CONNECTION_STATE_PEER_CLOSED:
        return LWMSG_ASSOC_STATE_PEER_CLOSED;
    case CONNECTION_STATE_PEER_ABORTED:
        return LWMSG_ASSOC_STATE_PEER_ABORTED;
    case CONNECTION_STATE_PEER_RESET:
        return LWMSG_ASSOC_STATE_PEER_RESET;
    default:
        return LWMSG_ASSOC_STATE_IN_PROGRESS;
    }
}

static LWMsgAssocClass connection_class =
{
    .private_size = sizeof(ConnectionPrivate),
    .construct = lwmsg_connection_construct,
    .destruct = lwmsg_connection_destruct,
    .send_msg = lwmsg_connection_send_msg,
    .recv_msg = lwmsg_connection_recv_msg,
    .close = lwmsg_connection_close,
    .reset = lwmsg_connection_reset,
    .get_peer_security_token = lwmsg_connection_get_peer_security_token,
    .get_session = lwmsg_connection_get_session,
    .get_state = lwmsg_connection_get_state
};

LWMsgStatus
lwmsg_connection_new(
    LWMsgProtocol* prot,
    LWMsgAssoc** out_assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;
    ConnectionPrivate* priv = NULL;

    BAIL_ON_ERROR(status = lwmsg_assoc_new(prot, &connection_class, &assoc));

    priv = lwmsg_assoc_get_private(assoc);
    priv->state = CONNECTION_STATE_START;

    *out_assoc = assoc;

done:

    return status;

error:

    goto done;
}

LWMsgStatus
lwmsg_connection_set_packet_size(
    LWMsgAssoc* assoc,
    size_t size
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    if (priv->session)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_STATE, "Cannot change packet size of established connection");
    }

    priv->packet_size = size;

error:

    return status;
}

LWMsgStatus
lwmsg_connection_set_fd(
    LWMsgAssoc* assoc,
    LWMsgConnectionMode mode,
    int fd
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    if (fd < 0)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_PARAMETER, "Invalid file descriptor");
    }

    if (priv->fd != -1 || priv->endpoint != NULL)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_STATE, "Connection parameters already set");
    }

    priv->fd = fd;
    priv->mode = mode;

error:

    return status;
}

LWMsgStatus
lwmsg_connection_set_endpoint(
    LWMsgAssoc* assoc,
    LWMsgConnectionMode mode,
    const char* endpoint
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    if (priv->fd != -1 || priv->endpoint != NULL)
    {
        ASSOC_RAISE_ERROR(assoc, status = LWMSG_STATUS_INVALID_STATE, "Connection parameters already set");
    }

    priv->endpoint = strdup(endpoint);
    
    if (!priv->endpoint)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    priv->mode = mode;

error:

    return status;
}

LWMsgStatus
lwmsg_connection_set_interrupt_signal(
    LWMsgAssoc* assoc,
    LWMsgConnectionSignal* signal
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);  

    priv->interrupt = signal;

    return status;
}

LWMsgStatus
lwmsg_connection_signal_new(
    LWMsgConnectionSignal** out_signal
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgConnectionSignal* signal = NULL;

    signal = calloc(1, sizeof(*signal));

    if (signal == NULL)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    if (pipe(signal->fd))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    *out_signal = signal;

error:

    return status;
}

LWMsgStatus
lwmsg_connection_signal_raise(
    LWMsgConnectionSignal* signal
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    char c = 0;
    int ret = 0;

    ret = write(signal->fd[1], &c, 1);

    if (ret < 1)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

error:

    return status;
}

LWMsgStatus
lwmsg_connection_signal_lower(
    LWMsgConnectionSignal* signal
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    char c = 0;
    int ret = 0;

    ret = read(signal->fd[0], &c, 1);

    if (ret < 1)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

error:

    return status;
}

void
lwmsg_connection_signal_delete(
    LWMsgConnectionSignal* signal
    )
{
    close(signal->fd[0]);
    close(signal->fd[1]);

    free(signal);
}

LWMsgStatus
lwmsg_connection_establish(
    LWMsgAssoc* assoc
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionPrivate* priv = lwmsg_assoc_get_private(assoc);

    if (!priv->session)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_run(assoc, CONNECTION_STATE_NONE, CONNECTION_EVENT_NONE));
    }

error:

     return status;
}
