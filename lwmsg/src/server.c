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

    memset(&server->timeout, 0xFF, sizeof(server->timeout));

    lwmsg_context_setup(&server->context, &protocol->context);

    err = pthread_mutex_init(&server->lock, NULL);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }
    err = pthread_mutex_init(&server->io.lock, NULL);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }
    err = pthread_mutex_init(&server->dispatch.lock, NULL);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    err = pthread_cond_init(&server->event, NULL);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }
    err = pthread_cond_init(&server->dispatch.event, NULL);
    if (err)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    lwmsg_ring_init(&server->io_tasks);
    lwmsg_ring_init(&server->dispatch.tasks);

    BAIL_ON_ERROR(status = lwmsg_shared_session_manager_new(&server->manager));

    server->max_clients = 100;
    server->max_io = 2;
    server->max_dispatch = 4;
    server->max_backlog = 8;
    server->protocol = protocol;

    *out_server = server;

error:

    return status;
}

void
lwmsg_server_delete(
    LWMsgServer* server
    )
{
    lwmsg_context_cleanup(&server->context);
    lwmsg_session_manager_delete(server->manager);

    pthread_mutex_destroy(&server->lock);
    pthread_mutex_destroy(&server->io.lock);
    pthread_mutex_destroy(&server->dispatch.lock);

    pthread_cond_destroy(&server->event);
    pthread_cond_destroy(&server->dispatch.event);

    if (server->dispatch.vector)
    {
        free(server->dispatch.vector);
    }

    free(server);
}

LWMsgStatus
lwmsg_server_set_timeout(
    LWMsgServer* server,
    LWMsgTimeout type,
    LWMsgTime* value
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTime* target = NULL;

    lwmsg_server_lock(server);

    if (server->state != SERVER_STATE_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    if (value != NULL &&
        (value->seconds < 0 || value->microseconds < 0))
    {
        SERVER_RAISE_ERROR(server, status = LWMSG_STATUS_INVALID_PARAMETER,
                          "Invalid (negative) timeout value");
    }

    switch (type)
    {
    case LWMSG_TIMEOUT_MESSAGE:
        target = &server->timeout.message;
        break;
    case LWMSG_TIMEOUT_ESTABLISH:
        target = &server->timeout.establish;
        break;
    case LWMSG_TIMEOUT_IDLE:
        target = &server->timeout.idle;
        break;
    default:
        SERVER_RAISE_ERROR(server, status = LWMSG_STATUS_UNSUPPORTED,
                          "Unsupported timeout type");
    }

    if (value)
    {
        *target = *value;
    }
    else
    {
        memset(target, 0xFF, sizeof(*target));
    }

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

    if (server->state != SERVER_STATE_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    server->max_clients = max_clients;

error:

    lwmsg_server_unlock(server);

    return status;
}

LWMsgStatus
lwmsg_server_set_max_io(
    LWMsgServer* server,
    unsigned int max_io
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_server_lock(server);

    if (server->state != SERVER_STATE_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    server->max_io = max_io;

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

    if (server->state != SERVER_STATE_STOPPED)
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

    if (server->state != SERVER_STATE_STOPPED)
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
    LWMsgDispatchSpec** new_vector = NULL;
    size_t i;

    lwmsg_server_lock(server);

    for (i = 0; table[i].type != LWMSG_DISPATCH_TYPE_END; i++)
    {
        if (table[i].tag > max_message_tag)
        {
            max_message_tag = table[i].tag;
        }
    }

    if (server->dispatch.vector_length < max_message_tag + 1)
    {
        new_vector = realloc(server->dispatch.vector, sizeof(*new_vector) * (max_message_tag + 1));

        if (!new_vector)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
        }

        /* Zero out new elements of vector */
        memset(new_vector + server->dispatch.vector_length, 0,
               (max_message_tag + 1 - server->dispatch.vector_length) * sizeof(*new_vector));

        server->dispatch.vector_length = max_message_tag + 1;
        server->dispatch.vector = new_vector;
    }

    for (i = 0; table[i].type != LWMSG_DISPATCH_TYPE_END; i++)
    {
        server->dispatch.vector[table[i].tag] = &table[i];
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
    ServerTask* task = NULL;

    lwmsg_server_lock(server);

    if (server->state != SERVER_STATE_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    if (fd < 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_PARAMETER);
    }

    BAIL_ON_ERROR(status = lwmsg_server_task_new_listen(fd, &task));

    lwmsg_ring_insert_after(&server->io_tasks, &task->ring);

error:

    lwmsg_server_unlock(server);

    return status;
}

static LWMsgStatus
lwmsg_server_create_local_socket(
    LWMsgServer* server,
    const char* endpoint,
    mode_t mode,
    int* fd
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

    if (strlen(endpoint) > sizeof(sockaddr.sun_path))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_PARAMETER);
    }

    strcpy(sockaddr.sun_path, endpoint);
    unlink(sockaddr.sun_path);

    if (bind(sock, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) == -1)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    if (chmod(sockaddr.sun_path, mode) < 0)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    *fd = sock;

error:

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
    ServerTask* task = NULL;
    int fd = -1;

    lwmsg_server_lock(server);

    if (server->state != SERVER_STATE_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    if (endpoint == NULL)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_PARAMETER);
    }

    switch (mode)
    {
    case LWMSG_SERVER_MODE_LOCAL:
        BAIL_ON_ERROR(status = lwmsg_server_create_local_socket(
                          server,
                          endpoint,
                          permissions,
                          &fd));
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_UNSUPPORTED);
    }

    BAIL_ON_ERROR(status = lwmsg_server_task_new_listen(fd, &task));
    fd = -1;

    lwmsg_ring_insert_after(&server->io_tasks, &task->ring);
    task = NULL;

error:

    lwmsg_server_unlock(server);

    if (fd != -1)
    {
        close(fd);
    }

    if (task)
    {
        lwmsg_server_task_delete(task);
    }

    return status;
}

static
LWMsgStatus
lwmsg_server_init_dispatch_thread(
    LWMsgServer* server,
    ServerDispatchThread* dispatch_thread
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    dispatch_thread->server = server;

    if (pthread_create(
            &dispatch_thread->thread,
            NULL,
            lwmsg_server_dispatch_thread,
            dispatch_thread))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
lwmsg_server_init_io_thread(
    LWMsgServer* server,
    ServerIoThread* io_thread
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool lock_init = LWMSG_FALSE;
    LWMsgBool event_init = LWMSG_FALSE;

    io_thread->server = server;
    lwmsg_ring_init((LWMsgRing*) &io_thread->tasks);
    if (pthread_mutex_init(&io_thread->lock, NULL))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }
    lock_init = LWMSG_TRUE;

    if (pipe(io_thread->event))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }
    event_init = LWMSG_TRUE;

    if (pthread_create(
            &io_thread->thread,
            NULL,
            lwmsg_server_io_thread,
            io_thread))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

done:

    return status;

error:

    if (lock_init)
    {
        pthread_mutex_destroy(&io_thread->lock);
    }

    if (event_init)
    {
        close(io_thread->event[0]);
        close(io_thread->event[1]);
    }

    goto done;
}

static
void
lwmsg_server_destroy_io_thread(
    ServerIoThread* thread
    )
{
    pthread_mutex_lock(&thread->lock);
    thread->shutdown = LWMSG_TRUE;
    thread->num_events++;
    lwmsg_server_signal_io_thread(thread);
    pthread_mutex_unlock(&thread->lock);

    pthread_join(thread->thread, NULL);

    pthread_mutex_destroy(&thread->lock);
    close(thread->event[0]);
    close(thread->event[1]);
}

static
void
lwmsg_server_destroy_dispatch_thread(
    ServerDispatchThread* thread
    )
{
    pthread_join(thread->thread, NULL);
}

static
LWMsgStatus
lwmsg_server_startup(
    LWMsgServer* server
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgRing* iter = NULL;
    LWMsgRing* next = NULL;
    ServerTask* task = NULL;
    size_t io_index = 0;
    size_t dispatch_index = 0;
    ServerIoThread* io_thread = NULL;
    ServerDispatchThread* dispatch_thread = NULL;

    /* Allocate and spin up IO threads */
    BAIL_ON_ERROR(status = LWMSG_ALLOC_ARRAY(
                      server->max_io,
                      &server->io.threads));

    for (io_index = 0; io_index < server->max_io; io_index++)
    {
        io_thread = &server->io.threads[io_index];

        BAIL_ON_ERROR(status = lwmsg_server_init_io_thread(server, io_thread));
    }

    /* Allocate and spin up dispatch threads */
    BAIL_ON_ERROR(status = LWMSG_ALLOC_ARRAY(
                      server->max_dispatch,
                      &server->dispatch.threads));

    for (dispatch_index = 0; dispatch_index <  server->max_dispatch; dispatch_index++)
    {
        dispatch_thread = &server->dispatch.threads[dispatch_index];

        BAIL_ON_ERROR(status = lwmsg_server_init_dispatch_thread(server, dispatch_thread));
    }

    /* Inject initial IO tasks into IO threads */
    for (iter = server->io_tasks.next; iter != &server->io_tasks; iter = next)
    {
        next = iter->next;
        task = LWMSG_OBJECT_FROM_MEMBER(iter, ServerTask, ring);

        lwmsg_ring_remove(iter);
        lwmsg_server_queue_io_task(server, task);
    }

done:

    return status;

error:

    if (server->io.threads)
    {
        if (io_index)
        {
            for (--io_index; io_index; --io_index)
            {
                lwmsg_server_destroy_io_thread(&server->io.threads[io_index]);
            }
        }

        free(server->io.threads);
        server->io.threads = NULL;
    }

    if (server->dispatch.threads)
    {
        if (dispatch_index)
        {
            pthread_mutex_lock(&server->dispatch.lock);
            server->dispatch.shutdown = LWMSG_TRUE;
            pthread_cond_broadcast(&server->dispatch.event);
            pthread_mutex_unlock(&server->dispatch.lock);

            for (--dispatch_index; dispatch_index; --dispatch_index)
            {
                lwmsg_server_destroy_dispatch_thread(&server->dispatch.threads[dispatch_index]);
            }
        }

        free(server->dispatch.threads);
        server->dispatch.threads = NULL;
    }

    goto done;
}

LWMsgStatus
lwmsg_server_start(
    LWMsgServer* server
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool locked = LWMSG_FALSE;

    SERVER_LOCK(server, locked);

    if (server->state == SERVER_STATE_STOPPED)
    {
        server->state = SERVER_STATE_STARTING;
        SERVER_UNLOCK(server, locked);

        BAIL_ON_ERROR(status = lwmsg_server_startup(server));

        SERVER_LOCK(server, locked);
        server->state = SERVER_STATE_STARTED;
        pthread_cond_broadcast(&server->event);
    }
    else if (server->state == SERVER_STATE_STARTING)
    {
        /* Wait for someone else to finish starting server */
        while (server->state == SERVER_STATE_STARTING)
        {
            pthread_cond_wait(&server->event, &server->lock);
        }
    }

    if (server->state == SERVER_STATE_ERROR)
    {
        BAIL_ON_ERROR(status = server->error);
    }

done:

    SERVER_UNLOCK(server, locked);

    return status;

error:

    SERVER_LOCK(server, locked);
    server->state = SERVER_STATE_ERROR;
    server->error = status;
    pthread_cond_broadcast(&server->event);

    goto done;
}

static
LWMsgStatus
lwmsg_server_shutdown(
    LWMsgServer* server
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerIoThread* io_thread = NULL;
    ServerDispatchThread* dispatch_thread = NULL;
    LWMsgRing* iter = NULL;
    LWMsgRing* next = NULL;
    ServerTask* task = NULL;
    size_t i = 0;

    /* Notify IO threads */
    for (i = 0; i < server->max_io; i++)
    {
        io_thread = &server->io.threads[i];

        pthread_mutex_lock(&io_thread->lock);
        io_thread->shutdown = LWMSG_TRUE;
        io_thread->num_events++;
        lwmsg_server_signal_io_thread(io_thread);
        pthread_mutex_unlock(&io_thread->lock);
    }

    /* Notify dispatch threads */
    pthread_mutex_lock(&server->dispatch.lock);
    server->dispatch.shutdown = LWMSG_TRUE;
    pthread_cond_broadcast(&server->dispatch.event);
    pthread_mutex_unlock(&server->dispatch.lock);

    /* Clean up IO threads */
    for (i = 0; i < server->max_io; i++)
    {
        io_thread = &server->io.threads[i];

        pthread_join(io_thread->thread, NULL);

        for (iter = io_thread->tasks.next; iter != &io_thread->tasks; iter = next)
        {
            next = iter->next;

            task = LWMSG_OBJECT_FROM_MEMBER(iter, ServerTask, ring);

            lwmsg_server_task_delete(task);
        }

        pthread_mutex_destroy(&io_thread->lock);
        close(io_thread->event[0]);
        close(io_thread->event[1]);
    }

    free(server->io.threads);
    server->io.threads = NULL;

    /* Clean up dispatch threads */
    for (i = 0; i < server->max_dispatch; i++)
    {
        dispatch_thread = &server->dispatch.threads[i];

        pthread_join(dispatch_thread->thread, NULL);
    }

    for (iter = server->dispatch.tasks.next; iter != &server->dispatch.tasks; iter = next)
    {
         next = iter->next;

         task = LWMSG_OBJECT_FROM_MEMBER(iter, ServerTask, ring);

         lwmsg_server_task_delete(task);
    }

    free(server->dispatch.threads);
    server->dispatch.threads = NULL;

    return status;
}

LWMsgStatus
lwmsg_server_stop(
    LWMsgServer* server
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool locked = LWMSG_FALSE;

    SERVER_LOCK(server, locked);

    if (server->state == SERVER_STATE_STARTED)
    {
        server->state = SERVER_STATE_STOPPING;
        SERVER_UNLOCK(server, locked);

        BAIL_ON_ERROR(status = lwmsg_server_shutdown(server));

        SERVER_LOCK(server, locked);
        server->state = SERVER_STATE_STOPPED;
        pthread_cond_broadcast(&server->event);
    }
    else if (server->state == SERVER_STATE_STOPPING)
    {
        /* Wait for someone else to finish stopping server */
        while (server->state == SERVER_STATE_STOPPING)
        {
            pthread_cond_wait(&server->event, &server->lock);
        }
    }

    if (server->state == SERVER_STATE_ERROR)
    {
        BAIL_ON_ERROR(status = server->error);
    }

done:

    SERVER_UNLOCK(server, locked);

    return status;

error:

    SERVER_LOCK(server, locked);
    server->state = SERVER_STATE_ERROR;
    server->error = status;
    pthread_cond_broadcast(&server->event);

    goto done;
}

LWMsgStatus
lwmsg_server_set_session_functions(
    LWMsgServer* server,
    LWMsgSessionConstructor construct,
    LWMsgSessionDestructor destruct,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_server_lock(server);

    if (server->state != SERVER_STATE_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    server->session_construct = construct;
    server->session_destruct = destruct;
    server->session_construct_data = data;

error:

    lwmsg_server_unlock(server);

    return status;
}

LWMsgStatus
lwmsg_server_set_dispatch_data(
    LWMsgServer* server,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    lwmsg_server_lock(server);

    if (server->state != SERVER_STATE_STOPPED)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_STATE);
    }

    server->dispatch_data = data;

error:

    lwmsg_server_unlock(server);

    return status;
}

void*
lwmsg_server_get_dispatch_data(
    LWMsgServer* server
    )
{
    return server->dispatch_data;
}

void
lwmsg_server_signal_io_thread(
    ServerIoThread* thread
    )
{
    char c = 0;
    int res = 0;

    do
    {
        res = write(thread->event[1], &c, sizeof(c));
    } while (res == -1 && errno == EINTR);

    ABORT_IF_FALSE(res == sizeof(c));
}

void
lwmsg_server_queue_io_task(
    LWMsgServer* server,
    ServerTask* task
    )
{
    ServerIoThread* thread = NULL;

    pthread_mutex_lock(&server->io.lock);
    thread = &server->io.threads[server->io.next_index];
    server->io.next_index = (server->io.next_index + 1) % server->max_io;
    pthread_mutex_unlock(&server->io.lock);

    pthread_mutex_lock(&thread->lock);
    lwmsg_ring_insert_before((LWMsgRing*) &thread->tasks, &task->ring);
    thread->num_events++;
    lwmsg_server_signal_io_thread(thread);
    pthread_mutex_unlock(&thread->lock);
}

void
lwmsg_server_queue_dispatch_task(
    LWMsgServer* server,
    ServerTask* task
    )
{
    pthread_mutex_lock(&server->dispatch.lock);
    lwmsg_ring_insert_before(&server->dispatch.tasks, &task->ring);
    pthread_cond_signal(&server->dispatch.event);
    pthread_mutex_unlock(&server->dispatch.lock);
}

static
void
lwmsg_server_wake_io_threads(
    LWMsgServer* server
    )
{
    size_t i = 0;
    ServerIoThread* thread = NULL;

    for (i = 0; i < server->max_io; i++)
    {
        thread = &server->io.threads[i];
        pthread_mutex_lock(&thread->lock);
        thread->num_events++;
        lwmsg_server_signal_io_thread(thread);
        pthread_mutex_unlock(&thread->lock);
    }
}

LWMsgBool
lwmsg_server_acquire_client_slot(
    LWMsgServer* server
    )
{
    LWMsgBool result = LWMSG_FALSE;
    LWMsgBool wake = LWMSG_FALSE;

    lwmsg_server_lock(server);
    if (server->num_clients < server->max_clients)
    {
        if (++server->num_clients == server->max_clients)
        {
            wake = LWMSG_TRUE;
        }
        result = LWMSG_TRUE;
    }

    lwmsg_server_unlock(server);

    if (wake)
    {
        lwmsg_server_wake_io_threads(server);
    }

    return result;
}

void
lwmsg_server_release_client_slot(
    LWMsgServer* server
    )
{
    LWMsgBool wake = LWMSG_FALSE;

    lwmsg_server_lock(server);
    if (server->num_clients-- == server->max_clients)
    {
        wake = LWMSG_TRUE;
    }
    lwmsg_server_unlock(server);

    if (wake)
    {
        lwmsg_server_wake_io_threads(server);
    }
}

size_t
lwmsg_server_get_num_clients(
    LWMsgServer* server
    )
{
    size_t result = 0;

    lwmsg_server_lock(server);
    result = server->num_clients;
    lwmsg_server_unlock(server);

    return result;
}
