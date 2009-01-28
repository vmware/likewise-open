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
#include "util-private.h"
#include "connection-private.h"
#include "protocol-private.h"
#include "session-private.h"
#include "assoc-private.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

static LWMsgStatus
assoc_queue_setup(
    LWMsgServer* server,
    AssocQueue* queue,
    size_t size
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    queue->size = size;
    queue->count = 0;
    queue->queue = calloc(size, sizeof(*(queue->queue)));

    if (!queue->queue)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    if (pthread_cond_init(&queue->event, NULL))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

done:

    return status;

error:

    goto done;
}

static void
assoc_queue_teardown(
    LWMsgServer* server,
    AssocQueue* queue
    )
{
    size_t i;
    pthread_cond_destroy(&queue->event);

    if (queue->queue)
    {
        for (i = 0; i < queue->size; i++)
        {
            if (queue->queue[i])
            {
                lwmsg_assoc_delete(queue->queue[i]);
            }
        }
        free(queue->queue);
    }
}

static void
assoc_queue_add(
    LWMsgServer* server,
    AssocQueue* queue,
    LWMsgAssoc* assoc
    )
{
    size_t i;

    while (queue->count == queue->size && server->state == LWMSG_SERVER_RUNNING)
    {
        pthread_cond_wait(&queue->event, &server->lock);
    }

    if (server->state == LWMSG_SERVER_SHUTDOWN)
    {
        return;
    }

    for (i = 0; i < queue->size; i++)
    {
        if (queue->queue[i] == NULL)
        {
            queue->queue[i] = assoc;
            queue->count++;
            pthread_cond_signal(&queue->event);
            break;
        }
    }
}

static void
assoc_queue_remove(
    LWMsgServer* server,
    AssocQueue* queue,
    LWMsgAssoc** assoc
    )
{
    size_t i;

    while (queue->count == 0 && server->state == LWMSG_SERVER_RUNNING)
    {
        pthread_cond_wait(&queue->event, &server->lock);
    }

    if (server->state == LWMSG_SERVER_SHUTDOWN)
    {
        *assoc = NULL;
        return;
    }

    for (i = 0; i < queue->size; i++)
    {
        if (queue->queue[i])
        {
            *assoc = queue->queue[i];
            queue->queue[i] = NULL;
            queue->count--;
            pthread_cond_signal(&queue->event);
            break;
        }
    }
}

static void
assoc_queue_remove_at_index(
    LWMsgServer* server,
    AssocQueue* queue,
    size_t index,
    LWMsgAssoc** assoc
    )
{
    if (assoc)
    {
        *assoc = queue->queue[index];
    }

    queue->queue[index] = NULL;
    queue->count--;
    pthread_cond_signal(&queue->event);
}

static void
lwmsg_server_lock(
    LWMsgServer* server
    )
{
    pthread_mutex_lock(&server->lock);
}

static void
lwmsg_server_unlock(
    LWMsgServer* server
    )
{
    pthread_mutex_unlock(&server->lock);
}

static void
lwmsg_server_wait_state_changed(
    LWMsgServer* server
    )
{
    pthread_cond_wait(&server->state_changed, &server->lock);
}

static void
lwmsg_server_change_state(
    LWMsgServer* server,
    LWMsgServerState state
    )
{
    server->state = state;
    pthread_cond_broadcast(&server->state_changed);
}


/* Should be called with server locked */
static LWMsgStatus
lwmsg_server_accept_client(
    LWMsgServer* server,
    int sock
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;
    ConnectionPrivate* priv = NULL;

    BAIL_ON_ERROR(status = lwmsg_connection_new(server->protocol, &assoc));

    priv = lwmsg_assoc_get_private(assoc);

    /* Make assoc's internal context route through us */
    assoc->context.parent = &server->context;

    if (server->timeout_set)
    {
        lwmsg_assoc_set_timeout(assoc, &server->timeout);
    }

    if (server->interrupt)
    {
        lwmsg_connection_set_interrupt_signal(assoc, server->interrupt);
    }

    if (server->manager)
    {
        lwmsg_assoc_set_session_manager(assoc, server->manager);
    }

    switch (server->mode)
    {
    case LWMSG_SERVER_MODE_LOCAL:
        BAIL_ON_ERROR(status = lwmsg_connection_set_fd(assoc, LWMSG_CONNECTION_MODE_LOCAL, sock));
        break;
    case LWMSG_SERVER_MODE_REMOTE:
        BAIL_ON_ERROR(status = lwmsg_connection_set_fd(assoc, LWMSG_CONNECTION_MODE_REMOTE, sock));
        break;
    default:
        SERVER_RAISE_ERROR(server, status = LWMSG_STATUS_INVALID_STATE, "Invalid connection mode");
        break;
    }

    BAIL_ON_ERROR(status = lwmsg_connection_establish(assoc));

    /* Invoke connection callback */
    if (server->connect_callback)
    {
        status = server->connect_callback(
            server,
            assoc,
            server->user_data);
    }

    if (status == LWMSG_STATUS_SUCCESS &&
        server->session_callback && priv->is_session_leader)
    {
        status = server->session_callback(
            server,
            assoc,
            server->user_data);
    }

    /* Handle error from callback */
    switch(status)
    {
    case LWMSG_STATUS_SUCCESS:
        /* Queue assocation for listening */
        assoc_queue_add(server, &server->listen_assocs, assoc);
        server->num_clients++;
        break;
    default:
        /* Throw out association */
        lwmsg_assoc_close(assoc);
        lwmsg_assoc_delete(assoc);
        status = LWMSG_STATUS_SUCCESS;
        break;
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_server_timeout_clients(
    LWMsgServer* server,
    LWMsgTime* next
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i;
    LWMsgTime now;
    LWMsgTime abs_timeout;
    LWMsgTime soonest_timeout;
    ConnectionPrivate* priv = NULL;
    LWMsgAssoc* assoc = NULL;
    LWMsgServerTimeoutLevel level;
    LWMsgSession* session = NULL;
    size_t num_handles;
    size_t num_assocs;

    BAIL_ON_ERROR(status = lwmsg_time_now(&now));

    soonest_timeout.seconds = 0x7fffffff;
    soonest_timeout.microseconds = 0x7fffffff;

    /* Move through our available levels of aggressiveness
       until we manage to free up some client slots */
    for (level = LWMSG_SERVER_TIMEOUT_SAFE;
         server->num_clients == server->max_clients && level < LWMSG_SERVER_TIMEOUT_END;
         level++)
    {
        for (i = 0; i < server->listen_assocs.size; i++)
        {
            assoc = server->listen_assocs.queue[i];

            if (assoc)
            {
                priv = lwmsg_assoc_get_private(assoc);

                lwmsg_time_sum(&priv->last_time, &server->timeout, &abs_timeout);

                /* If the connection has exceeded its timeout */
                if (lwmsg_time_compare(&abs_timeout, &now) == LWMSG_TIME_LESSER)
                {
                    /* Find out how many connections and handles are in its session */
                    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, &session));
                    num_assocs = lwmsg_session_manager_get_session_assoc_count(server->manager, session);
                    num_handles = lwmsg_session_manager_get_session_handle_count(server->manager, session);

                    if (num_assocs == 1 &&
                        num_handles > 0 &&
                        level <= LWMSG_SERVER_TIMEOUT_SAFE)
                    {
                        /* Try not to close a connection if it is the last
                           in its session and there are handles that would be
                           invalidated */
                        continue;
                    }
                    else
                    {
                        /* We don't care if the reset fails */
                        lwmsg_assoc_reset(assoc);
                        lwmsg_assoc_delete(assoc);
                        assoc_queue_remove_at_index(server, &server->listen_assocs, i, NULL);
                        server->num_clients--;
                    }
                }
                else
                {
                    /* Calculate when we'll next need to wake up */
                    if (lwmsg_time_compare(&abs_timeout, &soonest_timeout) == LWMSG_TIME_LESSER)
                    {
                        soonest_timeout = abs_timeout;
                    }
                }
            }
        }
    }

    /* Schedule ourselves to wake up and do timeout processing again if:
       - We are out of free client slots
       - There are currently idle clients
    */
    if (server->num_clients == server->max_clients && server->listen_assocs.count)
    {
        *next = soonest_timeout;
    }
    else
    {
        next->seconds = -1;
        next->microseconds = -1;
    }

error:

    return status;
}


static void*
lwmsg_server_listen_thread(
    void* arg
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgServer* server = (LWMsgServer*) arg;
    struct sockaddr clientaddr;
    socklen_t clientaddr_len;
    int sock = 0;
    int locked = 0;
    fd_set readfds;
    int nfds = 0;
    int ret = 0;
    size_t i = 0;
    size_t count = 0;
    ConnectionPrivate* priv = NULL;
    char c = 0;
    LWMsgAssoc* assoc = NULL;
    LWMsgTime next;
    LWMsgTime timeout;
    LWMsgTime now;
    struct timeval timeval;

    SERVER_LOCK(server, locked);

    while (1)
    {
        /* Begin setting up set of descriptors to wait on */
        next.seconds = -1;
        FD_ZERO(&readfds);
        nfds = 0;

        /* We always wait on the notify fd so we can be woken up
           by another thread */
        FD_SET(server->listen_notify[0], &readfds);
        if (nfds < server->listen_notify[0] + 1)
        {
            nfds = server->listen_notify[0] + 1;
        }

        if (server->state == LWMSG_SERVER_SHUTDOWN)
        {
            goto done;
        }

        /* If we are out of client slots, start timing out old connections */
        if (server->num_clients == server->max_clients &&
            server->timeout_set)
        {
            BAIL_ON_ERROR(status = lwmsg_server_timeout_clients(server, &next));
        }

        /* Listen for more connections if we have free slots */
        if (server->num_clients < server->max_clients)
        {
            FD_SET(server->fd, &readfds);
            if (nfds < server->fd + 1)
            {
                nfds = server->fd + 1;
            }
        }

        /* Add all assocs in the listen queue to fd set */
        for (i = 0, count = server->listen_assocs.count; i < server->listen_assocs.size && count; i++)
        {
            if (server->listen_assocs.queue[i])
            {
                priv = lwmsg_assoc_get_private(server->listen_assocs.queue[i]);

                /* If there is already residual data, don't bother waiting for more */
                if (priv->recvbuffer.base_length > 0)
                {
                    assoc_queue_remove_at_index(server, &server->listen_assocs, i, &assoc);
                    assoc_queue_add(server, &server->service_assocs, assoc);

                    if (server->state == LWMSG_SERVER_SHUTDOWN)
                    {
                        goto done;
                    }

                    pthread_cond_signal(&server->listen_assocs.event);
                }
                else
                {
                    FD_SET(priv->fd, &readfds);
                    if (nfds < priv->fd + 1)
                    {
                        nfds = priv->fd + 1;
                    }
                }

                count--;
            }
        }

        SERVER_UNLOCK(server, locked);

        /* Wait for something to happen */
        do
        {
            if (next.seconds != -1)
            {
                lwmsg_time_now(&now);
                lwmsg_time_difference(&now, &next, &timeout);
                timeval.tv_sec = timeout.seconds;
                timeval.tv_usec = timeout.microseconds;
            }
            ret = select(nfds, &readfds, NULL, NULL, next.seconds == -1 ? NULL: &timeval);
        } while (ret == -1 && errno == EINTR);

        if (ret == -1)
        {
            switch (errno)
            {
            case ENOMEM:
                BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
            default:
                BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
            }
        }

        /* If someone woke us up explicitly, consume the signal byte */
        if (FD_ISSET(server->listen_notify[0], &readfds))
        {
            if (read(server->listen_notify[0], &c, 1) != 1)
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
            }
        }

        /* Lock server to access listen queue and check state */
        SERVER_LOCK(server, locked);

        if (server->state == LWMSG_SERVER_SHUTDOWN)
        {
            goto done;
        }

        /* Check for associations that are ready to be serviced */
        for (i = 0, count = server->listen_assocs.count; i < server->listen_assocs.size && count; i++)
        {
            if (server->listen_assocs.queue[i])
            {
                priv = lwmsg_assoc_get_private(server->listen_assocs.queue[i]);
                if (FD_ISSET(priv->fd, &readfds))
                {
                    assoc_queue_remove_at_index(server, &server->listen_assocs, i, &assoc);
                    assoc_queue_add(server, &server->service_assocs, assoc);

                    if (server->state == LWMSG_SERVER_SHUTDOWN)
                    {
                        goto done;
                    }

                    pthread_cond_signal(&server->listen_assocs.event);
                }
                count--;
            }
        }

        /* Check for an incoming connection */
        if (FD_ISSET(server->fd, &readfds))
        {
            clientaddr_len = sizeof(clientaddr);
            sock = accept(server->fd, &clientaddr, &clientaddr_len);

            if (sock == -1)
            {
                switch (errno)
                {
                case EMFILE:
                case ENFILE:
                    BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
                default:
                    BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
                }
            }

            BAIL_ON_ERROR(status = lwmsg_server_accept_client(server, sock));
        }
    }

done:

error:

    SERVER_UNLOCK(server, locked);

    return NULL;
}

static
LWMsgStatus
lwmsg_server_dispatch_message(
    LWMsgAssoc* assoc,
    const LWMsgMessage* recv_message,
    LWMsgMessage* send_message,
    void* data
    )
{
    LWMsgServer* server = (LWMsgServer*) data;

    return server->dispatch_vector[recv_message->tag](
        assoc,
        recv_message,
        send_message,
        server->user_data);
}

static void*
lwmsg_server_worker_thread(void* arg)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgServer* server = (LWMsgServer*) arg;
    LWMsgAssoc* assoc = NULL;
    int locked = 0;
    char c = 0;

    while (1)
    {
        SERVER_LOCK(server, locked);

        assoc_queue_remove(server, &server->service_assocs, &assoc);

        if (server->state == LWMSG_SERVER_SHUTDOWN)
        {
            break;
        }

        SERVER_UNLOCK(server, locked);

        status = lwmsg_assoc_recv_message_transact(
            assoc,
            lwmsg_server_dispatch_message,
            server
            );

        /* Connection errors are not fatal to the thread, so handle them */
        switch (status)
        {
        case LWMSG_STATUS_SUCCESS:
            /* Put association back on listen queue */
            SERVER_LOCK(server, locked);
            assoc_queue_add(server, &server->listen_assocs, assoc);
            SERVER_UNLOCK(server, locked);
            break;
        default:
            /* Shut down and free the association */
            /* Ignore subsequent errors when attempting close */
            lwmsg_assoc_close(assoc);
            lwmsg_assoc_delete(assoc);
            SERVER_LOCK(server, locked);
            server->num_clients--;
            SERVER_UNLOCK(server, locked);
            status = LWMSG_STATUS_SUCCESS;
            break;
        }

        if (write(server->listen_notify[1], &c, 1) != 1)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
        }
    }

error:

    SERVER_UNLOCK(server, locked);

    if (assoc)
    {
        lwmsg_assoc_delete(assoc);
    }

    return NULL;
}

LWMsgStatus
lwmsg_server_new(
    LWMsgProtocol* protocol,
    LWMsgServer** out_server
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgServer* server = NULL;
    int err = 0;

    server = calloc(1, sizeof(*server));

    if (!server)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    server->fd = -1;
    server->max_clients = 8;
    server->max_backlog = 4;
    server->max_dispatch = 4;
    server->protocol = protocol;

    lwmsg_context_setup(&server->context, &protocol->context);

    err = pipe(server->listen_notify);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    err = pthread_mutex_init(&server->lock, NULL);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    err = pthread_cond_init(&server->state_changed, NULL);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    BAIL_ON_ERROR(status = lwmsg_connection_signal_new(&server->interrupt));

    BAIL_ON_ERROR(status = lwmsg_shared_session_manager_new(&server->manager));

    *out_server = server;

error:

    return status;
}

void
lwmsg_server_delete(
    LWMsgServer* server
    )
{
    lwmsg_server_lock(server);

    while (server->state != LWMSG_SERVER_STOPPED)
    {
        lwmsg_server_wait_state_changed(server);
    }

    lwmsg_server_unlock(server);

    lwmsg_context_cleanup(&server->context);
    lwmsg_connection_signal_delete(server->interrupt);
    lwmsg_session_manager_delete(server->manager);
    free(server->endpoint);
    free(server->worker_threads);
    if (server->dispatch_vector)
    {
        free(server->dispatch_vector);
    }
    free(server);
}

LWMsgStatus
lwmsg_server_set_timeout(
    LWMsgServer* server,
    LWMsgTime* timeout
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_server_lock(server);

    if (server->state != LWMSG_SERVER_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    if (timeout->seconds < 0 || timeout->microseconds < 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    server->timeout_set = LWMSG_TRUE;
    server->timeout = *timeout;

error:

    lwmsg_server_unlock(server);

    return status;
}

LWMsgStatus
lwmsg_server_set_max_clients(
    LWMsgServer* server,
    unsigned int max_clients
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_server_lock(server);

    if (server->state != LWMSG_SERVER_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    server->max_clients = max_clients;

error:

    lwmsg_server_unlock(server);

    return status;
}

LWMsgStatus
lwmsg_server_set_max_dispatch(
    LWMsgServer* server,
    unsigned int max_dispatch
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_server_lock(server);

    if (server->state != LWMSG_SERVER_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    server->max_dispatch = max_dispatch;

error:

    lwmsg_server_unlock(server);

    return status;
}

LWMsgStatus
lwmsg_server_set_max_backlog(
    LWMsgServer* server,
    unsigned int max_backlog
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_server_lock(server);

    if (server->state != LWMSG_SERVER_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    server->max_backlog = max_backlog;

error:

    lwmsg_server_unlock(server);

    return status;
}

LWMsgStatus
lwmsg_server_add_dispatch_spec(
    LWMsgServer* server,
    LWMsgDispatchSpec* table
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t max_message_tag = 0;
    LWMsgDispatchFunction* new_vector = NULL;
    size_t i;

    lwmsg_server_lock(server);

    for (i = 0; table[i].func; i++)
    {
        if (table[i].tag > max_message_tag)
        {
            max_message_tag = table[i].tag;
        }
    }

    if (server->dispatch_vector_length < max_message_tag + 1)
    {
        new_vector = realloc(server->dispatch_vector, sizeof(*new_vector) * (max_message_tag + 1));

        if (!new_vector)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
        }
        
        /* Zero out new elements of vector */
        memset(new_vector + server->dispatch_vector_length, 0, 
               (max_message_tag + 1 - server->dispatch_vector_length) * sizeof(*new_vector));

        server->dispatch_vector_length = max_message_tag + 1;
        server->dispatch_vector = new_vector;
    }

    for (i = 0; table[i].func; i++)
    {
        server->dispatch_vector[table[i].tag] = table[i].func;
    }

error:

    lwmsg_server_unlock(server);

    return status;
}

LWMsgStatus
lwmsg_server_set_fd(
    LWMsgServer* server,
    LWMsgServerMode mode,
    int fd
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_server_lock(server);

    if (server->state != LWMSG_SERVER_STOPPED || server->fd != -1 || server->endpoint != NULL)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    if (fd < 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_PARAMETER);
    }

    server->mode = mode;
    server->fd = fd;

error:

    lwmsg_server_unlock(server);

    return status;
}

LWMsgStatus
lwmsg_server_set_endpoint(
    LWMsgServer* server,
    LWMsgServerMode mode,
    const char* endpoint,
    mode_t      permissions
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_server_lock(server);

    if (server->state != LWMSG_SERVER_STOPPED || server->fd != -1 || server->endpoint != NULL)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    if (endpoint == NULL)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_PARAMETER);
    }

    server->mode = mode;
    server->endpoint = strdup(endpoint);

    if (!server->endpoint)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    server->permissions = permissions;

error:

    lwmsg_server_unlock(server);

    return status;
}

static LWMsgStatus
lwmsg_server_create_local_listen_socket(
    LWMsgServer* server
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

    if (strlen(server->endpoint) > sizeof(sockaddr.sun_path))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_PARAMETER);
    }

    strcpy(sockaddr.sun_path, server->endpoint);
    unlink(sockaddr.sun_path);

    if (bind(sock, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) == -1)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    if (chmod(sockaddr.sun_path, server->permissions) < 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    if (listen(sock, server->max_backlog))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    server->fd = sock;

error:

    return status;
}

LWMsgStatus
lwmsg_server_start(
    LWMsgServer* server
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i;
    int err = 0;

    lwmsg_server_lock(server);

    /* Create listen socket if we don't have one already */
    if (server->fd == -1)
    {
        switch (server->mode)
        {
        case LWMSG_SERVER_MODE_LOCAL:
            BAIL_ON_ERROR(status = lwmsg_server_create_local_listen_socket(server));
            break;
        case LWMSG_SERVER_MODE_REMOTE:
            BAIL_ON_ERROR(status = LWMSG_STATUS_UNIMPLEMENTED);
            break;
        default:
            BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
            break;
        }
    }

    /* Create assoc queues */
    BAIL_ON_ERROR(status = assoc_queue_setup(server, &server->listen_assocs, server->max_clients));
    BAIL_ON_ERROR(status = assoc_queue_setup(server, &server->service_assocs, server->max_clients));

    /* Allocate and populate thread pool */
    server->worker_threads = calloc(server->max_dispatch, sizeof(*server->worker_threads));
    if (!server->worker_threads)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    for (i = 0; i < server->max_dispatch; i++)
    {
        err = pthread_create(
            &server->worker_threads[i],
            NULL,
            lwmsg_server_worker_thread,
            server);
        if (err)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
        }
    }

    /* Create listen thread */
    err = pthread_create(&server->listen_thread, NULL, lwmsg_server_listen_thread, server);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    lwmsg_server_change_state(server, LWMSG_SERVER_RUNNING);

error:

    lwmsg_server_unlock(server);

    return status;
}

LWMsgStatus
lwmsg_server_stop(
    LWMsgServer* server
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int err;
    size_t i;
    int locked = 0;
    char c = 0;

    SERVER_LOCK(server, locked);

    if (server->state != LWMSG_SERVER_RUNNING)
    {
        SERVER_RAISE_ERROR(server, status = LWMSG_STATUS_INVALID_STATE, "Server not running");
    }

    lwmsg_server_change_state(server, LWMSG_SERVER_SHUTDOWN);

    /* Interrupt all worker_threads waiting on I/O */
    BAIL_ON_ERROR(status = lwmsg_connection_signal_raise(server->interrupt));

    /* Interrupt all worker_threads waiting on a condition */
    err = pthread_cond_broadcast(&server->service_assocs.event);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    /* Interrupt listener thread */
    err = pthread_cond_broadcast(&server->listen_assocs.event);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    if (write(server->listen_notify[1], &c, 1) != 1)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    SERVER_UNLOCK(server, locked);

    /* Wait for listener thread to stop */
    err = pthread_join(server->listen_thread, NULL);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    /* Wait for worker_threads in thread pool to stop */
    for (i = 0; i < server->max_dispatch; i++)
    {
        err = pthread_join(server->worker_threads[i], NULL);
        if (err)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
        }
    }

    SERVER_LOCK(server, locked);

    /* Tear down queues, freeing any remaining associations */
    assoc_queue_teardown(server, &server->listen_assocs);
    assoc_queue_teardown(server, &server->service_assocs);

    BAIL_ON_ERROR(status = lwmsg_connection_signal_lower(server->interrupt));
    close(server->fd);
    server->fd = -1;

    if (server->endpoint && server->mode == LWMSG_SERVER_MODE_LOCAL)
    {
        unlink(server->endpoint);
    }

    lwmsg_server_change_state(server, LWMSG_SERVER_STOPPED);
    SERVER_UNLOCK(server, locked);

error:

    SERVER_UNLOCK(server, locked);

    return status;
}

LWMsgStatus
lwmsg_server_set_connect_callback(
    LWMsgServer* server,
    LWMsgServerConnectFunction func
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_server_lock(server);

    if (server->state != LWMSG_SERVER_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    server->connect_callback = func;

error:

    lwmsg_server_unlock(server);

    return status;
}

LWMsgStatus
lwmsg_server_set_session_callback(
    LWMsgServer* server,
    LWMsgServerConnectFunction func
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_server_lock(server);

    if (server->state != LWMSG_SERVER_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    server->session_callback = func;

error:

    lwmsg_server_unlock(server);

    return status;
}

LWMsgStatus
lwmsg_server_set_user_data(
    LWMsgServer* server,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_server_lock(server);

    if (server->state != LWMSG_SERVER_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    server->user_data = data;

error:

    lwmsg_server_unlock(server);

    return status;
}

void*
lwmsg_server_get_user_data(
    LWMsgServer* server
    )
{
    return server->user_data;
}
