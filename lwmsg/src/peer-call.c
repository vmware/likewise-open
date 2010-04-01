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
 *        peer-call.c
 *
 * Abstract:
 *
 *        Peer call abstraction
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>

#include "peer-private.h"
#include "call-private.h"
#include "util-private.h"

static
LWMsgStatus
lwmsg_peer_call_complete_incoming(
    LWMsgCall* call,
    LWMsgStatus call_status
    );

static
void
lwmsg_peer_call_thunk(
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerCall* call = (PeerCall*) data;
    LWMsgMessage incoming_message = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage outgoing_message = LWMSG_MESSAGE_INITIALIZER;

    switch (call->params.incoming.spec->type)
    {
    case LWMSG_DISPATCH_TYPE_OLD:
        incoming_message.tag = call->params.incoming.in.tag;
        incoming_message.data = call->params.incoming.in.data;
        status = ((LWMsgAssocDispatchFunction) call->params.incoming.spec->data) (
            call->task->assoc,
            &incoming_message,
            &outgoing_message,
            call->params.incoming.dispatch_data);
        call->params.incoming.out.tag = outgoing_message.tag;
        call->params.incoming.out.data = outgoing_message.data;
        break;
    case LWMSG_DISPATCH_TYPE_BLOCK:
    case LWMSG_DISPATCH_TYPE_NONBLOCK:
        status = ((LWMsgPeerCallFunction) call->params.incoming.spec->data) (
            LWMSG_CALL(call),
            &call->params.incoming.in,
            &call->params.incoming.out,
            call->params.incoming.dispatch_data);

        pthread_mutex_lock(&call->task->call_lock);
        call->state |= PEER_CALL_DISPATCHED;

        if (call->state & PEER_CALL_COMPLETED)
        {
            /* The call was already completed */
            lwmsg_task_wake(call->task->event_task);
        }
        else if ((call->state & PEER_CALL_CANCELLED) &&
                 (call->state & PEER_CALL_PENDED))
        {
            /* The call was already cancelled */
            call->params.incoming.cancel(LWMSG_CALL(call), call->params.incoming.cancel_data);
        }

        pthread_mutex_unlock(&call->task->call_lock);
        break;
    default:
        status = LWMSG_STATUS_INTERNAL;
        break;
    }

    switch (status)
    {
    case LWMSG_STATUS_PENDING:
        /* Callee will asynchronously complete for us if it hasn't already */
        break;
    default:
        /* Manually invoke complete to wake up IO task */
        lwmsg_peer_call_complete_incoming(LWMSG_CALL(call), status);
        break;
    }
}

LWMsgStatus
lwmsg_peer_call_dispatch_incoming(
    PeerCall* call,
    LWMsgDispatchSpec* spec,
    void* dispatch_data,
    LWMsgMessage* incoming_message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    call->params.incoming.spec = spec;
    call->params.incoming.dispatch_data = dispatch_data;
    call->state = PEER_CALL_NONE;
    call->cookie = incoming_message->cookie;

    if (!spec->data)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_UNIMPLEMENTED);
    }

    switch (call->params.incoming.spec->type)
    {
    case LWMSG_DISPATCH_TYPE_OLD:
        BAIL_ON_ERROR(status = lwmsg_task_dispatch_work_item(
                          call->task->peer->task_manager,
                          lwmsg_peer_call_thunk,
                          call));
        status = LWMSG_STATUS_PENDING;
        break;
    case LWMSG_DISPATCH_TYPE_BLOCK:
    case LWMSG_DISPATCH_TYPE_NONBLOCK:
        call->params.incoming.in.tag = incoming_message->tag;
        call->params.incoming.in.data = incoming_message->data;
        call->params.incoming.out.tag = LWMSG_TAG_INVALID;
        call->params.incoming.out.data = NULL;
        if (call->params.incoming.spec->type == LWMSG_DISPATCH_TYPE_BLOCK)
        {
            BAIL_ON_ERROR(status = lwmsg_task_dispatch_work_item(
                              call->task->peer->task_manager,
                              lwmsg_peer_call_thunk,
                              call));
            status = LWMSG_STATUS_PENDING;
        }
        else
        {
            status = ((LWMsgPeerCallFunction) call->params.incoming.spec->data) (
                LWMSG_CALL(call),
                &call->params.incoming.in,
                &call->params.incoming.out,
                call->params.incoming.dispatch_data);

            call->state |= PEER_CALL_DISPATCHED;

            switch (status)
            {
            case LWMSG_STATUS_PENDING:
                if (call->state & PEER_CALL_COMPLETED)
                {
                    /* The call was completed before we even returned from
                       the dispatch function */
                    status = call->status;
                }
                break;
            default:
                call->status = status;
                call->state |= PEER_CALL_COMPLETED;
                break;
            }
        }
        break;
    default:
        status = LWMSG_STATUS_INTERNAL;
        break;
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_peer_call_dispatch_outgoing(
    LWMsgCall* call,
    const LWMsgParams* input,
    LWMsgParams* output,
    LWMsgCompleteFunction complete,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerCall* pcall = PEER_CALL(call);

    pthread_mutex_lock(&pcall->task->call_lock);

    BAIL_ON_ERROR(status = pcall->task->status);

    /* Are we out of cookie slots? */
    if (lwmsg_hash_get_count(&pcall->task->outgoing_calls) == ((LWMsgCookie) -1) + 1)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_BUSY);
    }

    pcall->state = PEER_CALL_NONE;
    pcall->params.outgoing.complete = complete;
    pcall->params.outgoing.complete_data = data;
    pcall->params.outgoing.in = input;
    pcall->params.outgoing.out = output;
    pcall->status = LWMSG_STATUS_SUCCESS;

    /* Find an unused cookie */
    while (lwmsg_hash_find_key(&pcall->task->outgoing_calls, (const void*) &pcall->task->next_cookie))
    {
        pcall->task->next_cookie++;
    }

    pcall->cookie = pcall->task->next_cookie++;

    lwmsg_hash_insert_entry(&pcall->task->outgoing_calls, pcall);

    lwmsg_task_wake(pcall->task->event_task);

    if (!complete)
    {
        while (!(pcall->state & PEER_CALL_COMPLETED))
        {
            pthread_cond_wait(&pcall->task->call_event, &pcall->task->call_lock);
        }

        status = pcall->status;
    }
    else
    {
        status = LWMSG_STATUS_PENDING;
    }

error:

    pthread_mutex_unlock(&pcall->task->call_lock);

    return status;
}

static
LWMsgStatus
lwmsg_peer_call_pend_incoming(
    LWMsgCall* call,
    LWMsgCancelFunction cancel,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerCall* scall = PEER_CALL(call);

    pthread_mutex_lock(&scall->task->call_lock);

    scall->state |= PEER_CALL_PENDED;
    scall->params.incoming.cancel = cancel;
    scall->params.incoming.cancel_data = data;

    pthread_mutex_unlock(&scall->task->call_lock);

    return status;
}

static
LWMsgStatus
lwmsg_peer_call_complete_incoming(
    LWMsgCall* call,
    LWMsgStatus call_status
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerCall* pcall = PEER_CALL(call);

    pthread_mutex_lock(&pcall->task->call_lock);

    pcall->status = call_status;
    pcall->state |= PEER_CALL_COMPLETED;

    /* Only wake up the task if the dispatch function
       has finished running. Otherwise, these steps will be
       performed once the dispatch function is finished */
    if (pcall->state & PEER_CALL_DISPATCHED)
    {
        lwmsg_task_wake(pcall->task->event_task);
    }

    pthread_mutex_unlock(&pcall->task->call_lock);

    return status;
}

LWMsgStatus
lwmsg_peer_call_complete_outgoing(
    PeerCall* call,
    LWMsgMessage* incoming_message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (call->state & PEER_CALL_RELEASED)
    {
        lwmsg_assoc_destroy_message(call->task->assoc, incoming_message);
        lwmsg_peer_call_delete(call);
    }
    else
    {
        call->status = incoming_message->status;
        call->params.outgoing.out->tag = incoming_message->tag;
        call->params.outgoing.out->data = incoming_message->data;
        call->state |= PEER_CALL_COMPLETED;

        lwmsg_message_init(incoming_message);

        if (call->params.outgoing.complete)
        {
            call->params.outgoing.complete(
                LWMSG_CALL(call),
                call->status,
                call->params.outgoing.complete_data);
        }
        else
        {
            pthread_cond_broadcast(&call->task->call_event);
        }
    }

    return status;
}

LWMsgStatus
lwmsg_peer_call_cancel_incoming(
    PeerCall* call
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (!(call->state & PEER_CALL_CANCELLED))
    {
        call->state |= PEER_CALL_CANCELLED;

        /* If the dispatch function is not finished running,
           we don't call the cancel callback right now --
           lwmsg_peer_call_dispatch_incoming() will handle it.

           If the call is already completed, we silently
           ignore the cancel request and return success */

        if ((call->state & PEER_CALL_DISPATCHED) &&
            !(call->state & PEER_CALL_COMPLETED) &&
            call->params.incoming.cancel != NULL)
        {
            call->params.incoming.cancel(LWMSG_CALL(call), call->params.incoming.cancel_data);
        }
    }

    return status;
}

static
LWMsgStatus
lwmsg_peer_call_cancel_outgoing(
    LWMsgCall* call
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerCall* pcall = PEER_CALL(call);

    pthread_mutex_lock(&pcall->task->call_lock);

    if (!(pcall->state & PEER_CALL_CANCELLED))
    {
        pcall->state |= PEER_CALL_CANCELLED;
        if (pcall->state & PEER_CALL_DISPATCHED &&
            !(pcall->state & PEER_CALL_COMPLETED))
        {
            lwmsg_task_wake(pcall->task->event_task);
        }
        else
        {
            lwmsg_ring_remove(&pcall->ring);
        }
    }

    pthread_mutex_unlock(&pcall->task->call_lock);

    return status;
}

static
void
lwmsg_peer_call_release_outgoing(
    LWMsgCall* call
    )
{
    PeerCall* pcall = PEER_CALL(call);
    PeerAssocTask* task = pcall->task;
    LWMsgBool delete = LWMSG_FALSE;

    pthread_mutex_lock(&task->call_lock);
    if (pcall->state & PEER_CALL_DISPATCHED &&
        !(pcall->state & PEER_CALL_COMPLETED))
    {
        pcall->state |= PEER_CALL_RELEASED;
    }
    else
    {
        delete = LWMSG_TRUE;
    }
    pthread_mutex_unlock(&task->call_lock);

    if (delete)
    {
        lwmsg_peer_call_delete(pcall);
    }

    return;
}

static
LWMsgSession*
lwmsg_peer_call_get_session(
    LWMsgCall* call
    )
{
    PeerCall* my_call = PEER_CALL(call);

    return my_call->task->session;
}

static
LWMsgStatus
lwmsg_peer_call_destroy_params(
    LWMsgCall* call,
    LWMsgParams* params
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMessage message = LWMSG_MESSAGE_INITIALIZER;
    PeerCall* pcall = PEER_CALL(call);

    message.tag = params->tag;
    message.data = params->data;

    BAIL_ON_ERROR(status = lwmsg_assoc_destroy_message(pcall->task->assoc, &message));

    params->tag = LWMSG_TAG_INVALID;
    params->data = NULL;

error:

    return status;
}

static
LWMsgStatus
lwmsg_peer_call_acquire_callback(
    LWMsgCall* call,
    LWMsgCall** callback
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerCall* pcall = PEER_CALL(call);
    PeerCall* my_callback = NULL;

    BAIL_ON_ERROR(status = lwmsg_peer_call_new(pcall->task, &my_callback));

    *callback = LWMSG_CALL(my_callback);

error:

    return status;
}

static LWMsgCallClass peer_call_class =
{
    .release = lwmsg_peer_call_release_outgoing,
    .dispatch = lwmsg_peer_call_dispatch_outgoing,
    .pend = lwmsg_peer_call_pend_incoming,
    .complete = lwmsg_peer_call_complete_incoming,
    .cancel = lwmsg_peer_call_cancel_outgoing,
    .get_session = lwmsg_peer_call_get_session,
    .destroy_params = lwmsg_peer_call_destroy_params,
    .acquire_callback = lwmsg_peer_call_acquire_callback
};

LWMsgStatus
lwmsg_peer_call_new(
    PeerAssocTask* task,
    PeerCall** call
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PeerCall* my_call = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_call));

    lwmsg_ring_init(&my_call->ring);

    my_call->base.vtbl = &peer_call_class;
    my_call->task = task;

    *call = my_call;

done:

    return status;

error:

    if (my_call)
    {
        free(my_call);
    }

    goto done;
}

void
lwmsg_peer_call_delete(
    PeerCall* call
    )
{
    if (call->task)
    {
        lwmsg_peer_task_unref(call->task);
    }
    free(call);
}
