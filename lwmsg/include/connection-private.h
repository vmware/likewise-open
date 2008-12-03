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
    /* Establishing basic connection */
    CONNECTION_STATE_ESTABLISH,
    /* Basic connection established */
    CONNECTION_STATE_CONNECTED,
    /* Send greeting */
    CONNECTION_STATE_SEND_GREETING,
    /* Recv greeting */
    CONNECTION_STATE_RECV_GREETING,
    /* Ready state (connected, handshake complete, no operation in progress) */
    CONNECTION_STATE_IDLE,
    /* Sending a message in progress */
    CONNECTION_STATE_SEND_MESSAGE,
    /* Receiving a reply in progress */
    CONNECTION_STATE_RECV_REPLY,
    /* Sending a reply in progress */
    CONNECTION_STATE_SEND_REPLY,
    /* Receiving a reply in progress */
    CONNECTION_STATE_RECV_MESSAGE,
    /* A message was received, waiting for client to give us a reply to send */
    CONNECTION_STATE_WAIT_SEND_REPLY,
    /* A message was sent, waiting for client to ask for reply */
    CONNECTION_STATE_WAIT_RECV_REPLY,
    /* We closed the connection */
    CONNECTION_STATE_LOCAL_CLOSED,
    /* We aborted the connection */
    CONNECTION_STATE_LOCAL_ABORTED,
    /* The peer closed the connection */
    CONNECTION_STATE_PEER_CLOSED,
    /* The peer reset the connection */
    CONNECTION_STATE_PEER_RESET,
    /* The peer aborted the connection */
    CONNECTION_STATE_PEER_ABORTED
} ConnectionState;

typedef enum ConnectionEvent
{
    /* No event */
    CONNECTION_EVENT_NONE,
    /* Send a message (start or send fragment) */
    CONNECTION_EVENT_SEND,
    /* Receive a message (start or receive fragment) */
    CONNECTION_EVENT_RECV,
    /* Operation complete (done sending or receiving message) */
    CONNECTION_EVENT_DONE,
    /* Close connection */
    CONNECTION_EVENT_CLOSE,
    /* Reset connection */
    CONNECTION_EVENT_RESET,
    /* Abort connection */
    CONNECTION_EVENT_ABORT,
} ConnectionEvent;

typedef struct ConnectionBuffer
{
    size_t base_capacity;
    size_t base_length;
    unsigned char* base;
    unsigned char* cursor;
    size_t fd_capacity;
    size_t fd_length;
    int* fd;
} ConnectionBuffer;

typedef enum ConnectionPacketType
{   
    CONNECTION_PACKET_MESSAGE = 1,
    CONNECTION_PACKET_REPLY = 2,
    CONNECTION_PACKET_FRAGMENT = 3,
    CONNECTION_PACKET_GREETING = 4,
    CONNECTION_PACKET_SHUTDOWN = 5
} ConnectionPacketType;

typedef struct ConnectionPrivate
{
    LWMsgConnectionMode mode;
    int fd;
    char* endpoint;
    ConnectionBuffer sendbuffer;
    ConnectionBuffer recvbuffer;
    ConnectionState state;
    LWMsgMessage* message;
    int timeout_set;
    LWMsgTime end_time;
    LWMsgTime last_time;
    size_t packet_size;
    LWMsgSecurityToken* sec_token;
    LWMsgSession* session;
    LWMsgConnectionSignal* interrupt;
    unsigned ready:1;
} ConnectionPrivate;

typedef enum ConnectionGreetingFlags
{
    CONNECTION_GREETING_AUTH_LOCAL = 1
} ConnectionGreetingFlags;

typedef enum ConnectionShutdownType
{
    CONNECTION_SHUTDOWN_CLOSE = 0,
    CONNECTION_SHUTDOWN_RESET = 1,
    CONNECTION_SHUTDOWN_ABORT = 2
} ConnectionShutdownType;

typedef enum ConnectionShutdownReason
{
    CONNECTION_SHUTDOWN_NORMAL = 0,
    CONNECTION_SHUTDOWN_TIMEOUT = 1,
    CONNECTION_SHUTDOWN_ACCESS_DENIED = 2,
    CONNECTION_SHUTDOWN_MALFORMED = 3
} ConnectionShutdownReason;

typedef struct ConnectionPacket
{
    uint32_t length;
    uint8_t type;
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
            uint8_t type;
            uint8_t reason;
        } PACKED shutdown;
    } PACKED contents;
} PACKED ConnectionPacket;

struct LWMsgConnectionSignal
{
    int fd[2];
};

typedef struct LocalTokenPrivate
{
    uid_t euid;
    gid_t egid;
} LocalTokenPrivate;


#define CONNECTION_PACKET_SIZE(_type_) (offsetof(struct ConnectionPacket, contents) + sizeof(struct _type_))
#define MAX_FD_PAYLOAD 256

LWMsgStatus
lwmsg_connection_construct_buffer(
    ConnectionBuffer* buffer
    );

void
lwmsg_connection_destruct_buffer(
    ConnectionBuffer* buffer
    );

LWMsgStatus
lwmsg_connection_buffer_resize(
    ConnectionBuffer* buffer,
    size_t size
    );

LWMsgStatus
lwmsg_connection_buffer_ensure_fd_capacity(
    ConnectionBuffer* buffer,
    size_t needed
    );

void
lwmsg_connection_buffer_reset(
    ConnectionBuffer* buffer
    );

LWMsgStatus
lwmsg_connection_run(
    LWMsgAssoc* assoc,
    ConnectionState target,
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
lwmsg_connection_transceive(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_recv_packet(
    LWMsgAssoc* assoc,
    ConnectionPacket** out_packet
    );

LWMsgStatus
lwmsg_connection_send_packet(
    LWMsgAssoc* assoc,
    ConnectionPacket* packet,
    ConnectionPacket** urgent
    );

LWMsgStatus
lwmsg_connection_discard_recv_packet(
    LWMsgAssoc* assoc,
    ConnectionPacket* packet
    );

LWMsgStatus
lwmsg_connection_discard_send_packet(
    LWMsgAssoc* assoc,
    ConnectionPacket* packet
    );

LWMsgBool
lwmsg_connection_packet_is_urgent(
    ConnectionPacket* packet
    );

LWMsgStatus
lwmsg_connection_queue_packet(
    LWMsgAssoc* assoc,
    ConnectionPacketType type,
    size_t size,
    ConnectionPacket** out_packet
    );

LWMsgStatus
lwmsg_connection_send_greeting(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_recv_greeting(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_connection_send_shutdown(
    LWMsgAssoc* assoc,
    ConnectionShutdownType type,
    ConnectionShutdownReason reason
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

#endif
