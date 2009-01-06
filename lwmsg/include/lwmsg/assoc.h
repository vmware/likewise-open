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
 * @brief The tag of a message
 *
 * A tag which identifies a message
 */
typedef unsigned int LWMsgMessageTag;

/**
 * @ingroup assoc
 * @brief A Message
 *
 * Contains all information needed to describe a message
 */
typedef struct LWMsgMessage
{
    /** The tag of the message */
    LWMsgMessageTag tag;
    /** The unmarshalled message payload */
    void *object;
} LWMsgMessage;

/**
 * @brief Callback to clean up a handle
 *
 * A callback used to clean up a handle after it is no longer in use.
 * A cleanup callback can be registered as part of lwmsg_assoc_register_handle().
 *
 * @param[in] handle the handle to clean up
 */
typedef void (*LWMsgHandleCleanupFunction) (void* handle);

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
    /** Unspecified state */
    LWMSG_ASSOC_STATE_NONE,
    /** Association not ready */
    LWMSG_ASSOC_STATE_NOT_READY,
    /** Operation is in progress */
    LWMSG_ASSOC_STATE_IN_PROGRESS,
    /** Ready to send or receive a message */
    LWMSG_ASSOC_STATE_READY_SEND_RECV,
    /** Ready to send a message */
    LWMSG_ASSOC_STATE_READY_SEND,
    /** Ready to receive a message */
    LWMSG_ASSOC_STATE_READY_RECV,
    /** Peer closed the association */
    LWMSG_ASSOC_STATE_PEER_CLOSED,
    /** Peer aborted the association */
    LWMSG_ASSOC_STATE_PEER_ABORTED,
    /** Peer reset the association */
    LWMSG_ASSOC_STATE_PEER_RESET,
    /** Assocation closed locally */
    LWMSG_ASSOC_STATE_LOCAL_CLOSED,
    /** Association aborted locally */
    LWMSG_ASSOC_STATE_LOCAL_ABORTED,
} LWMsgAssocState;

typedef enum LWMsgAssocException
{
    LWMSG_ASSOC_EXCEPTION_TIMEOUT,
    LWMSG_ASSOC_EXCEPTION_PEER_RESET,
    LWMSG_ASSOC_EXCEPTION_COUNT
} LWMsgAssocException;

/**
 * @ingroup assoc
 * @brief Recovery action
 *
 * Represents a possible action to take as passed to
 * lwmsg_assoc_set_action().
 */
typedef enum LWMsgAssocAction
{
    LWMSG_ASSOC_ACTION_NONE,
    LWMSG_ASSOC_ACTION_RETRY,
    LWMSG_ASSOC_ACTION_RESET_AND_RETRY,
    LWMSG_ASSOC_ACTION_COUNT
} LWMsgAssocAction;

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
     * @param[in] timeout the maximum time to allow before the operation times out, or NULL for no timeout
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{TIMEOUT, operation timed out}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*send_msg)(LWMsgAssoc* assoc, LWMsgMessage* message, LWMsgTime* timeout);
    /**
     * @ingroup assoc_impl
     * @brief Message receive method
     *
     * This method performs a message receive operation.
     *
     * @param[in] assoc the association
     * @param[out] message the received message
     * @param[in] timeout the maximum time to allow before the operation times out, or NULL for no timeout
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{TIMEOUT, operation timed out}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*recv_msg)(LWMsgAssoc* assoc, LWMsgMessage* message, LWMsgTime* timeout);
    /**
     * @ingroup assoc_impl
     * @brief Association close method
     *
     * This method performs logic to shut down an association, e.g.
     * notifying the remote peer.  As opposed to the destructor, this method may
     * fail or time out without successfully completing.
     *
     * @param[in] assoc the association
     * @param[in] timeout the maximum time to allow before the operation times out
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{TIMEOUT, operation timed out}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*close)(LWMsgAssoc* assoc, LWMsgTime* timeout);
    /**
     * @ingroup assoc_impl
     * @brief Association reset method
     *
     * This method performs logic to reset an association, e.g.
     * notifying the remote peer and resetting internal state.
     *
     * @param[in] assoc the association
     * @param[in] timeout the maximum time to allow before the operation times out
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{TIMEOUT, operation timed out}
     * @lwmsg_etc{implementation-specific failure}
     * @lwmsg_endstatus
     */
    LWMsgStatus (*reset)(LWMsgAssoc* assoc, LWMsgTime* timeout);
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
} LWMsgAssocClass;

/**
 * @ingroup assoc
 * @brief Dispatch callback
 *
 * A callback which handles a particular type of message in
 * a receive transaction.
 *
 * @param[in] assoc the association
 * @param[in] in the received messa
 * @param[out] out the reply
 * @param[in] user data pointer
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_etc{callback-specific failure}
 * @lwmsg_endstatus
 */
typedef LWMsgStatus (*LWMsgDispatchFunction) (
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
    LWMsgDispatchFunction dispatch,
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
 */
LWMsgStatus
lwmsg_assoc_send(
    LWMsgAssoc* assoc,
    LWMsgMessageTag type,
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
 */
LWMsgStatus
lwmsg_assoc_recv(
    LWMsgAssoc* assoc,
    LWMsgMessageTag* type,
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
 */
LWMsgStatus
lwmsg_assoc_send_transact(
    LWMsgAssoc* assoc,
    LWMsgMessageTag in_type,
    void* in_object,
    LWMsgMessageTag* out_type,
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

/**
 * @ingroup assoc
 * @brief Register a handle
 *
 * Explicitly registers a handle with the specified association.
 * Handles are pointers to structures which may be transmitted back
 * and forth in messages but are opaque to the peer -- their contents
 * are not transmitted.
 *
 * A handle will be implicitly registered the first time it is sent
 * as part of a message, but explicit registration allows a cleanup
 * function to be specified.  This means that application code does
 * not need to track handles manually because they will be automatically
 * cleaned up when the association is deleted.
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
 * @brief Unregister a handle
 *
 * Unregisters a handle that will no longer be used.  Both local and remote
 * handles must be explicitly unregistered to avoid resource leaks -- the
 * association cannot infer a handle's lifetime by itself.  This means
 * that most practical protocols will have symmetrical messages that open and
 * close handles so that both peers can agree on their life cycle.  New
 * handles received from a peer are implictly registered, so take care to
 * explicitly unregister them when they are no longer needed.
 *
 * If do_cleanup is true, the cleanup function specified when the handle was
 * registered will be run.
 *
 * @param[in] assoc the assocation
 * @param[in] handle the handle pointer
 * @param[in] do_cleanup whether the cleanup function should be run
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{NOT_FOUND, the specified handle was not registered}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_unregister_handle(
    LWMsgAssoc* assoc,
    void* handle,
    LWMsgBool do_cleanup
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
 * @lwmsg_code{NOT_FOUND, the specified handle was not registered}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_get_handle_location(
    LWMsgAssoc* assoc,
    void* handle,
    LWMsgHandleLocation* location
    );

/**
 * @ingroup assoc
 * @brief Free a message
 *
 * Frees the object graph of a message using the memory manager and
 * protocol of the specified association.
 *
 * @param[in] assoc the assocation
 * @param[in] message the message to free
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{NOT_FOUND, the message tag is not known by the association's protocol}
 * @lwmsg_etc{an error returned by the memory manager}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_free_message(
    LWMsgAssoc* assoc,
    LWMsgMessage* message
    );

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
 */
LWMsgStatus
lwmsg_assoc_free_graph(
    LWMsgAssoc* assoc,
    LWMsgMessageTag tag,
    void* root
    );

/**
 * @ingroup assoc
 * @brief Set operation timeout
 *
 * Sets the maximum time that a single operation (send, receive, close, etc.) may
 * take before the attempt fails with LWMSG_STATUS_TIMEOUT.
 *
 * @param[in] assoc the assocation
 * @param[in] timeout the time value, or NULL to disable timeouts
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_PARAMETER, the specified time was invalid}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_set_timeout(
    LWMsgAssoc* assoc,
    LWMsgTime* timeout
    );

/**
 * @ingroup assoc
 * @brief Set operation timeout in milliseconds
 *
 * Sets the timeout on the specified assocation without the need
 * to use a LWMsgTime structure.
 *
 * @param[in] assoc the assocation
 * @param[in] ms the time value in milliseconds
 */
void
lwmsg_assoc_set_timeout_ms(
    LWMsgAssoc* assoc,
    unsigned long ms
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

/**
 * @ingroup assoc
 * @brief Get association state
 *
 * Gets the current state of the specified association.  This function may be used after
 * another association operation has returned a non-success status code to determine
 * additional information about the cause or recoverability of the problem.  For example,
 * if a send operation results in LWMSG_STATUS_EOF, the association might be in one of the
 * following states:
 *
 * - <tt>LWMSG_ASSOC_STATE_PEER_CLOSED</tt>: the peer closed the association -- no further communication possible
 * - <tt>LWMSG_ASSOC_STATE_PEER_RESET</tt>: the peer reset the association -- communication can resume
 * after resetting the association locally with lwmsg_assoc_reset()
 * - <tt>LWMSG_ASSOC_STATE_PEER_ABORT</tt>: the peer aborted the association -- no further communication possible due
 * to an authentication failure, malformed message, or other fatal error
 *
 * @param[in] assoc the association
 * @return the current state of the association
 */
LWMsgAssocState
lwmsg_assoc_get_state(
    LWMsgAssoc* assoc
    );

LWMsgStatus
lwmsg_assoc_set_action(
    LWMsgAssoc* assoc,
    LWMsgAssocException exception,
    LWMsgAssocAction action
    );

/**
 * @ingroup assoc
 * @brief Set user data for session
 *
 * Sets a user data pointer for the session which the specified association is
 * part of.  If a cleanup function is provided, it will be called when the
 * session is destroyed.
 *
 * @param[in] assoc the association
 * @param[in] data the user data pointer
 * @param[in] cleanup a cleanup function for the data pointer
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_STATE, no session was established}
 * @lwmsg_etc{implementation-specific failure}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_assoc_set_session_data(
    LWMsgAssoc* assoc,
    void* data,
    LWMsgSessionDataCleanupFunction cleanup
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


#ifndef DOXYGEN
extern LWMsgCustomTypeClass lwmsg_handle_type_class;

LWMsgStatus
lwmsg_assoc_verify_handle_local(
    LWMsgContext* context,
    LWMsgBool unmarshalling,
    size_t object_size,
    void* object,
    void* data);

LWMsgStatus
lwmsg_assoc_verify_handle_remote(
    LWMsgContext* context,
    LWMsgBool unmarshalling,
    size_t object_size,
    void* object,
    void* data);

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
 * @brief Specify that handle is local
 *
 * Specifies that the previous type or member, which must be a handle,
 * must be a local handle from the perspective of the receiver.
 * @hideinitializer
 */
#define LWMSG_ATTR_HANDLE_LOCAL LWMSG_ATTR_VERIFY(lwmsg_assoc_verify_handle_local, NULL)

/**
 * @ingroup types
 * @brief Specify that handle is remote
 *
 * Specifies that the previous type or member, which must be a handle,
 * must be a remote handle from the perspective of the receiver.
 * @hideinitializer
 */
#define LWMSG_ATTR_HANDLE_REMOTE LWMSG_ATTR_VERIFY(lwmsg_assoc_verify_handle_remote, NULL)

#endif
