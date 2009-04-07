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
 *        Call dispatch API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "dispatch-private.h"
#include "util-private.h"

LWMsgStatus
lwmsg_dispatch_init(
    LWMsgDispatchHandle* handle
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (pthread_mutex_init(&handle->lock, NULL))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

error:

    return status;
}

void
lwmsg_dispatch_destroy(
    LWMsgDispatchHandle* handle
    )
{
    pthread_mutex_destroy(&handle->lock);
}

void
lwmsg_dispatch_begin(
    LWMsgDispatchHandle* handle,
    LWMsgDispatchCompleteFunction complete,
    void* complete_data
    )
{
    handle->state = LWMSG_DISPATCH_STATE_PENDING;
    handle->status = LWMSG_STATUS_NOT_FINISHED;
    handle->complete = complete;
    handle->complete_data = complete_data;
}

void
lwmsg_dispatch_set_interrupt_function(
    LWMsgDispatchHandle* handle,
    LWMsgDispatchInterruptFunction interrupt,
    void* interrupt_data
    )
{
    handle->interrupt = interrupt;
    handle->interrupt_data = interrupt_data;
}

void
lwmsg_dispatch_finish(
    LWMsgDispatchHandle* handle,
    LWMsgStatus status
    )
{
    LWMsgBool run_complete = LWMSG_FALSE;

    pthread_mutex_lock(&handle->lock);

    if (handle->state == LWMSG_DISPATCH_STATE_PENDING ||
        handle->state == LWMSG_DISPATCH_STATE_INTERRUPTED)
    {
        handle->status = status;
        handle->state = LWMSG_DISPATCH_STATE_FINISHED;
        run_complete = LWMSG_TRUE;
    }

    pthread_mutex_unlock(&handle->lock);

    if (run_complete)
    {
        handle->complete(handle, status, handle->complete_data);
    }
}

void
lwmsg_dispatch_interrupt(
    LWMsgDispatchHandle* handle
    )
{
    LWMsgBool run_interrupt = LWMSG_FALSE;

    pthread_mutex_lock(&handle->lock);

    if (handle->state == LWMSG_DISPATCH_STATE_PENDING)
    {
        handle->state = LWMSG_DISPATCH_STATE_INTERRUPTED;
        run_interrupt = LWMSG_TRUE;
    }

    pthread_mutex_unlock(&handle->lock);

    if (run_interrupt)
    {
        handle->interrupt(handle, handle->interrupt_data);
    }
}

LWMsgStatus
lwmsg_dispatch_get_result(
    LWMsgDispatchHandle* handle
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    pthread_mutex_lock(&handle->lock);
    status = handle->status;
    pthread_mutex_unlock(&handle->lock);

    return status;
}
