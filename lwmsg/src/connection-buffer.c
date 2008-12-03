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
 *        connection-buffer.c
 *
 * Abstract:
 *
 *        Connection API
 *        Packet buffer logic
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "connection-private.h"
#include "util-private.h"

LWMsgStatus
lwmsg_connection_construct_buffer(ConnectionBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    buffer->base_length = 0;
    buffer->base_capacity = 2048;
    buffer->base = buffer->cursor = malloc(buffer->base_capacity);

    if (!buffer->base)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    buffer->fd_capacity = MAX_FD_PAYLOAD;
    buffer->fd_length = 0;
    buffer->fd = malloc(buffer->fd_capacity);

    if (!buffer->fd)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

done:

    return status;

error:

    if (buffer->base)
    {
        free(buffer->base);
    }

    if (buffer->fd)
    {
        free(buffer->fd);
    }

    goto done;
}

void
lwmsg_connection_destruct_buffer(ConnectionBuffer* buffer)
{
    if (buffer->base)
    {
        free(buffer->base);
    }

    if (buffer->fd)
    {
        free(buffer->fd);
    }

    memset(buffer, 0, sizeof(buffer));
}

void
lwmsg_connection_buffer_reset(ConnectionBuffer* buffer)
{
    buffer->cursor = buffer->base;
    buffer->base_length = 0;
}

LWMsgStatus
lwmsg_connection_buffer_resize(ConnectionBuffer* buffer, size_t size)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char* newbase = NULL;

    newbase = realloc(buffer->base, size);

    if (!newbase)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    buffer->cursor = newbase + (buffer->cursor - buffer->base);
    buffer->base = newbase;
    buffer->base_capacity = size;

error:

    return status;
}

LWMsgStatus
lwmsg_connection_buffer_ensure_fd_capacity(ConnectionBuffer* buffer, size_t needed)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    int* newfd = NULL;

    if (buffer->fd_capacity < needed)
    {
        buffer->fd_capacity = 256;
    }

    while (buffer->fd_capacity - buffer->fd_length < needed)
    {
        buffer->fd_capacity *= 2;
    }

    newfd = realloc(buffer->fd, sizeof(*newfd) * needed);
    if (!newfd)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    buffer->fd = newfd;

error:
    
    return status;
}

