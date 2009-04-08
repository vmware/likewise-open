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
 *        server.h
 *
 * Abstract:
 *
 *        Call dispatch API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_DISPATCH_PRIVATE_H__
#define __LWMSG_DISPATCH_PRIVATE_H__

#include <lwmsg/dispatch.h>
#include <pthread.h>

typedef void
(*LWMsgDispatchCompleteFunction) (
    LWMsgDispatchHandle* handle,
    LWMsgStatus status,
    void* data
    );

typedef enum LWMsgDispatchState
{
    LWMSG_DISPATCH_STATE_PENDING,
    LWMSG_DISPATCH_STATE_INTERRUPTED,
    LWMSG_DISPATCH_STATE_FINISHED
} LWMsgDispatchState;

struct LWMsgDispatchHandle
{
    LWMsgDispatchState volatile state;
    LWMsgStatus volatile status;
    LWMsgDispatchCompleteFunction complete;
    void *complete_data;
    LWMsgDispatchInterruptFunction interrupt;
    void *interrupt_data;
    pthread_mutex_t lock;
};

LWMsgStatus
lwmsg_dispatch_init(
    LWMsgDispatchHandle* handle
    );

void
lwmsg_dispatch_destroy(
    LWMsgDispatchHandle* handle
    );

LWMsgStatus
lwmsg_dispatch_get_result(
    LWMsgDispatchHandle* handle
    );

void
lwmsg_dispatch_begin(
    LWMsgDispatchHandle* handle,
    LWMsgDispatchCompleteFunction complete,
    void* complete_data
    );

void
lwmsg_dispatch_interrupt(
    LWMsgDispatchHandle* handle
    );


#endif
