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

void
lwmsg_server_call_setup(
    ServerCall* call,
    ServerIoThread* owner,
    LWMsgDispatchSpec* spec,
    void* dispatch_data
    )
{
    call->owner = owner;
    call->spec = spec;
    call->dispatch_data = dispatch_data;
    call->state = SERVER_CALL_NONE;
    call->status = LWMSG_STATUS_SUCCESS;
    call->cancel = NULL;
}

static
LWMsgStatus
lwmsg_server_call_transact(
    LWMsgCall* call,
    LWMsgCompleteFunction complete,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerCall* scall = SERVER_CALL(call);

    if (!scall->spec->data)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_UNIMPLEMENTED);
    }

    switch (scall->spec->type)
    {
    case LWMSG_DISPATCH_TYPE_OLD:
        status = ((LWMsgAssocDispatchFunction) scall->spec->data) (
            scall->assoc,
            &scall->incoming,
            &scall->outgoing,
            scall->dispatch_data);
        break;
    case LWMSG_DISPATCH_TYPE_BLOCK:
    case LWMSG_DISPATCH_TYPE_NONBLOCK:
        status = ((LWMsgServerCallFunction) scall->spec->data) (
            call,
            &scall->incoming,
            &scall->outgoing,
            scall->dispatch_data);
        break;
    default:
        status = LWMSG_STATUS_INTERNAL;
        break;
    }

    pthread_mutex_lock(&scall->lock);
    scall->state |= SERVER_CALL_DISPATCHED;

    if ((scall->state & SERVER_CALL_CANCELLED) &&
        (scall->state & SERVER_CALL_PENDED) &&
        !(scall->state & SERVER_CALL_COMPLETED))
    {
        scall->cancel(call, scall->cancel_data);
    }

    pthread_mutex_unlock(&scall->lock);

error:

    return status;
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

    scall->state |= SERVER_CALL_PENDED;
    scall->cancel = cancel;
    scall->cancel_data = data;

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

    pthread_mutex_lock(&scall->lock);

    scall->state |= SERVER_CALL_COMPLETED;
    scall->status = call_status;

    /* Wake the IO thread that owns the call.

       We need to hold the thread lock and the call lock at the same time.
       Otherwise, during a server shutdown, an IO thread that is already awake
       might notice the call is complete, exit, and be freed before we can
       signal it */
    pthread_mutex_lock(&scall->owner->lock);
    lwmsg_server_signal_io_thread(scall->owner);
    pthread_mutex_unlock(&scall->owner->lock);
    pthread_mutex_unlock(&scall->lock);

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
    scall->state |= SERVER_CALL_CANCELLED;

    /* If the dispatch function is not finished running,
       we don't call the cancel callback right now --
       lwmsg_server_call_transact() will handle it.

       If the call is already completed, we silently
       ignore the cancel request and return success */

    if ((scall->state & SERVER_CALL_DISPATCHED) &&
        !(scall->state & SERVER_CALL_COMPLETED))
    {
        scall->cancel(call, scall->cancel_data);
    }

    pthread_mutex_unlock(&scall->lock);

    return status;
}

static
void
lwmsg_server_call_release(
    LWMsgCall* call
    )
{
    /* The call block is allocated as part of an ServerIoTask,
       so we do nothing here. */
    return;
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
    pthread_mutexattr_t mutex_attr;

    memset(call, 0, sizeof(*call));

    call->base.vtbl = &server_call_class;

    if (pthread_mutexattr_init(&mutex_attr))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    if (pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    if (pthread_mutex_init(&call->lock, &mutex_attr) < 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    lwmsg_message_init(&call->incoming);
    lwmsg_message_init(&call->outgoing);
    lwmsg_ring_init(&call->ring);

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
