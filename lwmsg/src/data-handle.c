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
 *        data-handle.c
 *
 * Abstract:
 *
 *        Data handle management
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>

#include "data-private.h"
#include "util-private.h"

LWMsgStatus
lwmsg_data_handle_new(
    const LWMsgContext* context,
    LWMsgDataHandle** handle
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgDataHandle* my_handle = NULL;

    my_handle = calloc(1, sizeof(*my_handle));
    if (!my_handle)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    my_handle->context = context;
    my_handle->byte_order = LWMSG_BIG_ENDIAN;

    *handle = my_handle;

done:

    return status;

error:

    *handle = NULL;

    if (my_handle)
    {
        free(my_handle);
    }

    goto done;
}

void
lwmsg_data_handle_delete(
    LWMsgDataHandle* handle
    )
{
    lwmsg_error_clear(&handle->error);

    free(handle);
}

const char*
lwmsg_data_handle_get_error_message(
    LWMsgDataHandle* handle,
    LWMsgStatus status
    )
{
    return lwmsg_error_message(status, &handle->error);
}

void
lwmsg_data_handle_set_byte_order(
    LWMsgDataHandle* handle,
    LWMsgByteOrder byte_order
    )
{
    handle->byte_order = byte_order;
}

LWMsgByteOrder
lwmsg_data_handle_get_byte_order(
    LWMsgDataHandle* handle
    )
{
    return handle->byte_order;
}

const LWMsgContext*
lwmsg_data_handle_get_context(
    LWMsgDataHandle* handle
    )
{
    return handle->context;
}

LWMsgStatus
lwmsg_data_handle_raise_error(
    LWMsgDataHandle* handle,
    LWMsgStatus status,
    const char* format,
    ...
    )
{
    va_list ap;

    va_start(ap, format);

    status = lwmsg_error_raise_v(&handle->error, status, format, ap);

    va_end(ap);

    return status;
}

