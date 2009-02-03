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
 *        status.h
 *
 * Abstract:
 *
 *        Status codes (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_STATUS_H__
#define __LWMSG_STATUS_H__

/**
 * @file status.h
 * @brief Status codes and support API
 */

/**
 * @defgroup status Status codes
 * @ingroup public
 * @brief Common status codes used throughout lwmsg
 *
 * lwmsg uses a common set of status codes to indicate errors
 * or exceptional conditions in its functions.
 */

/**
 * @ingroup status
 * @brief A status code
 */
typedef enum
{
    /** Success 
     * @hideinitializer
     */
    LWMSG_STATUS_SUCCESS = 0,
    /** Generic error
     * @hideinitializer
     */
    LWMSG_STATUS_ERROR = 1,
    /** Call is retriable
     * @hideinitializer
     */
    LWMSG_STATUS_AGAIN = 2,
    /** Out of memory
     * @hideinitializer
     */
    LWMSG_STATUS_MEMORY = 3,
    /** Malformed type or message
     * @hideinitializer
     */
    LWMSG_STATUS_MALFORMED = 4,
    /** End of file or stream, connection closed
     * @hideinitializer
     */
    LWMSG_STATUS_EOF = 5,
    /** Requested item not found
     * @hideinitializer
     */
    LWMSG_STATUS_NOT_FOUND = 6,   
    /** Not yet implemented
     * @hideinitializer
     */
    LWMSG_STATUS_UNIMPLEMENTED = 7,
    /** Invalid parameter or invalid state detected
     * @hideinitializer
     */
    LWMSG_STATUS_INVALID_PARAMETER = 8,
    /** Arithmetic overflow
     * @hideinitializer
     */
    LWMSG_STATUS_OVERFLOW = 9,
    /** Arithmetic underflow
     * @hideinitializer
     */
    LWMSG_STATUS_UNDERFLOW = 10,
    /** Unexpected system error
     * @hideinitializer
     */
    LWMSG_STATUS_SYSTEM = 11,
    /** Operation timed out
     * @hideinitializer
     */
    LWMSG_STATUS_TIMEOUT = 12,
    /** Security violation
     * @hideinitializer
     */
    LWMSG_STATUS_SECURITY = 13,
    /** Operation interrupted
     * @hideinitializer
     */
    LWMSG_STATUS_INTERRUPT = 14,
    /**
     * File not found
     * @hideinitializer
     */
    LWMSG_STATUS_FILE_NOT_FOUND = 15,
    /**
     * Remote server not listening
     * @hideinitializer
     */
    LWMSG_STATUS_CONNECTION_REFUSED = 16,
    /**
     * Invalid state detected
     * @hideinitializer
     */
    LWMSG_STATUS_INVALID_STATE = 17,
    /**
     * Peer reset association
     * @hideinitializer
     */
    LWMSG_STATUS_PEER_RESET = 18,
    /**
     * Peer closed association
     * @hideinitializer
     */
    LWMSG_STATUS_PEER_CLOSE = 19,
    /**
     * Peer aborted association
     * @hideinitializer
     */
    LWMSG_STATUS_PEER_ABORT = 20,
    /**
     * Session with peer was lost
     * @hideinitializer
     */
    LWMSG_STATUS_SESSION_LOST = 21,
    /**
     * Unsupported operation
     * @hideinitializer
     */
    LWMSG_STATUS_UNSUPPORTED = 22
} LWMsgStatus;

#define LWMSG_STATUS_COUNT (22)

#endif
