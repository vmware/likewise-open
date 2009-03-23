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
 *        type-analyze.c
 *
 * Abstract:
 *
 *        Type specification API
 *        Basic type analysis
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "type-private.h"
#include "util-private.h"
#include "convert.h"

LWMsgStatus
lwmsg_type_extract_discrim_tag(
    LWMsgTypeIter* iter,
    unsigned char* dominating_struct,
    intmax_t* tag
    )
{
    return lwmsg_convert_integer(
        dominating_struct + iter->info.kind_compound.discrim.offset,
        iter->info.kind_compound.discrim.size,
        LWMSG_NATIVE_ENDIAN,
        tag,
        sizeof(*tag),
        LWMSG_NATIVE_ENDIAN,
        LWMSG_SIGNED);
}

LWMsgStatus
lwmsg_type_extract_length(
    LWMsgTypeIter* iter,
    unsigned char* dominating_struct,
    size_t *length
    )
{
    return lwmsg_convert_integer(
        dominating_struct + iter->info.kind_indirect.term_info.member.offset,
        iter->info.kind_indirect.term_info.member.size,
        LWMSG_NATIVE_ENDIAN,
        length,
        sizeof(*length),
        LWMSG_NATIVE_ENDIAN,
        LWMSG_SIGNED);
}

LWMsgStatus
lwmsg_type_extract_active_arm(
    LWMsgTypeIter* iter,
    unsigned char* dominating_struct,
    LWMsgTypeIter* active_iter
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    intmax_t tag;

    BAIL_ON_ERROR(status = lwmsg_type_extract_discrim_tag(
                      iter,
                      dominating_struct,
                      &tag));
    
    for (lwmsg_type_enter(iter, active_iter);
         lwmsg_type_valid(active_iter);
         lwmsg_type_next(active_iter))
    {
        if (tag == active_iter->tag)
        {
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    
done:

    return status;

error:
        
    goto done;
}

static LWMsgStatus
lwmsg_object_is_zero(
    LWMsgTypeIter* iter,
    unsigned char* object, int* is_zero
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i;

    *is_zero = 1;

    for (i = 0; i < iter->size; i++)
    {
        if (object[i] != 0)
        {
            *is_zero = 0;
            break;
        }
    }

    return status;
}

LWMsgStatus
lwmsg_type_calculate_indirect_metrics(
    LWMsgTypeIter* iter,
    unsigned char* object,
    size_t* count,
    size_t* element_size
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char* element = NULL;
    int is_zero;
    LWMsgTypeIter inner;

    lwmsg_type_enter(iter, &inner);

    switch (iter->info.kind_indirect.term)
    {
    case LWMSG_TERM_STATIC:
        *count = iter->info.kind_indirect.term_info.static_length;
        break;
    case LWMSG_TERM_MEMBER:
        /* Extract the length out of the field of the actual structure */
        BAIL_ON_ERROR(status = lwmsg_type_extract_length(
                          iter,
                          iter->dom_object,
                          count));
        break;
    case LWMSG_TERM_ZERO:
        element = object;
        is_zero = 0;

        /* We have to calculate the count by searching for the zero element */
        for (*count = 0;;*count += 1)
        {
            BAIL_ON_ERROR(status = lwmsg_object_is_zero(
                              &inner,
                              element,
                              &is_zero));

            if (is_zero)
            {
                break;
            }

            element += inner.size;
        }
    }

    *element_size = inner.size;

error:

    return status;
}
