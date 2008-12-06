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
        dominating_struct + iter->assoc_discrim.offset,
        iter->assoc_discrim.size,
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
        dominating_struct + iter->assoc_length.offset,
        iter->assoc_length.size,
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
