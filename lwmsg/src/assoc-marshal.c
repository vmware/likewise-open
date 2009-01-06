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
 *        assoc-marshal.c
 *
 * Abstract:
 *
 *        Association API
 *        Marshalling logic for association-specific datatypes
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include <lwmsg/type.h>
#include "convert.h"
#include "util-private.h"
#include "assoc-private.h"

#include <stdlib.h>
#include <string.h>

static LWMsgStatus
lwmsg_assoc_marshal_handle(
    LWMsgContext* context, 
    size_t object_size,
    void* object,
    LWMsgBuffer* buffer,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    void* pointer = NULL;
    LWMsgHandleLocation location;
    unsigned long handle;
    LWMsgAssoc* assoc = NULL;
    LWMsgSessionManager* manager = NULL;
    LWMsgSession* session = NULL;
    unsigned char blob[5];

    BAIL_ON_ERROR(status = lwmsg_context_get_data(context, "assoc", (void**) (void*) &assoc));

    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, &session));

    pointer = *(void**) object;

    status = lwmsg_session_manager_handle_pointer_to_id(
                      manager,
                      session,
                      (const char*) data,
                      pointer,
                      LWMSG_TRUE,
                      &location,
                      &handle);

    switch (status)
    {
    case LWMSG_STATUS_NOT_FOUND:
        status = LWMSG_STATUS_MALFORMED;
    default:
        BAIL_ON_ERROR(status);
    }

    blob[0] = (unsigned char) location;
    
    BAIL_ON_ERROR(status = lwmsg_convert_integer(
                      &handle,
                      sizeof(handle),
                      LWMSG_NATIVE_ENDIAN,
                      &blob[1],
                      4,
                      LWMSG_BIG_ENDIAN,
                      LWMSG_UNSIGNED));

    BAIL_ON_ERROR(status = lwmsg_buffer_write(buffer, blob, sizeof(blob)));

error:

    return status;
}

static LWMsgStatus
lwmsg_assoc_unmarshal_handle(
    LWMsgContext* context,
    LWMsgBuffer* buffer,
    size_t object_size,
    void* object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;
    void* pointer = NULL;
    LWMsgHandleLocation location;
    unsigned long handle;
    char temp[5];
    LWMsgSessionManager* manager = NULL;
    LWMsgSession* session = NULL;
    
    BAIL_ON_ERROR(status = lwmsg_context_get_data(context, "assoc", (void**) (void*) &assoc));
    
    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, &session));
    
    /* Read handle type and number */
    BAIL_ON_ERROR(status = lwmsg_buffer_read(buffer, &temp, sizeof(temp)));
    
    location = !(LWMsgHandleLocation) temp[0];

    BAIL_ON_ERROR(status = lwmsg_convert_integer(
                      temp + 1,
                      4,
                      LWMSG_BIG_ENDIAN,
                      &handle,
                      sizeof(handle),
                      LWMSG_NATIVE_ENDIAN,
                      LWMSG_UNSIGNED));

    /* Convert handle to pointer */
    status = lwmsg_session_manager_handle_id_to_pointer(
        manager,
        session,
        (const char*) data,
        location,
        handle,
        location == LWMSG_HANDLE_REMOTE,
        &pointer);

    switch (status)
    {
    case LWMSG_STATUS_NOT_FOUND:
        status = LWMSG_STATUS_MALFORMED;
    default:
        BAIL_ON_ERROR(status);    
    }
    
    /* Set pointer on unmarshalled object */
    *(void**) object = pointer;

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_verify_handle_local(
    LWMsgContext* context,
    LWMsgBool unmarshalling,
    size_t object_size,
    void* object,
    void* data)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    void* handle = *(void**) object;
    LWMsgHandleLocation location;
    LWMsgAssoc* assoc = NULL;

    BAIL_ON_ERROR(status = lwmsg_context_get_data(context, "assoc", (void**) (void*) &assoc));

    if (handle)
    {
        status = lwmsg_assoc_get_handle_location(assoc, handle, &location);

        if (!status && unmarshalling && location != LWMSG_HANDLE_LOCAL)
        {
            RAISE_ERROR(context, status = LWMSG_STATUS_MALFORMED,
                        "Handle was remote when it was expected to be local");
        }
        else if (!status && !unmarshalling && location != LWMSG_HANDLE_REMOTE)
        {
            RAISE_ERROR(context, status = LWMSG_STATUS_MALFORMED,
                        "Handle was local when it was expected to be remote");
        }
        else if (!unmarshalling && status == LWMSG_STATUS_NOT_FOUND)
        {
            /* If we are marshalling and the handle is not yet known, it simply
               may not be registered yet */
            status = LWMSG_STATUS_SUCCESS;
        }
        else
        {
            BAIL_ON_ERROR(status);
        }
    }

error:

    return status;
}

LWMsgStatus
lwmsg_assoc_verify_handle_remote(
    LWMsgContext* context,
    LWMsgBool unmarshalling,
    size_t object_size,
    void* object,
    void* data)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    void* handle = *(void**) object;
    LWMsgHandleLocation location;
    LWMsgAssoc* assoc = NULL;

    BAIL_ON_ERROR(status = lwmsg_context_get_data(context, "assoc", (void**) (void*) &assoc));

    if (handle)
    {
        status = lwmsg_assoc_get_handle_location(assoc, handle, &location);

        if (!status && unmarshalling && location != LWMSG_HANDLE_REMOTE)
        {
            RAISE_ERROR(context, status = LWMSG_STATUS_MALFORMED,
                        "Handle was local when it was expected to be remote");
        }
        else if (!status && !unmarshalling && location != LWMSG_HANDLE_LOCAL)
        {
            RAISE_ERROR(context, status = LWMSG_STATUS_MALFORMED,
                        "Handle was remote when it was expected to be local");
        }
        else if (!unmarshalling && status == LWMSG_STATUS_NOT_FOUND)
        {
            /* If we are marshalling and the handle is not yet known, it
               may not be registered yet */
            status = LWMSG_STATUS_SUCCESS;
        }
        else
        {
            BAIL_ON_ERROR(status);
        }
    }

error:

    return status;
}

LWMsgCustomTypeClass lwmsg_handle_type_class =
{
    .marshal = lwmsg_assoc_marshal_handle,
    .unmarshal = lwmsg_assoc_unmarshal_handle
};
