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
lwmsg_connection_buffer_construct(ConnectionBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    memset(buffer, 0, sizeof(*buffer));

    lwmsg_ring_init(&buffer->pending);
    lwmsg_ring_init(&buffer->unused);

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

    if (buffer->fd)
    {
        free(buffer->fd);
    }

    goto done;
}

void
lwmsg_connection_buffer_destruct(ConnectionBuffer* buffer)
{
    LWMsgRing* ring, *next;

    if (buffer->fd)
    {
        free(buffer->fd);
    }

    for (ring = buffer->pending.next; ring != &buffer->pending; ring = next)
    {
        next = ring->next;
        free(LWMSG_OBJECT_FROM_MEMBER(ring, ConnectionFragment, ring));
    }

    for (ring = buffer->unused.next; ring != &buffer->unused; ring = next)
    {
        next = ring->next;
        free(LWMSG_OBJECT_FROM_MEMBER(ring, ConnectionFragment, ring));
    }

    memset(buffer, 0, sizeof(*buffer));
}

LWMsgStatus
lwmsg_connection_buffer_create_fragment(
    ConnectionBuffer* buffer,
    size_t length,
    ConnectionFragment** fragment
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ConnectionFragment* frag = NULL;

    if (buffer->unused.next != &buffer->unused)
    {
        frag = LWMSG_OBJECT_FROM_MEMBER(buffer->unused.next, ConnectionFragment, ring);
        lwmsg_ring_remove(buffer->unused.next);
        buffer->num_unused--;
        memset(frag->data, 0, length);
    }
    else
    {
        frag = calloc(1, offsetof(ConnectionFragment, data) + length);
        if (!frag)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
        }
        lwmsg_ring_init(&frag->ring);
    }

    frag->cursor = frag->data;

    *fragment = frag;

error:

    return status;
}

void
lwmsg_connection_buffer_queue_fragment(
    ConnectionBuffer* buffer,
    ConnectionFragment* fragment
    )
{
    lwmsg_ring_insert_before(&buffer->pending, &fragment->ring);
    buffer->num_pending++;
}

ConnectionFragment*
lwmsg_connection_buffer_dequeue_fragment(
    ConnectionBuffer* buffer
    )
{
    LWMsgRing* ring = NULL;

    if (buffer->pending.next == &buffer->pending)
    {
        return NULL;
    }
    else
    {
        ring = buffer->pending.next;
        lwmsg_ring_remove(ring);
        buffer->num_pending--;
        return LWMSG_OBJECT_FROM_MEMBER(ring, ConnectionFragment, ring);
    }
}

ConnectionFragment*
lwmsg_connection_buffer_get_first_fragment(
    ConnectionBuffer* buffer
    )
{
    if (buffer->pending.next != &buffer->pending)
    {
        return LWMSG_OBJECT_FROM_MEMBER(buffer->pending.next, ConnectionFragment, ring);
    }
    else
    {
        return NULL;
    }
}

ConnectionFragment*
lwmsg_connection_buffer_get_last_fragment(
    ConnectionBuffer* buffer
    )
{
    if (buffer->pending.prev != &buffer->pending)
    {
        return LWMSG_OBJECT_FROM_MEMBER(buffer->pending.prev, ConnectionFragment, ring);
    }
    else
    {
        return NULL;
    }
}

void
lwmsg_connection_buffer_free_fragment(
    ConnectionBuffer* buffer,
    ConnectionFragment* fragment
    )
{
    lwmsg_ring_insert_after(&buffer->unused, &fragment->ring);
    buffer->num_unused++;
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
