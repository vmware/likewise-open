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
#include "assoc-private.h"
#include "dispatch-private.h"

#include <errno.h>
#include <fcntl.h>

LWMsgStatus
lwmsg_server_task_new(
    ServerTaskType type,
    ServerTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerTask* my_task = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_task));

    BAIL_ON_ERROR(status = lwmsg_dispatch_init(&my_task->dispatch));

    lwmsg_ring_init(&my_task->ring);
    my_task->type = type;
    my_task->incoming_message.tag = -1;
    my_task->outgoing_message.tag = -1;
    my_task->fd = -1;
    my_task->deadline.seconds = -1;
    my_task->deadline.microseconds = -1;

    *task = my_task;

error:

    if (status && my_task)
    {
        free(my_task);
    }

    return status;
}

LWMsgStatus
lwmsg_server_task_new_listen(
    int fd,
    ServerTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerTask* my_task = NULL;

    BAIL_ON_ERROR(status = lwmsg_server_task_new(SERVER_TASK_LISTEN, &my_task));

    my_task->fd = fd;

    *task = my_task;

error:

    return status;
}

static
LWMsgStatus
lwmsg_server_task_new_establish(
    int fd,
    LWMsgAssoc* assoc,
    ServerTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerTask* my_task = NULL;

    BAIL_ON_ERROR(status = lwmsg_server_task_new(SERVER_TASK_BEGIN_ESTABLISH, &my_task));

    my_task->fd = fd;
    my_task->assoc = assoc;

    *task = my_task;

error:

    return status;
}


void
lwmsg_server_task_delete(
    ServerTask* task
    )
{
    lwmsg_ring_remove(&task->ring);

    lwmsg_dispatch_destroy(&task->dispatch);

    if (task->assoc && task->incoming_message.tag != -1)
    {
        lwmsg_assoc_free_message(task->assoc, &task->incoming_message);
    }

    if (task->assoc && task->outgoing_message.tag != -1)
    {
        lwmsg_assoc_free_message(task->assoc, &task->outgoing_message);
    }

    if (task->fd >= 0 && !task->assoc)
    {
        close(task->fd);
    }

    if (task->assoc)
    {
        lwmsg_assoc_delete(task->assoc);
    }

    free(task);
}

static
void
lwmsg_server_update_nfds(
    int* nfds,
    int fd
    )
{
    if (*nfds < fd + 1)
    {
        *nfds = fd + 1;
    }
}

static
void
lwmsg_server_task_update_deadline(
    ServerTask* task,
    LWMsgTime* timeout,
    LWMsgTime* now,
    LWMsgTime* next_deadline
    )
{
    if (timeout)
    {
        if (lwmsg_time_is_positive(timeout))
        {
            lwmsg_time_sum(now, timeout, &task->deadline);
        }
        else
        {
            task->deadline = *timeout;
        }
    }

    if (next_deadline &&
        lwmsg_time_is_positive(&task->deadline) &&
        (!lwmsg_time_is_positive(next_deadline) ||
         lwmsg_time_compare(&task->deadline, next_deadline) == LWMSG_TIME_LESSER))
    {
        *next_deadline = task->deadline;
    }
}

static
LWMsgBool
lwmsg_server_task_subject_to_timeout(
    LWMsgServer* server,
    ServerTask* task,
    LWMsgBool shutdown
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSessionManager* manager = NULL;
    LWMsgSession* session = NULL;
    size_t handle_count = 0;
    size_t num_clients = 0;

    if (!shutdown && task->type == SERVER_TASK_FINISH_RECV)
    {
        /* Clients that are sitting idle without sending a message
           are not subject to timeout if:

           - The server is not shutting down, and
              * The client has an open handle, or
              * There are still available client slots
         */
        BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(task->assoc, &manager));
        BAIL_ON_ERROR(status = task->assoc->aclass->get_session(task->assoc, &session));

        handle_count = lwmsg_session_manager_get_session_handle_count(manager, session);
        num_clients = lwmsg_server_get_num_clients(server);

        return handle_count == 0 && num_clients == server->max_clients;
    }
    else
    {
        return LWMSG_TRUE;
    }

error:

    return LWMSG_TRUE;
}

static
LWMsgBool
lwmsg_server_task_is_past_due(
    ServerTask* task,
    LWMsgTime* now
    )
{
    return
        lwmsg_time_is_positive(&task->deadline) &&
        lwmsg_time_compare(now, &task->deadline) == LWMSG_TIME_GREATER;
}

static
void
lwmsg_server_task_drop(
    LWMsgServer* server,
    ServerTask** task
    )
{
    if ((*task)->assoc)
    {
        lwmsg_server_release_client_slot(server);
    }

    lwmsg_server_task_delete(*task);
    *task = NULL;
}

LWMsgStatus
lwmsg_server_task_prepare_select(
    LWMsgServer* server,
    ServerTask* task,
    int* nfds,
    fd_set* readset,
    fd_set* writeset
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t num_clients = 0;

    switch (task->type)
    {
    case SERVER_TASK_ACCEPT:
        num_clients = lwmsg_server_get_num_clients(server);
        if (num_clients < server->max_clients)
        {
            /* For accept tasks, wait for listen socket to be readable */
            lwmsg_server_update_nfds(nfds, task->fd);
            FD_SET(task->fd, readset);
        }
        break;
    case SERVER_TASK_FINISH_ESTABLISH:
    case SERVER_TASK_FINISH_RECV:
    case SERVER_TASK_FINISH_SEND:
        /* For tasks performed on an association, use its state to
           determine what to wait for */
        switch (lwmsg_assoc_get_state(task->assoc))
        {
        case LWMSG_ASSOC_STATE_BLOCKED_SEND:
            lwmsg_server_update_nfds(nfds, task->fd);
            FD_SET(task->fd, writeset);
            break;
        case LWMSG_ASSOC_STATE_BLOCKED_RECV:
        case LWMSG_ASSOC_STATE_IDLE:
            lwmsg_server_update_nfds(nfds, task->fd);
            FD_SET(task->fd, readset);
            break;
        case LWMSG_ASSOC_STATE_BLOCKED_SEND_RECV:
            lwmsg_server_update_nfds(nfds, task->fd);
            FD_SET(task->fd, writeset);
            FD_SET(task->fd, readset);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    return status;
}

static
LWMsgStatus
lwmsg_server_task_handle_assoc_error(
    LWMsgServer* server,
    ServerTask** task,
    LWMsgStatus status
    )
{
    switch (status)
    {
    case LWMSG_STATUS_CONNECTION_REFUSED:
    case LWMSG_STATUS_PEER_CLOSE:
    case LWMSG_STATUS_PEER_RESET:
    case LWMSG_STATUS_PEER_ABORT:
    case LWMSG_STATUS_MALFORMED:
    case LWMSG_STATUS_SECURITY:
    case LWMSG_STATUS_OVERFLOW:
    case LWMSG_STATUS_UNDERFLOW:
    case LWMSG_STATUS_INTERRUPT:
        lwmsg_server_task_drop(server, task);
        status = LWMSG_STATUS_SUCCESS;
        break;
    default:
        BAIL_ON_ERROR(status);
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_server_task_perform_accept(
    LWMsgServer* server,
    ServerTask** task,
    LWMsgBool shutdown
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    struct sockaddr addr;
    socklen_t addrlen = sizeof(addr);
    int client_fd = -1;
    LWMsgAssoc* assoc = NULL;
    ServerTask* client_task = NULL;
    LWMsgBool slot = LWMSG_FALSE;


    if (shutdown)
    {
        lwmsg_server_task_drop(server, task);
    }
    else if (!(*task)->blocked)
    {
        if ((slot = lwmsg_server_acquire_client_slot(server)))
        {
            client_fd = accept((*task)->fd, &addr, &addrlen);

            if (client_fd < 0)
            {
                switch (errno)
                {
                case EAGAIN:
                case EINTR:
                    /* Don't propagate an error, just mark task as blocked */
                    (*task)->blocked = LWMSG_TRUE;
                    goto done;
                default:
                    BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
                }
            }

            /* Create new connection with client fd, put it into task, schedule task */
            BAIL_ON_ERROR(status = lwmsg_connection_new(server->protocol, &assoc));
            BAIL_ON_ERROR(status = lwmsg_connection_set_fd(assoc, LWMSG_CONNECTION_MODE_LOCAL, client_fd));
            BAIL_ON_ERROR(status = lwmsg_assoc_set_session_manager(assoc, server->manager));
            BAIL_ON_ERROR(status = lwmsg_assoc_set_session_functions(
                              assoc,
                              server->session_construct,
                              server->session_destruct,
                              server->session_construct_data));
            BAIL_ON_ERROR(status = lwmsg_assoc_set_nonblock(assoc, LWMSG_TRUE));
            BAIL_ON_ERROR(status = lwmsg_server_task_new_establish(client_fd, assoc, &client_task));
            assoc = NULL;
            slot = LWMSG_FALSE;
            lwmsg_server_queue_io_task(server, client_task);
        }
        else
        {
            (*task)->blocked = LWMSG_TRUE;
        }
    }

done:

    if (slot)
    {
        lwmsg_server_release_client_slot(server);
    }

    return status;

error:

    if (client_fd >= 0 && !assoc)
    {
        close(client_fd);
    }

    if (assoc)
    {
        lwmsg_assoc_delete(assoc);
    }

    if (client_task)
    {
        lwmsg_server_task_drop(server, &client_task);
    }

    goto done;
}

static
LWMsgStatus
lwmsg_server_task_perform_establish(
    LWMsgServer* server,
    ServerTask** task,
    LWMsgTime* current_time,
    LWMsgTime* next_deadline
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    status = lwmsg_assoc_establish((*task)->assoc);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        (*task)->type = SERVER_TASK_BEGIN_RECV;
        break;
    case LWMSG_STATUS_NOT_FINISHED:
        (*task)->blocked = LWMSG_TRUE;
        (*task)->type = SERVER_TASK_FINISH_ESTABLISH;
        lwmsg_server_task_update_deadline(
            (*task),
            &server->timeout.establish,
            current_time,
            next_deadline);
        status = LWMSG_STATUS_SUCCESS;
        break;
    default:
        BAIL_ON_ERROR(status = lwmsg_server_task_handle_assoc_error(
                          server,
                          task,
                          status));
        break;
    }

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
lwmsg_server_task_perform_close(
    LWMsgServer* server,
    ServerTask** task,
    LWMsgTime* now,
    LWMsgTime* next_deadline
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    status = lwmsg_assoc_close((*task)->assoc);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        lwmsg_server_task_drop(server, task);
        break;
    case LWMSG_STATUS_NOT_FINISHED:
        (*task)->blocked = LWMSG_TRUE;
        (*task)->type = SERVER_TASK_FINISH_CLOSE;
        status = LWMSG_STATUS_SUCCESS;
        lwmsg_server_task_update_deadline(
            *task,
            &server->timeout.establish,
            now,
            next_deadline);
        break;
    default:
        BAIL_ON_ERROR(status = lwmsg_server_task_handle_assoc_error(
                          server,
                          task,
                          status));
        break;
    }

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
lwmsg_server_task_perform_reset(
    LWMsgServer* server,
    ServerTask** task,
    LWMsgTime* now,
    LWMsgTime* next_deadline
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    status = lwmsg_assoc_reset((*task)->assoc);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        lwmsg_server_task_drop(server, task);
        break;
    case LWMSG_STATUS_NOT_FINISHED:
        (*task)->blocked = LWMSG_TRUE;
        (*task)->type = SERVER_TASK_FINISH_RESET;
        status = LWMSG_STATUS_SUCCESS;
        lwmsg_server_task_update_deadline(
            *task,
            &server->timeout.establish,
            now,
            next_deadline);
        break;
    default:
        BAIL_ON_ERROR(status = lwmsg_server_task_handle_assoc_error(
                          server,
                          task,
                          status));
        break;
    }

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
lwmsg_server_task_dispatch(
    LWMsgServer* server,
    ServerTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgDispatchSpec* spec = NULL;

    spec = server->dispatch.vector[(*task)->incoming_message.tag];

    switch (spec->type)
    {
    case LWMSG_DISPATCH_TYPE_OLD:
    case LWMSG_DISPATCH_TYPE_SYNC:
        (*task)->type = SERVER_TASK_DISPATCH;
        lwmsg_server_queue_dispatch_task(server, (*task));
        *task = NULL;
        break;
    case LWMSG_DISPATCH_TYPE_ASYNC:
        (*task)->type = SERVER_TASK_BEGIN_ASYNC;
        (*task)->blocked = LWMSG_FALSE;
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_INTERNAL);
    }

error:

    return status;
}

static
void
lwmsg_server_task_complete_async(
    LWMsgDispatchHandle* handle,
    LWMsgStatus status,
    void* data
    )
{
    ServerIoThread* thread = (ServerIoThread*) data;

    /* Poke the thread owning the task so it wakes up and
       notices the dispatch completed */
    pthread_mutex_lock(&thread->lock);
    thread->num_events++;
    lwmsg_server_signal_io_thread(thread);
    pthread_mutex_unlock(&thread->lock);
}

static
LWMsgStatus
lwmsg_server_task_perform_async(
    LWMsgServer* server,
    ServerIoThread* thread,
    ServerTask** task,
    LWMsgTime* now,
    LWMsgTime* next_deadline
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgDispatchSpec* spec = server->dispatch.vector[(*task)->incoming_message.tag];
    LWMsgServerDispatchFunction func = (LWMsgServerDispatchFunction) spec->data;

    lwmsg_dispatch_begin(&(*task)->dispatch,
                         lwmsg_server_task_complete_async,
                         thread);

    status = func(&(*task)->dispatch,
                  &(*task)->incoming_message,
                  &(*task)->outgoing_message,
                  server->dispatch_data);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        (*task)->type = SERVER_TASK_BEGIN_SEND;
        break;
    case LWMSG_STATUS_NOT_FINISHED:
        (*task)->type = SERVER_TASK_FINISH_ASYNC;
        status = LWMSG_STATUS_SUCCESS;
        break;
    default:
        BAIL_ON_ERROR(status = lwmsg_server_task_handle_assoc_error(server, task, status));
        break;
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_server_task_finish_async(
    LWMsgServer* server,
    ServerTask** task,
    LWMsgBool shutdown
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    status = lwmsg_dispatch_get_result(&(*task)->dispatch);

    if (status == LWMSG_STATUS_NOT_FINISHED)
    {
        if (shutdown)
        {
            /* Interrupt dispatch so we can shut down */
            lwmsg_dispatch_interrupt(&(*task)->dispatch);
            (*task)->blocked = LWMSG_TRUE;
        }
        else if (!(*task)->blocked)
        {
            /* Check for interrupt/close from peer.
               This is done within the dispatch lock
               to serialize access to the association */
            pthread_mutex_lock(&(*task)->dispatch.lock);
            status = lwmsg_assoc_finish((*task)->assoc);
            pthread_mutex_unlock(&(*task)->dispatch.lock);

            switch (status)
            {
            case LWMSG_STATUS_SUCCESS:
                /* Spuriously unblocked, go back to sleep */
                (*task)->blocked = LWMSG_TRUE;
                break;
            default:
                /* Interrupt dispatch go back to sleep waiting for completion */
                lwmsg_dispatch_interrupt(&(*task)->dispatch);
                (*task)->blocked = LWMSG_TRUE;
                break;
            }

            status = LWMSG_STATUS_SUCCESS;
        }
    }
    else
    {
        switch(status)
        {
        case LWMSG_STATUS_SUCCESS:
            (*task)->blocked = LWMSG_FALSE;
            (*task)->type = SERVER_TASK_BEGIN_SEND;
            break;
        default:
            BAIL_ON_ERROR(status = lwmsg_server_task_handle_assoc_error(server, task, status));
            break;
        }
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_server_task_perform_recv(
    LWMsgServer* server,
    ServerTask** task,
    LWMsgBool shutdown,
    LWMsgTime* now,
    LWMsgTime* next_deadline
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (shutdown)
    {
        (*task)->type = SERVER_TASK_BEGIN_CLOSE;
        (*task)->blocked = LWMSG_FALSE;
    }
    else
    {
        status = lwmsg_assoc_recv_message((*task)->assoc, &(*task)->incoming_message);

        switch (status)
        {
        case LWMSG_STATUS_SUCCESS:
            BAIL_ON_ERROR(status = lwmsg_server_task_dispatch(server, task));
            break;
        case LWMSG_STATUS_NOT_FINISHED:
            (*task)->blocked = LWMSG_TRUE;
            (*task)->type = SERVER_TASK_FINISH_RECV;
            lwmsg_server_task_update_deadline(
                *task,
                &server->timeout.idle,
                now,
                next_deadline);
            status = LWMSG_STATUS_SUCCESS;
            break;
        default:
            BAIL_ON_ERROR(status = lwmsg_server_task_handle_assoc_error(
                              server,
                              task,
                              status));
            break;
        }
    }

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
lwmsg_server_task_perform_send(
    LWMsgServer* server,
    ServerTask** task,
    LWMsgTime* now,
    LWMsgTime* next_deadline
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    status = lwmsg_assoc_send_message((*task)->assoc, &(*task)->outgoing_message);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        lwmsg_assoc_free_message((*task)->assoc, &(*task)->outgoing_message);
        (*task)->type = SERVER_TASK_BEGIN_RECV;
        break;
    case LWMSG_STATUS_NOT_FINISHED:
        (*task)->blocked = LWMSG_TRUE;
        (*task)->type = SERVER_TASK_FINISH_SEND;
        status = LWMSG_STATUS_SUCCESS;
        lwmsg_server_task_update_deadline(
            (*task),
            &server->timeout.message,
            now,
            next_deadline);
        break;
    default:
        BAIL_ON_ERROR(status = lwmsg_server_task_handle_assoc_error(
                          server,
                          task,
                          status));
        break;
    }

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
lwmsg_server_task_perform_finish(
    LWMsgServer* server,
    ServerTask** task,
    LWMsgBool shutdown,
    LWMsgTime* now,
    LWMsgTime* next_deadline
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (!(*task)->blocked)
    {
        /* Finish up task if main loop has marked it as unblocked */
        status = lwmsg_assoc_finish((*task)->assoc);

        switch (status)
        {
        case LWMSG_STATUS_SUCCESS:
            switch ((*task)->type)
            {
            case SERVER_TASK_FINISH_ESTABLISH:
                (*task)->type = SERVER_TASK_BEGIN_RECV;
                break;
            case SERVER_TASK_FINISH_RECV:
                BAIL_ON_ERROR(status = lwmsg_server_task_dispatch(server, task));
                break;
            case SERVER_TASK_FINISH_SEND:
                lwmsg_assoc_free_message((*task)->assoc, &(*task)->outgoing_message);
                (*task)->type = SERVER_TASK_BEGIN_RECV;
                break;
            case SERVER_TASK_FINISH_CLOSE:
            case SERVER_TASK_FINISH_RESET:
                lwmsg_server_task_drop(server, task);
                break;
            default:
                break;
            }
            break;
        case LWMSG_STATUS_NOT_FINISHED:
            (*task)->blocked = LWMSG_TRUE;
            status = LWMSG_STATUS_SUCCESS;
            break;
        default:
            BAIL_ON_ERROR(status = lwmsg_server_task_handle_assoc_error(
                              server,
                              task,
                              status));
            break;
        }
    }

    if (*task && (*task)->blocked &&
        lwmsg_server_task_subject_to_timeout(server, *task, shutdown))
    {
        /* Perform timeout logic on tasks that are still blocked */
        if (lwmsg_server_task_is_past_due(*task, now))
        {
            /* Task is past due */
            switch ((*task)->type)
            {
            case SERVER_TASK_FINISH_CLOSE:
            case SERVER_TASK_FINISH_RESET:
                /* Timed out during a close, so just drop it */
                lwmsg_server_task_drop(server, task);
                break;
            default:
                /* Begin reset */
                (*task)->type = SERVER_TASK_BEGIN_RESET;
                (*task)->blocked = LWMSG_FALSE;
                break;
            }
        }
        else
        {
            /* Task is still viable, so just update next deadline */
            lwmsg_server_task_update_deadline(
                *task,
                NULL,
                now,
                next_deadline);
        }
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_server_task_perform_listen(
    LWMsgServer* server,
    ServerTask** task,
    LWMsgBool shutdown
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    long opts = 0;

    if (shutdown)
    {
        lwmsg_server_task_drop(server, task);
    }
    else
    {
        /* Get socket flags */
        if ((opts = fcntl((*task)->fd, F_GETFL, 0)) < 0)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
        }

        /* Set non-blocking flag */
        opts |= O_NONBLOCK;

        /* Set socket flags */
        if (fcntl((*task)->fd, F_SETFL, opts) < 0)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
        }

        if (listen((*task)->fd, server->max_backlog))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
        }

        (*task)->type = SERVER_TASK_ACCEPT;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_server_task_perform(
    LWMsgServer* server,
    ServerIoThread* thread,
    ServerTask** task,
    LWMsgBool shutdown,
    LWMsgTime* now,
    LWMsgTime* next_deadline
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    switch ((*task)->type)
    {
    case SERVER_TASK_LISTEN:
        BAIL_ON_ERROR(status = lwmsg_server_task_perform_listen(
                          server,
                          task,
                          shutdown));
        break;
    case SERVER_TASK_ACCEPT:
        BAIL_ON_ERROR(status = lwmsg_server_task_perform_accept(
                          server,
                          task,
                          shutdown));
        break;
    case SERVER_TASK_BEGIN_ESTABLISH:
        BAIL_ON_ERROR(status = lwmsg_server_task_perform_establish(
                          server,
                          task,
                          now,
                          next_deadline));
        break;
    case SERVER_TASK_BEGIN_RECV:
        BAIL_ON_ERROR(status = lwmsg_server_task_perform_recv(
                          server,
                          task,
                          shutdown,
                          now,
                          next_deadline));
        break;
    case SERVER_TASK_BEGIN_SEND:
        BAIL_ON_ERROR(status = lwmsg_server_task_perform_send(
                          server,
                          task,
                          now,
                          next_deadline));
        break;
    case SERVER_TASK_BEGIN_CLOSE:
        BAIL_ON_ERROR(status = lwmsg_server_task_perform_close(
                          server,
                          task,
                          now,
                          next_deadline));
        break;
    case SERVER_TASK_BEGIN_RESET:
        BAIL_ON_ERROR(status = lwmsg_server_task_perform_reset(
                          server,
                          task,
                          now,
                          next_deadline));
        break;
    case SERVER_TASK_BEGIN_ASYNC:
        BAIL_ON_ERROR(status = lwmsg_server_task_perform_async(
                          server,
                          thread,
                          task,
                          now,
                          next_deadline));
        break;
    case SERVER_TASK_FINISH_ESTABLISH:
    case SERVER_TASK_FINISH_RECV:
    case SERVER_TASK_FINISH_SEND:
    case SERVER_TASK_FINISH_CLOSE:
    case SERVER_TASK_FINISH_RESET:
        BAIL_ON_ERROR(status = lwmsg_server_task_perform_finish(
                          server,
                          task,
                          shutdown,
                          now,
                          next_deadline));
        /* Finishing async dispatches has separate logic */
        break;
    case SERVER_TASK_FINISH_ASYNC:
        BAIL_ON_ERROR(status = lwmsg_server_task_finish_async(
                          server,
                          task,
                          shutdown));
        break;
    default:
        break;
    }

error:

    return status;
}
