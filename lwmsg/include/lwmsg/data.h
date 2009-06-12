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
 *        data.h
 *
 * Abstract:
 *
 *        Data model API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_DATA_H__
#define __LWMSG_DATA_H__

#include <lwmsg/status.h>
#include <lwmsg/type.h>
#include <lwmsg/buffer.h>
#include <lwmsg/context.h>

/**
 * @file data.h
 * @brief Data model API
 */

/**
 * @defgroup data Data model
 * @ingroup public
 * @brief Data marshaling and manipulation
 *
 * The LWMsg data model allows ordinary C data structures to
 * be converted to and from flat octet-stream representations.
 * Before data structures can be processed, a type specification
 * must be constructed which describes the layout and relationships
 * of the structure fields.  For details, see @ref types.
 *
 * The top level object passed to or returned by marshaller operations
 * must always be a pointer.  As a convenience, certain top-level type
 * specifications will automatically be promoted to pointers to accomodate
 * this restriction:
 * - Structures
 *
 * All operations take a data handle which controls tunable
 * parameters of the data model:
 *
 * <ul>
 * <li>Byte order of the octect representation</li>
 * </ul>
 *
 * In the event of an error, the data handle can be queried for
 * a human-readable diagnostic message.  The data handle may
 * also reference an LWMsg context.
 *
 */

/*@{*/

/**
 * @brief Data handle
 *
 * An opqaue data handle which facilitates all
 * data model operations.
 */
typedef struct LWMsgDataHandle LWMsgDataHandle;

/**
 * @brief Create a data handle
 *
 * Creates a new data handle with an optional context.  Data operations
 * which return allocated memory (e.g. #lwmsg_data_unmarshal()) will use
 * the context's memory management functions.
 *
 * @param[in] context an optional context
 * @param[out] handle the created data handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_data_handle_new(
    const LWMsgContext* context,
    LWMsgDataHandle** handle
    );

/**
 * @brief Delete a data handle
 *
 * Deletes the specified data handle.
 *
 * @param[in,out] handle the data handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_endstatus
 */
void
lwmsg_data_handle_delete(
    LWMsgDataHandle* handle
    );

/**
 * @brief Get last error message
 *
 * Get a descriptive diagnostic message for the last
 * error which occured on the specified data handle.
 * The returned string will remain valid until another
 * operation uses the handle.
 *
 * @param[in,out] handle the data handle
 * @param[in] status the status code returned from the failing function
 * @return a string describing the last error
 */
const char*
lwmsg_data_handle_get_error_message(
    LWMsgDataHandle* handle,
    LWMsgStatus status
    );

/**
 * @brief Set byte order
 *
 * Sets the byte order which will be used for atomic data elements
 * in subsequent marshal and unmarshal operations using the specified
 * data handle.
 *
 * The default byte order is network byte order (big endian).  This is
 * the preferred order for data destined for long-term storage or
 * transmission over a network.
 *
 * @param[out] handle the data handle
 * @param[in] order the data order
 */
void
lwmsg_data_handle_set_byte_order(
    LWMsgDataHandle* handle,
    LWMsgByteOrder order
    );

/**
 * @brief Get Byte order
 *
 * Gets the byte order which will be used for subsequent marshal and
 * unmarshal operations on the specified data handle.
 *
 * @param[in] handle the data handle
 * @return the current byte order
 */
LWMsgByteOrder
lwmsg_data_handle_get_byte_order(
    LWMsgDataHandle* handle
    );

/**
 * @brief Get context
 *
 * Gets the context which was given to #lwmsg_data_handle_new()
 * when the specified data handle was created.
 *
 * @param[in] handle the data handle
 * @return the context, or NULL if no context was given at creation time
 */
const LWMsgContext*
lwmsg_data_handle_get_context(
    LWMsgDataHandle* handle
    );

#ifndef DOXYGEN
LWMsgStatus
lwmsg_data_handle_raise_error(
    LWMsgDataHandle* handle,
    LWMsgStatus status,
    const char* format,
    ...
    );
#endif

/**
 * @brief Free in-memory data graph
 *
 * Recursively frees the data graph rooted at <tt>root</tt>
 * whose type is specified by <tt>type</tt>.  Each contiguous
 * memory object will be freed using the context given to
 * #lwmsg_data_handle_new() when the specified data handle
 * was created.
 *
 * The most common application of this function is to free
 * the entire data graph allocated by a prior unmarshal operation,
 * such as #lwmsg_data_unmarshal().
 *
 * Because the root of the data graph is always specified by
 * a generic pointer, <tt>type</tt> is subject to pointer
 * promotion.
 *
 * @param[in,out] handle the data handle
 * @param[in] type the type of the root node of the graph
 * @param[in,out] root the root of the graph
 */
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
 * @param[in,out] handle the data handle
 * @param[in] type the type specification which describes the type of the data
 * @param[in] object the root of the data to marshal
 * @param[in,out] buffer the marshalling buffer into which the result will be stored
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
 * @param[in,out] handle the data handle
 * @param[in] type the type specification which describes the type of the data
 * @param[in] object the root of the data to marshal
 * @param[out] buffer the buffer into which the result will be stored
 * @param[in] length the size of the buffer in bytes
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
 * functions in context passed to #lwmsg_data_handle_new().
 *
 * @param[in,out] handle the data handle
 * @param[in] type the type specification which describes the type of the data
 * @param[in] object the root of the data to marshal
 * @param[out] buffer the allocated buffer containing the serialized representation
 * @param[out] length the length of the buffer
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
 * @brief Unmarshal a data structure
 *
 * Converts a serialized data structure to its unmarshalled form, allocating memory as necessary
 * to form the object graph.
 *
 * @param[in,out] handle the data handle
 * @param[in] type the type specification which describes the type of the data
 * @param[in,out] buffer the marshalling buffer from which data will be read
 * @param[out] out the resulting unmarshalled object
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
 * @brief Unmarshal a data structure from a simple buffer
 *
 * Converts a serialized data structure to its unmarshalled form, allocating memory as necessary
 * to form the object graph.  The serialized form is read from a simple buffer rather than a
 * full #LWMsgBuffer.
 *
 * @param[in,out] handle the data handle
 * @param[in] type the type specification which describes the type of the data
 * @param[in] buffer the simple buffer from which data will be read
 * @param[in] length the length of the buffer in bytes
 * @param[out] out the resulting unmarshalled object
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

LWMsgStatus
lwmsg_data_print_graph(
    LWMsgDataHandle* handle,
    LWMsgTypeSpec* type,
    void* object,
    LWMsgDataPrintFunction print,
    void* print_data
    );

LWMsgStatus
lwmsg_data_print_graph_alloc(
    LWMsgDataHandle* handle,
    LWMsgTypeSpec* type,
    void* object,
    char** result
    );

/*@}*/

#endif
