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

#include <config.h>
#include <lwmsg/lwmsg.h>
#include <moonunit/moonunit.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "util-private.h"
#include "test-private.h"

/* Dispatch functions can be marked as blocking or
   nonblocking, with the only distinction being that
   functions marked as blocking may block indefinately
   when called.  Both blocking and nonblocking functions
   may complete synchronously (returning LWMSG_STATUS_SUCCESS)
   or asynchronously (returning LWMSG_STATUS_PENDING), so
   all 4 combinations must be tested.  We also test that
   asynchronously completed calls can be cancelled by
   disconnecting the client or by shutting down the server. */

typedef enum AsyncTag
{
    BLOCK_REQUEST_SYNCHROUNOUS,
    BLOCK_REQUEST_ASYNCHRONOUS,
    NONBLOCK_REQUEST_SYNCHROUNOUS,
    NONBLOCK_REQUEST_ASYNCHRONOUS,
    GENERIC_RESPONSE
} AsyncTag;

LWMsgProtocolSpec async_protocol_spec[] =
{
    LWMSG_MESSAGE(BLOCK_REQUEST_SYNCHROUNOUS, NULL),
    LWMSG_MESSAGE(BLOCK_REQUEST_ASYNCHRONOUS, NULL),
    LWMSG_MESSAGE(NONBLOCK_REQUEST_SYNCHROUNOUS, NULL),
    LWMSG_MESSAGE(NONBLOCK_REQUEST_ASYNCHRONOUS, NULL),
    LWMSG_MESSAGE(GENERIC_RESPONSE, NULL),
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
    struct timespec ts = {0, 0};

    ts.tv_sec = time(NULL) + 1;

    pthread_mutex_lock(&request->lock);

    while (!request->interrupt)
    {
        if (pthread_cond_timedwait(&request->event, &request->lock, &ts) == ETIMEDOUT)
        {
            request->response->tag = GENERIC_RESPONSE;
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
void
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
    case BLOCK_REQUEST_ASYNCHRONOUS:
    case NONBLOCK_REQUEST_ASYNCHRONOUS:
        MU_TRY(LWMSG_ALLOC(&req));

        pthread_mutex_init(&req->lock, NULL);
        pthread_cond_init(&req->event, NULL);

        req->call = call;
        req->response = response;

        lwmsg_call_pend(call, async_interrupt, req);

        pthread_create(&thread, NULL, async_response_thread, req);
        pthread_detach(thread);
        return LWMSG_STATUS_PENDING;
    case BLOCK_REQUEST_SYNCHROUNOUS:
    case NONBLOCK_REQUEST_SYNCHROUNOUS:
        response->tag = GENERIC_RESPONSE;
        return LWMSG_STATUS_SUCCESS;
    default:
        return LWMSG_STATUS_INTERNAL;
    }
}

LWMsgDispatchSpec async_dispatch_spec[] =
{
    LWMSG_DISPATCH_NONBLOCK(BLOCK_REQUEST_SYNCHROUNOUS, async_request),
    LWMSG_DISPATCH_NONBLOCK(BLOCK_REQUEST_ASYNCHRONOUS, async_request),
    LWMSG_DISPATCH_NONBLOCK(NONBLOCK_REQUEST_SYNCHROUNOUS, async_request),
    LWMSG_DISPATCH_NONBLOCK(NONBLOCK_REQUEST_ASYNCHRONOUS, async_request),
    LWMSG_DISPATCH_END
};

static LWMsgProtocol* protocol;
static LWMsgClient* client;
static LWMsgServer* server;

MU_FIXTURE_SETUP(async)
{
    MU_TRY(lwmsg_protocol_new(NULL, &protocol));
    MU_TRY(lwmsg_protocol_add_protocol_spec(protocol, async_protocol_spec));

    MU_TRY(lwmsg_server_new(NULL, protocol, &server));
    MU_TRY(lwmsg_server_add_dispatch_spec(server, async_dispatch_spec));
    MU_TRY(lwmsg_server_set_endpoint(server, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT, 0600));
    MU_TRY(lwmsg_server_start(server));

    MU_TRY(lwmsg_client_new(NULL, protocol, &client));
    MU_TRY(lwmsg_client_set_endpoint(client, LWMSG_CONNECTION_MODE_LOCAL, TEST_ENDPOINT));
}

/* Nonblocking function returning result sychronously */
MU_TEST(async, nonblock_synchrounous)
{
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage reply_msg = LWMSG_MESSAGE_INITIALIZER;

    request_msg.tag = NONBLOCK_REQUEST_SYNCHROUNOUS;

    MU_TRY(lwmsg_client_send_message_transact(
               client,
               &request_msg,
               &reply_msg));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, GENERIC_RESPONSE);

    MU_TRY(lwmsg_client_shutdown(client));
    lwmsg_client_delete(client);

    MU_TRY(lwmsg_server_stop(server));
    lwmsg_server_delete(server);
}

MU_TEST(async, nonblock_asynchronous)
{
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage reply_msg = LWMSG_MESSAGE_INITIALIZER;

    request_msg.tag = NONBLOCK_REQUEST_ASYNCHRONOUS;

    MU_TRY(lwmsg_client_send_message_transact(
               client,
               &request_msg,
               &reply_msg));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, GENERIC_RESPONSE);

    MU_TRY(lwmsg_client_shutdown(client));
    lwmsg_client_delete(client);

    MU_TRY(lwmsg_server_stop(server));
    lwmsg_server_delete(server);
}

MU_TEST(async, nonblock_asynchronous_disconnect)
{
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgAssoc* assoc = NULL;

    request_msg.tag = NONBLOCK_REQUEST_ASYNCHRONOUS;

    MU_TRY(lwmsg_client_acquire_assoc(client, &assoc));

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_message(assoc, &request_msg));

    MU_TRY_ASSOC(assoc, lwmsg_assoc_close(assoc));

    MU_TRY(lwmsg_client_release_assoc(client, assoc));

    MU_TRY(lwmsg_client_shutdown(client));
    lwmsg_client_delete(client);

    MU_TRY(lwmsg_server_stop(server));
    lwmsg_server_delete(server);
}

MU_TEST(async, nonblock_asynchronous_shutdown)
{
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgAssoc* assoc = NULL;

    request_msg.tag = NONBLOCK_REQUEST_ASYNCHRONOUS;

    MU_TRY(lwmsg_client_acquire_assoc(client, &assoc));

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_message(assoc, &request_msg));

    MU_TRY(lwmsg_server_stop(server));
    lwmsg_server_delete(server);

    MU_TRY(lwmsg_client_release_assoc(client, assoc));

    MU_TRY(lwmsg_client_shutdown(client));
    lwmsg_client_delete(client);
}

MU_TEST(async, block_synchrounous)
{
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage reply_msg = LWMSG_MESSAGE_INITIALIZER;

    request_msg.tag = BLOCK_REQUEST_SYNCHROUNOUS;

    MU_TRY(lwmsg_client_send_message_transact(
               client,
               &request_msg,
               &reply_msg));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, GENERIC_RESPONSE);

    MU_TRY(lwmsg_client_shutdown(client));
    lwmsg_client_delete(client);

    MU_TRY(lwmsg_server_stop(server));
    lwmsg_server_delete(server);
}

MU_TEST(async, block_asynchronous)
{
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage reply_msg = LWMSG_MESSAGE_INITIALIZER;

    request_msg.tag = BLOCK_REQUEST_ASYNCHRONOUS;

    MU_TRY(lwmsg_client_send_message_transact(
               client,
               &request_msg,
               &reply_msg));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, GENERIC_RESPONSE);

    MU_TRY(lwmsg_client_shutdown(client));
    lwmsg_client_delete(client);

    MU_TRY(lwmsg_server_stop(server));
    lwmsg_server_delete(server);
}

MU_TEST(async, block_asynchronous_disconnect)
{
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgAssoc* assoc = NULL;

    request_msg.tag = BLOCK_REQUEST_ASYNCHRONOUS;

    MU_TRY(lwmsg_client_acquire_assoc(client, &assoc));

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_message(assoc, &request_msg));

    MU_TRY_ASSOC(assoc, lwmsg_assoc_close(assoc));

    MU_TRY(lwmsg_client_release_assoc(client, assoc));

    MU_TRY(lwmsg_client_shutdown(client));
    lwmsg_client_delete(client);

    MU_TRY(lwmsg_server_stop(server));
    lwmsg_server_delete(server);
}

MU_TEST(async, block_asynchronous_shutdown)
{
    LWMsgMessage request_msg = LWMSG_MESSAGE_INITIALIZER;
    LWMsgAssoc* assoc = NULL;

    request_msg.tag = BLOCK_REQUEST_ASYNCHRONOUS;

    MU_TRY(lwmsg_client_acquire_assoc(client, &assoc));

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_message(assoc, &request_msg));

    MU_TRY(lwmsg_server_stop(server));
    lwmsg_server_delete(server);

    MU_TRY(lwmsg_client_release_assoc(client, assoc));

    MU_TRY(lwmsg_client_shutdown(client));
    lwmsg_client_delete(client);
}
