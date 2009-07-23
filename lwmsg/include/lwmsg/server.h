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
#include <lwmsg/message.h>
#include <lwmsg/call.h>

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

/*@{*/

#ifndef DOXYGEN
typedef enum LWMsgDispatchType
{
    LWMSG_DISPATCH_TYPE_END,
    LWMSG_DISPATCH_TYPE_OLD,
    LWMSG_DISPATCH_TYPE_BLOCK,
    LWMSG_DISPATCH_TYPE_NONBLOCK
} LWMsgDispatchType;
#endif

/**
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
    LWMsgDispatchType type;
    LWMsgTag tag;
    void* data;
}
#endif
const LWMsgDispatchSpec;

/**
 * @brief Define message handler in a dispatch table <b>(DEPRECATED)</b>
 *
 * This macro is used in dispatch table construction to
 * define the handler for a particular message type.
 * @param tag the message tag
 * @param func the callback to handle the specified message type
 * @hideinitializer
 * @deprecated use LWMSG_DISPATCH_BLOCK() or LWMSG_DISPATCH_NONBLOCK() instead
 */
#define LWMSG_DISPATCH(tag, func) \
    {LWMSG_DISPATCH_TYPE_OLD, (tag), (void*) (LWMsgAssocDispatchFunction) (func)}

/**
 * @brief Define blocking message handler
 *
 * Defines a message handler function for the given message tag
 * within a dispatch specification.  The provided callback function
 * may block indefinitely in the process of servicing the request.
 * It may also opt to complete the request asynchronously with
 * #lwmsg_call_pend() and #lwmsg_call_complete().
 *
 * @param tag the message tag
 * @param func an #LWMsgServerCallFunction
 * @hideinitializer
 */
#define LWMSG_DISPATCH_BLOCK(tag, func) \
    {LWMSG_DISPATCH_TYPE_BLOCK, (tag), (void*) (LWMsgServerCallFunction) (func)}

/**
 * @brief Define non-blocking message handler
 *
 * Defines a message handler function for the given message tag
 * within a dispatch specification.  The provided callback function
 * must not block indefinitely in the process of servicing the request.
 * If the request cannot be completed immediately, it must complete
 * it asynchronously.
 *
 * @param tag the message tag
 * @param func an #LWMsgServerCallFunction
 * @hideinitializer
 */
#define LWMSG_DISPATCH_NONBLOCK(tag, func) \
    {LWMSG_DISPATCH_TYPE_NONBLOCK, (tag), (void*) (LWMsgServerCallFunction) (func)}

/**
 * @brief Terminate dispatch table
 *
 * This macro is used in dispatch table construction to
 * mark the end of the table
 * @hideinitializer
 */
#define LWMSG_DISPATCH_END {LWMSG_DISPATCH_TYPE_END, -1, NULL}


/**
 * @brief Opaque server object
 *
 * Opqaue server object which can be used to start a listening message server.
 */
typedef struct LWMsgServer LWMsgServer;

/**
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
 * @brief Call handler function
 *
 * A callback function which handles an incoming call request.  The function
 * may complete the call immediately by filling in the response structure
 * and returning #LWMSG_STATUS_SUCCESS, or asynchronously by invoking
 * #lwmsg_call_pend() on the call handle, returning #LWMSG_STATUS_PENDING,
 * and completing the call later with #lwmsg_call_complete().  Returning
 * and other status code will cause the client connection to be unceremoniously
 * terminated, and the status code will be propgated to the handler registered
 * on the server with #lwmsg_server_set_exception_function().
 *
 * Regardless of the status code returned, the data payloads in the
 * request and response structures will be automatically freed by
 * the server as long as the tags are set to valid values.  This means
 * that data in the request must not be referenced after the function returns,
 * Data in the response must be allocated with the same memory allocator
 * as used by the server -- by default, plain malloc().
 *
 * @param call the call handle
 * @param[in] in the input parameters
 * @param[out] out the output parameters
 * @param data the data pointer set by #lwmsg_server_set_dispatch_data()
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{PENDING, the request will be completed asynchronously}
 * @lwmsg_etc{call-specific failure}
 * @lwmsg_endstatus
 * @see #lwmsg_context_set_memory_functions()
 * @see #lwmsg_server_new()
 */
typedef
LWMsgStatus
(*LWMsgServerCallFunction) (
    LWMsgCall* call,
    LWMsgParams* in,
    LWMsgParams* out,
    void* data
    );

/**
 * @brief Exception handler function
 *
 * A callback function which is invoked when an exception occurs within
 * the server.
 *
 * @param server the server handle
 * @param status the status code of the error
 * @param data a user data pointer
 */
typedef
void
(*LWMsgServerExceptionFunction) (
    LWMsgServer* server,
    LWMsgStatus status,
    void* data
    );

/**
 * @brief Create a new server object
 *
 * Creates a new server object
 *
 * @param[in] context an optional context
 * @param[in] protocol a protocol object which describes the protocol spoken by the server
 * @param[out] server the created server object
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{INVALID_PARAMETER, protocol was <tt>NULL</tt>}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_new(
    const LWMsgContext* context,
    LWMsgProtocol* protocol,
    LWMsgServer** server
    );

/**
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
 * @brief Set timeout
 *
 * Sets the specified timeout to the specified value.
 * See #lwmsg_assoc_set_timeout() for more information.
 *
 * @param server the server object
 * @param type the type of timeout to set
 * @param value the value, or NULL for no timeout
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{UNSUPPORTED, the specified timeout type is not supported}
 * @lwmsg_code{INVALID_PARAMETER, the timeout was invalid}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_set_timeout(
    LWMsgServer* server,
    LWMsgTimeout type,
    LWMsgTime* value
    );

/**
 * @brief Set maximum number of simultaneous active connections
 *
 * Sets the maximum numbers of connections which the server will track
 * simultaneously.  Connections beyond this will wait until a slot becomes
 * available.
 *
 * @param server the server object
 * @param max_clients the maximum number of simultaneous clients to support
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the server is already active}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_set_max_clients(
    LWMsgServer* server,
    unsigned int max_clients
    );


/**
 * @brief Set maximum number of simultaneous dispatched messages
 *
 * Sets the maximum numbers of simultaneous messages which will be
 * handed off to dispatch functions.  Messages beyond this number
 * will be queued until another dispatch function finishes.
 *
 * @param server the server object
 * @param max_dispatch the maximum number of simultaneous messages to dispatch
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the server is already active}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_set_max_dispatch(
    LWMsgServer* server,
    unsigned int max_dispatch
    );

/**
 * @brief Set maximum number of simultaneous IO operations
 *
 * Sets the maximum numbers of simultaneous IO operations which will be
 * performed.
 *
 * @param server the server object
 * @param max_io the maximum number of simultaneous IO operations
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the server is already active}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_set_max_io(
    LWMsgServer* server,
    unsigned int max_io
    );

/**
 * @brief Set maximum number of backlogged connections
 *
 * Sets the maximum numbers of pending connections which the server will keep
 * waiting until a client slot becomes available.  Pending connections beyond
 * this value will be rejected outright.
 *
 * @param server the server object
 * @param max_backlog the maximum number of clients to queue
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the server is already active}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_set_max_backlog(
    LWMsgServer* server,
    unsigned int max_backlog
    );

/**
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
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the server is already active}
 * @lwmsg_code{INVALID_PARAMETER, the file descriptor was invalid}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_set_fd(
    LWMsgServer* server,
    LWMsgServerMode mode,
    int fd
    );
    
/**
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
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the server is already active}
 * @lwmsg_code{INVALID_PARAMETER, the endpoint was invalid}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_set_endpoint(
    LWMsgServer* server,
    LWMsgServerMode mode,
    const char* endpoint,
    mode_t      permissions
    );

/**
 * @brief Set session construct and destruct functions
 *
 * Sets functions which will be called when a new session
 * is established with a client.  The constructor function
 * may set up a session context which the destructor function
 * should clean up when the session is terminated.
 *
 * @param server the server handle
 * @param construct a session constructor function
 * @param destruct a session destructor function
 * @param data a user data pointer to be passed to both functions
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the server is already active}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_set_session_functions(
    LWMsgServer* server,
    LWMsgSessionConstructor construct,
    LWMsgSessionDestructor destruct,
    void* data
    );

/**
 * @brief Set dispatch data pointer
 *
 * Sets the user data pointer which is passed to dispatch functions.
 * This function may only be used while the server is stopped.
 *
 * @param server the server object
 * @param data the data pointer
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the server is already running}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_set_dispatch_data(
    LWMsgServer* server,
    void* data
    );

/**
 * @brief Get dispatch data pointer
 *
 * Gets the user data pointer which is passed to dispatch functions.
 * If no pointer was explicitly set, the value defaults to NULL.
 *
 * @param server the server object
 * @return the data pointer
 */
void*
lwmsg_server_get_dispatch_data(
    LWMsgServer* server
    );

/**
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

/**
 * @brief Set exception handler
 *
 * Sets a callback function which will be invoked when an error occurs
 * within the running server.  The function may take appropriate action depending
 * on the error, such as logging a warning or instructing the main application
 * thread to shut down.
 *
 * @warning Do not call #lwmsg_server_stop() from an exception handler
 *
 * @param server the server handle
 * @param except the handler function
 * @param except_data a user data pointer to pass to the handler function
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_server_set_exception_function(
    LWMsgServer* server,
    LWMsgServerExceptionFunction except,
    void* except_data
    );

/*@}*/

#endif
