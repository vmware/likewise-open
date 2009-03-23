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
 *        util-private.h
 *
 * Abstract:
 *
 *        Utility functions (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_UTIL_PRIVATE_H__
#define __LWMSG_UTIL_PRIVATE_H__

#include "status-private.h"
#include "context-private.h"
#include <lwmsg/buffer.h>

#include <stdarg.h>
#include <string.h>

#define BAIL_ON_ERROR(_x_)                      \
    do                                          \
    {                                           \
        if ((_x_))                              \
            goto error;                         \
    } while (0)

#define RAISE_ERROR(_context_, _status_, ...)        \
    do                                               \
    {                                                \
        (void) lwmsg_error_raise(                    \
            &(_context_)->error,                     \
            (_status_),                              \
            __VA_ARGS__);                            \
        goto error;                                  \
    } while (0)

#define PROPAGATE_ERROR(_from_, _to_, _status_) \
    do {                                        \
        if (lwmsg_error_propagate(              \
                &(_from_)->error,               \
                &(_to_)->error,                 \
                (_status_)))                    \
        {                                       \
            goto error;                         \
        }                                       \
    } while (0)

char* lwmsg_formatv(const char* fmt, va_list ap);
char* lwmsg_format(const char* fmt, ...);

static inline LWMsgStatus
lwmsg_buffer_read(
    LWMsgBuffer* buffer,
    void* out,
    size_t count
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    unsigned char* out_bytes = (unsigned char*) out;

    while (count)
    {
        if (buffer->cursor + count > buffer->end)
        {
            /* Not enough bytes remain in buffer, so copy what we have and ask for more */
            size_t remaining = buffer->end - buffer->cursor;
            memcpy(out_bytes, buffer->cursor, remaining);
            count -= remaining;
            buffer->cursor += remaining;
            out_bytes += remaining;

            if (buffer->wrap)
            {
                BAIL_ON_ERROR(status = buffer->wrap(buffer, count));
            }
            else
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_EOF);
            }
        }
        else
        {
            memcpy(out_bytes, buffer->cursor, count);
            buffer->cursor += count;
            break;
        }
    }

error:

    return status;
}

static inline LWMsgStatus
lwmsg_buffer_write(LWMsgBuffer* buffer, unsigned char* in_bytes, size_t count)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    size_t remaining;
    size_t writable;

    while (count)
    {
        remaining = buffer->end - buffer->cursor;

        if (count > remaining)
            writable = remaining;
        else
            writable = count;

        memcpy(buffer->cursor, in_bytes, writable);

        in_bytes += writable;
        count -= writable;
        buffer->cursor += writable;

        if (count)
        {
            if (buffer->wrap)
            {
                BAIL_ON_ERROR(buffer->wrap(buffer, count));
            }
            else
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_EOF);
            }
        }
    }

error:

    return status;
}

ssize_t
lwmsg_convert_string_alloc(
    void* input,
    size_t input_len,
    void** output,
    const char* input_type,
    const char* output_type
    );

ssize_t
lwmsg_convert_string_buffer(
    void* input,
    size_t input_len,
    void* output,
    size_t output_len,
    const char* input_type,
    const char* output_type
    );

LWMsgStatus
lwmsg_add_unsigned(
    size_t operand_a,
    size_t operand_b,
    size_t* result
    );

LWMsgStatus
lwmsg_multiply_unsigned(
    size_t operand_a,
    size_t operand_b,
    size_t* result
    );

#endif
