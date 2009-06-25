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
 *        test-client-server.c
 *
 * Abstract:
 *
 *        Multi-threaded client/server unit tests
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#define LWMSG_SPEC_META

#include <lwmsg/lwmsg.h>
#include <moonunit/moonunit.h>
#include <config.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "util-private.h"
#include "test-private.h"

typedef struct CounterHandle
{
    pthread_mutex_t lock;
    int counter;
} CounterHandle;

typedef struct CounterRequest
{
    int counter;
} CounterRequest;

typedef struct CounterReply
{
    int counter;
} CounterReply;

typedef struct CounterAdd
{
    CounterHandle* handle;
    int delta;
} CounterAdd;

LWMsgTypeSpec counterhandle_spec[] =
{
    LWMSG_HANDLE(CounterHandle),
    LWMSG_TYPE_END
};

LWMsgTypeSpec counterrequest_spec[] =
{
    LWMSG_STRUCT_BEGIN(CounterRequest),
    LWMSG_MEMBER_INT16(CounterRequest, counter),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec counteradd_spec[] =
{
    LWMSG_STRUCT_BEGIN(CounterAdd),
    LWMSG_MEMBER_HANDLE(CounterAdd, handle, CounterHandle),
    LWMSG_MEMBER_INT16(CounterAdd, delta),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec counterreply_spec[] =
{
    LWMSG_STRUCT_BEGIN(CounterReply),
    LWMSG_MEMBER_INT16(CounterReply, counter),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


typedef enum CounterType
{
    COUNTER_OPEN,
    COUNTER_OPEN_SUCCESS,
    COUNTER_CLOSE,
    COUNTER_CLOSE_SUCCESS,
    COUNTER_ADD,
    COUNTER_ADD_SUCCESS,
    COUNTER_READ,
    COUNTER_READ_SUCCESS
} CounterType;

LWMsgProtocolSpec counterprotocol_spec[] =
{
    LWMSG_MESSAGE(COUNTER_OPEN, counterrequest_spec),
    LWMSG_MESSAGE(COUNTER_OPEN_SUCCESS, counterhandle_spec),
    LWMSG_MESSAGE(COUNTER_ADD, counteradd_spec),
    LWMSG_MESSAGE(COUNTER_ADD_SUCCESS, counterreply_spec),
    LWMSG_MESSAGE(COUNTER_READ, counterhandle_spec),
    LWMSG_MESSAGE(COUNTER_READ_SUCCESS, counterreply_spec),
    LWMSG_MESSAGE(COUNTER_CLOSE, counterhandle_spec),
    LWMSG_MESSAGE(COUNTER_CLOSE_SUCCESS, counterreply_spec),
    LWMSG_PROTOCOL_END
};

static LWMsgStatus
counter_srv_open(LWMsgCall* call, const LWMsgMessage* request_msg, LWMsgMessage* reply_msg, void* data)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    CounterHandle* handle = malloc(sizeof(*handle));
    CounterRequest* request = request_msg->data;
    LWMsgSession* session = lwmsg_server_call_get_session(call);

    pthread_mutex_init(&handle->lock, NULL);

    handle->counter = request->counter;

    MU_TRY(lwmsg_session_register_handle(session, "CounterHandle", handle, free));

    reply_msg->tag = COUNTER_OPEN_SUCCESS;
    reply_msg->data = handle;

    MU_TRY(lwmsg_session_retain_handle(session, handle));

    return status;
}
static LWMsgStatus
counter_srv_add(LWMsgCall* call, const LWMsgMessage* request_msg, LWMsgMessage* reply_msg, void* data)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    CounterAdd* add = request_msg->data;
    CounterHandle* handle = add->handle;
    CounterReply* reply = malloc(sizeof(*reply));

    pthread_mutex_lock(&handle->lock);
    reply->counter = handle->counter;
    handle->counter += add->delta;
    pthread_mutex_unlock(&handle->lock);

    reply_msg->tag = COUNTER_ADD_SUCCESS;
    reply_msg->data = reply;

    return status;
}

static LWMsgStatus
counter_srv_read(LWMsgCall* call, const LWMsgMessage* request_msg, LWMsgMessage* reply_msg, void* data)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    CounterHandle* handle = request_msg->data;
    CounterReply* reply = malloc(sizeof(*reply));

    pthread_mutex_lock(&handle->lock);
    reply->counter = handle->counter;
    pthread_mutex_unlock(&handle->lock);

    reply_msg->tag = COUNTER_READ_SUCCESS;
    reply_msg->data = reply;

    return status;
}

static LWMsgStatus
counter_srv_close(LWMsgCall* call, const LWMsgMessage* request_msg, LWMsgMessage* reply_msg, void* data)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    CounterHandle* handle = request_msg->data;
    CounterReply* reply = malloc(sizeof(*reply));
    LWMsgSession* session = lwmsg_server_call_get_session(call);

    pthread_mutex_lock(&handle->lock);
    reply->counter = handle->counter;
    pthread_mutex_unlock(&handle->lock);
    
    pthread_mutex_destroy(&handle->lock);
    lwmsg_session_unregister_handle(session, handle);

    reply_msg->tag = COUNTER_CLOSE_SUCCESS;
    reply_msg->data = reply;

    return status;
}

LWMsgDispatchSpec counter_dispatch[] =
{
    LWMSG_DISPATCH_BLOCK(COUNTER_OPEN, counter_srv_open),
    LWMSG_DISPATCH_BLOCK(COUNTER_ADD, counter_srv_add),
    LWMSG_DISPATCH_BLOCK(COUNTER_READ, counter_srv_read),
    LWMSG_DISPATCH_BLOCK(COUNTER_CLOSE, counter_srv_close),
    LWMSG_DISPATCH_END
};

typedef struct
{
    LWMsgClient* client;
    CounterHandle* handle;
    int iters;
    pthread_mutex_t lock;
    pthread_cond_t event;
    int volatile go;
} Data;

static void*
add_thread(void* _data)
{
    Data *data = _data;
    int i;
    LWMsgAssoc* assoc = NULL;
    CounterAdd add;
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage reply_msg = LWMSG_MESSAGE_INITIALIZER;

    pthread_mutex_lock(&data->lock);
    while (!data->go)
    {
        pthread_cond_wait(&data->event, &data->lock);
    }
    pthread_mutex_unlock(&data->lock);

    add.handle = data->handle;
    add.delta = 1;

    MU_TRY(lwmsg_client_acquire_assoc(data->client, &assoc));

    for (i = 0; i < data->iters; i++)
    {
        request_msg.tag = COUNTER_ADD;
        request_msg.data = &add;

        MU_TRY_ASSOC(assoc, lwmsg_assoc_send_message_transact(assoc, &request_msg, &reply_msg));
        MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, COUNTER_ADD_SUCCESS);

        MU_VERBOSE("(0x%lx) counter: %i -> %i",
                   (unsigned long) (pthread_self()), 
                   ((CounterReply*) reply_msg.data)->counter,
                   ((CounterReply*) reply_msg.data)->counter+1);
        lwmsg_assoc_destroy_message(assoc, &reply_msg);
    }

    MU_TRY(lwmsg_client_release_assoc(data->client, assoc));

    return NULL;
}

#define MAX_CLIENTS 16
#define MAX_DISPATCH 4
#define NUM_THREADS 16
#define NUM_ITERS 10

MU_TEST(client_server, parallel)
{
    Data data;
    pthread_t threads[NUM_THREADS];
    int i;
    LWMsgContext* context = NULL;
    LWMsgProtocol* protocol = NULL;
    LWMsgClient* client = NULL;
    LWMsgServer* server = NULL;
    CounterRequest request;
    CounterReply* reply;
    LWMsgMessage request_msg;
    LWMsgMessage reply_msg;
    LWMsgTime timeout = {1, 0};

    MU_TRY(lwmsg_context_new(NULL, &context));
    lwmsg_context_set_log_function(context, lwmsg_test_log_function, NULL);

    MU_TRY(lwmsg_protocol_new(context, &protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(protocol, counterprotocol_spec));

    MU_TRY(lwmsg_server_new(context, protocol, &server));
    MU_TRY(lwmsg_server_add_dispatch_spec(server, counter_dispatch));
    MU_TRY(lwmsg_server_set_endpoint(server, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT, 0600));
    MU_TRY(lwmsg_server_set_max_clients(server, MAX_CLIENTS));
    MU_TRY(lwmsg_server_set_max_dispatch(server, MAX_DISPATCH));
    MU_TRY(lwmsg_server_set_timeout(server, LWMSG_TIMEOUT_IDLE, &timeout));
    MU_TRY(lwmsg_server_start(server));

    MU_TRY(lwmsg_client_new(context, protocol, &client));
    MU_TRY(lwmsg_client_set_max_concurrent(client, NUM_THREADS));
    MU_TRY(lwmsg_client_set_endpoint(client, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT));

    request.counter = 0;
    request_msg.tag = COUNTER_OPEN;
    request_msg.data = &request;

    MU_TRY(lwmsg_client_send_message_transact(
               client,
               &request_msg,
               &reply_msg));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, COUNTER_OPEN_SUCCESS);

    data.client = client;
    data.handle = reply_msg.data;
    data.iters = NUM_ITERS;
    data.go = 0;
    
    pthread_mutex_init(&data.lock, NULL);
    pthread_cond_init(&data.event, NULL);

    pthread_mutex_lock(&data.lock);
    for (i = 0; i < NUM_THREADS; i++)
    {
        pthread_create(&threads[i], NULL, add_thread, &data);
    }
    data.go = 1;
    pthread_cond_broadcast(&data.event);
    pthread_mutex_unlock(&data.lock);

    for (i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    request_msg.tag = COUNTER_READ;
    request_msg.data = data.handle;

    MU_TRY(lwmsg_client_send_message_transact(
               client,
               &request_msg,
               &reply_msg));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, COUNTER_READ_SUCCESS);

    reply = reply_msg.data;

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply->counter, NUM_THREADS * NUM_ITERS);

    free(reply);

    request_msg.tag = COUNTER_CLOSE;
    request_msg.data = data.handle;

    MU_TRY(lwmsg_client_send_message_transact(
               client,
               &request_msg,
               &reply_msg));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, COUNTER_CLOSE_SUCCESS);

    free(reply_msg.data);

    MU_TRY(lwmsg_client_shutdown(client));
    lwmsg_client_delete(client);

    MU_TRY(lwmsg_server_stop(server));
    lwmsg_server_delete(server);

    pthread_mutex_destroy(&data.lock);
    pthread_cond_destroy(&data.event);
}

typedef enum AsyncTag
{
    ASYNC_REQUEST_IMMEDIATE,
    ASYNC_REQUEST_DELAY,
    ASYNC_RESPONSE
} AsyncTag;


LWMsgProtocolSpec async_protocol_spec[] =
{
    LWMSG_MESSAGE(ASYNC_REQUEST_IMMEDIATE, NULL),
    LWMSG_MESSAGE(ASYNC_REQUEST_DELAY, NULL),
    LWMSG_MESSAGE(ASYNC_RESPONSE, NULL),
    LWMSG_PROTOCOL_END
};

typedef struct
{
    LWMsgCall* call;
    LWMsgMessage* response;
    LWMsgBool interrupt;
    pthread_mutex_t lock;
    pthread_cond_t event;
} AsyncRequest;

static
void*
async_response_thread(
    void* data
    )
{
    AsyncRequest* request = data;
    struct timespec ts;

    ts.tv_sec = time(NULL) + 2;

    pthread_mutex_lock(&request->lock);

    while (!request->interrupt)
    {
        if (pthread_cond_timedwait(&request->event, &request->lock, &ts) == ETIMEDOUT)
        {
            request->response->tag = ASYNC_RESPONSE;
            lwmsg_call_complete(request->call, LWMSG_STATUS_SUCCESS);
            goto done;
        }
    }

    MU_INFO("Request interrupted");

    lwmsg_call_complete(request->call, LWMSG_STATUS_CANCELLED);

done:

    pthread_mutex_destroy(&request->lock);
    pthread_cond_destroy(&request->event);
    free(request);

    return NULL;
}

static
LWMsgStatus
async_interrupt(
    LWMsgCall* call,
    void* data
    )
{
    AsyncRequest* request = data;

    pthread_mutex_lock(&request->lock);
    request->interrupt = LWMSG_TRUE;
    pthread_cond_signal(&request->event);
    pthread_mutex_unlock(&request->lock);

    return LWMSG_STATUS_SUCCESS;
}

static
LWMsgStatus
async_request(
    LWMsgCall* call,
    LWMsgMessage* request,
    LWMsgMessage* response
    )
{
    AsyncRequest* req = NULL;
    pthread_t thread = (pthread_t) -1;

    switch (request->tag)
    {
    case ASYNC_REQUEST_DELAY:
        MU_TRY(LWMSG_ALLOC(&req));

        pthread_mutex_init(&req->lock, NULL);
        pthread_cond_init(&req->event, NULL);

        req->call = call;
        req->response = response;

        lwmsg_call_pend(call, async_interrupt, req);

        pthread_create(&thread, NULL, async_response_thread, req);
        pthread_detach(thread);
        return LWMSG_STATUS_PENDING;
    case ASYNC_REQUEST_IMMEDIATE:
        response->tag = ASYNC_RESPONSE;
        return LWMSG_STATUS_SUCCESS;
    default:
        return LWMSG_STATUS_INTERNAL;
    }
}

LWMsgDispatchSpec async_dispatch_spec[] =
{
    LWMSG_DISPATCH_NONBLOCK(ASYNC_REQUEST_IMMEDIATE, async_request),
    LWMSG_DISPATCH_NONBLOCK(ASYNC_REQUEST_DELAY, async_request),
    LWMSG_DISPATCH_END
};

MU_TEST(client_server, async_immediate)
{
    LWMsgProtocol* protocol = NULL;
    LWMsgClient* client = NULL;
    LWMsgServer* server = NULL;
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage reply_msg = LWMSG_MESSAGE_INITIALIZER;

    MU_TRY(lwmsg_protocol_new(NULL, &protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(protocol, async_protocol_spec));

    MU_TRY(lwmsg_server_new(NULL, protocol, &server));
    MU_TRY(lwmsg_server_add_dispatch_spec(server, async_dispatch_spec));
    MU_TRY(lwmsg_server_set_endpoint(server, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT, 0600));
    MU_TRY(lwmsg_server_start(server));

    MU_TRY(lwmsg_client_new(NULL, protocol, &client));
    MU_TRY(lwmsg_client_set_endpoint(client, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT));

    request_msg.tag = ASYNC_REQUEST_IMMEDIATE;

    MU_TRY(lwmsg_client_send_message_transact(
               client,
               &request_msg,
               &reply_msg));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, ASYNC_RESPONSE);

    MU_TRY(lwmsg_client_shutdown(client));
    lwmsg_client_delete(client);

    MU_TRY(lwmsg_server_stop(server));
    lwmsg_server_delete(server);
}

MU_TEST(client_server, async_delay)
{
    LWMsgProtocol* protocol = NULL;
    LWMsgClient* client = NULL;
    LWMsgServer* server = NULL;
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage reply_msg = LWMSG_MESSAGE_INITIALIZER;

    MU_TRY(lwmsg_protocol_new(NULL, &protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(protocol, async_protocol_spec));

    MU_TRY(lwmsg_server_new(NULL, protocol, &server));
    MU_TRY(lwmsg_server_add_dispatch_spec(server, async_dispatch_spec));
    MU_TRY(lwmsg_server_set_endpoint(server, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT, 0600));
    MU_TRY(lwmsg_server_start(server));

    MU_TRY(lwmsg_client_new(NULL, protocol, &client));
    MU_TRY(lwmsg_client_set_endpoint(client, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT));

    request_msg.tag = ASYNC_REQUEST_DELAY;

    MU_TRY(lwmsg_client_send_message_transact(
               client,
               &request_msg,
               &reply_msg));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, ASYNC_RESPONSE);

    MU_TRY(lwmsg_client_shutdown(client));
    lwmsg_client_delete(client);

    MU_TRY(lwmsg_server_stop(server));
    lwmsg_server_delete(server);
}

MU_TEST(client_server, async_interrupt)
{
    LWMsgProtocol* protocol = NULL;
    LWMsgClient* client = NULL;
    LWMsgServer* server = NULL;
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgAssoc* assoc = NULL;

    MU_TRY(lwmsg_protocol_new(NULL, &protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(protocol, async_protocol_spec));

    MU_TRY(lwmsg_server_new(NULL, protocol, &server));
    MU_TRY(lwmsg_server_add_dispatch_spec(server, async_dispatch_spec));
    MU_TRY(lwmsg_server_set_endpoint(server, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT, 0600));
    MU_TRY(lwmsg_server_start(server));

    MU_TRY(lwmsg_client_new(NULL, protocol, &client));
    MU_TRY(lwmsg_client_set_endpoint(client, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT));

    request_msg.tag = ASYNC_REQUEST_DELAY;

    MU_TRY(lwmsg_client_acquire_assoc(client, &assoc));

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_message(assoc, &request_msg));

    MU_TRY_ASSOC(assoc, lwmsg_assoc_close(assoc));

    MU_TRY(lwmsg_client_release_assoc(client, assoc));

    MU_TRY(lwmsg_client_shutdown(client));
    lwmsg_client_delete(client);

    MU_TRY(lwmsg_server_stop(server));
    lwmsg_server_delete(server);
}

MU_TEST(client_server, async_shutdown)
{
    LWMsgProtocol* protocol = NULL;
    LWMsgClient* client = NULL;
    LWMsgServer* server = NULL;
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgAssoc* assoc = NULL;

    MU_TRY(lwmsg_protocol_new(NULL, &protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(protocol, async_protocol_spec));

    MU_TRY(lwmsg_server_new(NULL, protocol, &server));
    MU_TRY(lwmsg_server_add_dispatch_spec(server, async_dispatch_spec));
    MU_TRY(lwmsg_server_set_endpoint(server, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT, 0600));
    MU_TRY(lwmsg_server_start(server));

    MU_TRY(lwmsg_client_new(NULL, protocol, &client));
    MU_TRY(lwmsg_client_set_endpoint(client, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT));

    request_msg.tag = ASYNC_REQUEST_DELAY;

    MU_TRY(lwmsg_client_acquire_assoc(client, &assoc));

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_message(assoc, &request_msg));

    MU_TRY(lwmsg_server_stop(server));
    lwmsg_server_delete(server);

    MU_TRY(lwmsg_client_release_assoc(client, assoc));

    MU_TRY(lwmsg_client_shutdown(client));
    lwmsg_client_delete(client);
}
