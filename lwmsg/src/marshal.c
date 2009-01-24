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
 *        marshal.c
 *
 * Abstract:
 *
 *        Marshalling API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "marshal-private.h"
#include "util-private.h"
#include "type-private.h"
#include "convert.h"
#include "config.h"

#include <string.h>

static LWMsgStatus
lwmsg_marshal_internal(
    LWMsgContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer
    );

static LWMsgStatus
lwmsg_object_is_zero(LWMsgMarshalState* state, LWMsgTypeIter* iter, unsigned char* object, int* is_zero)
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

static LWMsgStatus
lwmsg_marshal_indirect_prologue(
    LWMsgContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer,
    LWMsgTypeIter* inner_iter,
    size_t* count)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char* element = NULL;
    unsigned char implicit_length[4];
    int is_zero;

    lwmsg_type_enter(iter, inner_iter);

    switch (iter->info.kind_indirect.term)
    {
    case LWMSG_TERM_STATIC:
        *count = iter->info.kind_indirect.term_info.static_length;
        break;
    case LWMSG_TERM_MEMBER:
        /* Extract the length out of the field of the actual structure */
        BAIL_ON_ERROR(status = lwmsg_type_extract_length(
                          iter,
                          state->dominating_object,
                          count));
        break;
    case LWMSG_TERM_ZERO:
        element = object;
        is_zero = 0;

        /* We have to calculate the count by searching for the zero element */
        for (*count = 0;;*count += 1)
        {
            BAIL_ON_ERROR(status = lwmsg_object_is_zero(
                              state,
                              inner_iter,
                              element,
                              &is_zero));

            if (is_zero)
            {
                break;
            }

            element += inner_iter->size;
        }

        /* The length is implicitly written into the output to
           facilitate easy unmarshalling */
        BAIL_ON_ERROR(status = lwmsg_convert_integer(
                          count,
                          sizeof(count),
                          LWMSG_NATIVE_ENDIAN,
                          implicit_length,
                          4,
                          LWMSG_BIG_ENDIAN,
                          LWMSG_UNSIGNED));

        BAIL_ON_ERROR(status = lwmsg_buffer_write(buffer, implicit_length, 4));
        break;
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_marshal_indirect(
    LWMsgContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i;
    size_t count = 0;
    unsigned char* element = NULL;
    LWMsgTypeIter inner_iter;

    BAIL_ON_ERROR(status = lwmsg_marshal_indirect_prologue(
                      context,
                      state,
                      iter,
                      object,
                      buffer,
                      &inner_iter,
                      &count));
    
    if (inner_iter.kind == LWMSG_KIND_INTEGER &&
        inner_iter.info.kind_integer.width == 1 &&
        inner_iter.size == 1)
    {
        /* As an optimization, if the element type if an 1-byte integer both
           packed and unpacked, we can write the entire array directly into
           the output.  This is important for character strings */
        BAIL_ON_ERROR(status = lwmsg_buffer_write(
                          buffer, 
                          object, 
                          count));
    }
    else if (count)
    {
        /* Otherwise, marshal each element individually */
        element = object;

        for (i = 0; i < count; i++)
        {
            BAIL_ON_ERROR(status = lwmsg_marshal_internal(
                              context,
                              state,
                              &inner_iter, 
                              element, 
                              buffer));
            element += inner_iter.size;
        }
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_marshal_integer(
    LWMsgContext* context, 
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char* in = object;
    unsigned char out[MAX_INTEGER_SIZE];
    size_t in_size;
    size_t out_size;

    out_size = iter->info.kind_integer.width;
    in_size = iter->size;

    /* If a valid range is defined, check value against it */
    if (iter->attrs.range_low < iter->attrs.range_high)
    {
        BAIL_ON_ERROR(status = lwmsg_type_verify_range(
                          context,
                          iter,
                          object,
                          in_size));
    }

    BAIL_ON_ERROR(status = lwmsg_convert_integer(in,
                                                 in_size,
                                                 LWMSG_NATIVE_ENDIAN,
                                                 out,
                                                 out_size,
                                                 LWMSG_BIG_ENDIAN,
                                                 iter->info.kind_integer.sign));
    
    BAIL_ON_ERROR(status = lwmsg_buffer_write(buffer, out, out_size));


error:

    return status;
}

static LWMsgStatus
lwmsg_marshal_custom(
    LWMsgContext* context, 
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = iter->info.kind_custom.typeclass->marshal(
                      context,
                      iter->size,
                      object,
                      &iter->attrs,
                      buffer,
                      iter->info.kind_custom.typedata));

error:

    return status;
}

static LWMsgStatus
lwmsg_marshal_struct_member(
    LWMsgContext* context, 
    LWMsgTypeIter* struct_iter,
    LWMsgTypeIter* member_iter,
    unsigned char* object,
    LWMsgBuffer* buffer)
{
    LWMsgMarshalState my_state;
    unsigned char* member_object = object + member_iter->offset;
    
    my_state.dominating_object = object;

    return lwmsg_marshal_internal(
        context, 
        &my_state,
        member_iter,
        member_object,
        buffer);
}

static LWMsgStatus
lwmsg_marshal_struct(LWMsgContext* context, LWMsgTypeIter* iter, unsigned char* object, LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter member;

    iter->dom_object = object;

    for (lwmsg_type_enter(iter, &member);
         lwmsg_type_valid(&member);
         lwmsg_type_next(&member))
    {
        BAIL_ON_ERROR(status = lwmsg_marshal_struct_member(
                          context,
                          iter,
                          &member,
                          object,
                          buffer));
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_marshal_union(LWMsgContext* context, LWMsgMarshalState* state, LWMsgTypeIter* iter, unsigned char* object, LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter arm;

    /* Find the active arm */
    BAIL_ON_ERROR(status = lwmsg_type_extract_active_arm(
                      iter,
                      state->dominating_object,
                      &arm));

    /* Simply marshal the active arm */
    BAIL_ON_ERROR(status = lwmsg_marshal_internal(
                      context,
                      state,
                      &arm,
                      object,
                      buffer));

error:

    return status;
}

static LWMsgStatus
lwmsg_marshal_pointer(
    LWMsgContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char ptr_flag;

    /* Indicator byte showing whether the pointer is set */
    ptr_flag = *(void**) object ? 0xFF : 0x00;

    /* Enforce nullability */
    if (iter->attrs.nonnull && !ptr_flag)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
    }

    /* Only write pointer flag for nullable pointers */
    if (!iter->attrs.nonnull)
    {
        BAIL_ON_ERROR(status = lwmsg_buffer_write(buffer, &ptr_flag, 1));
    }

    if (ptr_flag)
    {
        /* Pointer is present, so also write pointee */
        BAIL_ON_ERROR(status = lwmsg_marshal_indirect(
                          context,
                          state,
                          iter,
                          *(unsigned char**) object,
                          buffer));
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_marshal_internal(
    LWMsgContext* context,
    LWMsgMarshalState* state,
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    if (iter->verify)
    {
        BAIL_ON_ERROR(status = iter->verify(context, LWMSG_FALSE, iter->size, object, iter->verify_data));
    }

    switch (iter->kind)
    {
    case LWMSG_KIND_VOID:
        /* Nothing to marshal */
        break;
    case LWMSG_KIND_INTEGER:
        BAIL_ON_ERROR(status = lwmsg_marshal_integer(context, state, iter, object, buffer));
        break;
    case LWMSG_KIND_CUSTOM:
        BAIL_ON_ERROR(status = lwmsg_marshal_custom(context, state, iter, object, buffer));
        break;
    case LWMSG_KIND_STRUCT:
        BAIL_ON_ERROR(status = lwmsg_marshal_struct(context, iter, object, buffer));
        break;
    case LWMSG_KIND_UNION:
        BAIL_ON_ERROR(status = lwmsg_marshal_union(context, state, iter, object, buffer));
        break;
    case LWMSG_KIND_POINTER:
        BAIL_ON_ERROR(status = lwmsg_marshal_pointer(context, state, iter, object, buffer));
        break;
    case LWMSG_KIND_ARRAY:
        BAIL_ON_ERROR(status = lwmsg_marshal_indirect(
                          context,
                          state,
                          iter,
                          object,
                          buffer));
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_MALFORMED);
        break;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_marshal(LWMsgContext* context, LWMsgTypeSpec* type, void* object, LWMsgBuffer* buffer)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgMarshalState state = {NULL};
    LWMsgTypeIter iter;

    lwmsg_type_iterate_promoted(type, &iter);

    BAIL_ON_ERROR(status = lwmsg_marshal_internal(context, &state, &iter, (unsigned char*) &object, buffer));

    if (buffer->wrap)
    {
        BAIL_ON_ERROR(status = buffer->wrap(buffer, 0));
    }

error:

    return status;
}

LWMsgStatus
lwmsg_marshal_simple(
    LWMsgContext* context,
    LWMsgTypeSpec* type,
    void* object,
    void* buffer,
    size_t length
    )
{
    LWMsgBuffer mbuf;

    mbuf.base = buffer;
    mbuf.end = buffer + length;
    mbuf.cursor = buffer;
    mbuf.wrap = NULL;

    return lwmsg_marshal(context, type, object, &mbuf);
}

typedef struct
{
    LWMsgReallocFunction realloc;
    LWMsgFreeFunction free;
    void* data;
} AllocInfo;

static
LWMsgStatus
lwmsg_marshal_alloc_wrap(
    LWMsgBuffer* buffer,
    size_t needed
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    AllocInfo* info = buffer->data;
    size_t oldlen = buffer->end - buffer->base;
    size_t newlen = oldlen == 0 ? 256 : oldlen * 2;
    void* newmem = NULL;
    size_t offset = 0;

    BAIL_ON_ERROR(status = info->realloc(buffer->base, oldlen, newlen, &newmem, info->data));

    offset = buffer->cursor - buffer->base;
    buffer->base = newmem;
    buffer->cursor = newmem + offset;
    buffer->end = newmem + newlen;

error:

    return status;
}

LWMsgStatus
lwmsg_marshal_alloc(
    LWMsgContext* context,
    LWMsgTypeSpec* type,
    void* object,
    void** buffer,
    size_t* length
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    AllocInfo info;
    LWMsgBuffer mbuf;

    info.realloc = lwmsg_context_get_realloc(context);
    info.free = lwmsg_context_get_free(context);
    info.data = lwmsg_context_get_memdata(context);

    mbuf.base = NULL;
    mbuf.cursor = NULL;
    mbuf.end = NULL;
    mbuf.wrap = lwmsg_marshal_alloc_wrap;
    mbuf.data = &info;

    BAIL_ON_ERROR(status = lwmsg_marshal(context, type, object, &mbuf));

    *buffer = mbuf.base;
    *length = (mbuf.cursor - mbuf.base);

done:

    return status;

error:

    if (mbuf.base)
    {
        info.free(mbuf.base, info.data);
    }

    *buffer = NULL;
    *length = 0;

    goto done;
}
