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
 *        call.h
 *
 * Abstract:
 *
 *        Call handle interface
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#ifndef __LWMSG_CALL_H__
#define __LWMSG_CALL_H__

#include <lwmsg/message.h>
#include <lwmsg/status.h>
#include <lwmsg/context.h>

typedef struct LWMsgCall LWMsgCall;

typedef void
(*LWMsgCompleteFunction) (
    LWMsgCall* call,
    LWMsgStatus status,
    void* data
    );

typedef void
(*LWMsgCancelFunction) (
    LWMsgCall* call,
    void* data
    );

LWMsgStatus
lwmsg_call_transact(
    LWMsgCall* call,
    LWMsgCompleteFunction complete,
    void* data
    );

void
lwmsg_call_pend(
    LWMsgCall* call,
    LWMsgCancelFunction cancel,
    void* data
    );

void
lwmsg_call_complete(
    LWMsgCall* call,
    LWMsgStatus status
    );

void
lwmsg_call_cancel(
    LWMsgCall* call
    );

void
lwmsg_call_release(
    LWMsgCall* call
    );

#endif
