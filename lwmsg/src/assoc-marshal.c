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
#include <lwmsg/assoc.h>
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
    LWMsgTypeAttrs* attrs,
    LWMsgBuffer* buffer,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    void* pointer = NULL;
    LWMsgHandleType location;
    unsigned long handle;
    LWMsgAssoc* assoc = NULL;
    LWMsgSessionManager* manager = NULL;
    LWMsgSession* session = NULL;
    unsigned char blob[5];
    const char* type;

    BAIL_ON_ERROR(status = lwmsg_context_get_data(context, "assoc", (void**) (void*) &assoc));

    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, &session));

    pointer = *(void**) object;

    if (pointer != NULL)
    {
        status = lwmsg_session_manager_handle_pointer_to_id(
            manager,
            session,
            pointer,
            &type,
            &location,
            &handle);

        switch (status)
        {
        case LWMSG_STATUS_NOT_FOUND:
            /* Implicitly register handle */
            if (attrs->custom & LWMSG_ASSOC_HANDLE_LOCAL_FOR_RECEIVER)
            {
                /* Automatic registration doesn't make sense if the receiver is
                   exepcting a handle it created */
                BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
            }

            location = LWMSG_HANDLE_LOCAL;
            type = (const char*) data;

            BAIL_ON_ERROR(status = lwmsg_session_manager_register_handle_local(
                              manager,
                              session,
                              (const char*) data,
                              pointer,
                              NULL,
                              &handle));
            break;
        default:
            BAIL_ON_ERROR(status);
        }

        /* Confirm that the handle is of the expected type */
        if (strcmp((const char*) data, type))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }

        /* Confirm that handle origin is correct according to type attributes */
        if ((attrs->custom & LWMSG_ASSOC_HANDLE_LOCAL_FOR_RECEIVER && location != LWMSG_HANDLE_REMOTE) ||
            (attrs->custom & LWMSG_ASSOC_HANDLE_LOCAL_FOR_SENDER && location != LWMSG_HANDLE_LOCAL))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
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
    }
    else
    {
        /* If the handle was not supposed to be NULL, raise an error */
        if (attrs->nonnull)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }

        blob[0] = (unsigned char) LWMSG_HANDLE_NULL;

        BAIL_ON_ERROR(status = lwmsg_buffer_write(buffer, blob, sizeof(blob[0])));
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_assoc_unmarshal_handle(
    LWMsgContext* context,
    LWMsgBuffer* buffer,
    size_t object_size,
    LWMsgTypeAttrs* attrs,
    void* object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgAssoc* assoc = NULL;
    void* pointer = NULL;
    LWMsgHandleType location;
    unsigned long handle;
    char temp[5];
    LWMsgSessionManager* manager = NULL;
    LWMsgSession* session = NULL;
    
    BAIL_ON_ERROR(status = lwmsg_context_get_data(context, "assoc", (void**) (void*) &assoc));
    
    BAIL_ON_ERROR(status = lwmsg_assoc_get_session_manager(assoc, &manager));
    BAIL_ON_ERROR(status = assoc->aclass->get_session(assoc, &session));
    
    /* Read handle type */
    BAIL_ON_ERROR(status = lwmsg_buffer_read(buffer, &temp[0], sizeof(temp[0])));
    
    location = (LWMsgHandleType) temp[0];

    if (location != LWMSG_HANDLE_NULL)
    {
         /* Read handle ID */
        BAIL_ON_ERROR(status = lwmsg_buffer_read(buffer, &temp[1], sizeof(temp) - sizeof(temp[0])));

        /* Invert sense of handle location */
        if (location == LWMSG_HANDLE_LOCAL)
        {
            location = LWMSG_HANDLE_REMOTE;
        }
        else
        {
            location = LWMSG_HANDLE_LOCAL;
        }

        if ((attrs->custom & LWMSG_ASSOC_HANDLE_LOCAL_FOR_RECEIVER && location != LWMSG_HANDLE_LOCAL) ||
            (attrs->custom & LWMSG_ASSOC_HANDLE_LOCAL_FOR_SENDER && location != LWMSG_HANDLE_REMOTE))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }

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
            &pointer);

        switch (status)
        {
        case LWMSG_STATUS_NOT_FOUND:
            /* Implicitly register handle seen from the peer for the first time */
            BAIL_ON_ERROR(status = lwmsg_session_manager_register_handle_remote(
                              manager,
                              session,
                              (const char*) data,
                              handle,
                              NULL,
                              &pointer));
            break;
        default:
            BAIL_ON_ERROR(status);
        }

        /* Set pointer on unmarshalled object */
        *(void**) object = pointer;
    }
    else
    {
        if (attrs->nonnull)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        }
        *(void**) object = NULL;
    }

error:

    return status;
}

LWMsgCustomTypeClass lwmsg_handle_type_class =
{
    .marshal = lwmsg_assoc_marshal_handle,
    .unmarshal = lwmsg_assoc_unmarshal_handle
};
