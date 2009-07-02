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
#include "session-private.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>

static
void
lwmsg_server_task_drop(
    LWMsgServer* server,
    ServerTask** task
    );

static
void
lwmsg_server_session_string_for_session(
    LWMsgSession* session,
    LWMsgSessionString string
    )
{
    const LWMsgSessionID* rsmid = NULL;

    rsmid = lwmsg_session_get_id(session);
    lwmsg_session_id_to_string(rsmid, string);
}

static
void
lwmsg_server_session_string_for_assoc(
    LWMsgAssoc* assoc,
    LWMsgSessionString string
    )
{
    LWMsgSession* session = NULL;

    if (lwmsg_assoc_get_session(assoc, &session) == LWMSG_STATUS_SUCCESS)
    {
        lwmsg_server_session_string_for_session(session, string);
    }
    else
    {
        strncpy(string, "<null session>", sizeof (string));
    }
}

static
LWMsgStatus
lwmsg_server_log_incoming_message(
    LWMsgServer* server,
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    char* msg_text = NULL;

    if (lwmsg_context_would_log(server->context, LWMSG_LOGLEVEL_TRACE))
    {
        BAIL_ON_ERROR(lwmsg_assoc_print_message_alloc(assoc, message, &msg_text));
        LWMSG_LOG_TRACE(
            server->context,
            "(assoc:0x%lx) => %s",
            LWMSG_POINTER_AS_ULONG(assoc),
            msg_text);
    }

cleanup:

    if (msg_text)
    {
        lwmsg_context_free(server->context, msg_text);
    }

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
lwmsg_server_log_outgoing_message(
    LWMsgServer* server,
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    char* msg_text = NULL;

    if (lwmsg_context_would_log(server->context, LWMSG_LOGLEVEL_TRACE))
    {
        BAIL_ON_ERROR(lwmsg_assoc_print_message_alloc(assoc, message, &msg_text));
        LWMSG_LOG_TRACE(
            server->context,
            "(assoc:0x%lx) <= %s",
            LWMSG_POINTER_AS_ULONG(assoc),
            msg_text);
    }

cleanup:

    if (msg_text)
    {
        lwmsg_context_free(server->context, msg_text);
    }

    return status;

error:

    goto cleanup;
}

static
LWMsgStatus
lwmsg_server_log_establish(
    LWMsgServer* server,
    LWMsgAssoc* assoc
    )
{
    LWMsgSessionString buffer;

    if (lwmsg_context_would_log(server->context, LWMSG_LOGLEVEL_VERBOSE))
    {
        lwmsg_server_session_string_for_assoc(assoc, buffer);

        LWMSG_LOG_VERBOSE(server->context, "(session:%s) Established association 0x%lx",
                          buffer, (unsigned long) (size_t) assoc);
    }

    return LWMSG_STATUS_SUCCESS;
}

LWMsgStatus
lwmsg_server_task_new(
    ServerTaskType type,
    ServerTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerTask* my_task = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_task));

    BAIL_ON_ERROR(status = lwmsg_server_call_init(&my_task->info.call));

    lwmsg_ring_init(&my_task->ring);
    my_task->type = type;
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
    LWMsgServerMode mode,
    const char* endpoint,
    mode_t perms,
    int fd,
    ServerTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerTask* my_task = NULL;

    BAIL_ON_ERROR(status = lwmsg_server_task_new(SERVER_TASK_LISTEN, &my_task));

    if (endpoint)
    {
        my_task->info.listen.endpoint = strdup(endpoint);

        if (!my_task->info.listen.endpoint)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
        }
    }

    my_task->fd = fd;
    my_task->info.listen.mode = mode;
    my_task->info.listen.perms = perms;

    *task = my_task;

done:

    return status;

error:

    if (my_task)
    {
        lwmsg_server_task_delete(my_task);
    }

    goto done;
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
    my_task->info.call.assoc = assoc;

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

    switch (task->type)
    {
    case SERVER_TASK_LISTEN:
    case SERVER_TASK_ACCEPT:
        if (task->info.listen.endpoint)
        {
            free(task->info.listen.endpoint);
        }
        break;
    default:
        lwmsg_server_call_destroy(&task->info.call);
        if (task->info.call.assoc)
        {
            lwmsg_assoc_destroy_message(task->info.call.assoc, &task->info.call.incoming);
            lwmsg_assoc_destroy_message(task->info.call.assoc, &task->info.call.outgoing);
            task->fd = -1;
            lwmsg_assoc_delete(task->info.call.assoc);
        }
        break;
    }

    if (task->fd != -1)
    {
        close(task->fd);
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
    size_t handle_count = 0;
    size_t num_clients = 0;

    if (!shutdown && task->type == SERVER_TASK_FINISH_RECV)
    {
        handle_count = lwmsg_session_get_handle_count(task->info.call.session);
        num_clients = lwmsg_server_get_num_clients(server);

        return handle_count == 0 && num_clients == server->max_clients;
    }
    else
    {
        return LWMSG_TRUE;
    }

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
    switch ((*task)->type)
    {
    case SERVER_TASK_LISTEN:
    case SERVER_TASK_ACCEPT:
        if ((*task)->info.listen.endpoint)
        {
            unlink((*task)->info.listen.endpoint);
        }
    default:
        if ((*task)->info.call.assoc)
        {
            lwmsg_server_release_client_slot(server);
        }
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
    case SERVER_TASK_FINISH_CALL:
        /* For tasks performed on an association, use its state to
           determine what to wait for */
        switch (lwmsg_assoc_get_state(task->info.call.assoc))
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
    LWMsgSessionString buffer;

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
    case LWMSG_STATUS_CANCELLED:
        if (lwmsg_context_would_log(server->context, LWMSG_LOGLEVEL_VERBOSE))
        {
            lwmsg_server_session_string_for_assoc((*task)->info.call.assoc, buffer);
            LWMSG_LOG_VERBOSE(server->context, "(assoc:0x%lx) %s",
                              LWMSG_POINTER_AS_ULONG((*task)->info.call.assoc),
                              lwmsg_assoc_get_error_message((*task)->info.call.assoc, status));
        }

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
            BAIL_ON_ERROR(status = lwmsg_connection_new(server->context, server->protocol, &assoc));
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

    status = lwmsg_assoc_establish((*task)->info.call.assoc);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        BAIL_ON_ERROR(status = lwmsg_assoc_get_session((*task)->info.call.assoc, &(*task)->info.call.session));
        (*task)->type = SERVER_TASK_BEGIN_RECV;
        BAIL_ON_ERROR(status = lwmsg_server_log_establish(server, (*task)->info.call.assoc));
        break;
    case LWMSG_STATUS_PENDING:
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

    lwmsg_assoc_destroy_message((*task)->info.call.assoc, &(*task)->info.call.incoming);
    lwmsg_assoc_destroy_message((*task)->info.call.assoc, &(*task)->info.call.outgoing);
    status = lwmsg_assoc_close((*task)->info.call.assoc);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        lwmsg_server_task_drop(server, task);
        break;
    case LWMSG_STATUS_PENDING:
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

    status = lwmsg_assoc_reset((*task)->info.call.assoc);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        lwmsg_server_task_drop(server, task);
        break;
    case LWMSG_STATUS_PENDING:
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
lwmsg_server_task_perform_call(
    LWMsgServer* server,
    ServerIoThread* thread,
    ServerTask** task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgDispatchSpec* spec = NULL;
    LWMsgTag tag = (*task)->info.call.incoming.tag;

    if (tag < 0 || tag >= server->dispatch.vector_length)
    {
        status = lwmsg_server_task_handle_assoc_error(
            server,
            task,
            LWMSG_STATUS_MALFORMED);
        goto error;
    }

    spec = server->dispatch.vector[(*task)->info.call.incoming.tag];

    lwmsg_server_call_setup(&(*task)->info.call, thread, spec, server->dispatch_data);

    switch (spec->type)
    {
    case LWMSG_DISPATCH_TYPE_OLD:
    case LWMSG_DISPATCH_TYPE_BLOCK:
        lwmsg_server_queue_call(server, &(*task)->info.call);
        (*task)->type = SERVER_TASK_FINISH_CALL;
        break;
    case LWMSG_DISPATCH_TYPE_NONBLOCK:
        status = lwmsg_call_transact(LWMSG_CALL(&(*task)->info.call), NULL, NULL);
        switch (status)
        {
        case LWMSG_STATUS_SUCCESS:
            lwmsg_assoc_destroy_message((*task)->info.call.assoc, &(*task)->info.call.incoming);
            (*task)->type = SERVER_TASK_BEGIN_SEND;
            break;
        case LWMSG_STATUS_PENDING:
            (*task)->type = SERVER_TASK_FINISH_CALL;
            status = LWMSG_STATUS_SUCCESS;
            break;
        default:
            BAIL_ON_ERROR(status = lwmsg_server_task_handle_assoc_error(
                              server,
                              task,
                              status));
        }
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_INTERNAL);
        break;
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_server_task_finish_call(
    LWMsgServer* server,
    ServerTask** task,
    LWMsgBool shutdown
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool completed = LWMSG_FALSE;
    LWMsgBool cancelled = LWMSG_FALSE;

    pthread_mutex_lock(&(*task)->info.call.lock);
    completed = ((*task)->info.call.state & SERVER_CALL_COMPLETED) ? LWMSG_TRUE : LWMSG_FALSE;
    cancelled = ((*task)->info.call.state & SERVER_CALL_CANCELLED) ? LWMSG_TRUE : LWMSG_FALSE;
    status = (*task)->info.call.status;
    pthread_mutex_unlock(&(*task)->info.call.lock);

    if (!completed)
    {
        if (shutdown && !cancelled)
        {
            /* Interrupt call so we can shut down */
            lwmsg_call_cancel(LWMSG_CALL(&(*task)->info.call));
            (*task)->blocked = LWMSG_TRUE;
        }
        else if (!(*task)->blocked)
        {
            /* Check for activity on association */
            status = lwmsg_assoc_finish((*task)->info.call.assoc);

            switch (status)
            {
            case LWMSG_STATUS_SUCCESS:
                /* Spuriously unblocked, nothing to do */
                break;
            default:
                /* Association dead, cancel call */
                if (!cancelled)
                {
                    lwmsg_call_cancel(LWMSG_CALL(&(*task)->info.call));
                }
                break;
            }

            /* Go back to sleep */
            (*task)->blocked = LWMSG_TRUE;
            status = LWMSG_STATUS_SUCCESS;
        }
    }
    else
    {
        switch(status)
        {
        case LWMSG_STATUS_SUCCESS:
            (*task)->blocked = LWMSG_FALSE;
            if (cancelled)
            {
                (*task)->type = SERVER_TASK_BEGIN_CLOSE;
            }
            else
            {
                (*task)->type = SERVER_TASK_BEGIN_SEND;
            }
            lwmsg_assoc_destroy_message((*task)->info.call.assoc, &(*task)->info.call.incoming);
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
        status = lwmsg_assoc_recv_message((*task)->info.call.assoc, &(*task)->info.call.incoming);

        switch (status)
        {
        case LWMSG_STATUS_SUCCESS:
            BAIL_ON_ERROR(status = lwmsg_server_log_incoming_message(server, (*task)->info.call.assoc, &(*task)->info.call.incoming));
            (*task)->type = SERVER_TASK_BEGIN_CALL;
            break;
        case LWMSG_STATUS_PENDING:
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

    BAIL_ON_ERROR(status = lwmsg_server_log_outgoing_message(server, (*task)->info.call.assoc, &(*task)->info.call.outgoing));
    status = lwmsg_assoc_send_message((*task)->info.call.assoc, &(*task)->info.call.outgoing);

    switch (status)
    {
    case LWMSG_STATUS_SUCCESS:
        lwmsg_assoc_destroy_message((*task)->info.call.assoc, &(*task)->info.call.outgoing);
        (*task)->type = SERVER_TASK_BEGIN_RECV;
        break;
    case LWMSG_STATUS_PENDING:
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
        status = lwmsg_assoc_finish((*task)->info.call.assoc);

        switch (status)
        {
        case LWMSG_STATUS_SUCCESS:
            switch ((*task)->type)
            {
            case SERVER_TASK_FINISH_ESTABLISH:
                BAIL_ON_ERROR(status = lwmsg_assoc_get_session((*task)->info.call.assoc, &(*task)->info.call.session));
                BAIL_ON_ERROR(status = lwmsg_server_log_establish(server, (*task)->info.call.assoc));
                (*task)->type = SERVER_TASK_BEGIN_RECV;
                break;
            case SERVER_TASK_FINISH_RECV:
                BAIL_ON_ERROR(status = lwmsg_server_log_incoming_message(server, (*task)->info.call.assoc, &(*task)->info.call.incoming));
                (*task)->type = SERVER_TASK_BEGIN_CALL;
                break;
            case SERVER_TASK_FINISH_SEND:
                lwmsg_assoc_destroy_message((*task)->info.call.assoc, &(*task)->info.call.outgoing);
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
        case LWMSG_STATUS_PENDING:
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

static LWMsgStatus
lwmsg_server_task_create_local_socket(
    LWMsgServer* server,
    ServerTask* task
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int sock = -1;
    struct sockaddr_un sockaddr;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sock == -1)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    sockaddr.sun_family = AF_UNIX;

    if (strlen(task->info.listen.endpoint) > sizeof(sockaddr.sun_path))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_PARAMETER);
    }

    strcpy(sockaddr.sun_path, task->info.listen.endpoint);
    unlink(sockaddr.sun_path);

    if (bind(sock, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) == -1)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    // ignore errors
    chmod(sockaddr.sun_path, task->info.listen.perms);

    task->fd = sock;
    sock = -1;

done:

    return status;

error:

    if (sock != -1)
    {
        close(sock);
    }

    goto done;
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
        /* Create and bind socket if needed */
        if ((*task)->fd == -1)
        {
            BAIL_ON_ERROR(status = lwmsg_server_task_create_local_socket(
                              server,
                              *task));
        }

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

        if ((*task)->info.listen.endpoint)
        {
            LWMSG_LOG_INFO(server->context, "Listening on endpoint %s", (*task)->info.listen.endpoint);
        }
        else
        {
            LWMSG_LOG_INFO(server->context, "Listening on fd %i", (*task)->fd);
        }

        /* lwmsg_server_startup() waits for all endpoints to be
           ready before returning, so update the counter and wake
           it if needed */
        lwmsg_server_lock(server);
        server->num_running_endpoints++;
        if (server->num_running_endpoints == server->num_endpoints)
        {
            pthread_cond_signal(&server->event);
        }
        lwmsg_server_unlock(server);
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
    case SERVER_TASK_BEGIN_CALL:
        BAIL_ON_ERROR(status = lwmsg_server_task_perform_call(
                          server,
                          thread,
                          task));
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
        /* Finishing calls has separate logic */
        break;
    case SERVER_TASK_FINISH_CALL:
        BAIL_ON_ERROR(status = lwmsg_server_task_finish_call(
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
