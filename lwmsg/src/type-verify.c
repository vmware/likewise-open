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
 *        type-verify.c
 *
 * Abstract:
 *
 *        Data verification functions for built-in types
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "type-private.h"
#include "util-private.h"
#include "convert.h"

LWMsgStatus
lwmsg_type_verify_range(
    LWMsgContext* context,
    LWMsgBool unmarshalling,
    size_t object_size,
    void* object,
    void* data)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter* iter = (LWMsgTypeIter*) data;
    intmax_t lower;
    intmax_t upper;
    intmax_t value;

    BAIL_ON_ERROR(status = lwmsg_convert_integer(
                      object,
                      object_size,
                      LWMSG_NATIVE_ENDIAN,
                      &value,
                      sizeof(value),
                      LWMSG_NATIVE_ENDIAN,
                      iter->info.kind_integer.sign));

    BAIL_ON_ERROR(status = lwmsg_convert_integer(
                      iter->info.kind_integer.range,
                      sizeof(size_t),
                      LWMSG_NATIVE_ENDIAN,
                      &lower,
                      sizeof(lower),
                      LWMSG_NATIVE_ENDIAN,
                      iter->info.kind_integer.sign));

    BAIL_ON_ERROR(status = lwmsg_convert_integer(
                      iter->info.kind_integer.range + 1,
                      sizeof(size_t),
                      LWMSG_NATIVE_ENDIAN,
                      &upper,
                      sizeof(upper),
                      LWMSG_NATIVE_ENDIAN,
                      iter->info.kind_integer.sign));

    if (value < lower || value > upper)
    {
        RAISE_ERROR(context, status = LWMSG_STATUS_MALFORMED,
                    "Integer value did not fall within specified range");
    }

error:

    return status;
}

LWMsgStatus
lwmsg_type_verify_not_null(
    LWMsgContext* context,
    LWMsgBool unmarshalling,
    size_t object_size,
    void* object,
    void* data)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    if (!*(void**) object)
    {
        RAISE_ERROR(context, status = LWMSG_STATUS_MALFORMED,
                    "Non-nullable pointer value was NULL");
    }

error:

    return status;
}
