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
 *        type-private.h
 *
 * Abstract:
 *
 *        Type specification API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __TYPE_PRIVATE_H__
#define __TYPE_PRIVATE_H__

#include <lwmsg/type.h>
#include <lwmsg/protocol.h>
#include <lwmsg/status.h>
#include <lwmsg/common.h>

/* Iteration */
typedef struct LWMsgTypeIter
{
    LWMsgTypeSpec* spec;
    LWMsgKind kind;
    size_t offset;
    size_t size;
    intmax_t tag;

    LWMsgVerifyFunction verify;
    void* verify_data;

    union
    {
        struct
        {
            size_t width;
            LWMsgSignage sign;
            size_t* range;
        } kind_integer;
        struct
        {
            struct
            {
                size_t offset;
                size_t size;
            } discrim;
        } kind_compound;
        struct
        {
            LWMsgArrayTermination term;
            union
            {
                struct
                {
                    size_t offset;
                    size_t size;
                } member;
                size_t static_length;
            } term_info;
            unsigned nonnull:1;
            unsigned aliasable:1;
        } kind_indirect;
        struct
        {
            LWMsgCustomTypeClass* typeclass;
            void* typedata;
        } kind_custom;
    } info;

    LWMsgTypeSpec* inner;
    LWMsgTypeSpec* next;
    unsigned char* dom_object;

    struct
    {
        const char* type_name;
        const char* member_name;
    } meta;

    struct
    {
        const char* file;
        unsigned int line;
    } debug;
} LWMsgTypeIter;

typedef LWMsgStatus (*LWMsgGraphVisitFunction) (
    LWMsgTypeIter* iter,
    unsigned char* object,
    void* data
    );

void
lwmsg_type_iterate(
    LWMsgTypeSpec* spec,
    LWMsgTypeIter* iter
    );

static inline
LWMsgBool
lwmsg_type_valid(
    LWMsgTypeIter* iter
    )
{
    return iter->kind != LWMSG_KIND_NONE;
}

static inline
void
lwmsg_type_next(
    LWMsgTypeIter* iter
    )
{
    if (iter->next)
    {
        lwmsg_type_iterate(iter->next, iter);
    }
    else
    {
        iter->kind = LWMSG_KIND_NONE;
    }
}

static inline
void
lwmsg_type_enter(
    LWMsgTypeIter* iter,
    LWMsgTypeIter* new_iter
    )
{
    if (iter->inner)
    {
        lwmsg_type_iterate(iter->inner, new_iter);

        new_iter->dom_object = iter->dom_object;
    }
    else
    {
        new_iter->kind = LWMSG_KIND_NONE;
    }
}

void
lwmsg_type_iterate_promoted(
    LWMsgTypeSpec* spec,
    LWMsgTypeIter* iter
    );

LWMsgStatus
lwmsg_type_extract_discrim_tag(
    LWMsgTypeIter* iter,
    unsigned char* dominating_struct,
    intmax_t* tag
    );

LWMsgStatus
lwmsg_type_extract_length(
    LWMsgTypeIter* iter,
    unsigned char* dominating_struct,
    size_t *length
    );

LWMsgStatus
lwmsg_type_extract_active_arm(
    LWMsgTypeIter* iter,
    unsigned char* dominating_struct,
    LWMsgTypeIter* active_iter
    );

LWMsgStatus
lwmsg_type_visit_graph(
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgGraphVisitFunction func,
    void* data
    );

LWMsgStatus
lwmsg_type_visit_graph_children(
    LWMsgTypeIter* iter,
    unsigned char* object,
    LWMsgGraphVisitFunction func,
    void* data
    );

LWMsgStatus
lwmsg_type_verify_range(
    LWMsgContext* context,
    LWMsgBool unmarshalling,
    size_t object_size,
    void* object,
    void* data);

LWMsgStatus
lwmsg_type_verify_not_null(
    LWMsgContext* context,
    LWMsgBool unmarshalling,
    size_t object_size,
    void* object,
    void* data);

#endif
