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
 *        unmarshal.h
 *
 * Abstract:
 *
 *        Unmarshalling API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_UNMARSHAL_H_
#define __LWMSG_UNMARSHAL_H__

/* FIXME: merge this with marshal.h */

#include <lwmsg/buffer.h>
#include <lwmsg/context.h>
#include <lwmsg/type.h>

/**
 * @file unmarshal.h
 * @brief Unmarshalling routines
 */

/**
 * @ingroup marshal
 * @brief Unmarshal a data structure
 *
 * Converts a serialized data structure to its unmarshalled form, allocating memory as necessary
 * to form the object graph.
 *
 * @param context the marshalling context
 * @param type the type specification which describes the type of the data
 * @param buffer the marshalling buffer from which data will be read
 * @param out the resulting unmarshalled object
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{MALFORMED, the provided data did not conform in some way to the provided type information}
 * @lwmsg_code{OVERFLOW, an arithmetic overflow occured while converting an integer to its unmarshalled form}
 * @lwmsg_code{UNDERFLOW, an arithmetic underflow occured while converting an integer to its unmarshalled form}
 * @lwmsg_etc{the provided context and buffer may raise implementation-specific errors}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_unmarshal(LWMsgContext* context, LWMsgTypeSpec* type, LWMsgBuffer* buffer, void** out);

#endif
