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
 *        marshal-private.h
 *
 * Abstract:
 *
 *        Marshalling API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_MARSHAL_PRIVATE_H__
#define __LWMSG_MARSHAL_PRIVATE_H__

#include <lwmsg/marshal.h>
#include "type-private.h"
#include "status-private.h"

struct LWMsgDataHandle
{
    LWMsgErrorContext error;
    const LWMsgContext* context;
    LWMsgByteOrder byte_order;
};

typedef struct LWMsgMarshalState
{
    unsigned char* dominating_object;
} LWMsgMarshalState;

#define MAX_INTEGER_SIZE (16)

#define MARSHAL_RAISE_ERROR(hand, expr, ...) \
    BAIL_ON_ERROR(lwmsg_data_handle_raise_error((hand), (expr), __VA_ARGS__))

#endif
