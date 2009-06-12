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
 *        data-print.c
 *
 * Abstract:
 *
 *        Data graph printing
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#include "data-private.h"
#include "util-private.h"
#include "convert.h"

#include <stdio.h>

typedef struct
{
    const LWMsgContext* context;
    unsigned int depth;
    LWMsgBool newline;
    LWMsgDataPrintFunction print;
    void* print_data;
} PrintInfo;

static
LWMsgStatus
lwmsg_data_print_graph_visit(
    LWMsgTypeIter* iter,
    unsigned char* object,
    void* data
    );

static
LWMsgStatus
print_wrap(
    const char* text,
    size_t length,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PrintInfo* info = data;
    int i;

    if (info->newline)
    {
        for (i = 0; i < info->depth; i++)
        {
            BAIL_ON_ERROR(status = info->print("  ", 2, info->print_data));
        }
        info->newline = LWMSG_FALSE;
    }

    BAIL_ON_ERROR(status = info->print(text, length, info->print_data));

error:

    return status;
}

static
LWMsgStatus
print(
    PrintInfo* info,
    const char* fmt,
    ...
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    va_list ap;
    char* text = NULL;

    va_start(ap, fmt);
    text = lwmsg_formatv(fmt, ap);
    va_end(ap);

    if (!text)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    print_wrap(text, strlen(text), info);

error:

    if (text)
    {
        free(text);
    }

    return status;
}

static
LWMsgStatus
newline(
    PrintInfo* info
    )
{
    info->print("\n", 1, info->print_data);
    info->newline = LWMSG_TRUE;

    return LWMSG_STATUS_SUCCESS;
}

static
LWMsgStatus
lwmsg_data_print_integer(
    LWMsgTypeIter* iter,
    unsigned char* object,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    uintmax_t value;

    BAIL_ON_ERROR(status =
                  lwmsg_convert_integer(
                      object,
                      iter->size,
                      LWMSG_NATIVE_ENDIAN,
                      &value,
                      sizeof(value),
                      LWMSG_NATIVE_ENDIAN,
                      iter->info.kind_integer.sign));

    if (iter->info.kind_integer.sign == LWMSG_SIGNED)
    {
        BAIL_ON_ERROR(status = print(info, "%li", (intmax_t) value));
    }
    else
    {
        BAIL_ON_ERROR(status = print(info, "%lu", (uintmax_t) value));
    }

error:

    return status;
}

static
LWMsgStatus
lwmsg_data_print_graph_visit_member(
    LWMsgTypeIter* iter,
    unsigned char* object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PrintInfo* info = (PrintInfo*) data;

    if (iter->meta.member_name)
    {
        BAIL_ON_ERROR(status = print(info, ".%s = ", iter->meta.member_name));
    }

    BAIL_ON_ERROR(status =
                  lwmsg_data_print_graph_visit(
                      iter,
                      object,
                      data));

    BAIL_ON_ERROR(status = newline(info));

error:

    return status;
}

static
LWMsgStatus
lwmsg_data_print_string(
    LWMsgTypeIter* iter,
    unsigned char* object,
    PrintInfo* info
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    void* input_string = NULL;
    void* output_string = NULL;
    size_t input_length = 0;
    ssize_t output_length = 0;
    size_t element_size = 0;
    size_t element_count = 0;

    if (iter->kind == LWMSG_KIND_POINTER)
    {
        input_string = *(void**) object;
    }
    else
    {
        input_string = object;
    }

    BAIL_ON_ERROR(status = lwmsg_data_calculate_indirect_metrics(
                      iter,
                      input_string,
                      &element_count,
                      &element_size));

    input_length = element_count * element_size;

    if (!strcmp(iter->info.kind_indirect.encoding, ""))
    {
        BAIL_ON_ERROR(status = info->print(input_string, input_length, info->print_data));
    }
    else
    {
        output_length = lwmsg_convert_string_alloc(
            input_string,
            input_length,
            (void**) (void*) &output_string,
            iter->info.kind_indirect.encoding,
            "");

        if (output_length == -1)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
        }

        BAIL_ON_ERROR(status = info->print(output_string, (size_t) output_length, info->print_data));
    }

error:

    if (output_string)
    {
        free(output_string);
    }

    return status;
}

static
LWMsgStatus
lwmsg_data_print_graph_visit(
    LWMsgTypeIter* iter,
    unsigned char* object,
    void* data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PrintInfo* info = (PrintInfo*) data;
    const char* prefix = NULL;

    switch (iter->kind)
    {
    case LWMSG_KIND_UNION:
    case LWMSG_KIND_STRUCT:
        if (iter->meta.type_name)
        {
            BAIL_ON_ERROR(status = print(info, "<%s>", iter->meta.type_name));
        }
        else
        {
            BAIL_ON_ERROR(status = print(info, iter->kind == LWMSG_KIND_STRUCT ? "<struct>" : "<union>"));
        }
        BAIL_ON_ERROR(status = newline(info));
        BAIL_ON_ERROR(status = print(info, "{"));
        BAIL_ON_ERROR(status = newline(info));
        info->depth++;
        BAIL_ON_ERROR(status = lwmsg_data_visit_graph_children(
                          iter,
                          object,
                          lwmsg_data_print_graph_visit_member,
                          data));
        info->depth--;
        BAIL_ON_ERROR(status = print(info, "}"));
        break;
    case LWMSG_KIND_ARRAY:
    case LWMSG_KIND_POINTER:
        prefix = iter->kind == LWMSG_KIND_ARRAY ? "<array>" : "<pointer> ->";

        /* NULL case */
        if (iter->kind == LWMSG_KIND_POINTER && *(void**) object == NULL)
        {
            BAIL_ON_ERROR(status = print(info, "<null>"));
        }
        /* String case */
        else if (iter->info.kind_indirect.encoding != NULL)
        {
            BAIL_ON_ERROR(status = print(info, "%s \"", prefix));
            BAIL_ON_ERROR(status = lwmsg_data_print_string(iter, object, info));
            BAIL_ON_ERROR(status = print(info, "\"", prefix));
        }
        /* Singleton case */
        else if (iter->info.kind_indirect.term == LWMSG_TERM_STATIC &&
                 iter->info.kind_indirect.term_info.static_length == 1)
        {
            BAIL_ON_ERROR(status = print(info, "%s ", prefix));
            BAIL_ON_ERROR(status = lwmsg_data_visit_graph_children(
                              iter,
                              object,
                              lwmsg_data_print_graph_visit,
                              data));
        }
        /* General case */
        else
        {
            BAIL_ON_ERROR(status = print(info, prefix));
            BAIL_ON_ERROR(status = newline(info));
            BAIL_ON_ERROR(status = print(info, "{"));
            BAIL_ON_ERROR(status = newline(info));
            info->depth++;
            BAIL_ON_ERROR(status = lwmsg_data_visit_graph_children(
                              iter,
                              object,
                              lwmsg_data_print_graph_visit_member,
                              data));
            info->depth--;
            BAIL_ON_ERROR(status = print(info, "}"));
        }
        break;
    case LWMSG_KIND_INTEGER:
        BAIL_ON_ERROR(status = lwmsg_data_print_integer(
                          iter,
                          object,
                          info));
        break;
    case LWMSG_KIND_CUSTOM:
        if (iter->info.kind_custom.typeclass->print)
        {
            BAIL_ON_ERROR(status = iter->info.kind_custom.typeclass->print(
                              info->context,
                              iter->size,
                              object,
                              &iter->attrs,
                              iter->info.kind_custom.typedata,
                              print_wrap,
                              info));
        }
        else
        {
            BAIL_ON_ERROR(status = print(info, "<custom>"));
        }
    case LWMSG_KIND_NONE:
    case LWMSG_KIND_VOID:
        break;
    }

error:

    return status;
}

LWMsgStatus
lwmsg_data_print_graph(
    const LWMsgContext* context,
    LWMsgTypeSpec* type,
    void* object,
    LWMsgDataPrintFunction print,
    void* print_data
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgTypeIter iter;
    PrintInfo info;

    info.newline = LWMSG_TRUE;
    info.depth = 0;
    info.context = context;
    info.print = print;
    info.print_data = print_data;

    lwmsg_type_iterate_promoted(type, &iter);

    BAIL_ON_ERROR(status = lwmsg_data_visit_graph(
                      &iter,
                      (unsigned char*) &object,
                      lwmsg_data_print_graph_visit,
                      &info));

    BAIL_ON_ERROR(status = newline(&info));

error:

    return status;
}
