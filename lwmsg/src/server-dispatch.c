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
 *        server.c
 *
 * Abstract:
 *
 *        Multi-threaded server API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>
#include "server-private.h"

static
LWMsgStatus
lwmsg_server_dispatch_loop(
    ServerDispatchThread* thread
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgBool in_lock = LWMSG_FALSE;
    LWMsgRing* head = NULL;
    ServerCall* call = NULL;
    LWMsgServer* server = thread->server;

    for (;;)
    {
        pthread_mutex_lock(&server->dispatch.lock);
        in_lock = LWMSG_TRUE;

        while (lwmsg_ring_is_empty(&server->dispatch.tasks) &&
               !server->dispatch.shutdown)
        {
            pthread_cond_wait(&server->dispatch.event, &server->dispatch.lock);
        }

        if (server->dispatch.shutdown)
        {
            break;
        }

        head = server->dispatch.tasks.next;
        lwmsg_ring_remove(head);
        call = LWMSG_OBJECT_FROM_MEMBER(head, ServerCall, ring);

        pthread_mutex_unlock(&server->dispatch.lock);
        in_lock = LWMSG_FALSE;

        status = lwmsg_call_transact(LWMSG_CALL(call), NULL, NULL);

        switch (status)
        {
        case LWMSG_STATUS_PENDING:
            /* Callee will asynchronously complete for us */
            break;
        default:
            /* Manually invoke complete to wake up IO thread */
            lwmsg_call_complete(LWMSG_CALL(call), status);
            break;
        }

        /* Ignore any error -- the IO thread will handle it */
        status = LWMSG_STATUS_SUCCESS;
    }

    if (in_lock)
    {
        pthread_mutex_unlock(&server->dispatch.lock);
    }

    return status;
}

void*
lwmsg_server_dispatch_thread(
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    ServerDispatchThread* thread = (ServerDispatchThread*) data;

    BAIL_ON_ERROR(status = lwmsg_server_dispatch_loop(thread));

done:

    return NULL;

error:

    goto done;
}
