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
 *        test-assoc.c
 *
 * Abstract:
 *
 *        Association unit tests
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

typedef enum EmptyTag
{
    EMPTY_REQUEST,
    EMPTY_REPLY
} EmptyTag;

static LWMsgProtocolSpec EmptyProtocol_spec[] =
{
    LWMSG_MESSAGE(EMPTY_REQUEST, NULL),
    LWMSG_MESSAGE(EMPTY_REPLY, NULL),
    LWMSG_PROTOCOL_END
};

static void*
empty_sender(void* _assoc)
{
    LWMsgAssoc* assoc = (LWMsgAssoc*) _assoc;
    LWMsgMessageTag reply_type;
    void* reply_object;
    
    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_transact(assoc, EMPTY_REQUEST, NULL, &reply_type, &reply_object));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_type, EMPTY_REPLY);
    MU_ASSERT_EQUAL(MU_TYPE_POINTER, reply_object, NULL);

    MU_TRY_ASSOC(assoc, lwmsg_assoc_close(assoc));
    lwmsg_assoc_delete(assoc);

    return NULL;
}

static void*
empty_receiver(void* _assoc)
{
    LWMsgAssoc* assoc = (LWMsgAssoc*) _assoc;
    LWMsgMessageTag request_type;
    void* request_object;
    
    MU_TRY_ASSOC(assoc, lwmsg_assoc_recv(assoc, &request_type, &request_object));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, request_type, EMPTY_REQUEST);
    MU_ASSERT_EQUAL(MU_TYPE_POINTER, request_object, NULL);
    
    MU_TRY_ASSOC(assoc, lwmsg_assoc_send(assoc, EMPTY_REPLY, NULL));

    MU_TRY_ASSOC(assoc, lwmsg_assoc_close(assoc));
    lwmsg_assoc_delete(assoc);

    return NULL;
}

MU_TEST(assoc, empty_send_recv)
{
    int err = 0;
    int sockets[2];
    LWMsgAssoc* send_assoc = NULL;
    LWMsgAssoc* recv_assoc = NULL;
    pthread_t sender;
    pthread_t receiver;
    LWMsgProtocol* empty_protocol = NULL;

    MU_TRY(lwmsg_protocol_new(NULL, &empty_protocol));
    MU_TRY_PROTOCOL(empty_protocol, lwmsg_protocol_add_protocol_spec(empty_protocol, EmptyProtocol_spec));

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        MU_FAILURE("socketpair(): %s", strerror(errno));
    }

    MU_TRY(lwmsg_connection_new(
               empty_protocol,
               &send_assoc));
    
    MU_TRY(lwmsg_connection_set_fd(
               send_assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[0]));

    MU_TRY(lwmsg_connection_new(
               empty_protocol,
               &recv_assoc));
    
    MU_TRY(lwmsg_connection_set_fd(
               recv_assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[1]));

    if ((err = pthread_create(&sender, NULL, empty_sender, send_assoc)))
    {
        MU_FAILURE("pthread_create(): %s", strerror(err));
    }

    if ((err = pthread_create(&receiver, NULL, empty_receiver, recv_assoc)))
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

typedef struct FooRequest
{
    int size;
} FooRequest;

typedef struct FooReply
{
    int size;
    int numbers[];
} FooReply;

typedef enum FooType
{
    FOO_REQUEST = 1,
    FOO_REPLY = 2
} FooType;

static LWMsgTypeSpec FooRequest_spec[] =
{
    LWMSG_STRUCT_BEGIN(FooRequest),
    LWMSG_MEMBER_INT16(FooRequest, size),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec FooReply_spec[] =
{
    LWMSG_STRUCT_BEGIN(FooReply),
    LWMSG_MEMBER_INT16(FooReply, size),
    LWMSG_MEMBER_ARRAY_BEGIN(FooReply, numbers),
    LWMSG_INT16(int),
    LWMSG_ARRAY_END,
    LWMSG_ATTR_LENGTH_MEMBER(FooReply, size),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgProtocolSpec FooProtocol_spec[] =
{
    LWMSG_MESSAGE(FOO_REQUEST, FooRequest_spec),
    LWMSG_MESSAGE(FOO_REPLY, FooReply_spec),
    LWMSG_PROTOCOL_END
};

static void*
foo_sender(void* _assoc)
{
    LWMsgAssoc* assoc = (LWMsgAssoc*) _assoc;
    LWMsgMessageTag reply_type;
    void* reply_object;
    FooRequest request;
    FooReply* reply;
    size_t i;
    
    request.size = 1024;

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_transact(assoc, FOO_REQUEST, &request, &reply_type, &reply_object));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_type, FOO_REPLY);

    reply = reply_object;

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply->size, request.size);

    for (i = 0; i < reply->size; i++)
    {
        MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply->numbers[i], i);
    }

    free(reply);

    MU_TRY_ASSOC(assoc, lwmsg_assoc_close(assoc));
    lwmsg_assoc_delete(assoc);

    return NULL;
}

static void*
foo_receiver(void* _assoc)
{
    LWMsgAssoc* assoc = (LWMsgAssoc*) _assoc;
    LWMsgMessageTag request_type;
    void* request_object;
    FooRequest* request;
    FooReply* reply;
    size_t i;
    
    MU_TRY_ASSOC(assoc, lwmsg_assoc_recv(assoc, &request_type, &request_object));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, request_type, FOO_REQUEST);
    request = request_object;

    reply = malloc(sizeof(*reply) + request->size * sizeof(*reply->numbers));
    reply->size = request->size;

    for (i = 0; i < reply->size; i++)
    {
        reply->numbers[i] = i;
    }

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send(assoc, FOO_REPLY, reply));

    free(request);
    free(reply);

    MU_TRY_ASSOC(assoc, lwmsg_assoc_close(assoc));
    lwmsg_assoc_delete(assoc);

    return NULL;
}

MU_TEST(assoc, foo_send_recv)
{
    int err = 0;
    int sockets[2];
    LWMsgAssoc* send_assoc = NULL;
    LWMsgAssoc* recv_assoc = NULL;
    pthread_t sender;
    pthread_t receiver;
    LWMsgProtocol* foo_protocol = NULL;

    MU_TRY(lwmsg_protocol_new(NULL, &foo_protocol));
    MU_TRY_PROTOCOL(foo_protocol, lwmsg_protocol_add_protocol_spec(foo_protocol, FooProtocol_spec));

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        MU_FAILURE("socketpair(): %s", strerror(errno));
    }

    MU_TRY(lwmsg_connection_new(
               foo_protocol,
               &send_assoc));
    
    MU_TRY(lwmsg_connection_set_fd(
               send_assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[0]));

    MU_TRY(lwmsg_connection_new(
               foo_protocol,
               &recv_assoc));
    
    MU_TRY(lwmsg_connection_set_fd(
               recv_assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[1]));

    if ((err = pthread_create(&sender, NULL, foo_sender, send_assoc)))
    {
        MU_FAILURE("pthread_create(): %s", strerror(err));
    }

    if ((err = pthread_create(&receiver, NULL, foo_receiver, recv_assoc)))
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

/* Tests that fragmentation of messages works by transmitting
   a message far larger than the packet size of the connection */
MU_TEST(assoc, foo_send_recv_fragment)
{
    int err = 0;
    int sockets[2];
    LWMsgAssoc* send_assoc = NULL;
    LWMsgAssoc* recv_assoc = NULL;
    pthread_t sender;
    pthread_t receiver;
    LWMsgProtocol* foo_protocol = NULL;

    MU_TRY(lwmsg_protocol_new(NULL, &foo_protocol));
    MU_TRY_PROTOCOL(foo_protocol, lwmsg_protocol_add_protocol_spec(foo_protocol, FooProtocol_spec));

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        MU_FAILURE("socketpair(): %s", strerror(errno));
    }

    MU_TRY(lwmsg_connection_new(
               foo_protocol,
               &send_assoc));
    
    MU_TRY(lwmsg_connection_set_packet_size(
               send_assoc,
               512));

    MU_TRY(lwmsg_connection_set_fd(
               send_assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[0]));

    MU_TRY(lwmsg_connection_new(
               foo_protocol,
               &recv_assoc));

    MU_TRY(lwmsg_connection_set_packet_size(
               recv_assoc,
               512));
    
    MU_TRY(lwmsg_connection_set_fd(
               recv_assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[1]));

    if ((err = pthread_create(&sender, NULL, foo_sender, send_assoc)))
    {
        MU_FAILURE("pthread_create(): %s", strerror(err));
    }

    if ((err = pthread_create(&receiver, NULL, foo_receiver, recv_assoc)))
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


MU_TEST(assoc, foo_send_timeout)
{
    int sockets[2];
    LWMsgAssoc* send_assoc = NULL;
    static LWMsgProtocol* foo_protocol = NULL;
    FooRequest request;
    LWMsgMessage request_msg;
    LWMsgMessage reply_msg;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    
    MU_TRY(lwmsg_protocol_new(NULL, &foo_protocol));
    MU_TRY_PROTOCOL(foo_protocol, lwmsg_protocol_add_protocol_spec(foo_protocol, FooProtocol_spec));

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        MU_FAILURE("socketpair(): %s", strerror(errno));
    }

    MU_TRY(lwmsg_connection_new(
               foo_protocol,
               &send_assoc));
    
    MU_TRY(lwmsg_connection_set_fd(
               send_assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[0]));
    
    request.size = 4;
    request_msg.tag = FOO_REQUEST;
    request_msg.object = &request;

    lwmsg_assoc_set_timeout_ms(send_assoc, 500);

    status = lwmsg_assoc_send_message_transact(send_assoc, &request_msg, &reply_msg);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, status, LWMSG_STATUS_TIMEOUT);

    MU_TRY_ASSOC(send_assoc, lwmsg_assoc_close(send_assoc));
    lwmsg_assoc_delete(send_assoc);
}

typedef struct AuthRequest
{
    int fortytwo;
} AuthRequest;

typedef struct AuthReply
{
    uid_t uid;
    gid_t gid;
} AuthReply;

typedef enum AuthType
{
    AUTH_REQUEST,
    AUTH_REPLY
} AuthType;

static LWMsgTypeSpec AuthRequest_spec[] =
{
    LWMSG_STRUCT_BEGIN(AuthRequest),
    LWMSG_MEMBER_UINT8(AuthRequest, fortytwo),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec AuthReply_spec[] =
{
    LWMSG_STRUCT_BEGIN(AuthReply),
      LWMSG_MEMBER_UINT32(AuthReply, uid),
      LWMSG_MEMBER_UINT32(AuthReply, gid),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgProtocolSpec AuthProtocol_spec[] =
{
    LWMSG_MESSAGE(AUTH_REQUEST, AuthRequest_spec),
    LWMSG_MESSAGE(AUTH_REPLY, AuthReply_spec),
    LWMSG_PROTOCOL_END
};

static void*
auth_sender(void* _assoc)
{
    LWMsgAssoc* assoc = (LWMsgAssoc*) _assoc;
    AuthRequest request;
    AuthReply* reply;
    LWMsgMessage request_msg;
    LWMsgMessage reply_msg;
    
    request.fortytwo = 42;
    request_msg.tag = AUTH_REQUEST;
    request_msg.object = &request;

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_message_transact(assoc, &request_msg, &reply_msg));

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, AUTH_REPLY);

    reply = reply_msg.object;

    MU_INFO("uid = %u", (unsigned int) reply->uid);
    MU_INFO("gid = %u", (unsigned int) reply->gid);

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply->uid, geteuid());
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply->gid, getegid());

    free(reply);

    MU_TRY_ASSOC(assoc, lwmsg_assoc_close(assoc));
    lwmsg_assoc_delete(assoc);

    return NULL;
}

static void*
auth_receiver(void* _assoc)
{
    LWMsgAssoc* assoc = (LWMsgAssoc*) _assoc;
    AuthRequest* request;
    AuthReply reply;
    LWMsgMessage request_msg;
    LWMsgMessage reply_msg;
    LWMsgSecurityToken* token;
    
    MU_TRY_ASSOC(assoc, lwmsg_assoc_recv_message(assoc, &request_msg));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, request_msg.tag, AUTH_REQUEST);
    request = request_msg.object;

    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, request->fortytwo, 42);

    MU_TRY_ASSOC(assoc, lwmsg_assoc_get_peer_security_token(assoc, &token));
    MU_ASSERT(token != NULL);
    MU_ASSERT_EQUAL(MU_TYPE_STRING, lwmsg_security_token_get_type(token), "local");
    MU_TRY(lwmsg_local_token_get_eid(token, &reply.uid, &reply.gid));

    reply_msg.tag = AUTH_REPLY;
    reply_msg.object = &reply;

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_message(assoc, &reply_msg));

    free(request);

    MU_TRY_ASSOC(assoc, lwmsg_assoc_close(assoc));
    lwmsg_assoc_delete(assoc);

    return NULL;
}

MU_TEST(assoc, auth_send_recv)
{
    int err = 0;
    int sockets[2];
    LWMsgAssoc* send_assoc = NULL;
    LWMsgAssoc* recv_assoc = NULL;
    pthread_t sender;
    pthread_t receiver;
    LWMsgProtocol* auth_protocol = NULL;

    MU_TRY(lwmsg_protocol_new(NULL, &auth_protocol));
    MU_TRY_PROTOCOL(auth_protocol, lwmsg_protocol_add_protocol_spec(auth_protocol, AuthProtocol_spec));

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        MU_FAILURE("socketpair(): %s", strerror(errno));
    }

    MU_TRY(lwmsg_connection_new(
               auth_protocol,
               &send_assoc));
    
    MU_TRY(lwmsg_connection_set_fd(
               send_assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[0]));

    MU_TRY(lwmsg_connection_new(
               auth_protocol,
               &recv_assoc));
    
    MU_TRY(lwmsg_connection_set_fd(
               recv_assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[1]));

    if ((err = pthread_create(&sender, NULL, auth_sender, send_assoc)))
    {
        MU_FAILURE("pthread_create(): %s", strerror(err));
    }

    if ((err = pthread_create(&receiver, NULL, auth_receiver, recv_assoc)))
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

typedef struct Handle
{
    char* saved_string;
    int saved_integer;
} Handle;

static LWMsgTypeSpec Handle_local_spec[] =
{
    LWMSG_HANDLE(Handle),
    LWMSG_ATTR_HANDLE_LOCAL,
    LWMSG_TYPE_END,
};

static LWMsgTypeSpec Handle_remote_spec[] =
{
    LWMSG_HANDLE(Handle),
    LWMSG_ATTR_HANDLE_REMOTE,
    LWMSG_TYPE_END,
};

typedef struct HandleCreateRequest
{
    char* save_string;
    int save_integer;
} HandleCreateRequest;

static LWMsgTypeSpec HandleCreateRequest_spec[] =
{
    LWMSG_STRUCT_BEGIN(HandleCreateRequest),
      LWMSG_MEMBER_PSTR(HandleCreateRequest, save_string),
      LWMSG_MEMBER_INT32(HandleCreateRequest, save_integer),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

typedef enum HandleGetType
{
    HANDLE_GET_STRING = 0,
    HANDLE_GET_INTEGER = 1
} HandleGetType;

typedef struct HandleGetRequest
{
    Handle* handle;
    HandleGetType type;
} HandleGetRequest;

static LWMsgTypeSpec HandleGetRequest_spec[] =
{
    LWMSG_STRUCT_BEGIN(HandleGetRequest),
    LWMSG_MEMBER_TYPESPEC(HandleGetRequest, handle, Handle_local_spec),
    LWMSG_MEMBER_UINT8(HandleGetRequest, type),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

typedef struct HandleGetReply
{
    HandleGetType type;
    union ReplyUnion
    {
        char* string;
        int integer;
    } value;
} HandleGetReply;

static LWMsgTypeSpec HandleGetReply_spec[] =
{
    LWMSG_STRUCT_BEGIN(HandleGetReply),
    LWMSG_MEMBER_UINT8(HandleGetReply, type),
    LWMSG_MEMBER_UNION_BEGIN(HandleGetReply, value),
    LWMSG_MEMBER_PSTR(union ReplyUnion, string),
    LWMSG_ATTR_TAG(HANDLE_GET_STRING),
    LWMSG_MEMBER_INT32(union ReplyUnion, integer),
    LWMSG_ATTR_TAG(HANDLE_GET_INTEGER),
    LWMSG_UNION_END,
    LWMSG_ATTR_DISCRIM(HandleGetReply, type),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

typedef struct HandleDestroyReply
{
    int status;
} HandleDestroyReply;

static LWMsgTypeSpec HandleDestroyReply_spec[] =
{
    LWMSG_STRUCT_BEGIN(HandleDestroyReply),
    LWMSG_MEMBER_UINT8(HandleDestroyReply, status),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

typedef enum HandleMessageType
{
    HANDLE_CREATE_REQUEST,
    HANDLE_CREATE_REPLY,
    HANDLE_DESTROY_REQUEST,
    HANDLE_DESTROY_REPLY,
    HANDLE_GET_REQUEST,
    HANDLE_GET_REPLY
} HandleMessageType;

static LWMsgProtocolSpec HandleProtocol_spec[] =
{
    LWMSG_MESSAGE(HANDLE_CREATE_REQUEST, HandleCreateRequest_spec),
    LWMSG_MESSAGE(HANDLE_CREATE_REPLY, Handle_remote_spec),
    LWMSG_MESSAGE(HANDLE_GET_REQUEST, HandleGetRequest_spec),
    LWMSG_MESSAGE(HANDLE_GET_REPLY, HandleGetReply_spec),
    LWMSG_MESSAGE(HANDLE_DESTROY_REQUEST, Handle_local_spec),
    LWMSG_MESSAGE(HANDLE_DESTROY_REPLY, HandleDestroyReply_spec),
    LWMSG_PROTOCOL_END
};

static void*
handle_sender(void* _assoc)
{
    LWMsgAssoc* assoc = (LWMsgAssoc*) _assoc;
    Handle* handle;
    HandleCreateRequest create_request;
    HandleGetRequest get_request;
    HandleGetReply* get_reply;
    HandleDestroyReply* destroy_reply;
    LWMsgMessage request_msg;
    LWMsgMessage reply_msg;
    
    create_request.save_string = (char*) "Hello, world!";
    create_request.save_integer = 42;
    request_msg.tag = HANDLE_CREATE_REQUEST;
    request_msg.object = &create_request;

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_message_transact(assoc, &request_msg, &reply_msg));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, HANDLE_CREATE_REPLY);

    handle = reply_msg.object;

    get_request.type = HANDLE_GET_STRING;
    get_request.handle = handle;
    request_msg.tag = HANDLE_GET_REQUEST;
    request_msg.object = &get_request;

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_message_transact(assoc, &request_msg, &reply_msg));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, HANDLE_GET_REPLY);
    
    get_reply = reply_msg.object;
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, get_reply->type, HANDLE_GET_STRING);
    MU_ASSERT_EQUAL(MU_TYPE_STRING, get_reply->value.string, "Hello, world!");
    free(get_reply->value.string);
    free(get_reply);

    get_request.type = HANDLE_GET_INTEGER;
    get_request.handle = handle;
    request_msg.tag = HANDLE_GET_REQUEST;
    request_msg.object = &get_request;

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_message_transact(assoc, &request_msg, &reply_msg));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, HANDLE_GET_REPLY);
    
    get_reply = reply_msg.object;
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, get_reply->type, HANDLE_GET_INTEGER);
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, get_reply->value.integer, 42);
    free(get_reply);

    request_msg.tag = HANDLE_DESTROY_REQUEST;
    request_msg.object = handle;
    
    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_message_transact(assoc, &request_msg, &reply_msg));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_msg.tag, HANDLE_DESTROY_REPLY);
    destroy_reply = reply_msg.object;
    
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, destroy_reply->status, 0);
    free(destroy_reply);
    
    MU_TRY_ASSOC(assoc, lwmsg_assoc_unregister_handle(assoc, handle, LWMSG_FALSE));

    MU_TRY_ASSOC(assoc, lwmsg_assoc_close(assoc));
    lwmsg_assoc_delete(assoc);

    return NULL;
}

static LWMsgStatus
handle_create_srv(
    LWMsgAssoc* assoc,
    const LWMsgMessage* in,
    LWMsgMessage* out,
    void* data
    )
{
    HandleCreateRequest* request = in->object;
    Handle* handle = NULL;

    handle = malloc(sizeof(*handle));
    handle->saved_string = strdup(request->save_string);
    handle->saved_integer = request->save_integer;

    MU_TRY_ASSOC(assoc, lwmsg_assoc_register_handle(assoc, "Handle", handle, free));

    out->tag = HANDLE_CREATE_REPLY;
    out->object = handle;
    
    return LWMSG_STATUS_SUCCESS;
}

static LWMsgStatus
handle_destroy_srv(
    LWMsgAssoc* assoc,
    const LWMsgMessage* in,
    LWMsgMessage* out,
    void* data
    )
{
    Handle* handle = in->object;
    HandleDestroyReply* reply = malloc(sizeof(*reply));

    reply->status = 0;

    out->tag = HANDLE_DESTROY_REPLY;
    out->object = reply;

    MU_TRY_ASSOC(assoc, lwmsg_assoc_unregister_handle(assoc, handle, LWMSG_FALSE));

    return LWMSG_STATUS_EOF;
}

static LWMsgStatus
handle_get_srv(
    LWMsgAssoc* assoc,
    const LWMsgMessage* in,
    LWMsgMessage* out,
    void* data
    )
{
    HandleGetRequest* request = in->object;
    HandleGetReply* reply = malloc(sizeof(*reply));

    reply->type = request->type;

    switch (request->type)
    {
    case HANDLE_GET_STRING:
        reply->value.string = request->handle->saved_string;
        break;
    case HANDLE_GET_INTEGER:
        reply->value.integer = request->handle->saved_integer;
        break;
    }
    
    out->tag = HANDLE_GET_REPLY;
    out->object = reply;

    return LWMSG_STATUS_SUCCESS;
}

static LWMsgDispatchSpec handle_dispatch[] =
{
    LWMSG_DISPATCH(HANDLE_CREATE_REQUEST, handle_create_srv),
    LWMSG_DISPATCH(HANDLE_DESTROY_REQUEST, handle_destroy_srv),
    LWMSG_DISPATCH(HANDLE_GET_REQUEST, handle_get_srv),
    LWMSG_DISPATCH_END
};

static void*
handle_receiver(void* _assoc)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = _assoc;
    LWMsgMessage recv_message;
    LWMsgMessage send_message;
    size_t i;

    do
    {
        MU_TRY_ASSOC(assoc, lwmsg_assoc_recv_message(assoc, &recv_message));
        for (i = 0; handle_dispatch[i].func; i++)
        {
            if (recv_message.tag == handle_dispatch[i].tag)
            {
                break;
            }
        }

        status = handle_dispatch[i].func(assoc, &recv_message, &send_message, NULL);

        MU_TRY_ASSOC(assoc, lwmsg_assoc_send_message(assoc, &send_message));

        if (status == LWMSG_STATUS_EOF)
        {
            break;
        }
    } while (1);

    MU_TRY_ASSOC(assoc, lwmsg_assoc_close(assoc));
    lwmsg_assoc_delete(assoc);

    return NULL;
}

MU_TEST(assoc, handle_send_recv)
{
    int err = 0;
    int sockets[2];
    LWMsgAssoc* send_assoc = NULL;
    LWMsgAssoc* recv_assoc = NULL;
    pthread_t sender;
    pthread_t receiver;
    LWMsgProtocol* handle_protocol = NULL;

    MU_TRY(lwmsg_protocol_new(NULL, &handle_protocol));
    MU_TRY_PROTOCOL(handle_protocol, lwmsg_protocol_add_protocol_spec(handle_protocol, HandleProtocol_spec));

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        MU_FAILURE("socketpair(): %s", strerror(errno));
    }

    MU_TRY(lwmsg_connection_new(
               handle_protocol,
               &send_assoc));
    
    MU_TRY(lwmsg_connection_set_fd(
               send_assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[0]));

    MU_TRY(lwmsg_connection_new(
               handle_protocol,
               &recv_assoc));
    
    MU_TRY(lwmsg_connection_set_fd(
               recv_assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[1]));

    if ((err = pthread_create(&sender, NULL, handle_sender, send_assoc)))
    {
        MU_FAILURE("pthread_create(): %s", strerror(err));
    }

    if ((err = pthread_create(&receiver, NULL, handle_receiver, recv_assoc)))
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


typedef struct FdRequest
{
    int readfd;
} FdRequest;

typedef struct FdReply
{
    char* message;
} FdReply;

typedef enum FdType
{
    FD_REQUEST = 1,
    FD_REPLY = 2
} FdType;

static LWMsgTypeSpec FdRequest_spec[] =
{
    LWMSG_STRUCT_BEGIN(FdRequest),
    LWMSG_MEMBER_FD(FdRequest, readfd),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec FdReply_spec[] =
{
    LWMSG_STRUCT_BEGIN(FdReply),
    LWMSG_MEMBER_PSTR(FdReply, message),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgProtocolSpec FdProtocol_spec[] =
{
    LWMSG_MESSAGE(FD_REQUEST, FdRequest_spec),
    LWMSG_MESSAGE(FD_REPLY, FdReply_spec),
    LWMSG_PROTOCOL_END
};

static void*
fd_sender(void* _assoc)
{
    const char message[] = "hello";
    LWMsgAssoc* assoc = (LWMsgAssoc*) _assoc;
    LWMsgMessageTag reply_type;
    void* reply_object;
    FdRequest request;
    FdReply* reply;
    int fds[2];

    if (pipe(fds))
    {
        MU_FAILURE("pipe(): %s", strerror(errno));
    }
    
    request.readfd = fds[0];

    if (write(fds[1], message, sizeof(message)) != sizeof(message))
    {
        MU_FAILURE("write(): %s", strerror(errno));
    }

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send_transact(assoc, FD_REQUEST, &request, &reply_type, &reply_object));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, reply_type, FD_REPLY);

    reply = reply_object;

    MU_ASSERT_EQUAL(MU_TYPE_STRING, reply->message, message);

    lwmsg_assoc_free(assoc, reply_type, reply_object);
    MU_TRY_ASSOC(assoc, lwmsg_assoc_close(assoc));
    lwmsg_assoc_delete(assoc);

    close(fds[1]);
    close(fds[0]);

    return NULL;
}

static void*
fd_receiver(void* _assoc)
{
    LWMsgAssoc* assoc = (LWMsgAssoc*) _assoc;
    LWMsgMessageTag request_type;
    void* request_object;
    FdRequest* request;
    FdReply reply;
    char buffer[1024];
    
    MU_TRY_ASSOC(assoc, lwmsg_assoc_recv(assoc, &request_type, &request_object));
    MU_ASSERT_EQUAL(MU_TYPE_INTEGER, request_type, FD_REQUEST);
    request = request_object;

    if (read(request->readfd, buffer, sizeof(buffer)) < 0)
    {
        MU_FAILURE("read(): %s", strerror(errno));
    }

    reply.message = buffer;

    MU_TRY_ASSOC(assoc, lwmsg_assoc_send(assoc, FD_REPLY, &reply));

    close(request->readfd);
    lwmsg_assoc_free(assoc, request_type, request_object);

    MU_TRY_ASSOC(assoc, lwmsg_assoc_close(assoc));
    lwmsg_assoc_delete(assoc);

    return NULL;
}

MU_TEST(assoc, fd_send_recv)
{
    int err = 0;
    int sockets[2];
    LWMsgAssoc* send_assoc = NULL;
    LWMsgAssoc* recv_assoc = NULL;
    pthread_t sender;
    pthread_t receiver;
    LWMsgProtocol* fd_protocol = NULL;

    MU_TRY(lwmsg_protocol_new(NULL, &fd_protocol));
    MU_TRY_PROTOCOL(fd_protocol, lwmsg_protocol_add_protocol_spec(fd_protocol, FdProtocol_spec));

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        MU_FAILURE("socketpair(): %s", strerror(errno));
    }

    MU_TRY(lwmsg_connection_new(
               fd_protocol,
               &send_assoc));
    
    MU_TRY(lwmsg_connection_set_fd(
               send_assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[0]));

    MU_TRY(lwmsg_connection_new(
               fd_protocol,
               &recv_assoc));
    
    MU_TRY(lwmsg_connection_set_fd(
               recv_assoc,
               LWMSG_CONNECTION_MODE_PAIR,
               sockets[1]));

    if ((err = pthread_create(&sender, NULL, fd_sender, send_assoc)))
    {
        MU_FAILURE("pthread_create(): %s", strerror(err));
    }

    if ((err = pthread_create(&receiver, NULL, fd_receiver, recv_assoc)))
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
