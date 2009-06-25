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
 *        server-call.c
 *
 * Abstract:
 *
 *        Server call abstraction
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>

#include "server-private.h"
#include "call-private.h"
#include "util-private.h"

static
LWMsgStatus
lwmsg_server_call_transact(
    LWMsgCall* call,
    LWMsgCompleteFunction complete,
    void* data
    )
{
    return LWMSG_STATUS_UNSUPPORTED;
}

static
LWMsgStatus
lwmsg_server_call_pend(
    LWMsgCall* call,
    LWMsgCancelFunction cancel,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerCall* scall = SERVER_CALL(call);

    pthread_mutex_lock(&scall->lock);

    if (scall->state != SERVER_CALL_TRANSACTING)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    scall->state = SERVER_CALL_PENDING;
    scall->cancel = cancel;
    scall->cancel_data = data;

error:

    pthread_mutex_unlock(&scall->lock);

    return status;
}

static
LWMsgStatus
lwmsg_server_call_complete(
    LWMsgCall* call,
    LWMsgStatus call_status
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerCall* scall = SERVER_CALL(call);
    LWMsgBool locked = LWMSG_FALSE;

    pthread_mutex_lock(&scall->lock);
    locked = LWMSG_TRUE;

    if (scall->state != SERVER_CALL_PENDING &&
        scall->state != SERVER_CALL_CANCELED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    scall->state = SERVER_CALL_COMPLETE;
    scall->status = call_status;

    pthread_mutex_unlock(&scall->lock);
    locked = LWMSG_FALSE;

    /* Poke the thread owning the call so it wakes up and notices the completion */
    pthread_mutex_lock(&scall->owner->lock);
    scall->owner->num_events++;
    lwmsg_server_signal_io_thread(scall->owner);
    pthread_mutex_unlock(&scall->owner->lock);

error:

    if (locked)
    {
        pthread_mutex_unlock(&scall->lock);
        locked = LWMSG_FALSE;
    }

    return status;
}

static
LWMsgStatus
lwmsg_server_call_cancel(
    LWMsgCall* call
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerCall* scall = SERVER_CALL(call);

    pthread_mutex_lock(&scall->lock);

    if (scall->state != SERVER_CALL_PENDING &&
        scall->state != SERVER_CALL_COMPLETE)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    if (scall->state == SERVER_CALL_PENDING)
    {
        scall->state = SERVER_CALL_CANCELED;

        BAIL_ON_ERROR(status = scall->cancel(call, scall->cancel_data));
    }

error:

    pthread_mutex_unlock(&scall->lock);

    return status;
}

static
void
lwmsg_server_call_release(
    LWMsgCall* call
    )
{
    ServerCall* scall = SERVER_CALL(call);

    scall->state = SERVER_CALL_IDLE;
}

static LWMsgCallClass server_call_class =
{
    .release = lwmsg_server_call_release,
    .transact = lwmsg_server_call_transact,
    .pend = lwmsg_server_call_pend,
    .complete = lwmsg_server_call_complete,
    .cancel = lwmsg_server_call_cancel
};

LWMsgStatus
lwmsg_server_call_init(
    ServerCall* call
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    memset(call, 0, sizeof(*call));

    call->base.vtbl = &server_call_class;
    call->state = SERVER_CALL_IDLE;

    if (pthread_mutex_init(&call->lock, NULL) < 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

error:

    return status;
}

void
lwmsg_server_call_destroy(
    ServerCall* call
    )
{
    pthread_mutex_destroy(&call->lock);
}

LWMsgSession*
lwmsg_server_call_get_session(
    LWMsgCall* call
    )
{
    ServerCall* my_call = SERVER_CALL(call);

    return my_call->session;
}
