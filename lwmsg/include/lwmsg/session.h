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
 *        session.h
 *
 * Abstract:
 *
 *        Sesssion management API (public header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_SESSION_H__
#define __LWMSG_SESSION_H__

#include <lwmsg/status.h>
#include <lwmsg/security.h>
#include <lwmsg/common.h>
#include <lwmsg/context.h>

typedef struct LWMsgSessionID
{
    unsigned char bytes[8];
} LWMsgSessionID;

typedef char LWMsgSessionString[33];

typedef struct LWMsgSession LWMsgSession;

/**
 * @brief Callback to clean up a handle
 *
 * A callback used to clean up a handle after it is no longer in use.
 * A cleanup callback can be registered as part of lwmsg_sesion_register_handle().
 *
 * @param[in] handle the handle to clean up
 */
typedef void (*LWMsgHandleCleanupFunction) (void* handle);

typedef LWMsgStatus
(*LWMsgSessionConstructor) (
    LWMsgSecurityToken* token,
    void* data,
    void** session_data
    );

typedef void
(*LWMsgSessionDestructor) (
    LWMsgSecurityToken* token,
    void* session_data
    );

/**
 * @brief Handle locality
 *
 * Specifies the physical location of a handle (local or remote).  Only local
 * handles may safely be dereferenced (have their contents accessed).
 * Remote handles are a proxy and may only be used in messages.
 */
typedef enum LWMsgHandleType
{
    /** The handle is NULL */
    LWMSG_HANDLE_NULL = 0,
    /** The handle originated from the local host and process */
    LWMSG_HANDLE_LOCAL = 1,
    /** The handle originated from a remote host or process */
    LWMSG_HANDLE_REMOTE = 2
} LWMsgHandleType;

typedef unsigned long LWMsgHandleID;

typedef struct LWMsgSessionManager LWMsgSessionManager;

void
lwmsg_session_manager_delete(
    LWMsgSessionManager* manager
    );

const LWMsgSessionID*
lwmsg_session_manager_get_id(
    LWMsgSessionManager* manager
    );

void
lwmsg_session_id_to_string(
    const LWMsgSessionID* smid,
    LWMsgSessionString buffer
    );

size_t
lwmsg_session_get_assoc_count(
    LWMsgSession* session
    );

size_t
lwmsg_session_get_handle_count(
    LWMsgSession* session
    );

LWMsgStatus
lwmsg_session_register_handle(
    LWMsgSession* session,
    const char* typename,
    void* handle,
    LWMsgHandleCleanupFunction cleanup
    );

LWMsgStatus
lwmsg_session_retain_handle(
    LWMsgSession* session,
    void* handle
    );

LWMsgStatus
lwmsg_session_release_handle(
    LWMsgSession* session,
    void* handle
    );

LWMsgStatus
lwmsg_session_unregister_handle(
    LWMsgSession* session,
    void* handle
    );

LWMsgStatus
lwmsg_session_get_handle_location(
    LWMsgSession* session,
    void* handle,
    LWMsgHandleType* location
    );

void*
lwmsg_session_get_data(
    LWMsgSession* session
    );

LWMsgSecurityToken*
lwmsg_session_get_peer_security_token(
    LWMsgSession* session
    );

const LWMsgSessionID*
lwmsg_session_get_id(
    LWMsgSession* session
    );

#ifndef LWMSG_NO_THREADS
LWMsgStatus
lwmsg_shared_session_manager_new(
    LWMsgSessionManager** out_manager
    );
#endif

#endif
