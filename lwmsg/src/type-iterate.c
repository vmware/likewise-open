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
 *        type-iterate.c
 *
 * Abstract:
 *
 *        Type specification API
 *        Type iteration and object graph visiting
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "type-private.h"
#include "util-private.h"

static inline
void
lwmsg_type_find_end(
    LWMsgTypeSpec** in_out_spec
    )
{
    unsigned int depth = 1;
    LWMsgTypeSpec* spec = *in_out_spec;
    LWMsgTypeDirective cmd;
    LWMsgBool is_meta;
    LWMsgBool is_member;
    LWMsgBool is_debug;
    LWMsgArrayTermination term;

    for (;;)
    {
        cmd = *(spec++);

        is_meta = cmd & LWMSG_FLAG_META;
        is_debug = cmd & LWMSG_FLAG_DEBUG;
        is_member = cmd & LWMSG_FLAG_MEMBER;

        spec += is_meta ? (is_member ? 2 : 1) : 0;
        spec += is_debug ? 2 : 0;
        
        switch (cmd & LWMSG_CMD_MASK)
        {
        case LWMSG_CMD_END:
            if (--depth == 0)
            {
                goto done;
            }
            break;
        case LWMSG_CMD_INTEGER:
            spec += is_member ? 4 : 3;
            break;
        case LWMSG_CMD_STRUCT:
        case LWMSG_CMD_UNION:
            spec += is_member ? 2 : 1;
            depth++;
            break;
        case LWMSG_CMD_POINTER:
        case LWMSG_CMD_ARRAY:
            spec += is_member ? 1 : 0;
            depth++;
            break;
        case LWMSG_CMD_TYPESPEC:
            spec += is_member ? 3 : 1;
            break;
        case LWMSG_CMD_TERMINATION:
            term = *(spec++);
            switch (term)
            {
            case LWMSG_TERM_STATIC:
                spec += 1;
                break;
            case LWMSG_TERM_MEMBER:
                spec += 2;
                break;
            case LWMSG_TERM_ZERO:
                break;
            }
            break;
        case LWMSG_CMD_TAG:
            spec += 1;
            break;
        case LWMSG_CMD_DISCRIM:
        case LWMSG_CMD_VERIFY:
        case LWMSG_CMD_RANGE:
            spec += 2;
            break;
        case LWMSG_CMD_CUSTOM:
            spec += is_member ? 4 : 2;
            break;
        case LWMSG_CMD_CUSTOM_ATTR:
            spec += 1;
            break;
        case LWMSG_CMD_NOT_NULL:
            break;
        default:
            abort();
        }
    }

done:

    *in_out_spec = spec;
}

void
lwmsg_type_iterate(
    LWMsgTypeSpec* spec,
    LWMsgTypeIter* iter
    )
{
    LWMsgTypeDirective cmd;
    LWMsgArrayTermination term;
    size_t my_size;
    size_t my_offset;

    iter->spec = spec;
    iter->verify = NULL;
    iter->size = 0;
    iter->offset = 0;

    memset(&iter->attrs, 0, sizeof(iter->attrs));

    cmd = *(spec++);

    if (cmd & LWMSG_FLAG_META)
    {      
        iter->meta.type_name = (const char*) *(spec++);
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            iter->meta.member_name = (const char*) *(spec++);
        }
    }

    if (cmd & LWMSG_FLAG_DEBUG)
    {
        iter->debug.file = (const char*) *(spec++);
        iter->debug.line = (unsigned int) *(spec++);
    }

    switch (cmd & LWMSG_CMD_MASK)
    {
    case LWMSG_CMD_INTEGER:
        iter->size = *(spec++);
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            iter->offset = *(spec++);
        }

        iter->kind = LWMSG_KIND_INTEGER;
        iter->info.kind_integer.width = *(spec++);
        iter->info.kind_integer.sign = *(spec++);
        break;
    case LWMSG_CMD_STRUCT:
        iter->size = *(spec++);
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            iter->offset = *(spec++);
        }
        iter->kind = LWMSG_KIND_STRUCT;
        iter->inner = spec;
        lwmsg_type_find_end(&spec);
        break;
    case LWMSG_CMD_UNION:
        iter->size = *(spec++);
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            iter->offset = *(spec++);
        }
        iter->kind = LWMSG_KIND_UNION;
        iter->inner = spec;
        lwmsg_type_find_end(&spec);
        break;
    case LWMSG_CMD_POINTER:
        iter->size = sizeof(void*);
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            iter->offset = *(spec++);
        }
        iter->kind = LWMSG_KIND_POINTER;
        iter->inner = spec;
        /* Default termination settings for a pointer */
        iter->info.kind_indirect.term = LWMSG_TERM_STATIC;
        iter->info.kind_indirect.term_info.static_length = 1;
        lwmsg_type_find_end(&spec);
        break;
    case LWMSG_CMD_ARRAY:
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            iter->offset = *(spec++);
        }
        iter->kind = LWMSG_KIND_ARRAY;
        iter->inner = spec;
        /* Default termination settings for an array */
        iter->info.kind_indirect.term = LWMSG_TERM_STATIC;
        iter->info.kind_indirect.term_info.static_length = 1;
        lwmsg_type_find_end(&spec);
        break;
    case LWMSG_CMD_TYPESPEC:
        my_size = 0;
        my_offset = 0;
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            my_size = *(spec++);
            my_offset = *(spec++);
        }
        lwmsg_type_iterate((LWMsgTypeSpec*) *(spec++), iter);
        if (!iter->size)
        {
            iter->size = my_size;
        }
        iter->offset = my_offset;
        break;
    case LWMSG_CMD_CUSTOM:
        if (cmd & LWMSG_FLAG_MEMBER)
        {
            iter->size = *(spec++);
            iter->offset = *(spec++);
        }
        iter->kind = LWMSG_KIND_CUSTOM;
        iter->info.kind_custom.typeclass = (LWMsgCustomTypeClass*) *(spec++);
        iter->info.kind_custom.typedata = (void*) *(spec++);
        break;
    case LWMSG_CMD_TERMINATION:
    case LWMSG_CMD_TAG:
    case LWMSG_CMD_DISCRIM:
    case LWMSG_CMD_CUSTOM_ATTR:
        abort();
    case LWMSG_CMD_END:
        iter->kind = LWMSG_KIND_NONE;
        goto done;
    }

    /* Apply all attributes */
    for (;;)
    {
        iter->next = spec;

        cmd = *(spec++);

        if (cmd & LWMSG_FLAG_DEBUG)
        {
            spec += 2;
        }
        
        switch (cmd & LWMSG_CMD_MASK)
        {
        case LWMSG_CMD_TERMINATION:
            term = *(spec++);

            iter->info.kind_indirect.term = term;

            switch (term)
            {
            case LWMSG_TERM_STATIC:
                iter->info.kind_indirect.term_info.static_length = *(spec++);
                break;
            case LWMSG_TERM_MEMBER:
                iter->info.kind_indirect.term_info.member.offset = *(spec++);
                iter->info.kind_indirect.term_info.member.size = *(spec++);
                break;
            case LWMSG_TERM_ZERO:
                break;
            }
            break;
        case LWMSG_CMD_TAG:
            iter->tag = *(spec++);
            break;
        case LWMSG_CMD_DISCRIM:
            iter->info.kind_compound.discrim.offset = *(spec++);
            iter->info.kind_compound.discrim.size = *(spec++);
            break;
        case LWMSG_CMD_VERIFY:
            iter->verify = (LWMsgVerifyFunction) *(spec++);
            iter->verify_data = (void*) *(spec++);
            break;
        case LWMSG_CMD_RANGE:
            iter->verify = lwmsg_type_verify_range;
            iter->verify_data = iter;
            iter->info.kind_integer.range = (size_t*) spec;
            spec += 2;
            break;
        case LWMSG_CMD_NOT_NULL:
            iter->attrs.nonnull = LWMSG_TRUE;
            break;
        case LWMSG_CMD_CUSTOM_ATTR:
            iter->attrs.custom |= (size_t) *(spec++);
            break;
        case LWMSG_CMD_END:
            /* No more members/types left to iterate */
            iter->next = NULL;
            goto done;
        default:
            goto done;
        }
    }

done:
    
    return;
}

void
lwmsg_type_iterate_promoted(
    LWMsgTypeSpec* spec,
    LWMsgTypeIter* iter
    )
{
    switch (*spec & LWMSG_CMD_MASK)
    {
    case LWMSG_CMD_INTEGER:
    case LWMSG_CMD_STRUCT:
        memset(iter, 0, sizeof(*iter));
        iter->kind = LWMSG_KIND_POINTER;
        iter->info.kind_indirect.term = LWMSG_TERM_STATIC;
        iter->info.kind_indirect.term_info.static_length = 1;
        iter->inner = spec;
        iter->verify = lwmsg_type_verify_not_null;
        iter->size = sizeof(void*);
        iter->attrs.nonnull = LWMSG_TRUE;
        break;
    default:
        lwmsg_type_iterate(spec, iter);
        break;
    }
}

LWMsgStatus
lwmsg_type_visit_graph(
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgGraphVisitFunction func,
    void* data
    )
{
    return func(iter, object, data);
}

static inline
LWMsgBool
lwmsg_is_zero(
    unsigned char* object,
    size_t size
    )
{
    size_t i;

    for (i = 0; i < size; i++)
    {
        if (object[i] != 0)
        {
            return LWMSG_FALSE;
        }
    }

    return LWMSG_TRUE;
}

static
LWMsgStatus
lwmsg_type_visit_graph_indirect(
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgGraphVisitFunction func,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t count = 0;
    size_t i;
    void* element;
    LWMsgTypeIter inner;

    lwmsg_type_enter(iter, &inner);

    switch (iter->info.kind_indirect.term)
    {
    case LWMSG_TERM_STATIC:
        count = iter->info.kind_indirect.term_info.static_length;
        break;
    case LWMSG_TERM_MEMBER:
        BAIL_ON_ERROR(status = lwmsg_type_extract_length(
                          iter,
                          iter->dom_object,
                          &count));
        break;
    case LWMSG_TERM_ZERO:
        count = 1;
        for (element = object; !lwmsg_is_zero(element, inner.size); element += inner.size)
        {
            count++;
        }
        break;
    }

    element = object;
    for (i = 0; i < count; i++)
    {
        BAIL_ON_ERROR(status = func(&inner, element, data));
        element += inner.size;
    }

error:

    return status;
}
    

LWMsgStatus
lwmsg_type_visit_graph_children(
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgGraphVisitFunction func,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter inner;

    switch (iter->kind)
    {
    case LWMSG_KIND_STRUCT:
        lwmsg_type_enter(iter, &inner);
        inner.dom_object = object;
        for (; lwmsg_type_valid(&inner); lwmsg_type_next(&inner))
        {
            BAIL_ON_ERROR(status = func(&inner, object + inner.offset, data));
        }
        break;
    case LWMSG_KIND_UNION:
        /* Find the active arm */
        BAIL_ON_ERROR(status = lwmsg_type_extract_active_arm(
                          iter,
                          iter->dom_object,
                          &inner));
        BAIL_ON_ERROR(status = func(&inner, object, data));
        break;
    case LWMSG_KIND_POINTER:
        if (*(void**) object)
        {
            BAIL_ON_ERROR(status = lwmsg_type_visit_graph_indirect(
                              iter,
                              *(unsigned char**) object,
                              func,
                              data));
        }
        break;
    case LWMSG_KIND_ARRAY:
        BAIL_ON_ERROR(status = lwmsg_type_visit_graph_indirect(
                          iter,
                          object,
                          func,
                          data));
        break;
    default:
        break;
    }

error:

    return status;
}
