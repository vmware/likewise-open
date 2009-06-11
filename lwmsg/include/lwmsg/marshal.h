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
 *        marshal.h
 *
 * Abstract:
 *
 *        Marshalling API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_MARSHAL_H__
#define __LWMSG_MARSHAL_H__

#include <lwmsg/status.h>
#include <lwmsg/type.h>
#include <lwmsg/buffer.h>
#include <lwmsg/context.h>

/**
 * @file marshal.h
 * @brief Marshalling routines
 */

/**
 * @defgroup marshal Marshaller
 * @ingroup public
 * @brief Marshal and unmarshal data structures
 *
 * The LWMsg marshaller interface allows data structures to
 * be converted to and from flat octet-stream representations.
 * Before data structures can be processed by the marshaller, a
 * type specification must be constructed which describes the layout
 * and relationships of the structure fields.  For details, see
 * @ref types.
 *
 * The top level object passed to or returned by marshaller operations
 * must always be a pointer.  As a convenience, certain top-level type
 * specifications will automatically be promoted to pointers to accomodate
 * this restriction:
 * - Structures
 *  
 * All marshalling operations take place in a marshalling context
 * which controls ancillary behavior of the marshaller, such as:
 *
 * <ul>
 * <li>Memory management</li>
 * <li>Error message access</li>
 * </ul>
 *
 * Contexts can optionally reference a parent context,
 * allowing settings to be inherited and overridden.  For
 * example, a child context could override the memory manager
 * of its parent.
 *
 */

/*@{*/

typedef struct LWMsgDataHandle LWMsgDataHandle;

LWMsgStatus
lwmsg_data_handle_new(
    const LWMsgContext* context,
    LWMsgDataHandle** handle
    );

void
lwmsg_data_handle_delete(
    LWMsgDataHandle* handle
    );

const char*
lwmsg_data_handle_get_error_message(
    LWMsgDataHandle* handle,
    LWMsgStatus status
    );

void
lwmsg_data_handle_set_byte_order(
    LWMsgDataHandle* handle,
    LWMsgByteOrder order
    );

LWMsgByteOrder
lwmsg_data_handle_get_byte_order(
    LWMsgDataHandle* handle
    );

const LWMsgContext*
lwmsg_data_handle_get_context(
    LWMsgDataHandle* handle
    );

LWMsgStatus
lwmsg_data_handle_raise_error(
    LWMsgDataHandle* handle,
    LWMsgStatus status,
    const char* format,
    ...
    );

LWMsgStatus
lwmsg_data_free_graph(
    LWMsgDataHandle* handle,
    LWMsgTypeSpec* type,
    void* root
    );

/**
 * @brief Marshal a data structure
 *
 * Converts a data structure of the specified type to a flat, serialized form, storing
 * the result in the provided marshalling buffer.
 *
 * @param context the marshalling context
 * @param type the type specification which describes the type of the data
 * @param object the root of the data to marshal
 * @param buffer the marshalling buffer into which the result will be stored
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{MALFORMED, the provided data did not conform in some way to the provided type information}
 * @lwmsg_code{OVERFLOW, an arithmetic overflow occured while converting an integer to its marshalled form}
 * @lwmsg_code{UNDERFLOW, an arithmetic underflow occured while converting an integer to its marshalled form}
 * @lwmsg_etc{the provided context and buffer may raise implementation-specific errors}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_marshal(
    LWMsgDataHandle* handle,
    LWMsgTypeSpec* type,
    void* object,
    LWMsgBuffer* buffer
    );

/**
 * @brief Marshal a data structure into a simple buffer
 *
 * Converts a data structure of the specified type to a flat, serialized form, storing
 * the result in the provided simple buffer.
 *
 * @param context the marshalling context
 * @param type the type specification which describes the type of the data
 * @param object the root of the data to marshal
 * @param buffer the buffer into which the result will be stored
 * @param length the size of the buffer in bytes
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{MALFORMED, the provided data did not conform in some way to the provided type information}
 * @lwmsg_code{OVERFLOW, an arithmetic overflow occured while converting an integer to its marshalled form}
 * @lwmsg_code{UNDERFLOW, an arithmetic underflow occured while converting an integer to its marshalled form}
 * @lwmsg_code{EOF, the buffer was not large enough to hold the entire serialized representation}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_marshal_flat(
    LWMsgDataHandle* handle,
    LWMsgTypeSpec* type,
    void* object,
    void* buffer,
    size_t length
    );

/**
 * @brief Marshal a data structure while allocating a buffer
 *
 * Converts a data structure of the specified type to a flat, serialized form, automatically
 * allocating sufficient space for the result.  The buffer is allocated using the memory management
 * functions in context passed to lwmsg_data_handle_new().
 *
 * @param context the marshalling context
 * @param type the type specification which describes the type of the data
 * @param object the root of the data to marshal
 * @param buffer the allocated buffer containing the serialized representation
 * @param length the length of the buffer
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{MALFORMED, the provided data did not conform in some way to the provided type information}
 * @lwmsg_code{OVERFLOW, an arithmetic overflow occured while converting an integer to its marshalled form}
 * @lwmsg_code{UNDERFLOW, an arithmetic underflow occured while converting an integer to its marshalled form}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_marshal_flat_alloc(
    LWMsgDataHandle* handle,
    LWMsgTypeSpec* type,
    void* object,
    void** buffer,
    size_t* length
    );

/**
 * @ingroup marshal
 * @brief Unmarshal a data structure
 *
 * Converts a serialized data structure to its unmarshalled form, allocating memory as necessary
 * to form the object graph.
 *
 * @param context the marshalling context
 * @param type the type specification which describes the type of the data
 * @param buffer the marshalling buffer from which data will be read
 * @param out the resulting unmarshalled object
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{MALFORMED, the provided data did not conform in some way to the provided type information}
 * @lwmsg_code{OVERFLOW, an arithmetic overflow occured while converting an integer to its unmarshalled form}
 * @lwmsg_code{UNDERFLOW, an arithmetic underflow occured while converting an integer to its unmarshalled form}
 * @lwmsg_etc{the provided context and buffer may raise implementation-specific errors}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_unmarshal(
    LWMsgDataHandle* handle,
    LWMsgTypeSpec* type,
    LWMsgBuffer* buffer,
    void** out
    );

/**
 * @ingroup marshal
 * @brief Unmarshal a data structure from a simple buffer
 *
 * Converts a serialized data structure to its unmarshalled form, allocating memory as necessary
 * to form the object graph.  The serialized form is read from a simple buffer rather than a
 * full #LWMsgBuffer.
 *
 * @param context the marshalling context
 * @param type the type specification which describes the type of the data
 * @param buffer the simple buffer from which data will be read
 * @param length the length of the buffer in bytes
 * @param out the resulting unmarshalled object
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{MALFORMED, the provided data did not conform in some way to the provided type information}
 * @lwmsg_code{OVERFLOW, an arithmetic overflow occured while converting an integer to its unmarshalled form}
 * @lwmsg_code{UNDERFLOW, an arithmetic underflow occured while converting an integer to its unmarshalled form}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_unmarshal_flat(
    LWMsgDataHandle* handle,
    LWMsgTypeSpec* type,
    void* buffer,
    size_t length,
    void** out
    );

/*@}*/

#endif
