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
 *        test-session.c
 *
 * Abstract:
 *
 *        Session management unit tests
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
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

#include "test-private.h"

typedef struct TrivialHandle
{
    int trivial;
} TrivialHandle;

typedef struct TrivialRequest
{
    int trivial;
} TrivialRequest;

typedef struct TrivialReply
{
    int trivial;
} TrivialReply;

LWMsgTypeSpec trivialhandle_spec[] =
{
    LWMSG_HANDLE(TrivialHandle),
    LWMSG_TYPE_END
};

LWMsgTypeSpec trivialrequest_spec[] =
{
    LWMSG_STRUCT_BEGIN(TrivialRequest),
    LWMSG_MEMBER_INT8(TrivialRequest, trivial),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

LWMsgTypeSpec trivialreply_spec[] =
{
    LWMSG_STRUCT_BEGIN(TrivialReply),
    LWMSG_MEMBER_INT8(TrivialReply, trivial),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};


typedef enum TrivialType
{
    TRIVIAL_OPEN,
    TRIVIAL_OPEN_SUCCESS,
    TRIVIAL_CLOSE,
    TRIVIAL_CLOSE_SUCCESS
} TrivialType;

LWMsgProtocolSpec trivialprotocol_spec[] =
{
    LWMSG_MESSAGE(TRIVIAL_OPEN, trivialrequest_spec),
    LWMSG_MESSAGE(TRIVIAL_OPEN_SUCCESS, trivialhandle_spec),
    LWMSG_MESSAGE(TRIVIAL_CLOSE, trivialhandle_spec),
    LWMSG_MESSAGE(TRIVIAL_CLOSE_SUCCESS, trivialreply_spec),
    LWMSG_PROTOCOL_END
};

static void*
trivial_sender(void* _assocs)
{
    LWMsgAssoc** assocs = (LWMsgAssoc**) _assocs;
    LWMsgMessageTag reply_type;
    void* reply_object;
    TrivialRequest request;
    TrivialHandle* handle;
    
    request.trivial = 42;

    MU_TRY_ASSOC(assocs[0], lwmsg_assoc_send_transact(assocs[0], TRIVIAL_OPEN, &request, &reply_type, &reply_object));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_type, TRIVIAL_OPEN_SUCCESS);

    handle = reply_object;

    MU_TRY_ASSOC(assocs[1], lwmsg_assoc_send_transact(assocs[1], TRIVIAL_CLOSE, handle, &reply_type, &reply_object));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_type, TRIVIAL_CLOSE_SUCCESS);

    MU_TRY_ASSOC(assocs[0], lwmsg_assoc_close(assocs[0]));
    MU_TRY_ASSOC(assocs[1], lwmsg_assoc_close(assocs[1]));
    lwmsg_assoc_delete(assocs[0]);
    lwmsg_assoc_delete(assocs[1]);

    return NULL;
}

static void*
trivial_receiver(void* _assocs)
{
    LWMsgAssoc** assocs = (LWMsgAssoc**) _assocs;
    LWMsgMessageTag request_type;
    void* request_object;
    TrivialHandle* handle;
    TrivialReply reply;
    
    MU_TRY_ASSOC(assocs[0], lwmsg_assoc_recv(assocs[0], &request_type, &request_object));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, request_type, TRIVIAL_OPEN);

    handle = malloc(sizeof(*handle));
    handle->trivial = 42;
    
    MU_TRY_ASSOC(assocs[0], lwmsg_assoc_send(assocs[0], TRIVIAL_OPEN_SUCCESS, handle));

    MU_TRY_ASSOC(assocs[1], lwmsg_assoc_recv(assocs[1], &request_type, &request_object));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, request_type, TRIVIAL_CLOSE);

    MU_ASSERT_EQUAL(MU_TYPE_POINTER, handle, request_object);

    free(handle);

    reply.trivial = 42;

    MU_TRY_ASSOC(assocs[1], lwmsg_assoc_send(assocs[1], TRIVIAL_CLOSE_SUCCESS, &reply));
    
    MU_TRY_ASSOC(assocs[0], lwmsg_assoc_close(assocs[0]));
    MU_TRY_ASSOC(assocs[1], lwmsg_assoc_close(assocs[1]));
    lwmsg_assoc_delete(assocs[0]);
    lwmsg_assoc_delete(assocs[1]);

    return NULL;
}

MU_TEST(session, trivial_send_recv)
{
    int err = 0;
    int sockets_a[2];
    int sockets_b[2];
    LWMsgAssoc* send_assocs[2];
    LWMsgAssoc* recv_assocs[2];
    pthread_t sender;
    pthread_t receiver;
    LWMsgProtocol* trivial_protocol = NULL;
    LWMsgSessionManager* manager = NULL;
    char smid_str[17];

    MU_TRY(lwmsg_protocol_new(NULL, &trivial_protocol));
    MU_TRY_PROTOCOL(trivial_protocol, lwmsg_protocol_add_protocol_spec(trivial_protocol, trivialprotocol_spec));

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets_a))
    {
        MU_FAILURE("socketpair(): %s", strerror(errno));
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets_b))
    {
        MU_FAILURE("socketpair(): %s", strerror(errno));
    }

    MU_TRY(lwmsg_connection_new(
               trivial_protocol,
               &send_assocs[0]));

    MU_TRY(lwmsg_connection_new(
               trivial_protocol,
               &send_assocs[1]));
    
    MU_TRY(lwmsg_connection_set_fd(
               send_assocs[0],
               LWMSG_CONNECTION_MODE_PAIR,
               sockets_a[0]));

    MU_TRY(lwmsg_connection_set_fd(
               send_assocs[1],
               LWMSG_CONNECTION_MODE_PAIR,
               sockets_b[0]));

    MU_TRY(lwmsg_connection_new(
               trivial_protocol,
               &recv_assocs[0]));

    MU_TRY(lwmsg_connection_new(
               trivial_protocol,
               &recv_assocs[1]));
    
    MU_TRY(lwmsg_connection_set_fd(
               recv_assocs[0],
               LWMSG_CONNECTION_MODE_PAIR,
               sockets_a[1]));

    MU_TRY(lwmsg_connection_set_fd(
               recv_assocs[1],
               LWMSG_CONNECTION_MODE_PAIR,
               sockets_b[1]));

    /* Put send sockets and recv sockets into same session managers */
    MU_TRY(lwmsg_assoc_get_session_manager(send_assocs[0], &manager));
    MU_TRY(lwmsg_assoc_set_session_manager(send_assocs[1], manager));

    lwmsg_session_id_to_string(lwmsg_session_manager_get_id(manager), smid_str);

    MU_INFO("send smid: %s", smid_str);

    MU_TRY(lwmsg_assoc_get_session_manager(recv_assocs[0], &manager));
    MU_TRY(lwmsg_assoc_set_session_manager(recv_assocs[1], manager));

    lwmsg_session_id_to_string(lwmsg_session_manager_get_id(manager), smid_str);          

    MU_INFO("recv smid: %s", smid_str);

    if ((err = pthread_create(&sender, NULL, trivial_sender, send_assocs)))
    {
        MU_FAILURE("pthread_create(): %s", strerror(err));
    }

    if ((err = pthread_create(&receiver, NULL, trivial_receiver, recv_assocs)))
    {
        MU_FAILURE("pthread_create(): %s", strerror(err));
    }

    if ((err = pthread_join(sender, NULL)))
    {
        MU_FAILURE("pthread_join(): %s", strerror(err));
    }

    if ((err = pthread_join(receiver, NULL)))
    {
        MU_FAILURE("pthread_join(): %s", strerror(err));
    }
}


