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
 *        data-unmarshal.c
 *
 * Abstract:
 *
 *        Unmarshalling API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "data-private.h"
#include "util-private.h"
#include "type-private.h"
#include "config.h"
#include "convert.h"
#include "context-private.h"

#include <string.h>

static LWMsgStatus
lwmsg_data_unmarshal_internal(
    LWMsgDataHandle* handle,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object);

static LWMsgStatus
lwmsg_data_unmarshal_struct_pointee(
    LWMsgDataHandle* handle,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgTypeIter* inner,
    LWMsgBuffer* buffer,
    unsigned char** out
    );

static LWMsgStatus
lwmsg_object_alloc(
    LWMsgDataHandle* handle,
    size_t size,
    unsigned char** out
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    void* my_out = NULL;
    LWMsgAllocFunction alloc = NULL;
    void* data = NULL;

    lwmsg_context_get_memory_functions(handle->context, &alloc, NULL, NULL, &data);

    BAIL_ON_ERROR(status = alloc(size, &my_out, data));

    *out = my_out;

error:

    return status;
}

static LWMsgStatus
lwmsg_object_realloc(
    LWMsgDataHandle* handle,
    unsigned char* object,
    size_t old_size,
    size_t new_size,
    unsigned char** out
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    void* my_out = NULL;
    LWMsgReallocFunction realloc = NULL;
    void* data = NULL;

    lwmsg_context_get_memory_functions(handle->context, NULL, NULL, &realloc, &data);

    BAIL_ON_ERROR(status = realloc(object, old_size, new_size, &my_out, data));

    *out = my_out;

error:

    return status;
}

static LWMsgStatus
lwmsg_object_free(
    LWMsgDataHandle* handle,
    unsigned char* object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgFreeFunction free = NULL;
    void* data = NULL;

    lwmsg_context_get_memory_functions(handle->context, NULL, &free, NULL, &data);

    free(object, data);

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_integer_immediate(
    LWMsgDataHandle* handle,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char temp[MAX_INTEGER_SIZE];
    size_t in_size;
    size_t out_size;

    in_size = iter->info.kind_integer.width;
    out_size = iter->size;

    BAIL_ON_ERROR(status = lwmsg_buffer_read(buffer, temp, in_size));

    BAIL_ON_ERROR(status = lwmsg_convert_integer(
                      temp,
                      in_size,
                      handle->byte_order,
                      object,
                      out_size,
                      LWMSG_NATIVE_ENDIAN,
                      iter->info.kind_integer.sign));

    /* If a valid range is defined, check value against it */
    if (iter->attrs.range_low < iter->attrs.range_high)
    {
        BAIL_ON_ERROR(status = lwmsg_data_verify_range(
                          &handle->error,
                          iter,
                          object,
                          out_size));
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_custom_immediate(
    LWMsgDataHandle* handle,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    BAIL_ON_ERROR(status = iter->info.kind_custom.typeclass->unmarshal(
                      handle,
                      buffer,
                      iter->size,
                      &iter->attrs,
                      object,
                      iter->info.kind_custom.typedata));

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_struct_member(
    LWMsgDataHandle* handle,
    LWMsgTypeIter* struct_iter,
    LWMsgTypeIter* member_iter,
    LWMsgBuffer* buffer,
    unsigned char* struct_object
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgUnmarshalState my_state;
    unsigned char* member_object = struct_object + member_iter->offset;

    my_state.dominating_object = struct_object;

    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_internal(
                      handle,
                      &my_state,
                      member_iter,
                      buffer,
                      member_object));

error:

    return status;
}

/* Find or unmarshal the length of a pointer or array */
static LWMsgStatus
lwmsg_data_unmarshal_indirect_prologue(
    LWMsgDataHandle* handle,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgTypeIter* inner,
    LWMsgBuffer* buffer,
    size_t* out_count
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char temp[4];

    switch (iter->info.kind_indirect.term)
    {
    case LWMSG_TERM_STATIC:
        /* Static lengths are easy */
        *out_count = iter->info.kind_indirect.term_info.static_length;
        break;
    case LWMSG_TERM_MEMBER:
        /* The length is present in a member we have already unmarshalled */
        BAIL_ON_ERROR(status = lwmsg_data_extract_length(
                          iter,
                          state->dominating_object,
                          out_count));
        break;
    case LWMSG_TERM_ZERO:
        /* The length is present in the data stream as an unsigned 32-bit integer */
        BAIL_ON_ERROR(status = lwmsg_buffer_read(buffer, temp, sizeof(temp)));
        BAIL_ON_ERROR(status = lwmsg_convert_integer(
                          temp,
                          sizeof(temp),
                          handle->byte_order,
                          out_count,
                          sizeof(*out_count),
                          LWMSG_NATIVE_ENDIAN,
                          LWMSG_UNSIGNED));
        break;
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_indirect(
    LWMsgDataHandle* handle,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgTypeIter* inner,
    LWMsgBuffer* buffer,
    unsigned char* object,
    size_t count
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t i;
    unsigned char* element = NULL;

    if (inner->kind == LWMSG_KIND_INTEGER &&
        inner->info.kind_integer.width == 1 &&
        inner->size == 1)
    {
        /* If the element type is an integer and the packed and unpacked sizes are both 1,
           then we can copy directly from the marshalled data into the object.  This
           is an important optimization for unmarshalling character strings */
        BAIL_ON_ERROR(status = lwmsg_buffer_read(buffer, object, count * inner->size));
    }
    else
    {
        /* Otherwise, we need to unmarshal each element individually */
        element = object;

        for (i = 0; i < count; i++)
        {
            BAIL_ON_ERROR(status = lwmsg_data_unmarshal_internal(
                              handle,
                              state,
                              inner,
                              buffer,
                              element));
            element += inner->size;
        }
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_pointees(
    LWMsgDataHandle* handle,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char** out)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t count = 0;
    size_t full_count = 0;
    size_t referent_size = 0;
    unsigned char* object = NULL;
    LWMsgTypeIter inner;

    lwmsg_type_enter(iter, &inner);

    /* Determine element count */
    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_indirect_prologue(
                      handle,
                      state,
                      iter,
                      &inner,
                      buffer,
                      &count));

    /* Handle a single struct pointee as a special case as it could
       hold a flexible array member */
    if (inner.kind == LWMSG_KIND_STRUCT && count == 1)
    {
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_struct_pointee(
                          handle,
                          state,
                          iter,
                          &inner,
                          buffer,
                          out));
    }
    else
    {
        /* Calculate total number of elements */
        if (iter->info.kind_indirect.term == LWMSG_TERM_ZERO)
        {
            /* If the referent is zero-terminated, we need to allocate an extra element */
            BAIL_ON_ERROR(status = lwmsg_add_unsigned(count, 1, &full_count));
        }
        else
        {
            full_count = count;
        }

        /* Calculate the referent size */
        BAIL_ON_ERROR(status = lwmsg_multiply_unsigned(full_count, inner.size, &referent_size));

        /* Allocate the referent */
        BAIL_ON_ERROR(status = lwmsg_object_alloc(handle, referent_size, &object));

        /* Unmarshal elements */
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_indirect(
                          handle,
                          state,
                          iter,
                          &inner,
                          buffer,
                          object,
                          count));

        *out = object;
    }

done:

    return status;

error:

    *out = NULL;

    if (object)
    {
        lwmsg_data_free_graph_internal(handle, iter, (unsigned char*) &object);
    }

    goto done;
}

static LWMsgStatus
lwmsg_data_unmarshal_pointer(
    LWMsgDataHandle* handle,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char** out)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char ptr_flag;

    if (iter->attrs.nonnull)
    {
        /* If pointer is never null, there is no flag in the stream */
        ptr_flag = 0xFF;
    }
    else
    {
        /* A flag in the stream indicates whether the pointer is NULL or not. */
        BAIL_ON_ERROR(status = lwmsg_buffer_read(buffer, &ptr_flag, sizeof(ptr_flag)));
    }

    if (ptr_flag)
    {
        /* If the pointer is non-null, unmarshal the pointees */
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_pointees(
                          handle,
                          state,
                          iter,
                          buffer,
                          (unsigned char**) out));
    }

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_array(
    LWMsgDataHandle* handle,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t count = 0;
    LWMsgTypeIter inner;

    lwmsg_type_enter(iter, &inner);

    /* Determine element size and count */
    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_indirect_prologue(
                      handle,
                      state,
                      iter,
                      &inner,
                      buffer,
                      &count));

    /* Unmarshal elements */
    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_indirect(
                      handle,
                      state,
                      iter,
                      &inner,
                      buffer,
                      object,
                      count));

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_struct(
    LWMsgDataHandle* handle,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object,
    LWMsgTypeSpec** flexible_member
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter member;

    if (flexible_member)
    {
        *flexible_member = NULL;
    }

    iter->dom_object = object;

    for (lwmsg_type_enter(iter, &member);
         lwmsg_type_valid(&member);
         lwmsg_type_next(&member))
    {
        if (member.kind == LWMSG_KIND_ARRAY &&
            member.info.kind_indirect.term != LWMSG_TERM_STATIC)
        {
            if (flexible_member)
            {
                *flexible_member = member.spec;
            }
            continue;
        }

        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_struct_member(
                          handle,
                          iter,
                          &member,
                          buffer,
                          object));
    }

error:

    return status;
}

/* Free a structure that is missing its flexible array member */
static LWMsgStatus
lwmsg_data_unmarshal_free_partial_struct(
    LWMsgDataHandle* handle,
    LWMsgTypeIter* iter,
    unsigned char* object)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter member;

    for (lwmsg_type_enter(iter, &member);
         lwmsg_type_valid(&member);
         lwmsg_type_next(&member))
    {
        if (member.kind == LWMSG_KIND_ARRAY &&
            member.info.kind_indirect.term != LWMSG_TERM_STATIC)
        {
            break;
        }

        BAIL_ON_ERROR(status = lwmsg_data_free_graph_internal(
                          handle,
                          &member,
                          object + member.offset));
    }

    lwmsg_object_free(handle, object);

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_struct_pointee(
    LWMsgDataHandle* handle,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* pointer_iter,
    LWMsgTypeIter* struct_iter,
    LWMsgBuffer* buffer,
    unsigned char** out
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char* base_object = NULL;
    unsigned char* full_object = NULL;
    LWMsgTypeSpec* flexible_member = NULL;
    size_t full_size = 0;

    /* Allocate enough memory to hold the base of the object */
    BAIL_ON_ERROR(status = lwmsg_object_alloc(
                      handle,
                      struct_iter->size,
                      &base_object));

    /* Unmarshal all base members of the structure and find any flexible member */
    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_struct(
                      handle,
                      struct_iter,
                      buffer,
                      base_object,
                      &flexible_member
                      ));

    /* Now that the base of the object is unmarshalled, we can see if we need to
       reallocate space for a flexible member */
    if (flexible_member)
    {
        LWMsgTypeIter flex_iter;
        LWMsgTypeIter inner_iter;
        LWMsgUnmarshalState my_state;
        size_t count = 0;
        size_t full_count = 0;
        size_t flexible_size = 0;

        lwmsg_type_iterate(flexible_member, &flex_iter);
        lwmsg_type_enter(&flex_iter, &inner_iter);

        my_state.dominating_object = base_object;

        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_indirect_prologue(
                          handle,
                          &my_state,
                          &flex_iter,
                          &inner_iter,
                          buffer,
                          &count));

        /* Calculate total number of elements */
        if (flex_iter.info.kind_indirect.term == LWMSG_TERM_ZERO)
        {
            /* If the referent is zero-terminated, we need to allocate an extra element */
            BAIL_ON_ERROR(status = lwmsg_add_unsigned(count, 1, &full_count));
        }
        else
        {
            full_count = count;
        }

        /* Calculate the size of the flexible member */
        BAIL_ON_ERROR(status = lwmsg_multiply_unsigned(full_count, inner_iter.size, &flexible_size));

        /* Calculate the size of the full structure */
        BAIL_ON_ERROR(status = lwmsg_add_unsigned(struct_iter->size, flexible_size, &full_size));

        /* Allocate the full object */
        BAIL_ON_ERROR(status = lwmsg_object_realloc(
                          handle,
                          base_object,
                          struct_iter->size,
                          full_size,
                          &full_object));

        base_object = NULL;

        /* Unmarshal the flexible array member */
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_indirect(
                          handle,
                          &my_state,
                          &flex_iter,
                          &inner_iter,
                          buffer,
                          full_object + flex_iter.offset,
                          count));
    }
    else
    {
        full_object = base_object;
        base_object = NULL;
    }

    *out = full_object;

done:

    return status;

error:

    if (base_object)
    {
        /* We must avoid visiting flexible array members when
           freeing the base object because it does not have
           space for them in the allocated block */
        lwmsg_data_unmarshal_free_partial_struct(
            handle,
            struct_iter,
            base_object);
    }

    if (full_object)
    {
        lwmsg_data_free_graph_internal(handle, pointer_iter, (unsigned char*) &base_object);
    }

    goto done;
}

static LWMsgStatus
lwmsg_data_unmarshal_union(
    LWMsgDataHandle* handle,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter arm;

    /* Find the active arm */
    BAIL_ON_ERROR(status = lwmsg_data_extract_active_arm(
                      iter,
                      state->dominating_object,
                      &arm));

    /* Simply unmarshal the object as the particular arm */
    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_internal(
                      handle,
                      state,
                      &arm,
                      buffer,
                      object));

error:

    return status;
}

static LWMsgStatus
lwmsg_data_unmarshal_internal(
    LWMsgDataHandle* handle,
    LWMsgUnmarshalState* state,
    LWMsgTypeIter* iter,
    LWMsgBuffer* buffer,
    unsigned char* object)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    switch (iter->kind)
    {
    case LWMSG_KIND_VOID:
        /* Nothing to unmarshal */
        break;
    case LWMSG_KIND_INTEGER:
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_integer_immediate(
                          handle,
                          state,
                          iter,
                          buffer,
                          object));
        break;
   case LWMSG_KIND_CUSTOM:
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_custom_immediate(
                          handle,
                          state,
                          iter,
                          buffer,
                          object));
        break;
    case LWMSG_KIND_STRUCT:
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_struct(
                          handle,
                          iter,
                          buffer,
                          object,
                          NULL));
        break;
    case LWMSG_KIND_UNION:
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_union(
                          handle,
                          state,
                          iter,
                          buffer,
                          object));
        break;
    case LWMSG_KIND_POINTER:
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_pointer(
                          handle,
                          state,
                          iter,
                          buffer,
                          (unsigned char **) object));
        break;
    case LWMSG_KIND_ARRAY:
        BAIL_ON_ERROR(status = lwmsg_data_unmarshal_array(
                          handle,
                          state,
                          iter,
                          buffer,
                          object));
        break;
    default:
        BAIL_ON_ERROR(status = LWMSG_STATUS_UNIMPLEMENTED);
    }

    if (iter->verify)
    {
        BAIL_ON_ERROR(status = iter->verify(handle, LWMSG_TRUE, iter->size, object, iter->verify_data));
    }

error:

    return status;

}

LWMsgStatus
lwmsg_data_unmarshal(LWMsgDataHandle* handle, LWMsgTypeSpec* type, LWMsgBuffer* buffer, void** out)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgUnmarshalState my_state = {NULL};
    LWMsgTypeIter iter;

    lwmsg_type_iterate_promoted(type, &iter);

    BAIL_ON_ERROR(status = lwmsg_data_unmarshal_internal(handle, &my_state, &iter, buffer, (unsigned char*) out));

    if (buffer->wrap)
    {
        BAIL_ON_ERROR(status = buffer->wrap(buffer, 0));
    }

error:

    return status;
}

LWMsgStatus
lwmsg_data_unmarshal_flat(LWMsgDataHandle* handle, LWMsgTypeSpec* type, void* buffer, size_t length, void** out)
{
    LWMsgBuffer mbuf;

    mbuf.base = buffer;
    mbuf.cursor = buffer;
    mbuf.end = buffer + length;
    mbuf.wrap = NULL;

    return lwmsg_data_unmarshal(handle, type, &mbuf, out);
}
