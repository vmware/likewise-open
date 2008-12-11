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

#include <pthread.h>

typedef enum LWMsgServerState
{
    LWMSG_SERVER_STOPPED = 0,
    LWMSG_SERVER_RUNNING,
    LWMSG_SERVER_SHUTDOWN
} LWMsgServerState;

typedef struct AssocQueue
{
    size_t size;
    volatile size_t count;
    LWMsgAssoc** volatile queue;
    pthread_cond_t event;
} AssocQueue;

struct LWMsgServer
{    
    LWMsgContext context;
    LWMsgProtocol* protocol;
    LWMsgSessionManager* manager;
    LWMsgDispatchFunction* dispatch_vector;
    size_t dispatch_vector_length;
    LWMsgServerMode mode;
    int fd;
    char* endpoint;
    mode_t permissions;
    size_t max_clients;
    size_t max_dispatch;
    size_t max_backlog;
    LWMsgTime timeout;
    void* user_data;
    LWMsgServerConnectFunction connect_callback;

    /* Worker thread pool */
    pthread_t* worker_threads;
    /* Listener thread */
    pthread_t listen_thread;

    /* Associations that we are listening for messages from */
    AssocQueue listen_assocs;
    /* Associations that need to be serviced */
    AssocQueue service_assocs;

    /* Synchronization lock for accessing this structure */
    pthread_mutex_t lock;
    /* State change event */
    pthread_cond_t state_changed;
    /* Interrupt for blocking operations in worker threads */
    LWMsgConnectionSignal* interrupt;
    /* Self-pipe to wake up listener thread when an association goes back in its queue */
    int listen_notify[2];

    /* Server state */
    LWMsgServerState volatile state;

    unsigned timeout_set:1;
};

#define SERVER_RAISE_ERROR(_server_, _status_, ...) RAISE_ERROR(&(_server_)->context, _status_, __VA_ARGS__)

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

#endif
