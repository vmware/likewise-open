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
 *        status.c
 *
 * Abstract:
 *
 *        Status codes
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "status-private.h"
#include "util-private.h"

#include <string.h>
#include <stdlib.h>

static const char* default_message[] =
{
    [LWMSG_STATUS_SUCCESS] = "Success",
    [LWMSG_STATUS_ERROR] = "Error",
    [LWMSG_STATUS_AGAIN] = "Retry",
    [LWMSG_STATUS_MEMORY] = "Out of memory",
    [LWMSG_STATUS_MALFORMED] = "Malformed type or message",
    [LWMSG_STATUS_EOF] = "End of file",
    [LWMSG_STATUS_NOT_FOUND] = "Item not found",
    [LWMSG_STATUS_UNIMPLEMENTED] = "Unimplemented",
    [LWMSG_STATUS_INVALID_PARAMETER] = "Invalid parameter",
    [LWMSG_STATUS_INVALID_STATE] = "Invalid state",
    [LWMSG_STATUS_OVERFLOW] = "Arithmetic overflow",
    [LWMSG_STATUS_UNDERFLOW] = "Arithmetic underflow",
    [LWMSG_STATUS_SYSTEM] = "Unhandled system error",
    [LWMSG_STATUS_TIMEOUT] = "Operation timed out",
    [LWMSG_STATUS_SECURITY] = "Security violation",
    [LWMSG_STATUS_INTERRUPT] = "Operation interrupted",
    [LWMSG_STATUS_FILE_NOT_FOUND] = "File not found",
    [LWMSG_STATUS_CONNECTION_REFUSED] = "Connection refused",
    [LWMSG_STATUS_PEER_CLOSE] = "Connection closed by peer",
    [LWMSG_STATUS_PEER_RESET] = "Connection reset by peer",
    [LWMSG_STATUS_PEER_ABORT] = "Connection aborted by peer",
    [LWMSG_STATUS_SESSION_LOST] = "Session lost"
};

void
lwmsg_error_clear(
    LWMsgErrorContext* context
    )
{
    if (context->message)
    {
        free(context->message);
    }

    context->status = LWMSG_STATUS_SUCCESS;
}

LWMsgStatus
lwmsg_error_raise_str(
    LWMsgErrorContext* context,
    LWMsgStatus status,
    const char* message
    )
{
    lwmsg_error_clear(context);

    context->status = status;

    if (message != NULL)
    {
        context->message = strdup(message);
        
        if (!context->message)
        {
            /* Out of memory */
            return LWMSG_STATUS_MEMORY;
        }
    }

    return status;
}
                  
LWMsgStatus
lwmsg_error_raise_v(
    LWMsgErrorContext* context,
    LWMsgStatus status,
    const char* fmt,
    va_list ap
    )
{
    char* message = lwmsg_formatv(fmt, ap);
    
    if (!message)
    {
        return lwmsg_error_raise_str(context, LWMSG_STATUS_MEMORY, NULL);
    }
    else
    {
        lwmsg_error_clear(context);
        context->status = status;
        context->message = message;
        return status;
    }
}

LWMsgStatus
lwmsg_error_raise(
    LWMsgErrorContext* context,
    LWMsgStatus status,
    const char* fmt,
    ...
    )
{
    va_list ap;
    char* message;

    va_start(ap, fmt);
    message = lwmsg_formatv(fmt, ap);
    va_end(ap);

    if (!message)
    {
        return lwmsg_error_raise_str(context, LWMSG_STATUS_MEMORY, NULL);
    }
    else
    {
        lwmsg_error_clear(context);
        context->status = status;
        context->message = message;
        return status;
    }
}

const char*
lwmsg_error_message(
    LWMsgStatus status,
    LWMsgErrorContext* context
    )
{
    if (context != NULL && context->message != NULL)
    {
        return context->message;
    }
    else
    {
        return default_message[status];
    }
}

LWMsgStatus
lwmsg_error_propagate(
    LWMsgErrorContext* from_context,
    LWMsgErrorContext* to_context,
    LWMsgStatus status
    )
{
    if (status != LWMSG_STATUS_SUCCESS && from_context->status == status)
    {
        to_context->message = from_context->message;
        from_context->message = NULL;
    }

    to_context->status = status;

    return status;
}
