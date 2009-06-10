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
 *        protocol.h
 *
 * Abstract:
 *
 *        Protocol specification and construction API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_PROTOCOL_H__
#define __LWMSG_PROTOCOL_H__

#include <lwmsg/status.h>
#include <lwmsg/context.h>
#include <lwmsg/type.h>
#include <lwmsg/buffer.h>

/**
 * @file protocol.h
 * @brief Protocol API
 */

/**
 * @defgroup protocol Protocols
 * @ingroup public
 * @brief Describe messages and message contents
 *
 * Protocols consist of an enumerated set of message tags.
 * Each message tag has an associated marshaller type
 * which describes the layout of the payload for that message.
 * Protocols fully specify the messages available to peers
 * communicating through an association.
 *
 * A protocol object includes one or more protocol specifications,
 * which are simple statically-initialized C arrays.
 */

/*@{*/

/**
 * @brief A protocol object
 *
 * An opaque protocol object suitable for creating associations
 */
typedef struct LWMsgProtocol LWMsgProtocol;

/**
 * @brief Protocol specification structure
 *
 * Defines the messages and payload types available to a protocol.
 * You should initialize a static array of this structure in your source code
 * using #LWMSG_MESSAGE() and #LWMSG_PROTOCOL_END.  The result will be suitable
 * to pass to lwmsg_protocol_add_protocol_spec() or lwmsg_server_add_protocol().
 * Consider the following example:
 *
 * @code
 * enum FooMessageType
 * {
 *     FOO_REQUEST_BAR = 1,
 *     FOO_REPLY_BAR = 2,
 *     FOO_REQUEST_BAZ = 3,
 *     FOO_REPLY_BAZ = 4
 * };
 *
 * static LWMsgProtocolSpec foo_spec[] =
 * {
 *     LWMSG_MESSAGE(FOO_REQUEST_BAR, foo_request_bar_spec),
 *     LWMSG_MESSAGE(FOO_REPLY_BAR, foo_reply_bar_spec),
 *     LWMSG_MESSAGE(FOO_REQUEST_BAZ, foo_request_baz_spec),
 *     LWMSG_MESSAGE(FOO_REPLY_BAZ, foo_reply_baz_spec),
 *     LWMSG_PROTOCOL_END
 * };
 * @endcode
 * 
 * This example assumes the existence of the marshaller type specifications 
 * <tt>foo_request_bar_spec</tt>, <tt>foo_request_baz_spec</tt>,
 * <tt>foo_reply_bar_spec</tt>, and <tt>foo_reply_baz_spec</tt>.  See @ref types
 * for more information on specifying marshaller types.
 */
typedef struct LWMsgProtocolSpec
#ifndef DOXYGEN
{
    unsigned int tag;
    LWMsgTypeSpec* type;
}
#endif
const LWMsgProtocolSpec;

/**
 * @brief Get marshaller type by message tag
 *
 * Gets the marshaller type associated with a given message tag.
 * The retrieved type may be passed directly to the marshaller to
 * marshal or unmarshal message payloads of the given type.
 *
 * @param prot the protocol
 * @param tag the message type
 * @param out_type the marshaller type
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{NOT_FOUND, no such message type exists in the specified protocol}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_protocol_get_message_type(
    LWMsgProtocol* prot,
    unsigned int tag,
    LWMsgTypeSpec** out_type
    );

/**
 * @brief Create a new protocol object
 *
 * Creates a new protocol object with no known messages.
 * Messages must be added with lwmsg_protocol_add_protocol_spec().
 *
 * @param context a marshalling context, or <tt>NULL</tt> for default settings
 * @param prot the created protocol
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_protocol_new(
    LWMsgContext* context,
    LWMsgProtocol** prot
    );

/**
 * @brief Add messages from a protocol specification
 *
 * Adds all messages in the specified protocol specification
 * to the specified protocol object.  This may be performed
 * multiple times to aggregate several protocol specifications.
 *
 * @param prot the protocol object
 * @param spec the protocol specification
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{MALFORMED, an error was detected in the protocol specification or one of the type specifications}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_protocol_add_protocol_spec(
    LWMsgProtocol* prot,
    LWMsgProtocolSpec* spec
    );

/**
 * @brief Delete a protocol object
 *
 * Deletes the specified protocol object.  It is the caller's responsibility
 * to ensure than no users of the object remain.
 *
 * @param prot the protocol object to delete
 */
void
lwmsg_protocol_delete(
    LWMsgProtocol* prot
    );

/**
 * @brief Retrieve last error message
 *
 * Retrives a human-readable error message for the last error that occured
 * on the specified protocol object.  The returned string will become undefined
 * if another function called on the protocol returns an error or the protocol
 * is deleted.
 *
 * @param prot the protocol object
 * @param status the status code of the last error
 * @return a human-readable error string
 */
const char*
lwmsg_protocol_get_error_message(
    LWMsgProtocol* prot,
    LWMsgStatus status
    );
/**
 * @brief Marshal a protocol message
 *
 * Marshals a message of the given tag and payload object into the specified
 * marshalling buffer.
 *
 * @param prot the protocol object
 * @param tag the message tag
 * @param object the message paylaod
 * @param buffer the marshalling buffer
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{NOT_FOUND, the specified message tag is unknown}
 * @lwmsg_code{MALFORMED, the message payload was malformed in some way}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_protocol_marshal(
    LWMsgProtocol* prot,
    unsigned int tag,
    void* object,
    LWMsgBuffer* buffer
    );

/**
 * @brief Unmarshal a protocol message
 *
 * Unmarshals a message of the given tag from a marshalling buffer.
 *
 * @param prot the protocol object
 * @param tag the message tag
 * @param buffer the marshalling buffer
 * @param out_object the unmarshalled message payload
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{NOT_FOUND, the specified message tag is unknown}
 * @lwmsg_code{MALFORMED, the message was malformed in some way}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_protocol_unmarshal(
    LWMsgProtocol* prot,
    unsigned int tag,
    LWMsgBuffer* buffer,
    void** out_object
    );

/**
 * @brief Free a message object graph
 *
 * Frees an object graph for the specified message tag
 *
 * @param prot the protocol object
 * @param tag the message tag
 * @param root the root of the object graph
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{NOT_FOUND, the specified message tag is unknown}
 * @lwmsg_etc{an error returned by the memory manager}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_protocol_free_graph(
    LWMsgProtocol* prot,
    unsigned int tag,
    void* root
    );

/**
 * @ingroup protocol
 * @brief Specify a message tag and type
 *
 * This macro is used in the construction of protocol
 * specifications.  It declares a message by its
 * integer tag and associated marshaller type
 * specification.
 *
 * @param tag the integer identifier for the message
 * @param spec the marshaller type specifier that describes the message payload
 * @hideinitializer
 */
#define LWMSG_MESSAGE(tag, spec) \
    { (tag), (spec) }

/**
 * @ingroup protocol
 * @brief Mark end of protocol specification
 *
 * This macro marks the end of a protocol specification.  All
 * protocol specifications must end with this macro.
 * @hideinitializer
 */
#define LWMSG_PROTOCOL_END { -1, NULL }

/*@}*/

#endif
