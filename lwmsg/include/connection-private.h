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
 *        connection-private.h
 *
 * Abstract:
 *
 *        Connection API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_CONNECTION_PRIVATE_H__
#define __LWMSG_CONNECTION_PRIVATE_H__

#include <config.h>
#include <stddef.h>
#include <inttypes.h>
#include <lwmsg/connection.h>
#include <lwmsg/data.h>

#include "util-private.h"

#if (defined(HAVE_GETPEEREID) && HAVE_DECL_GETPEEREID) || HAVE_DECL_SO_PEERCRED
#define HAVE_PEERID_METHOD
#endif

#define PACKED __attribute__((packed))

typedef enum ConnectionState
{
    /* No state */
    CONNECTION_STATE_NONE,
    /* Start state */
    CONNECTION_STATE_START,
    /* Begin connect */
    CONNECTION_STATE_BEGIN_CONNECT,
    /* Finish connect() call */
    CONNECTION_STATE_FINISH_CONNECT,
    /* Begin sending handshake */
    CONNECTION_STATE_BEGIN_SEND_HANDSHAKE,
    /* Finish sending handshake */
    CONNECTION_STATE_FINISH_SEND_HANDSHAKE,
    /* Begin receiving handshake */
    CONNECTION_STATE_BEGIN_RECV_HANDSHAKE,
    /* Finish receiving handshake */
    CONNECTION_STATE_FINISH_RECV_HANDSHAKE,
    /* Idle */
    CONNECTION_STATE_IDLE,
    /* Begin sending a message */
    CONNECTION_STATE_BEGIN_SEND_MESSAGE,
    /* Begin receiving a message */
    CONNECTION_STATE_BEGIN_RECV_MESSAGE,
    /* Begin a close */
    CONNECTION_STATE_BEGIN_CLOSE,
    /* Finish a close */
    CONNECTION_STATE_FINISH_CLOSE,
    /* Begin a reset */
    CONNECTION_STATE_BEGIN_RESET,
    /* Finish a reset */
    CONNECTION_STATE_FINISH_RESET,
    /* Finish sending a message */
    CONNECTION_STATE_FINISH_SEND_MESSAGE,
    /* Finish receiving a message */
    CONNECTION_STATE_FINISH_RECV_MESSAGE,
    /* Closed */
    CONNECTION_STATE_CLOSED,
    /* Error */
    CONNECTION_STATE_ERROR
} ConnectionState;

typedef enum ConnectionEvent
{
    /* No event */
    CONNECTION_EVENT_NONE,
    /* Establish session */
    CONNECTION_EVENT_ESTABLISH,
    /* Send a message (start or send fragment) */
    CONNECTION_EVENT_SEND,
    /* Receive a message (start or receive fragment) */
    CONNECTION_EVENT_RECV,
    /* Close connection */
    CONNECTION_EVENT_CLOSE,
    /* Reset connection */
    CONNECTION_EVENT_RESET,
    /* Abort connection */
    CONNECTION_EVENT_ABORT,
    /* Finish operation */
    CONNECTION_EVENT_FINISH
} ConnectionEvent;

typedef struct ConnectionFragment
{
    LWMsgRing ring;
    unsigned char* cursor;
    unsigned char data[];
} ConnectionFragment;

typedef struct ConnectionBuffer
{
    LWMsgRing pending;
    LWMsgRing unused;
    size_t num_pending;
    size_t num_unused;
    size_t fd_capacity;
    size_t fd_length;
    int* fd;
} ConnectionBuffer;

typedef enum ConnectionPacketType
{   
    CONNECTION_PACKET_MESSAGE = 1,
    CONNECTION_PACKET_GREETING = 4,
    CONNECTION_PACKET_SHUTDOWN = 5
} ConnectionPacketType;

typedef struct ConnectionPrivate
{
    /* Connection mode */
    LWMsgConnectionMode mode;
    /* Socket file descriptor */
    int fd;
    /* Endpoint string */
    char* endpoint;
    /* Buffer for outgoing packets */
    ConnectionBuffer sendbuffer;
    /* Buffer for incoming packets */
    ConnectionBuffer recvbuffer;
    /* Current state of connection state machine */
    ConnectionState state;
    /* Parameters passed into state machine */
    union
    {
        LWMsgMessage* message;
        struct
        {
            LWMsgSessionConstructor construct;
            LWMsgSessionDestructor destruct;
            void* construct_data;
        } establish;
    } params;

    /* Timeouts (relative) */
    struct
    {
        /* Timeout for establishing session */
        LWMsgTime establish;
        /* Timeout for transceiving messages */
        LWMsgTime message;
        /* Currently relevant timeout value */
        LWMsgTime current;
    } timeout;
    /* Deadline for current activity (absolute) */
    LWMsgTime end_time;
    /* Time of last activity (absolute) */
    LWMsgTime last_time;
    /* Negotiated packet size */
    size_t packet_size;
    /* Peer security token */
    LWMsgSecurityToken* sec_token;
    /* Session handle */
    LWMsgSession* session;
    /* Flag: this connection is nonblocking */
    unsigned is_nonblock:1;
    /* Marshal handle */
    LWMsgDataContext* marshal_context;
} ConnectionPrivate;

typedef enum ConnectionGreetingFlags
{
    CONNECTION_GREETING_AUTH_LOCAL = 1
} ConnectionGreetingFlags;

typedef enum ConnectionPacketFlags
{
    CONNECTION_PACKET_FLAG_FIRST_FRAGMENT = 0x1,
    CONNECTION_PACKET_FLAG_LAST_FRAGMENT = 0x2
} ConnectionPacketFlags;

#define CONNECTION_PACKET_FLAG_SINGLE (CONNECTION_PACKET_FLAG_FIRST_FRAGMENT | CONNECTION_PACKET_FLAG_LAST_FRAGMENT)

typedef struct ConnectionPacket
{
    uint32_t length;
    uint8_t type;
    uint8_t flags;
    union
    {
        struct ConnectionPacketBase
        {
        } PACKED base;
        struct ConnectionPacketMsg
        {
            uint16_t type;
        } PACKED msg;
        struct ConnectionPacketGreeting
        {
            uint8_t flags;
            uint32_t packet_size;
            uint8_t smid[8];
        } PACKED greeting;
        struct ConnectionPacketShutdown
        {
            uint32_t status;
        } PACKED shutdown;
    } PACKED contents;
} PACKED ConnectionPacket;

typedef struct LocalTokenPrivate
{
    uid_t euid;
    gid_t egid;
} LocalTokenPrivate;


#define CONNECTION_PACKET_SIZE(_type_) (offsetof(struct ConnectionPacket, contents) + sizeof(struct _type_))
#define MAX_FD_PAYLOAD 256

LWMsgStatus
lwmsg_connection_buffer_construct(
    ConnectionBuffer* buffer
    );

void
lwmsg_connection_buffer_destruct(
    ConnectionBuffer* buffer
    );

LWMsgStatus
lwmsg_connection_buffer_ensure_fd_capacity(
    ConnectionBuffer* buffer,
    size_t needed
    );

LWMsgStatus
lwmsg_connection_buffer_create_fragment(
    ConnectionBuffer* buffer,
    size_t length,
    ConnectionFragment** fragment
    );

void
lwmsg_connection_buffer_queue_fragment(
    ConnectionBuffer* buffer,
    ConnectionFragment* fragment
    );

ConnectionFragment*
lwmsg_connection_buffer_dequeue_fragment(
    ConnectionBuffer* buffer
    );

ConnectionFragment*
lwmsg_connection_buffer_get_first_fragment(
    ConnectionBuffer* buffer
    );

ConnectionFragment*
lwmsg_connection_buffer_get_last_fragment(
    ConnectionBuffer* buffer
    );

void
lwmsg_connection_buffer_free_fragment(
    ConnectionBuffer* buffer,
    ConnectionFragment* fragment
    );

LWMsgStatus
lwmsg_connection_run(
    LWMsgAssoc* assoc,
    ConnectionEvent event
    );

LWMsgStatus
lwmsg_connection_queue_fd(
    LWMsgAssoc* assoc,
    int fd
    );

LWMsgStatus
lwmsg_connection_dequeue_fd(
    LWMsgAssoc* assoc,
    int* out_fd
    );

LWMsgStatus
lwmsg_connection_begin_timeout(
    LWMsgAssoc* assoc,
    LWMsgTime* value
    );

LWMsgStatus
lwmsg_connection_flush(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_check(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_local_token_new(
    uid_t euid,
    gid_t egid,
    LWMsgSecurityToken** out_token
    );

LWMsgStatus
lwmsg_local_token_from_socket_peer(
    int fd,
    LWMsgSecurityToken** out_token
    );

LWMsgStatus
lwmsg_connection_get_endpoint_owner(
    LWMsgAssoc* assoc,
    const char* endpoint,
    uid_t *uid,
    gid_t *gid
    );

LWMsgStatus
lwmsg_connection_begin_connect(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_finish_connect(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_connect_existing(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_begin_send_handshake(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_finish_send_handshake(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_begin_recv_handshake(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_finish_recv_handshake(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_begin_send_message(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_finish_send_message(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_begin_recv_message(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_finish_recv_message(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_begin_send_shutdown(
    LWMsgAssoc* assoc,
    LWMsgStatus reason
    );

LWMsgStatus
lwmsg_connection_finish_send_shutdown(
    LWMsgAssoc* assoc
    );

#endif
