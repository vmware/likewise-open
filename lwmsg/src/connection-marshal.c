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
 *        Connection-marshal.c
 *
 * Abstract:
 *
 *        Connection API
 *        Marshalling logic for connection-specific data types
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include <lwmsg/type.h>
#include <lwmsg/data.h>
#include "convert.h"
#include "util-private.h"
#include "connection-private.h"

#include <stdlib.h>
#include <string.h>

static LWMsgStatus
lwmsg_connection_marshal_fd(
    LWMsgDataHandle* handle,
    size_t object_size,
    void* object,
    LWMsgTypeAttrs* attrs,
    LWMsgBuffer* buffer,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int fd = -1;
    unsigned char indicator;

    if (object_size == 0)
    {
        object_size = sizeof(fd);
    }

    BAIL_ON_ERROR(status = lwmsg_convert_integer(object,
                                                 object_size,
                                                 LWMSG_NATIVE_ENDIAN,
                                                 &fd,
                                                 sizeof(fd),
                                                 LWMSG_NATIVE_ENDIAN,
                                                 LWMSG_SIGNED));

    if (fd != -1)
    {
        indicator = 0xFF;
        BAIL_ON_ERROR(status = lwmsg_connection_queue_fd((LWMsgAssoc*) buffer->data, fd));
    }
    else
    {
        indicator = 0x00;
    }

    BAIL_ON_ERROR(status = lwmsg_buffer_write(buffer, &indicator, sizeof(indicator)));

error:

    return status;
}

static LWMsgStatus
lwmsg_connection_unmarshal_fd(
    LWMsgDataHandle* handle,
    LWMsgBuffer* buffer,
    size_t object_size,
    LWMsgTypeAttrs* attrs,
    void* object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char flag = 0;
    int fd = -1;

    if (object_size == 0)
    {
        object_size = sizeof(fd);
    }

    BAIL_ON_ERROR(status = lwmsg_buffer_read(buffer, &flag, 1));

    if (flag)
    {
        BAIL_ON_ERROR(status = lwmsg_connection_dequeue_fd((LWMsgAssoc*) buffer->data, &fd));
    }

    BAIL_ON_ERROR(status = lwmsg_convert_integer(
                      &fd,
                      sizeof(fd),
                      LWMSG_NATIVE_ENDIAN,
                      object,
                      object_size,
                      LWMSG_NATIVE_ENDIAN,
                      LWMSG_SIGNED));

error:

    return status;
}

static
void
lwmsg_connection_free_fd(
    const LWMsgContext* context,
    size_t object_size,
    LWMsgTypeAttrs* attrs,
    void* object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    int fd = -1;

    if (object_size == 0)
    {
        object_size = sizeof(fd);
    }

    BAIL_ON_ERROR(status = lwmsg_convert_integer(
                      object,
                      object_size,
                      LWMSG_NATIVE_ENDIAN,
                      &fd,
                      sizeof(fd),
                      LWMSG_NATIVE_ENDIAN,
                      LWMSG_SIGNED));

    if (fd >= 0)
    {
        close(fd);
    }

error:

    return;
}

LWMsgCustomTypeClass lwmsg_fd_type_class =
{
    .is_pointer = LWMSG_FALSE,

    .marshal = lwmsg_connection_marshal_fd,
    .unmarshal = lwmsg_connection_unmarshal_fd,
    .free = lwmsg_connection_free_fd
};
