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
 *        server.h
 *
 * Abstract:
 *
 *        Multi-threaded server API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_SERVER_H__
#define __LWMSG_SERVER_H__

#include <lwmsg/status.h>
#include <lwmsg/protocol.h>
#include <lwmsg/time.h>
#include <lwmsg/assoc.h>

/**
 * @file server.h
 * @brief Server API
 */

/**
 * @defgroup server Servers
 * @ingroup public
 * @brief Multi-threaded server implementation
 *
 * The server API provided here automates the process of creating listening
 * servers which wait for incoming connections and can service multiple clients
 * simultaneously.  It uses a thread pool internally to service clients and a separate
 * listener thread to queue new connections for the pool, providing a completely
 * asynchronous external interface.
 *
 * Because this API requires threads, it is not available in <tt>liblwmsg_nothr</tt>.
 * Type definitions, function prototypes, and macros for this API may be disabled
 * by defining <tt>LWMSG_NO_THREADS</tt> before including <tt>&lt;lwmsg/lwmsg.h&gt;</tt>.
 *
 */

/**
 * @ingroup server
 * @brief Dispatch specification
 *
 * This structure defines a table of dispatch functions
 * to handle incoming messages in a server.  It should
 * be constructed as a statically-initialized array
 * using #LWMSG_DISPATCH() and #LWMSG_DISPATCH_END macros.
 */
typedef struct LWMsgDispatchSpec
#ifndef DOXYGEN
{
    LWMsgMessageTag tag;
    LWMsgDispatchFunction func;
}
#endif
const LWMsgDispatchSpec;

/**
 * @ingroup server
 * @brief Define message handle in a dispatch table
 *
 * This macro is used in dispatch table construction to
 * define the handler for a particular message type.
 * @param _type the message type
 * @param _func the callback to handle the specified message type
 * @hideinitializer
 */
#define LWMSG_DISPATCH(_type, _func) {(_type), (_func)}

/**
 * @ingroup server
 * @brief Terminate dispatch table
 *
 * This macro is used in dispatch table construction to
 * mark the end of the table
 * @hideinitializer
 */
#define LWMSG_DISPATCH_END {0, NULL}


/**
 * @ingroup server
 * @brief Opaque server object
 *
 * Opqaue server object which can be used to start a listening message server.
 */
typedef struct LWMsgServer LWMsgServer;

/**
 * @ingroup server
 * @brief Server listening mode
 *
 * Specifies whether a server should listen on a local or remote socket
 */
typedef enum LWMsgServerMode
{
    /** No server mode specified */
    LWMSG_SERVER_MODE_NONE,
    /** Server should be local only (listen on a local UNIX domain socket) */
    LWMSG_SERVER_MODE_LOCAL,
    /** Server should support remote connectiosn (listen on a TCP socket) */
    LWMSG_SERVER_MODE_REMOTE
} LWMsgServerMode;

/**
 * @ingroup server
 * @brief Connection callback
 *
 * A function which is invoked whenever a new connection is
 * established with a client.  Use #lwmsg_server_set_connect_callback()
 * to register one with a server. There is no guarantee as to which server
 * thread the callback will be invoked in. It is guaranteed that no other
 * server thread will attempt to use the association until the callback
 * returns.  Returning a status code other than #LWMSG_STATUS_SUCCESS will
 * cause the connection to be rejected.
 *
 * @warning This function must leave the association in a usable state --
 * #lwmsg_assoc_get_state() should return #LWMSG_ASSOC_STATE_READY_RECV
 * or #LWMSG_ASSOC_STATE_READY_SEND_RECV.
 *
 * @param server the server object
 * @param assoc the association with the client
 * @param data the user data pointer set with #lwmsg_server_set_user_data()
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_etc{a status code indicating the reason for rejection}
 * (e.g. #LWMSG_STATUS_SECURITY)
 */
typedef LWMsgStatus
(*LWMsgServerConnectFunction) (
    LWMsgServer* server,
    LWMsgAssoc* assoc,
    void* data
    );

/**
 * @ingroup server
 * @brief Create a new server object
 *
 * Creates a new server object
 *
 * @param[in] protocol a protocol object which describes the protocol spoken by the server
 * @param[out] server the created server object
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{INVALID, protocol was <tt>NULL</tt>}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_new(
    LWMsgProtocol* protocol,
    LWMsgServer** server
    );

/**
 * @ingroup server
 * @brief Delete a server object
 *
 * Deletes a server object.
 *
 * @warning Attempting to delete a server which has been started
 * but not stopped will block until the server stops
 *
 * @param server the server object to delete
 */
void
lwmsg_server_delete(
    LWMsgServer* server
    );

/**
 * @ingroup server
 * @brief Set operation timeout
 *
 * Sets the timeout that will be used for client associations.
 * See lwmsg_assoc_set_timeout() for more information.
 *
 * @param server the server object
 * @param timeout the timeout to set, or NULL for no timeout
 * @return LWMSG_STATUS_SUCCESS on success, LWMSG_STATUS_INVALID on invalid timeout
 */
LWMsgStatus
lwmsg_server_set_timeout(
    LWMsgServer* server,
    LWMsgTime* timeout
    );

/**
 * @ingroup server
 * @brief Set maximum number of simultaneous active connections
 *
 * Sets the maximum numbers of connections which the server will track
 * simultaneously.  Connections beyond this will wait until a slot becomes
 * available.
 *
 * @param server the server object
 * @param max_clients the maximum number of simultaneous clients to support
 * @return LWMSG_STATUS_SUCCESS on success, LWMSG_STATUS_INVALID if the server
 * has already been started
 */
LWMsgStatus
lwmsg_server_set_max_clients(
    LWMsgServer* server,
    unsigned int max_clients
    );


/**
 * @ingroup server
 * @brief Set maximum number of simultaneous dispatched messages
 *
 * Sets the maximum numbers of simultaneous messages which will be
 * handed off to dispatch functions.  Messages beyond this number
 * will be queued until another dispatch function finishes.
 *
 * @param server the server object
 * @param max_clients the maximum number of simultaneous clients to support
 * @return LWMSG_STATUS_SUCCESS on success, LWMSG_STATUS_INVALID if the server
 * has already been started
 */
LWMsgStatus
lwmsg_server_set_max_dispatch(
    LWMsgServer* server,
    unsigned int max_dispatch
    );

/**
 * @ingroup server
 * @brief Set maximum number of backlogged connections
 *
 * Sets the maximum numbers of pending connections which the server will keep
 * waiting until a client slot becomes available.  Pending connections beyond
 * this value will be rejected outright.
 *
 * @param server the server object
 * @param max_backlog the maximum number of clients to queue
 * @return LWMSG_STATUS_SUCCESS on success, LWMSG_STATUS_INVALID if the server
 * has already been started
 */
LWMsgStatus
lwmsg_server_set_max_backlog(
    LWMsgServer* server,
    unsigned int max_backlog
    );

/**
 * @ingroup server
 * @brief Add a message dispatch specification
 *
 * Adds a set of message dispatch functions to the specified
 * server object.
 *
 * @param server the server object
 * @param spec the dispatch specification
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_add_dispatch_spec(
    LWMsgServer* server,
    LWMsgDispatchSpec* spec
    );

/**
 * @ingroup server
 * @brief Set listening socket
 *
 * Sets the socket on which the server will listen for connections.
 * This function must be passed a valid file descriptor which is socket
 * that matches the specified mode and is already listening (has had
 * listen() called on it).
 *
 * @param server the server object
 * @param mode the connection mode (local or remote)
 * @param fd the socket on which to listen
 * @return LWMSG_STATUS_SUCCESS on success, LWMSG_STATUS_INVALID on a bad file descriptor
 * or if a listening endpoint is already set
 */
LWMsgStatus
lwmsg_server_set_fd(
    LWMsgServer* server,
    LWMsgServerMode mode,
    int fd
    );
    
/**
 * @ingroup server
 * @brief Set listening endpoint
 *
 * Sets the endpoint on which the server will listen for connections.
 * For local (UNIX domain) endpoints, this is the path of the socket file.
 * For remote (TCP) endpoints, this is the address and port to bind to.
 *
 * @param server the server object
 * @param mode the connection mode (local or remote)
 * @param endpoint the endpoint on which to listen
 * @param permissions permissions for the endpoint (only applicable to local mode)
 * @return LWMSG_STATUS_SUCCESS on success, LWMSG_STATUS_INVALID on a bad endpoint
 * or if a listening endpoint is already set
 */
LWMsgStatus
lwmsg_server_set_endpoint(
    LWMsgServer* server,
    LWMsgServerMode mode,
    const char* endpoint,
    mode_t      permissions
    );

/**
 * @ingroup server
 * @brief Set user data pointer
 *
 * Sets the user data pointer which is passed to various callback
 * functions invoked by the server, such as:
 *
 * - Message dispatch functions
 * - Connect callback
 *
 * This function may only be used while the server is stopped.
 *
 * @param server the server object
 * @param data the data pointer
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID, the server is already running}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_set_user_data(
    LWMsgServer* server,
    void* data
    );

/**
 * @ingroup server
 * @brief Get user data pointer
 *
 * Gets the user data pointer which is passed to various callback
 * functions invoked by the server.  If no pointer was explicitly
 * set, the value defaults to NULL.
 *
 *
 * @param server the server object
 * @return the data pointer
 */
void*
lwmsg_server_get_user_data(
    LWMsgServer* server
    );

/**
 * @ingroup server
 * @brief Set connection callback
 *
 * Sets a function which will be invoked whenever a new connection
 * is created.
 *
 * This function may only be used while the server is stopped.
 *
 * @param server the server object
 * @param func the callback function
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID, the server is already running}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_set_connect_callback(
    LWMsgServer* server,
    LWMsgServerConnectFunction func
    );

/**
 * @ingroup server
 * @brief Start accepting connections
 *
 * Starts listening for and servicing connections in a separate thread.
 * This function will not block, so the calling function should arrange
 * to do something afterwards while the server is running, such as handling
 * UNIX signals.
 *
 * @param server the server object
 * @return LWMSG_STATUS_SUCCESS on success, or an appropriate status code on failure
 */
LWMsgStatus
lwmsg_server_start(
    LWMsgServer* server
    );

/**
 * @ingroup server
 * @brief Aggressively stop server
 *
 * Stops the specified server accepting new connections and aggressively
 * terminates any connections in progress or queued for service.  After
 * this function returns successfully, the server may be safely deleted with
 * lwmsg_server_delete().
 *
 * @param server the server object
 * @return LWMSG_STATUS_SUCCESS on success, or an appropriate status code on failure
 */
LWMsgStatus
lwmsg_server_stop(
    LWMsgServer* server
    );

#endif
