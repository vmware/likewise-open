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
 *        peer.h
 *
 * Abstract:
 *
 *        Multi-threaded peer API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_PEER_H__
#define __LWMSG_PEER_H__

#include <lwmsg/status.h>
#include <lwmsg/protocol.h>
#include <lwmsg/time.h>
#include <lwmsg/assoc.h>
#include <lwmsg/message.h>
#include <lwmsg/call.h>

/**
 * @file peer.h
 * @brief Peer API
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
 * to handle incoming messages in a peer.  It should
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
 * @param func an #LWMsgPeerCallFunction
 * @hideinitializer
 */
#define LWMSG_DISPATCH_BLOCK(tag, func) \
    {LWMSG_DISPATCH_TYPE_BLOCK, (tag), (void*) (LWMsgPeerCallFunction) (func)}

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
 * @param func an #LWMsgPeerCallFunction
 * @hideinitializer
 */
#define LWMSG_DISPATCH_NONBLOCK(tag, func) \
    {LWMSG_DISPATCH_TYPE_NONBLOCK, (tag), (void*) (LWMsgPeerCallFunction) (func)}

/**
 * @brief Terminate dispatch table
 *
 * This macro is used in dispatch table construction to
 * mark the end of the table
 * @hideinitializer
 */
#define LWMSG_DISPATCH_END {LWMSG_DISPATCH_TYPE_END, -1, NULL}

typedef struct LWMsgPeer LWMsgPeer;

/**
 * @brief Call handler function
 *
 * A callback function which handles an incoming call request.  The function
 * may complete the call immediately by filling in the out params structure
 * and returning #LWMSG_STATUS_SUCCESS, or asynchronously by invoking
 * #lwmsg_call_pend() on the call handle, returning #LWMSG_STATUS_PENDING,
 * and completing the call later with #lwmsg_call_complete().  Returning
 * any other status code will cause the client session to be unceremoniously
 * terminated, and the status code will be propgated to the handler registered
 * on the peer with #lwmsg_peer_set_exception_function().
 *
 * The contents of the in params structure is defined only for the duration
 * of the function call and must not be referenced after the function returns.
 *
 * Data inserted into the out params structure must be allocated with the
 * same memory manager as the peer -- by default, plain malloc().
 * Regardless of the status code returned, all such data will be automatically
 * freed by the peer as long as the tag is set to a valid value
 * (i.e. not #LWMSG_TAG_INVALID).
 *
 * @param[in,out] call the call handle
 * @param[in] in the input parameters
 * @param[out] out the output parameters
 * @param[in] data the data pointer set by #lwmsg_peer_set_dispatch_data()
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{PENDING, the request will be completed asynchronously}
 * @lwmsg_etc{call-specific failure}
 * @lwmsg_endstatus
 * @see #lwmsg_context_set_memory_functions() and #lwmsg_peer_new() for
 * customizing the peer's memory manager.
 */
typedef
LWMsgStatus
(*LWMsgPeerCallFunction) (
    LWMsgCall* call,
    LWMsgParams* in,
    LWMsgParams* out,
    void* data
    );

/**
 * @brief Exception handler function
 *
 * A callback function which is invoked when an exception occurs within
 * the peer.
 *
 * @param peer the peer handle
 * @param status the status code of the error
 * @param data a user data pointer
 */
typedef
void
(*LWMsgPeerExceptionFunction) (
    LWMsgPeer* peer,
    LWMsgStatus status,
    void* data
    );

/**
 * @brief Create a new peer object
 *
 * Creates a new peer object
 *
 * @param[in] context an optional context
 * @param[in] protocol a protocol object which describes the protocol spoken by the peer
 * @param[out] peer the created peer object
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{INVALID_PARAMETER, protocol was <tt>NULL</tt>}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_new(
    const LWMsgContext* context,
    LWMsgProtocol* protocol,
    LWMsgPeer** peer
    );

/**
 * @brief Delete a peer object
 *
 * Deletes a peer object.
 *
 * @warning Attempting to delete a peer which has been started
 * but not stopped will block until the peer stops
 *
 * @param peer the peer object to delete
 */
void
lwmsg_peer_delete(
    LWMsgPeer* peer
    );

/**
 * @brief Set timeout
 *
 * Sets the specified timeout to the specified value.
 * See #lwmsg_assoc_set_timeout() for more information.
 *
 * @param peer the peer object
 * @param type the type of timeout to set
 * @param value the value, or NULL for no timeout
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{UNSUPPORTED, the specified timeout type is not supported}
 * @lwmsg_code{INVALID_PARAMETER, the timeout was invalid}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_set_timeout(
    LWMsgPeer* peer,
    LWMsgTimeout type,
    LWMsgTime* value
    );

/**
 * @brief Set maximum number of simultaneous active connections
 *
 * Sets the maximum numbers of connections which the peer will track
 * simultaneously.  Connections beyond this will wait until a slot becomes
 * available.
 *
 * @param peer the peer object
 * @param max_clients the maximum number of simultaneous clients to support
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the peer is already active}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_set_max_listen_clients(
    LWMsgPeer* peer,
    unsigned int max_clients
    );

/**
 * @brief Set maximum number of backlogged connections
 *
 * Sets the maximum numbers of pending connections which the peer will keep
 * waiting until a client slot becomes available.  Pending connections beyond
 * this value will be rejected outright.
 *
 * @param peer the peer object
 * @param max_backlog the maximum number of clients to queue
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the peer is already active}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_set_max_listen_backlog(
    LWMsgPeer* peer,
    unsigned int max_backlog
    );

/**
 * @brief Add a message dispatch specification
 *
 * Adds a set of message dispatch functions to the specified
 * peer object.
 *
 * @param peer the peer object
 * @param spec the dispatch specification
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_add_dispatch_spec(
    LWMsgPeer* peer,
    LWMsgDispatchSpec* spec
    );

/**
 * @brief Set listening socket
 *
 * Sets the socket on which the peer will listen for connections.
 * This function must be passed a valid file descriptor which is socket
 * that matches the specified mode and is already listening (has had
 * listen() called on it).
 *
 * @param peer the peer object
 * @param mode the connection mode (local or remote)
 * @param fd the socket on which to listen
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the peer is already active}
 * @lwmsg_code{INVALID_PARAMETER, the file descriptor was invalid}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_add_listen_fd(
    LWMsgPeer* peer,
    LWMsgEndpointType type,
    int fd
    );

/**
 * @brief Set listening endpoint
 *
 * Sets the endpoint on which the peer will listen for connections.
 * For local (UNIX domain) endpoints, this is the path of the socket file.
 * For remote (TCP) endpoints, this is the address and port to bind to.
 *
 * @param peer the peer object
 * @param mode the connection mode (local or remote)
 * @param endpoint the endpoint on which to listen
 * @param permissions permissions for the endpoint (only applicable to local mode)
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the peer is already active}
 * @lwmsg_code{INVALID_PARAMETER, the endpoint was invalid}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_add_listen_endpoint(
    LWMsgPeer* peer,
    LWMsgEndpointType type,
    const char* endpoint,
    mode_t permissions
    );

/**
 * @brief Set session construct and destruct functions
 *
 * Sets functions which will be called when a new session
 * is established with a client.  The constructor function
 * may set up a session context which the destructor function
 * should clean up when the session is terminated.
 *
 * @param peer the peer handle
 * @param construct a session constructor function
 * @param destruct a session destructor function
 * @param data a user data pointer to be passed to both functions
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the peer is already active}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_set_listen_session_functions(
    LWMsgPeer* peer,
    LWMsgSessionConstructFunction construct,
    LWMsgSessionDestructFunction destruct,
    void* data
    );

/**
 * @brief Set dispatch data pointer
 *
 * Sets the user data pointer which is passed to dispatch functions.
 * This function may only be used while the peer is stopped.
 *
 * @param peer the peer object
 * @param data the data pointer
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the peer is already running}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_set_dispatch_data(
    LWMsgPeer* peer,
    void* data
    );

/**
 * @brief Get dispatch data pointer
 *
 * Gets the user data pointer which is passed to dispatch functions.
 * If no pointer was explicitly set, the value defaults to NULL.
 *
 * @param peer the peer object
 * @return the data pointer
 */
void*
lwmsg_peer_get_dispatch_data(
    LWMsgPeer* peer
    );

/**
 * @brief Start accepting connections
 *
 * Starts listening for and servicing connections in a separate thread.
 * This function will not block, so the calling function should arrange
 * to do something afterwards while the peer is running, such as handling
 * UNIX signals.
 *
 * @param peer the peer object
 * @return LWMSG_STATUS_SUCCESS on success, or an appropriate status code on failure
 */
LWMsgStatus
lwmsg_peer_start_listen(
    LWMsgPeer* peer
    );

/**
 * @brief Aggressively stop peer
 *
 * Stops the specified peer accepting new connections and aggressively
 * terminates any connections in progress or queued for service.  After
 * this function returns successfully, the peer may be safely deleted with
 * lwmsg_peer_delete().
 *
 * @param peer the peer object
 * @return LWMSG_STATUS_SUCCESS on success, or an appropriate status code on failure
 */
LWMsgStatus
lwmsg_peer_stop_listen(
    LWMsgPeer* peer
    );

/**
 * @brief Set exception handler
 *
 * Sets a callback function which will be invoked when an error occurs
 * within the running peer.  The function may take appropriate action depending
 * on the error, such as logging a warning or instructing the main application
 * thread to shut down.
 *
 * @warning Do not call #lwmsg_peer_stop() from an exception handler
 *
 * @param peer the peer handle
 * @param except the handler function
 * @param except_data a user data pointer to pass to the handler function
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_peer_set_exception_function(
    LWMsgPeer* peer,
    LWMsgPeerExceptionFunction except,
    void* except_data
    );

LWMsgStatus
lwmsg_peer_add_connect_endpoint(
    LWMsgPeer* peer,
    LWMsgEndpointType type,
    const char* endpoint
    );

LWMsgStatus
lwmsg_peer_connect(
    LWMsgPeer* peer
    );

LWMsgStatus
lwmsg_peer_disconnect(
    LWMsgPeer* peer
    );

LWMsgStatus
lwmsg_peer_acquire_call(
    LWMsgPeer* peer,
    LWMsgCall** call
    );

/*@}*/

#endif
