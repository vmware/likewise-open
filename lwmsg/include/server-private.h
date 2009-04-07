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
 *        server-private.h
 *
 * Abstract:
 *
 *        Multi-threaded server API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_SERVER_PRIVATE_H__
#define __LWMSG_SERVER_PRIVATE_H__

#include <lwmsg/server.h>
#include <lwmsg/protocol.h>
#include <lwmsg/connection.h>
#include <lwmsg/assoc.h>
#include <lwmsg/session.h>
#include "context-private.h"
#include "util-private.h"
#include "dispatch-private.h"

#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

typedef enum ServerTaskType
{
    SERVER_TASK_LISTEN,
    SERVER_TASK_ACCEPT,
    SERVER_TASK_BEGIN_ESTABLISH,
    SERVER_TASK_FINISH_ESTABLISH,
    SERVER_TASK_BEGIN_RECV,
    SERVER_TASK_FINISH_RECV,
    SERVER_TASK_BEGIN_SEND,
    SERVER_TASK_FINISH_SEND,
    SERVER_TASK_BEGIN_CLOSE,
    SERVER_TASK_FINISH_CLOSE,
    SERVER_TASK_BEGIN_RESET,
    SERVER_TASK_FINISH_RESET,
    SERVER_TASK_DISPATCH,
    SERVER_TASK_BEGIN_ASYNC,
    SERVER_TASK_FINISH_ASYNC
} ServerTaskType;

typedef struct ServerTask
{
    LWMsgRing ring;
    ServerTaskType volatile type;
    LWMsgBool blocked;
    int fd;
    LWMsgAssoc* assoc;
    LWMsgDispatchHandle dispatch;
    LWMsgMessage incoming_message;
    LWMsgMessage outgoing_message;
    LWMsgTime deadline;
} ServerTask;

typedef struct ServerIoThread
{
    LWMsgServer* server;
    pthread_t thread;
    LWMsgRing volatile tasks;
    size_t volatile num_events;
    pthread_mutex_t lock;
    int event[2];
    LWMsgBool shutdown;
} ServerIoThread;

typedef struct ServerDispatchThread
{
    LWMsgServer* server;
    pthread_t thread;
} ServerDispatchThread;

typedef enum ServerState
{
    SERVER_STATE_STOPPED = 0,
    SERVER_STATE_STARTING,
    SERVER_STATE_STARTED,
    SERVER_STATE_STOPPING,
    SERVER_STATE_ERROR
} ServerState;

struct LWMsgServer
{    
    LWMsgContext context;
    LWMsgProtocol* protocol;
    LWMsgSessionManager* manager;
    size_t max_clients;
    size_t max_dispatch;
    size_t max_io;
    size_t max_backlog;
    struct
    {
        LWMsgTime message;
        LWMsgTime establish;
        LWMsgTime idle;
    } timeout;
    void* dispatch_data;
    LWMsgSessionConstructor session_construct;
    LWMsgSessionDestructor session_destruct;
    void* session_construct_data;

    /* IO Block */
    struct
    {
        ServerIoThread* threads;
        unsigned int volatile next_index;
        pthread_mutex_t lock;
    } io;

    /* Dispatch block */
    struct
    {
        LWMsgDispatchSpec** vector;
        size_t vector_length;
        ServerDispatchThread* threads;
        LWMsgRing tasks;
        pthread_mutex_t lock;
        pthread_cond_t event;
        LWMsgBool shutdown;
    } dispatch;

    /* Initial set of IO tasks */
    LWMsgRing io_tasks;

    /* Total number of connected clients */
    size_t num_clients;

    pthread_mutex_t lock;
    pthread_cond_t event;
    ServerState state;
    LWMsgStatus error;
};

#define SERVER_RAISE_ERROR(_server_, _status_, ...) \
    RAISE_ERROR(&(_server_)->context, _status_, __VA_ARGS__)

#define SERVER_RAISE_ERROR_LOCK(_server_, _lock_, _status_, ...) \
    do                                                           \
    {                                                            \
        SERVER_LOCK(_server_, _lock_);                           \
        SERVER_RAISE_ERROR(_server_, _status_, __VA_ARGS__);     \
    } while (0)

#define SERVER_LOCK(_server_, _lock_)           \
    do                                          \
    {                                           \
        if (!(_lock_))                          \
        {                                       \
            (_lock_) = 1;                       \
            lwmsg_server_lock((_server_));      \
        }                                       \
    } while (0)

#define SERVER_UNLOCK(_server_, _lock_)         \
    do                                          \
    {                                           \
        if ((_lock_))                           \
        {                                       \
            (_lock_) = 0;                       \
            lwmsg_server_unlock((_server_));    \
        }                                       \
    } while (0)

LWMsgStatus
lwmsg_server_task_new(
    ServerTaskType type,
    ServerTask** task
    );

LWMsgStatus
lwmsg_server_task_new_listen(
    int fd,
    ServerTask** task
    );

void
lwmsg_server_task_delete(
    ServerTask* task
    );

LWMsgStatus
lwmsg_server_task_perform(
    LWMsgServer* server,
    ServerIoThread* thread,
    ServerTask** task,
    LWMsgBool shutdown,
    LWMsgTime* current_time,
    LWMsgTime* next_deadline
    );

LWMsgStatus
lwmsg_server_task_prepare_select(
    LWMsgServer* server,
    ServerTask* task,
    int* nfds,
    fd_set* readset,
    fd_set* writeset
    );

void
lwmsg_server_queue_io_task(
    LWMsgServer* server,
    ServerTask* task
    );

void
lwmsg_server_queue_dispatch_task(
    LWMsgServer* server,
    ServerTask* task
    );

void*
lwmsg_server_dispatch_thread(
    void* data
    );

void*
lwmsg_server_io_thread(
    void* data
    );

LWMsgBool
lwmsg_server_acquire_client_slot(
    LWMsgServer* server
    );

void
lwmsg_server_release_client_slot(
    LWMsgServer* server
    );

size_t
lwmsg_server_get_num_clients(
    LWMsgServer* server
    );

#endif
