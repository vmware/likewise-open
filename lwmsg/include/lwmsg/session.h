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

typedef struct LWMsgSession LWMsgSession;

/**
 * @brief Handle locality
 *
 * Specifies the physical location of a handle (local or remote).  Only local
 * handles may safely be dereferenced (have their contents accessed).
 * Remote handles are a proxy and may only be used in messages.
 */
typedef enum LWMsgHandleLocation
{
    /** The handle originated from the local host and process and may be dereferenced */
    LWMSG_HANDLE_LOCAL = 0,
    /** The handle originated from a remote host or process and may not be dereferenced */
    LWMSG_HANDLE_REMOTE = 1
} LWMsgHandleLocation;

typedef struct LWMsgSessionManager LWMsgSessionManager;

typedef struct LWMsgSessionManagerClass
{
    size_t private_size;

    LWMsgStatus
    (*construct) (
        LWMsgSessionManager* manager
        );
    void
    (*destruct) (
        LWMsgSessionManager* manager
        );

    LWMsgStatus
    (*enter_session) (
        LWMsgSessionManager* manager,
        const LWMsgSessionID* rsmid,
        LWMsgSecurityToken* rtoken,
        LWMsgSession** session
        );

    LWMsgStatus 
    (*leave_session) (
        LWMsgSessionManager* manager,
        LWMsgSession* session
        );

    LWMsgStatus
    (*register_handle) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        const char* type,
        void* ptr,
        void (*cleanup)(void* ptr)
        );

    LWMsgStatus
    (*unregister_handle) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        void* ptr,
        LWMsgBool do_cleanup
        );

    LWMsgStatus
    (*handle_pointer_to_id) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        const char* type,
        void* ptr,
        LWMsgBool autoreg,
        LWMsgHandleLocation* out_location,
        unsigned long* out_hid
        );

    LWMsgStatus
    (*handle_id_to_pointer) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        const char* type,
        LWMsgHandleLocation location,
        unsigned long hid,
        LWMsgBool autoreg,
        void** out_ptr
        );
} LWMsgSessionManagerClass;

LWMsgStatus
lwmsg_session_manager_new(
    LWMsgSessionManagerClass* mclass,
    LWMsgSessionManager** out_manager
    );

void
lwmsg_session_manager_delete(
    LWMsgSessionManager* manager
    );

void*
lwmsg_session_manager_get_private(
    LWMsgSessionManager* manager
    );

const LWMsgSessionID*
lwmsg_session_manager_get_id(
    LWMsgSessionManager* manager
    );

void
lwmsg_session_id_to_string(
    const LWMsgSessionID* smid,
    char buffer[17]
    );

#endif
