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
 *        call.c
 *
 * Abstract:
 *
 *        Call handle API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include <config.h>
#include "call-private.h"
#include "util-private.h"

LWMsgStatus
lwmsg_call_transact(
    LWMsgCall* call,
    LWMsgCompleteFunction complete,
    void* data
    )
{
    return call->vtbl->transact(call, complete, data);
}

void
lwmsg_call_pend(
    LWMsgCall* call,
    LWMsgCancelFunction cancel,
    void* data
    )
{
    call->vtbl->pend(call, cancel, data);
}

void
lwmsg_call_complete(
    LWMsgCall* call,
    LWMsgStatus status
    )
{
    call->vtbl->complete(call, status);
}

void
lwmsg_call_cancel(
    LWMsgCall* call
    )
{
    call->vtbl->cancel(call);
}

void
lwmsg_call_release(
    LWMsgCall* call
    )
{
    call->vtbl->release(call);
}
