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
 *        server.c
 *
 * Abstract:
 *
 *        Multi-threaded server API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>
#include "server-private.h"

static
LWMsgStatus
lwmsg_server_dispatch_loop(
    ServerDispatchThread* thread
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool in_lock = LWMSG_FALSE;
    LWMsgRing* head = NULL;
    ServerTask* task = NULL;
    LWMsgServer* server = thread->server;
    LWMsgDispatchSpec* spec = NULL;

    for (;;)
    {
        pthread_mutex_lock(&server->dispatch.lock);
        in_lock = LWMSG_TRUE;

        while (lwmsg_ring_is_empty(&server->dispatch.tasks) &&
               !server->dispatch.shutdown)
        {
            pthread_cond_wait(&server->dispatch.event, &server->dispatch.lock);
        }

        if (server->dispatch.shutdown)
        {
            break;
        }

        head = server->dispatch.tasks.next;
        lwmsg_ring_remove(head);
        task = LWMSG_OBJECT_FROM_MEMBER(head, ServerTask, ring);

        pthread_mutex_unlock(&server->dispatch.lock);
        in_lock = LWMSG_FALSE;

        if (task->incoming_message.tag >= server->dispatch.vector_length)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_UNIMPLEMENTED);
        }

        spec = server->dispatch.vector[task->incoming_message.tag];

        if (!spec->data)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_UNIMPLEMENTED);
        }

        switch (spec->type)
        {
        case LWMSG_DISPATCH_TYPE_OLD:
            status = ((LWMsgAssocDispatchFunction) spec->data) (
                task->assoc,
                &task->incoming_message,
                &task->outgoing_message,
                server->dispatch_data);
            break;
        case LWMSG_DISPATCH_TYPE_SYNC:
            status = ((LWMsgServerDispatchFunction) spec->data) (
                &task->dispatch,
                &task->incoming_message,
                &task->outgoing_message,
                server->dispatch_data);
            break;
        default:
            BAIL_ON_ERROR(status = LWMSG_STATUS_INTERNAL);
            break;
        }

        switch (status)
        {
        case LWMSG_STATUS_SUCCESS:
            lwmsg_assoc_free_message(task->assoc, &task->incoming_message);
            task->type = SERVER_TASK_BEGIN_SEND;
            break;
        default:
            task->type = SERVER_TASK_BEGIN_CLOSE;
            break;
        }

        status = LWMSG_STATUS_SUCCESS;
        lwmsg_server_queue_io_task(server, task);
    }

    if (in_lock)
    {
        pthread_mutex_unlock(&server->dispatch.lock);
    }

error:

    return status;
}

void*
lwmsg_server_dispatch_thread(
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerDispatchThread* thread = (ServerDispatchThread*) data;

    BAIL_ON_ERROR(status = lwmsg_server_dispatch_loop(thread));

done:

    return NULL;

error:

    goto done;
}

LWMsgStatus
lwmsg_server_dispatch_get_security_token(
    LWMsgDispatchHandle* handle,
    LWMsgSecurityToken** token
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerTask* task = LWMSG_OBJECT_FROM_MEMBER(handle, ServerTask, dispatch);

    pthread_mutex_lock(&task->dispatch.lock);

    BAIL_ON_ERROR(status = lwmsg_assoc_get_peer_security_token(task->assoc, token));

error:

    pthread_mutex_unlock(&task->dispatch.lock);

    return status;
}

LWMsgStatus
lwmsg_server_dispatch_get_session_data(
    LWMsgDispatchHandle* handle,
    void** session_data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerTask* task = LWMSG_OBJECT_FROM_MEMBER(handle, ServerTask, dispatch);

    pthread_mutex_lock(&task->dispatch.lock);

    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_data(task->assoc, session_data));

error:

    pthread_mutex_unlock(&task->dispatch.lock);

    return status;
}
