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
lwmsg_server_wait_client_queued(
    LWMsgServer* server
    )
{
    pthread_cond_wait(&server->client_queued, &server->lock);
}

static void
lwmsg_server_wait_client_dequeued(
    LWMsgServer* server
    )
{
    pthread_cond_wait(&server->client_dequeued, &server->lock);
}

static void
lwmsg_server_wait_state_changed(
    LWMsgServer* server
    )
{
    pthread_cond_wait(&server->state_changed, &server->lock);
}

static void
lwmsg_server_signal_client_queued(
    LWMsgServer* server
    )
{
    pthread_cond_signal(&server->client_queued);
}

static void
lwmsg_server_signal_client_dequeued(
    LWMsgServer* server
    )
{
    pthread_cond_signal(&server->client_dequeued);
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
lwmsg_server_queue_client(
    LWMsgServer* server,
    int sock
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;
    size_t i;

    BAIL_ON_ERROR(status = lwmsg_connection_new(server->protocol, &assoc));

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
        SERVER_RAISE_ERROR(server, status = LWMSG_STATUS_INVALID, "Invalid connection mode");
        break;
    }

    /* Wait for an empty slot to become available */
    while (server->num_pending_assocs >= server->max_backlog && server->state == LWMSG_SERVER_RUNNING)
    {
        lwmsg_server_wait_client_dequeued(server);
    }

    if (server->state == LWMSG_SERVER_SHUTDOWN)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INTERRUPT);
    }

    for (i = 0; i < server->max_backlog; i++)
    {
        if (server->pending_assocs[i] == NULL)
        {
            server->pending_assocs[i] = assoc;
            break;
        }
    }

    server->num_pending_assocs++;
    lwmsg_server_signal_client_queued(server);

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

    while (1)
    {
        FD_ZERO(&readfds);
        nfds = 0;

        FD_SET(server->fd, &readfds);
        if (nfds < server->fd + 1)
        {
            nfds = server->fd + 1;
        }

        FD_SET(server->interrupt->fd[0], &readfds);
        if (nfds < server->interrupt->fd[0] + 1)
        {
            nfds = server->interrupt->fd[0] + 1;
        }

        do
        {
            ret = select(nfds, &readfds, NULL, NULL, NULL);
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
        else if (ret == 0)
        {
            continue;
        }

        if (FD_ISSET(server->interrupt->fd[0], &readfds))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_INTERRUPT);
        }

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

            SERVER_LOCK(server, locked);

            if (server->state == LWMSG_SERVER_SHUTDOWN)
            {
                break;
            }

            BAIL_ON_ERROR(status = lwmsg_server_queue_client(server, sock));

            SERVER_UNLOCK(server, locked);
        }
    }

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
lwmsg_server_client_thread(void* arg)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgServer* server = (LWMsgServer*) arg;
    LWMsgAssoc* assoc = NULL;
    int locked = 0;
    size_t i;

    while (1)
    {
        SERVER_LOCK(server, locked);

        /* Wait for a client to be queued */
        while (server->num_pending_assocs == 0 && server->state == LWMSG_SERVER_RUNNING)
        {
            lwmsg_server_wait_client_queued(server);
        }

        if (server->state == LWMSG_SERVER_SHUTDOWN)
        {
            break;
        }

        for (i = 0; i < server->max_backlog; i++)
        {
            if (server->pending_assocs[i])
            {
                assoc = server->pending_assocs[i];
                server->pending_assocs[i] = NULL;
                break;
            }
        }

        server->num_pending_assocs--;

        lwmsg_server_signal_client_dequeued(server);

        SERVER_UNLOCK(server, locked);

        /* Invoke connection callback */
        if (server->connect_callback)
        {
            status = server->connect_callback(
                server,
                assoc,
                server->user_data);
        }

        /* Don't pump messages if the connect callback complained */
        if (!status)
        {
            do
            {
                status = lwmsg_assoc_recv_message_transact(
                    assoc,
                    lwmsg_server_dispatch_message,
                    server
                    );
            } while (status == LWMSG_STATUS_SUCCESS);
        }

        /* Connection errors are not fatal to the thread, so handle them */
        switch (status)
        {
        case LWMSG_STATUS_EOF:
        case LWMSG_STATUS_MALFORMED:
        case LWMSG_STATUS_OVERFLOW:
        case LWMSG_STATUS_UNDERFLOW:
        case LWMSG_STATUS_SECURITY:
            status = LWMSG_STATUS_SUCCESS;
            break;
        default:
            break;
        }
        BAIL_ON_ERROR(status);

        /* Shut down and free the association
           This is safe to do outside of a mutex as assoc is not shared */
        BAIL_ON_ERROR(status = lwmsg_assoc_close(assoc));

        lwmsg_assoc_delete(assoc);
        assoc = NULL;
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
    LWMsgServer** out_server
    )
{
    return lwmsg_server_new_with_context(NULL, out_server);
}

LWMsgStatus
lwmsg_server_new_with_context(
    LWMsgContext* context,
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
    server->max_clients = 4;
    server->max_backlog = 4;

    lwmsg_context_setup(&server->context, context);

    err = pthread_mutex_init(&server->lock, NULL);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    err = pthread_cond_init(&server->client_queued, NULL);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    err = pthread_cond_init(&server->client_dequeued, NULL);
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

    lwmsg_protocol_delete(server->protocol);
    lwmsg_connection_signal_delete(server->interrupt);
    lwmsg_session_manager_delete(server->manager);
    free(server->endpoint);
    free(server->worker_threads);
    free(server->pending_assocs);
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
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
    }

    if (timeout->seconds < 0 || timeout->microseconds < 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
    }

    server->timeout_set = 1;
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
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
    }

    server->max_clients = max_clients;

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
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
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
lwmsg_server_add_protocol_spec(
    LWMsgServer* server,
    LWMsgProtocolSpec* spec
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_server_lock(server);

    if (server->state != LWMSG_SERVER_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
    }

    if (server->protocol && !server->protocol_is_private)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
    }

    if (!server->protocol)
    {
        BAIL_ON_ERROR(status = lwmsg_protocol_new_with_context(&server->context, &server->protocol));
        server->protocol_is_private = 1;
    }

    PROPAGATE_ERROR(&server->protocol->context, &server->context,
                    status = lwmsg_protocol_add_protocol_spec(server->protocol, spec));

error:

    lwmsg_server_unlock(server);

    return status;
}

LWMsgStatus
lwmsg_server_set_protocol(
    LWMsgServer* server,
    LWMsgProtocol* prot
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_server_lock(server);

    if (server->state != LWMSG_SERVER_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
    }

    if (server->protocol && server->protocol_is_private)
    {
        lwmsg_protocol_delete(server->protocol);
    }

    server->protocol = prot;
    server->protocol_is_private = LWMSG_FALSE;

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

    if (server->state != LWMSG_SERVER_STOPPED || server->fd != -1 || server->endpoint != NULL || fd < 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
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

    if (server->state != LWMSG_SERVER_STOPPED || server->fd != -1 || server->endpoint != NULL || endpoint == NULL)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
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
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
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
            BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
            break;
        }
    }

    /* Allocate and populate thread pool */
    server->worker_threads = calloc(server->max_clients, sizeof(*server->worker_threads));
    if (!server->worker_threads)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    for (i = 0; i < server->max_clients; i++)
    {
        err = pthread_create(
            &server->worker_threads[i],
            NULL,
            lwmsg_server_client_thread,
            server);
        if (err)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
        }
    }

    /* Allocate assoc queue */
    server->pending_assocs = calloc(server->max_backlog, sizeof(*server->pending_assocs));
    if (!server->pending_assocs)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
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

    SERVER_LOCK(server, locked);

    lwmsg_server_change_state(server, LWMSG_SERVER_SHUTDOWN);

    /* Interrupt all worker_threads waiting on I/O */
    BAIL_ON_ERROR(status = lwmsg_connection_signal_raise(server->interrupt));

    /* Interrupt all worker_threads waiting on a condition */
    err = pthread_cond_broadcast(&server->client_queued);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }
    err = pthread_cond_broadcast(&server->client_dequeued);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    if (server->pending_assocs)
    {
        /* Close all pending associations */
        for (i = 0; i < server->max_backlog; i++)
        {
            if (server->pending_assocs[i] != NULL)
            {
                lwmsg_assoc_delete(server->pending_assocs[i]);
                server->pending_assocs[i] = NULL;
                server->num_pending_assocs--;
            }
        }
    }

    SERVER_UNLOCK(server, locked);

    /** FIXME: using a pthread_t this way is theoretically iffy according to POSIX */
    if (server->listen_thread)
    {
        /* Wait for listener thread to stop */
        err = pthread_join(server->listen_thread, NULL);
        if (err)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
        }
    }

    if (server->worker_threads)
    {
        /* Wait for worker_threads in thread pool to stop */
        for (i = 0; i < server->max_clients; i++)
        {
            err = pthread_join(server->worker_threads[i], NULL);
            if (err)
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
            }
        }
    }

    SERVER_LOCK(server, locked);
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
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
    }

    server->connect_callback = func;

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
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID);
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
