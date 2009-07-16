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
 *        message.h
 *
 * Abstract:
 *
 *        Message structure and functions
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_MESSAGE_H__
#define __LWMSG_MESSAGE_H__

#include <lwmsg/status.h>

/**
 * @file message.h
 * @brief Message structure and functions
 */

/**
 * @brief Message tag
 *
 * Identifies the type of data contained within a message
 * in the context of a particular protocol.  Values less
 * than 0 are reserved for internal use by LWMsg.
 */
typedef signed int LWMsgTag;

/**
 * @brief Message cookie
 *
 * Allows "request" and "response" messages to be
 * correlated when multiple requests might be outstanding
 * simultaneously.
 */
typedef unsigned int LWMsgCookie;

/**
 * @brief Invalid message tag
 *
 * A message with this tag is invalid and contains
 * no data.
 */
#define LWMSG_TAG_INVALID ((LWMsgTag) -1)

/**
 * @brief Control message tag
 *
 * A message with this tag never contains data but
 * is still considered valid.  It may be used for
 * for sending control messages.
 */
#define LWMSG_TAG_CONTROL ((LWMsgTag) -2)

/**
 * @brief Message structure
 *
 * Encapsulates all the elements of a message
 * in a single structure.
 */
typedef struct LWMsgMessage
{
    /** @brief A status code, usable for any purpose */
    LWMsgStatus status;
    /** @brief A call ID number for correlating request-response message pairs */
    LWMsgCookie cookie;
    /** @brief The message tag, indicating the type and payload of the message */
    LWMsgTag tag;
#ifdef LWMSG_DISABLE_DEPRECATED
    void* data;
#else
#ifndef DOXYGEN
    union
    {
        void* object;
#endif
        /**
         * @brief The message data payload.
         *
         * The type of the payload is determined by the tag and
         * the protocol used to send or receive it
         */
        void* data;
#ifndef DOXYGEN
    };
#endif
#endif
#ifndef DOXYGEN
    unsigned long reserved1;
#endif
} LWMsgMessage;


/**
 * @brief Message static initializer
 *
 * An #LWMsgMessage structure may be statically initialized
 * with this value in lieu of explicit initialization of the
 * various fields.
 */
#ifdef LWMSG_DISABLE_DEPRECATED
#define LWMSG_MESSAGE_INITIALIZER \
    {LWMSG_STATUS_SUCCESS, 0, LWMSG_TAG_INVALID, NULL, 0}
#else
#define LWMSG_MESSAGE_INITIALIZER \
    {LWMSG_STATUS_SUCCESS, 0, LWMSG_TAG_INVALID, {.data = NULL}, 0}
#endif

void
lwmsg_message_init(
    LWMsgMessage* message
    );

#endif /* __LWMSG_MESSAGE_H__ */
