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

typedef void
(*LWMsgSessionDataCleanupFunction) (
    void* data
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
        LWMsgSession** session,
        size_t* assoc_count
        );

    LWMsgStatus 
    (*leave_session) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        size_t* assoc_count
        );

    LWMsgStatus
    (*register_handle_local) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        const char* type,
        void* ptr,
        void (*cleanup)(void* ptr),
        LWMsgHandleID* hid
        );

    LWMsgStatus
    (*register_handle_remote) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        const char* type,
        LWMsgHandleID hid,
        void (*cleanup)(void* ptr),
        void** ptr
        );

    LWMsgStatus
    (*remap_handle) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        void* ptr,
        void* newptr,
        void (*cleanup)(void* ptr)
        );

    LWMsgStatus
    (*retain_handle) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        void* ptr
        );

    LWMsgStatus
    (*release_handle) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        void* ptr
        );

    LWMsgStatus
    (*handle_pointer_to_id) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        void* ptr,
        const char** type,
        LWMsgHandleType* out_htype,
        LWMsgHandleID* out_hid
        );

    LWMsgStatus
    (*handle_id_to_pointer) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        const char* type,
        LWMsgHandleType htype,
        LWMsgHandleID hid,
        void** out_ptr
        );

    LWMsgStatus
    (*set_session_data) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        void* data,
        LWMsgSessionDataCleanupFunction cleanup
        );

    void*
    (*get_session_data) (
        LWMsgSessionManager* manager,
        LWMsgSession* session
        );

    const LWMsgSessionID*
    (*get_session_id) (
        LWMsgSessionManager* manager,
        LWMsgSession* session
        );

    size_t
    (*get_session_assoc_count) (
        LWMsgSessionManager* manager,
        LWMsgSession* session
        );

    size_t
    (*get_session_handle_count) (
        LWMsgSessionManager* manager,
        LWMsgSession* session
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

size_t
lwmsg_session_manager_get_session_assoc_count(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    );

size_t
lwmsg_session_manager_get_session_handle_count(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    );

#endif
