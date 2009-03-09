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
 *        context.c
 *
 * Abstract:
 *
 *        Marshalling context API
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "context-private.h"
#include "status-private.h"
#include "util-private.h"
#include "type-private.h"
#include <lwmsg/type.h>

#include <stdlib.h>

static LWMsgStatus
lwmsg_context_default_alloc (
    size_t size,
    void** out,
    void* data)
{
    void* object = malloc(size);

    if (!object)
    {
        return LWMSG_STATUS_MEMORY;
    }
    else
    {
        memset(object, 0, size);
        *out = object;
        return LWMSG_STATUS_SUCCESS;
    }
}

static
void
lwmsg_context_default_free (
    void* object,
    void* data
    )
{
    free(object);
}

static LWMsgStatus
lwmsg_context_default_realloc (
    void* object,
    size_t old_size,
    size_t new_size,
    void** new_object,
    void* data)
{
    void* nobj = realloc(object, new_size);

    if (!nobj)
    {
        return LWMSG_STATUS_MEMORY;
    }
    else
    {
        if (new_size > old_size)
        {
            memset(nobj + old_size, 0, new_size - old_size);
        }
        *new_object = nobj;
        return LWMSG_STATUS_SUCCESS;
    }
}

void
lwmsg_context_setup(LWMsgContext* context, LWMsgContext* parent)
{
    context->parent = parent;

    if (!parent)
    {
        lwmsg_context_set_memory_functions(
            context,
            lwmsg_context_default_alloc,
            lwmsg_context_default_free,
            lwmsg_context_default_realloc,
            NULL);
    }
}

LWMsgStatus
lwmsg_context_new(LWMsgContext** out_context)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgContext* context = NULL;

    context = calloc(1, sizeof(*context));

    if (context == NULL)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    lwmsg_context_setup(context, NULL);

    *out_context = context;

error:

    return status;
}

void
lwmsg_context_cleanup(LWMsgContext* context)
{
    lwmsg_error_clear(&context->error);
}

void
lwmsg_context_delete(LWMsgContext* context)
{
    lwmsg_context_cleanup(context);
    free(context);
}

void
lwmsg_context_set_memory_functions(
    LWMsgContext* context,
    LWMsgAllocFunction alloc,
    LWMsgFreeFunction free,
    LWMsgReallocFunction realloc,
    void* data
    )
{
    context->alloc = alloc;
    context->free = free;
    context->realloc = realloc;
    context->memdata = data;
}

const char*
lwmsg_context_get_error_message(LWMsgContext* context, LWMsgStatus status)
{
    return lwmsg_error_message(status, &context->error);
}

LWMsgAllocFunction
lwmsg_context_get_alloc(LWMsgContext* context)
{
    if (context->alloc)
    {
        return context->alloc;
    }
    else if (context->parent)
    {
        return lwmsg_context_get_alloc(context->parent);
    }
    else
    {
        return NULL;
    }
}

LWMsgFreeFunction
lwmsg_context_get_free(LWMsgContext* context)
{
    if (context->free)
    {
        return context->free;
    }
    else if (context->parent)
    {
        return lwmsg_context_get_free(context->parent);
    }
    else
    {
        return NULL;
    }
}

LWMsgReallocFunction
lwmsg_context_get_realloc(LWMsgContext* context)
{
    if (context->realloc)
    {
        return context->realloc;
    }
    else if (context->parent)
    {
        return lwmsg_context_get_realloc(context->parent);
    }
    else
    {
        return NULL;
    }
}

void
lwmsg_context_set_data_function(
    LWMsgContext* context,
    LWMsgContextDataFunction fn,
    void* data
    )
{
    context->datafn = fn;
    context->datafndata = data;
}

void*
lwmsg_context_get_memdata(LWMsgContext* context)
{
    if (context->memdata)
    {
        return context->memdata;
    }
    else if (context->parent)
    {
        return lwmsg_context_get_memdata(context->parent);
    }
    else
    {
        return NULL;
    }
}

typedef struct freeinfo
{
    LWMsgContext* context;
    LWMsgFreeFunction free;
    void* data;
} freeinfo;

static
LWMsgStatus
lwmsg_context_free_graph_visit(
    LWMsgTypeIter* iter,
    unsigned char* object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    freeinfo* info = (freeinfo*) data;

    switch(iter->kind)
    {
    case LWMSG_KIND_CUSTOM:
        if (iter->info.kind_custom.typeclass->free)
        {
            iter->info.kind_custom.typeclass->free(
                info->context,
                iter->size,
                &iter->attrs,
                object,
                iter->info.kind_custom.typedata);
        }
        break;
    case LWMSG_KIND_POINTER:
        BAIL_ON_ERROR(status = lwmsg_type_visit_graph_children(
                          iter,
                          object,
                          lwmsg_context_free_graph_visit,
                          data));
        info->free(*(void **) object, info->data);
        break;
    default:
        BAIL_ON_ERROR(status = lwmsg_type_visit_graph_children(
                              iter,
                              object,
                              lwmsg_context_free_graph_visit,
                              data));
        break;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_context_free_graph_internal(
    LWMsgContext* context,
    LWMsgTypeIter* iter,
    unsigned char* object)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    freeinfo info;

    info.free = lwmsg_context_get_free(context);
    info.data = lwmsg_context_get_memdata(context);
    info.context = context;

    BAIL_ON_ERROR(status = lwmsg_type_visit_graph(
                      iter,
                      object,
                      lwmsg_context_free_graph_visit,
                      &info));

error:

    return status;
}

LWMsgStatus
lwmsg_context_free_graph(
    LWMsgContext* context,
    LWMsgTypeSpec* type,
    void* root)
{
    LWMsgTypeIter iter;

    lwmsg_type_iterate_promoted(type, &iter);

    return lwmsg_context_free_graph_internal(context, &iter, (unsigned char*) &root);
}

LWMsgStatus
lwmsg_context_get_data(
    LWMsgContext* context,
    const char* key,
    void** out_data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    for (; context; context = context->parent)
    {
        if (context->datafn)
        {
            status = context->datafn(key, out_data, context->datafndata);
            if (status == LWMSG_STATUS_NOT_FOUND)
            {
                status = LWMSG_STATUS_SUCCESS;
                continue;
            }
            BAIL_ON_ERROR(status);
            break;
        }
    }

error:

    return status;
}
