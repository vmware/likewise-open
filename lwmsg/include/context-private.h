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
 *        context-private.h
 *
 * Abstract:
 *
 *        Marshalling context API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_CONTEXT_PRIVATE_H__
#define __LWMSG_CONTEXT_PRIVATE_H__

#include <lwmsg/context.h>

#include "type-private.h"
#include "status-private.h"

struct LWMsgContext
{
    LWMsgAllocFunction alloc;
    LWMsgFreeFunction free;
    LWMsgReallocFunction realloc;
    void* memdata;
    LWMsgContextDataFunction datafn;
    void* datafndata;
    LWMsgLogFunction logfn;
    void* logfndata;
    LWMsgErrorContext error;
    
    struct LWMsgContext* parent;
};

void
lwmsg_context_setup(
    LWMsgContext* context,
    LWMsgContext* parent
    );

void
lwmsg_context_cleanup(
    LWMsgContext* context
    );

LWMsgAllocFunction
lwmsg_context_get_alloc(
    LWMsgContext* context
    );

LWMsgFreeFunction
lwmsg_context_get_free(
    LWMsgContext* context
    );

LWMsgReallocFunction
lwmsg_context_get_realloc(
    LWMsgContext* context
    );

void*
lwmsg_context_get_memdata(
    LWMsgContext* context
    );

LWMsgStatus
lwmsg_context_get_data(
    LWMsgContext* context,
    const char* key,
    void** out_data
    );

LWMsgStatus
lwmsg_context_free_graph_internal(
    LWMsgContext* context,
    LWMsgTypeIter* iter,
    unsigned char* object);

void
lwmsg_context_log(
    LWMsgContext* context,
    LWMsgLogLevel level,
    const char* message,
    const char* filename,
    unsigned int line
    );

void
lwmsg_context_log_printf(
    LWMsgContext* context,
    LWMsgLogLevel level,
    const char* filename,
    unsigned int line,
    const char* format,
    ...
    );

#define LWMSG_LOG(context, level, ...) \
    (lwmsg_context_log_printf((context), (level), __FILE__, __LINE__, __VA_ARGS__))

#define LWMSG_LOG_ERROR(context, ...) LWMSG_LOG(context, LWMSG_LOGLEVEL_ERROR, __VA_ARGS__)
#define LWMSG_LOG_WARNING(context, ...) LWMSG_LOG(context, LWMSG_LOGLEVEL_WARNING, __VA_ARGS__)
#define LWMSG_LOG_INFO(context, ...) LWMSG_LOG(context, LWMSG_LOGLEVEL_INFO, __VA_ARGS__)
#define LWMSG_LOG_VERBOSE(context, ...) LWMSG_LOG(context, LWMSG_LOGLEVEL_VERBOSE, __VA_ARGS__)
#define LWMSG_LOG_DEBUG(context, ...) LWMSG_LOG(context, LWMSG_LOGLEVEL_DEBUG, __VA_ARGS__)
#define LWMSG_LOG_TRACE(context, ...) LWMSG_LOG(context, LWMSG_LOGLEVEL_TRACE, __VA_ARGS__)

#endif
