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
 *        assoc.h
 *
 * Abstract:
 *
 *        Association API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_ASSOC_H__
#define __LWMSG_ASSOC_H__

#include <lwmsg/status.h>
#include <lwmsg/message.h>
#include <lwmsg/protocol.h>
#include <lwmsg/context.h>
#include <lwmsg/time.h>
#include <lwmsg/security.h>
#include <lwmsg/session.h>

#include <stdlib.h>
#include <unistd.h>

/**
 * @file assoc.h
 * @brief Message-oriented APIs
 */

/**
 * @defgroup assoc Associations
 * @ingroup public
 * @brief Send and receive messages with a peer
 *
 * Associations are a message-oriented abstraction which provide a more
 * convenient mechanism for communication than raw access to the
 * marshalling facility:
 *
 * <ul>
 * <li>Straightforward message sending and receiving</li>
 * <li>Stateful communication using handles</li>
 * <li>Access to metadata such as peer credentials</li>
 * </ul>
 *
 * Messages consist of a message type and a payload which
 * is marshalled and unmarshalled automatically.  The marshaller type
 * of the associated payload and the set of available messages
 * are defined by a protocol.  
 * 
 * Associations are an abstract data type.  For a concrete implementation
 * useful in interprocess or network communication, see Connections.
 */

/**
 * @defgroup assoc_impl Association Implementation
 * @ingroup ext
 * @brief Create custom association implementations
 *
 * Associations are abstract -- it is possible to create a new implementation
 * with a custom transport mechanism or extended features.
 */

/**
 * @ingroup assoc
 * @brief An association
 *
 * An opaque, abstract structure for message-oriented communication.
 * Associations are not inherently thread-safe and must not be used
 * concurrently from multiple threads.
 */
typedef struct LWMsgAssoc LWMsgAssoc;

/**
 * @ingroup assoc
 * @brief Association state
 *
 * Represents the current state of an association as returned
 * by lwmsg_assoc_get_state().
 */
typedef enum LWMsgAssocState
{
    /** @brief Unspecified state */
    LWMSG_ASSOC_STATE_NONE,
    /** @brief Association is not established */
    LWMSG_ASSOC_STATE_NOT_ESTABLISHED,
    /** @brief Association is idle */
    LWMSG_ASSOC_STATE_IDLE,
    /** @brief Association is blocked waiting to send */
    LWMSG_ASSOC_STATE_BLOCKED_SEND,
    /** @brief Association is blocked waiting to receive */
    LWMSG_ASSOC_STATE_BLOCKED_RECV,
    /** @brief Association is blocked waiting to send and/or receive */
    LWMSG_ASSOC_STATE_BLOCKED_SEND_RECV,
    /** @brief Association is closed */
    LWMSG_ASSOC_STATE_CLOSED,
    /** @brief Association is busy */
    LWMSG_ASSOC_STATE_BUSY,
    /** @brief Association experienced an error */
    LWMSG_ASSOC_STATE_ERROR
} LWMsgAssocState;

/**
 * @ingroup assoc
 * @brief Recovery action
 *
 * Represents a possible action to take as passed to
 * lwmsg_assoc_set_action().
 */
typedef enum LWMsgAssocAction
{
    /** @brief Do not take any action */
    LWMSG_ASSOC_ACTION_NONE,
    /** @brief Retry the operation immediately */
    LWMSG_ASSOC_ACTION_RETRY,
    /** @brief Attempt to reset the connection and then retry the operation */
    LWMSG_ASSOC_ACTION_RESET_AND_RETRY,
#ifndef DOXYGEN
    LWMSG_ASSOC_ACTION_COUNT
#endif
} LWMsgAssocAction;

/**
 * @ingroup assoc
 * @brief Timeout classification
 *
 * Represents a class of timeout which may be set
 * on an association with lwmsg_assoc_set_timeout()
 */
typedef enum LWMsgTimeout
{
    /** @brief Timeout for a message send or receive */
    LWMSG_TIMEOUT_MESSAGE,
    /** @brief Timeout for session establishment */
    LWMSG_TIMEOUT_ESTABLISH,
    /** @brief Idle timeout */
    LWMSG_TIMEOUT_IDLE
} LWMsgTimeout;

/**
 * @ingroup assoc_impl
 * @brief Assocation implementation structure
 *
 * Describes the concrete implementation of an association.
 */
typedef struct LWMsgAssocClass
{
    /** 
     * @ingroup assoc_impl
     * @brief Size of the private data structure used by the implementation
     *
     * Association implementations may maintain a private data structure
     * which can be accessed with lwmsg_assoc_get_private().  This field specificies
     * the size of that structure.
     */
    size_t private_size;
    /** 
     * @ingroup assoc_impl
     * @brief Constructor method
     *
     * This method performs initialization of implementation-specific state for a newly-created association.
     *
     * @param[in] assoc the association being constructed
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_memory
     * @lwmsg_endstatus
     */
    LWMsgStatus (*construct)(LWMsgAssoc* assoc);
    /**
     * @ingroup assoc_impl
     * @brief Destructor
     *
     * This method performs teardown of private state for an association which is being deleted.
     *
     * @param[in] assoc the association being deleted
     */
    void (*destruct)(LWMsgAssoc* assoc);
    /**
     * @ingroup assoc_impl
     * @brief Message send method
     *
     * This method performs a message send operation.
     *
     * @param[in] assoc the association
     * @param[in] message the message to send
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{TIMEOUT, operation timed out}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*send_msg)(LWMsgAssoc* assoc, LWMsgMessage* message);
    /**
     * @ingroup assoc_impl
     * @brief Message receive method
     *
     * This method performs a message receive operation.
     *
     * @param[in] assoc the association
     * @param[out] message the received message
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{TIMEOUT, operation timed out}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*recv_msg)(LWMsgAssoc* assoc, LWMsgMessage* message);
    /**
     * @ingroup assoc_impl
     * @brief Association close method
     *
     * This method performs logic to shut down an association, e.g.
     * notifying the remote peer.  As opposed to the destructor, this method may
     * fail or time out without successfully completing.
     *
     * @param[in] assoc the association
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{TIMEOUT, operation timed out}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*close)(LWMsgAssoc* assoc);
    /**
     * @ingroup assoc_impl
     * @brief Association reset method
     *
     * This method performs logic to reset an association, e.g.
     * notifying the remote peer and resetting internal state.
     *
     * @param[in] assoc the association
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{TIMEOUT, operation timed out}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*reset)(LWMsgAssoc* assoc);
    LWMsgStatus (*finish)(LWMsgAssoc* assoc);
    LWMsgStatus (*set_nonblock)(LWMsgAssoc* assoc, LWMsgBool nonblock);
    /**
     * @ingroup assoc_impl
     * @brief Peer security token access method
     *
     * This method retrieves a security information token for the peer.
     *
     * @param[in] assoc the association
     * @param[out] token the security token
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{INVALID_STATE, the security token is not available in the current state}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*get_peer_security_token)(LWMsgAssoc* assoc, LWMsgSecurityToken** token);
    /**
     * @ingroup assoc_impl
     * @brief Peer session manager ID access method
     *
     * This method retrieves the session handle for the association.
     *
     * @param[in] assoc the association
     * @param[out] session the retrieved session handle
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{INVALID_STATE, the session handle in not available in the current state}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*get_session)(LWMsgAssoc* assoc, LWMsgSession** session);
    /**
     * @ingroup assoc_impl
     * @brief Get association state
     *
     * This method returns the current state of the association.
     *
     * @param[in] assoc the association
     * @return the current state
     */
    LWMsgAssocState (*get_state)(LWMsgAssoc* assoc);

    /**
     * @ingroup assoc_impl
     * @brief Set timeout
     *
     * This method sets a timeout that should be used for subsequent operations
     *
     * @param[in] assoc the association
     * @param[in] type the type of timeout
     * @param[in] value the value of the timeout, or NULL for no timeout
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{UNSUPPORTED, the association does not support the specified timeout type}
     * @lwmsg_etc{implementation-specific error}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*set_timeout)(
        LWMsgAssoc* assoc,
        LWMsgTimeout type,
        LWMsgTime* value
        );

    /**
     * @ingroup assoc_impl
     * @brief Establish session with peer
     *
     * This method causes the association to establish a session with
     * its peer if it has not already.
     *
     * @param[in] assoc the association
     * @param[in] construct session constructor function
     * @param[in] destruct session destructor function
     * @param[in] data user data pointer to pass to the session constructor
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{TIMEOUT, the operation timed out}
     * @lwmsg_code{INVALID_STATE, the association cannot establish a session from its current state}
     * @lwmsg_etc{implementation-specific error}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*establish)(
        LWMsgAssoc* assoc,
        LWMsgSessionConstructor construct,
        LWMsgSessionDestructor destruct,
        void* data
        );
} LWMsgAssocClass;

/**
 * @ingroup assoc
 * @brief Dispatch callback
 *
 * A callback which handles a particular type of message in
 * a receive transaction.
 *
 * @param[in] assoc the association
 * @param[in] in the received message
 * @param[out] out the reply
 * @param[in] user data pointer
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_etc{callback-specific failure}
 * @lwmsg_endstatus
 */
typedef LWMsgStatus (*LWMsgAssocDispatchFunction) (
    LWMsgAssoc* assoc,
    const LWMsgMessage* in,
    LWMsgMessage* out,
    void* data);

/**
 * @ingroup assoc_impl
 * @brief Create a new association
 *
 * Creates a new association with the specified implementation and protocol.
 * 
 * @param[in] context an optional context
 * @param[in] prot the protocol understood by the association
 * @param[in] aclass the implementation structure for the new association
 * @param[out] assoc the created association
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_new(
    const LWMsgContext* context,
    LWMsgProtocol* prot,
    LWMsgAssocClass* aclass,
    LWMsgAssoc** assoc
    );

/**
 * @brief Set marshalling context
 *
 * Sets the marshalling context for the specified association.
 * This affects the marshalling settings used for messages
 * sent and received on the association.
 */
void
lwmsg_assoc_set_context(
    LWMsgAssoc* assoc,
    LWMsgContext* context
    );

/**
 * @ingroup assoc
 * @brief Delete an association
 *
 * Deletes an association, releasing all allocated resources
 * 
 * @param[in] assoc the association to delete
 */
void
lwmsg_assoc_delete(
    LWMsgAssoc* assoc
    );

/**
 * @ingroup assoc_impl
 * @brief Access implementation-private data structure
 *
 * Returns a pointer to the private structure used by the implementation.
 * 
 * @param[in] assoc the association
 * @return a pointer to the private data structure
 */
void*
lwmsg_assoc_get_private(
    LWMsgAssoc* assoc
    );

/**
 * @ingroup assoc
 * @brief Get association protocol
 *
 * Returns the protocol used by the association.
 *
 * @param[in] assoc the association
 * @return the protocol specified when the association was created
 */
LWMsgProtocol*
lwmsg_assoc_get_protocol(
    LWMsgAssoc* assoc
    );

/**
 * @ingroup assoc
 * @brief Send a message
 *
 * Sends a message on the specified association.  This function uses the
 * full LWMsgMessage data structure to specify the message
 *
 * @param[in] assoc the association
 * @param[in] message the message to send
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{TIMEOUT, operation timed out}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_send_message(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    );

/**
 * @ingroup assoc
 * @brief Receive a message
 *
 * Receives a message on the specified association.  This function uses the
 * full LWMsgMessage data structure to return the received message.
 *
 * @param[in] assoc the association
 * @param[out] message the received message
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{TIMEOUT, operation timed out}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_recv_message(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    );

/**
 * @ingroup assoc
 * @brief Send a message and receive a reply
 *
 * This function sends a message and receives a reply in a single
 * operation.
 *
 * @param[in] assoc the association
 * @param[in] send_message the message to send (request)
 * @param[out] recv_message the received message (reply)
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{TIMEOUT, operation timed out}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_send_message_transact(
    LWMsgAssoc* assoc,
    LWMsgMessage* send_message,
    LWMsgMessage* recv_message
    );

/**
 * @ingroup assoc
 * @brief Receive a message and send a reply
 *
 * This function receives a message and sends a reply in a single operation.
 * The received message is passed into the specified dispatch function,
 * which must provide a suitable reply.
 *
 * @param[in] assoc the association
 * @param[in] dispatch the dispatch function
 * @param[in] data user data pointer to pass to dispatch function
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{TIMEOUT, operation timed out}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_recv_message_transact(
    LWMsgAssoc* assoc,
    LWMsgAssocDispatchFunction dispatch,
    void* data
    );

/**
 * @ingroup assoc
 * @brief Send a message (simple)
 *
 * This function sends a message without the complexity of constructing an LWMsgMessage
 * structure.
 *
 * @param[in] assoc the association
 * @param[in] type the type of the message to send
 * @param[in] object the message payload
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{TIMEOUT, operation timed out}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 * @deprecated
 */
LWMsgStatus
lwmsg_assoc_send(
    LWMsgAssoc* assoc,
    LWMsgTag type,
    void* object
    );

/**
 * @ingroup assoc
 * @brief Receive a message (simple)
 *
 * This function receives a message without the complexity of using an LWMsgMessage structure.
 *
 * @param[in] assoc the association
 * @param[out] type the type of the received message
 * @param[out] object the received message payload
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{TIMEOUT, operation timed out}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 * @deprecated
 */
LWMsgStatus
lwmsg_assoc_recv(
    LWMsgAssoc* assoc,
    LWMsgTag* type,
    void** object
    );

/**
 * @ingroup assoc
 * @brief Send a message and receive a reply (simple)
 *
 * This function sends a message and receives a reply without the
 * complexity of using LWMsgMessage structures.
 *
 * @param[in] assoc the association
 * @param[in] in_type the type of the message to send
 * @param[in] in_object the payload of the message to send
 * @param[out] out_type the type of the received message
 * @param[out] out_object the payload of the received message
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{TIMEOUT, operation timed out}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 * @deprecated
 */
LWMsgStatus
lwmsg_assoc_send_transact(
    LWMsgAssoc* assoc,
    LWMsgTag in_type,
    void* in_object,
    LWMsgTag* out_type,
    void** out_object
    );

/**
 * @ingroup assoc
 * @brief Close an association
 *
 * Closes the specified assocation, which may include notifying the
 * peer and shutting down the underlying communication channel.
 * Unlike lwmsg_assoc_delete(), which aggressively closes the
 * association and releases all resources in constant time, this
 * function may block indefinitely, time out, or fail, but allows
 * for a controlled, orderly shutdown.  After an association is closed,
 * the result of performing further sends or receives is unspecified.
 *
 * @param[in] assoc the association
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{TIMEOUT, operation timed out}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_close(
    LWMsgAssoc* assoc
    );

/**
 * @ingroup assoc
 * @brief Reset an association
 *
 * Resets an association to its baseline state, which is similar
 * in immediate effect to closing it (e.g. shutting down the
 * underlying communication channel). However, while closing
 * an association with lwmsg_assoc_close() generally represents
 * an intent to cease further communication, a reset implies that
 * communication may resume once the problem that necessitated the
 * reset has been remedied.  For example, a server which times out an
 * idle client connection will typically reset it rather than close it,
 * indicating to the client that it should reset the association locally
 * and resume communication if it is still alive.
 *
 * The difference between close and reset is not purely symbolic.
 * The association implementation may release additional resources
 * and state it considers obsolete when closed but keep such state
 * intact when it is reset.
 *
 * A reset, like a close, removes an association from its session.
 * If this was the last association in that session, the session
 * and all its handles are no longer valid.
 *
 * @param[in] assoc the association
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{TIMEOUT, operation timed out}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_reset(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_assoc_finish(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_assoc_set_nonblock(
    LWMsgAssoc* assoc,
    LWMsgBool nonblock
    );

/**
 * @ingroup assoc
 * @brief Register a handle
 *
 * Explicitly registers a handle with the specified association.
 * Handles are pointers to structures which may be transmitted back
 * and forth in messages but are opaque to the peer -- their contents
 * are not transmitted.
 *
 * @param[in] assoc the association
 * @param[in] typename the type of the handle as a constant string, which should
 * @param[in] handle the handle pointer
 * be the same as that in the type specification
 * @param[in] cleanup the cleanup function to invoke when the handle is unregistered
 * or the association deleted
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{INVALID_PARAMETER, the specified handle was already registered}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_register_handle(
    LWMsgAssoc* assoc,
    const char* typename,
    void* handle,
    LWMsgHandleCleanupFunction cleanup
    );

/**
 * @ingroup assoc
 * @brief Retain a handle
 *
 * Indicates that there is an additional reference to a handle,
 * and that it should remain available in the session until the
 * reference is released.
 *
 * @param[in] assoc the assocation
 * @param[in] handle the handle pointer
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_HANDLE, the specified handle was not known}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_retain_handle(
    LWMsgAssoc* assoc,
    void* handle
    );

/**
 * @ingroup assoc
 * @brief Release a handle
 *
 * Releases a reference to a handle.  The handle will be
 * cleaned up when the last reference to it is released.
 *
 * @param[in] assoc the assocation
 * @param[in] handle the handle pointer
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_HANDLE, the specified handle was not known}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_release_handle(
    LWMsgAssoc* assoc,
    void* handle
    );

/**
 * @ingroup assoc
 * @brief Unregister a handle
 *
 * Releases a reference to a handle and unregisters it.
 * Subsequent calls to #lwmsg_assoc_retain_handle()
 * and #lwmsg_assoc_release_handle() on the handle will
 * succeed if there are still outstanding references,
 * but all other uses will be considered invalid.
 *
 * This allows other portions of the program which may still
 * hold outstanding references to the handle to finish gracefully
 * before the handle cleanup function is invoked, but prevents
 * any further use of the handle within the session.
 *
 * @param[in] assoc the assocation
 * @param[in] handle the handle pointer
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_HANDLE, the specified handle was not registered}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_unregister_handle(
    LWMsgAssoc* assoc,
    void* handle
    );

/**
 * @ingroup assoc
 * @brief Query handle location
 *
 * Queries the location of a handle (local or remote).  Local handles may
 * be dereferenced safely, but remote handles are proxies with undefined
 * contents -- their addresses are only guaranteed to be unique with respect
 * to other handles in the session.  This function allows the location
 * of a handle to be discovered before its contents are accessed.
 *
 * @param[in] assoc the assocation
 * @param[in] handle the handle pointer
 * @param[out] location the location of the handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_HANDLE, the specified handle was not registered}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_get_handle_location(
    LWMsgAssoc* assoc,
    void* handle,
    LWMsgHandleType* location
    );

/**
 * @ingroup assoc
 * @brief Destroy a message
 *
 * Destroys a message structure, freeing any data payload it may contain.
 *
 * @param[in] assoc the assocation
 * @param[in] message the message to destroy
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{NOT_FOUND, the message tag is not known by the association's protocol}
 * @lwmsg_etc{an error returned by the memory manager}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_destroy_message(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    );

/**
 * @brief Alias for #lwmsg_assoc_destroy_message()
 * @deprecated
 * @hideinitializer
 */
#define lwmsg_assoc_free_message(assoc, message) \
    lwmsg_assoc_destroy_message(assoc, message)

/**
 * @ingroup assoc
 * @brief Free a message (simple)
 *
 * Frees the object graph of a message using the memory manager and
 * protocol of the specified association.  This function does not
 * require a complete LWMsgMessage structure.
 *
 * @param[in] assoc the assocation
 * @param[in] tag the tag of the message to free
 * @param[in] root the root of the object graph
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_endstatus
 * @deprecated
 */
LWMsgStatus
lwmsg_assoc_free_graph(
    LWMsgAssoc* assoc,
    LWMsgTag tag,
    void* root
    );

/**
 * @ingroup assoc
 * @brief Retrieve peer security token
 *
 * Retrieves credentials of the peer from an association.
 *
 * @param[in] assoc the assocation
 * @param[out] token the retrieved security token
 * @lwmsg_status
 * @lwmsg_code{INVALID_STATE, no session is established}
 * @lwmsg_etc{an implementation-specific error}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_get_peer_security_token(
    LWMsgAssoc* assoc,
    LWMsgSecurityToken** token
    );

/**
 * @ingroup assoc
 * @brief Retrieve peer session ID
 *
 * Retrieves the session ID of the peer.  It is usually
 * not necessary for applications to access this value,
 * but it may be useful in some scenarios.
 *
 * @param[in] assoc the association
 * @param[out] id the session ID structure into which the ID will be written
 * @lwmsg_status
 * @lwmsg_code{INVALID_STATE, no session is established}
 * @lwmsg_etc{an implementation-specific error}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_get_peer_session_id(
    LWMsgAssoc* assoc,
    LWMsgSessionID* id
    );

/**
 * @ingroup assoc
 * @brief Retrieve last error message
 *
 * Fetches the error message for the last error which occured on the specified association.
 *
 * @param[in] assoc the assocation
 * @param[in] status the status code of the last error
 * @return a human-readable string describing the error.  The string becomes undefined
 * if another function is called on the assocation and returns an error or the
 * assocation is deleted.
 */
const char*
lwmsg_assoc_get_error_message(
    LWMsgAssoc* assoc,
    LWMsgStatus status
    );

/**
 * @ingroup assoc
 * @brief Set session manager
 * 
 * Sets the session manager (and thus the session) for the specified association.
 * Associations sharing the same session may share handles.
 *
 * @param[in] assoc the association
 * @param[in] manager the session manager
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the session manager cannot be changed in the association's current state}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_set_session_manager(
    LWMsgAssoc* assoc,
    LWMsgSessionManager* manager
    );

/**
 * @ingroup assoc
 * @brief Get session manager
 * 
 * Gets the session manager for the specified association.  If manager was not specifically
 * set, the association will implicitly create a private session manager.
 *
 * @param[in] assoc the association
 * @param[out] manager the session manager
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_get_session_manager(
    LWMsgAssoc* assoc,
    LWMsgSessionManager** manager
    );

LWMsgStatus
lwmsg_assoc_get_session(
    LWMsgAssoc* assoc,
    LWMsgSession** session
    );

/**
 * @ingroup assoc
 * @brief Get association state
 *
 * Gets the current state of the specified association.  This may
 * reveal details such as:
 *
 * - Whether the association has been closed
 * - Whether the association is part of an established session
 * - If the association is ready to send a message or receive a message
 *
 * @param[in] assoc the association
 * @return the current state of the association
 */
LWMsgAssocState
lwmsg_assoc_get_state(
    LWMsgAssoc* assoc
    );

/**
 * @ingroup assoc
 * @brief Configure automatic error handling
 *
 * This function allows the user to configure an association to
 * respond automatically to certain non-success status codes that
 * occur during use.  For example, setting up the action
 * #LWMSG_ASSOC_ACTION_RESET_AND_RETRY for the status code
 * #LWMSG_STATUS_PEER_CLOSE will cause the association to transparently
 * attempt to restablish its session with the peer and resume the current
 * operation should the peer close its end.
 *
 * @param[in] assoc the association
 * @param[in] condition the status code for which the action will be set
 * @param[in] action that action to take when the specified status code occurs
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_PARAMETER, the specified status code\, action\, or combination thereof was invalid}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_set_action(
    LWMsgAssoc* assoc,
    LWMsgStatus condition,
    LWMsgAssocAction action
    );

/**
 * @ingroup assoc
 * @brief Get user data for session
 *
 * Gets a user data pointer for the session which the specified assocation
 * is part of.
 *
 * @param assoc the association
 * @param[out] data the user data pointer
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, no session was established}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_get_session_data(
    LWMsgAssoc* assoc,
    void** data
    );

/**
 * @ingroup assoc
 * @brief Set timeout
 *
 * Sets a timeout that should be used for subsequent operations.
 *
 * @param[in] assoc the association
 * @param[in] type the type of timeout
 * @param[in] value the value of the timeout, or NULL for no timeout
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{UNSUPPORTED, the association does not support the specified timeout type}
 * @lwmsg_etc{implementation-specific error}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_set_timeout(
    LWMsgAssoc* assoc,
    LWMsgTimeout type,
    LWMsgTime* value
    );

/**
 * @ingroup assoc
 * @brief Establish session with peer
 *
 * Causes the specified association to establish a session with
 * its peer if it has not already.
 *
 * @param[in] assoc the association
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, the association cannot establish a session from its current state}
 * @lwmsg_etc{implementation-specific error}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_establish(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_assoc_set_session_functions(
    LWMsgAssoc* assoc,
    LWMsgSessionConstructor construct,
    LWMsgSessionDestructor destruct,
    void* data
    );

LWMsgStatus
lwmsg_assoc_print_message_alloc(
    LWMsgAssoc* assoc,
    LWMsgMessage* message,
    char** result
    );

#ifndef DOXYGEN
extern LWMsgCustomTypeClass lwmsg_handle_type_class;

#define LWMSG_ASSOC_HANDLE_LOCAL_FOR_RECEIVER 0x1
#define LWMSG_ASSOC_HANDLE_LOCAL_FOR_SENDER 0x2

#endif

/**
 * @ingroup types
 * @brief Define a handle
 *
 * Defines a handle type within a type specification.  Handles
 * are opaque pointer types which are only usable with associations.
 *
 * @param htype the name of the handle type
 * @hideinitializer
 */
#define LWMSG_HANDLE(htype) LWMSG_CUSTOM(&lwmsg_handle_type_class, (void*) #htype)

/**
 * @ingroup types
 * @brief Define a handle as a member
 *
 * Defines a handle type as a member of a struct or union.  Handles
 * are opaque pointer types which are only usable with associations.
 *
 * @param type the type of the containing struct or union
 * @param field the field within the containing type
 * @param htype the name of the handle type
 * @hideinitializer
 */
#define LWMSG_MEMBER_HANDLE(type, field, htype) LWMSG_MEMBER_CUSTOM(type, field, &lwmsg_handle_type_class, (void*) #htype)

/**
 * @ingroup types
 * @brief Ensure that handle is local to receiving peer
 *
 * Specifies that the previous type or member, which must be a handle,
 * must be a local handle from the perspective of the receiver.
 * @hideinitializer
 */
#define LWMSG_ATTR_HANDLE_LOCAL_FOR_RECEIVER LWMSG_ATTR_CUSTOM(LWMSG_ASSOC_HANDLE_LOCAL_FOR_RECEIVER)

/**
 * @ingroup types
 * @brief Ensure that handle is local to sending peer
 *
 * Specifies that the previous type or member, which must be a handle,
 * must be a local handle from the perspective of the sender.
 * @hideinitializer
 */
#define LWMSG_ATTR_HANDLE_LOCAL_FOR_SENDER LWMSG_ATTR_CUSTOM(LWMSG_ASSOC_HANDLE_LOCAL_FOR_SENDER)

#endif
