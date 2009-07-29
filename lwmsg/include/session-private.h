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
 *        session-private.h
 *
 * Abstract:
 *
 *        Session management API (private header)
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#ifndef __LWMSG_SESSION_PRIVATE_H__
#define __LWMSG_SESSION_PRIVATE_H__

#include <lwmsg/session.h>

/**
 * @file session-private.h
 * @brief Session API (INTERNAL)
 */

/**
 * @internal
 * @defgroup session_private Sessions
 * @ingroup private
 * @brief Session and session manager implementation
 */

/*@{*/

/**
 * @brief Session manager function table structure
 *
 * This structure contains the implementation of a session manager.
 */
typedef struct LWMsgSessionManagerClass
{
    /**
     * @brief Delete session manager
     *
     * Deletes the session manager
     *
     * @param manager the session manager
     */
    void
    (*delete) (
        LWMsgSessionManager* manager
        );

    /**
     * @brief Enter session
     *
     * Called to indicate entry into a new or existing
     * session with a peer.
     *
     * @param[in,out] manager the session manager
     * @param[in] rsmid the ID of the peer's session manager
     * @param[in] rtoken a security token representing the identity of the peer
     * @param[in] construct an optional constructor function to initialize custom session data
     * @param[in] destruct an optional destructor function to destroy custom session data
     * @param[in] construct_data a data pointer to pass to the construct function
     * @param[out] session the session handle for the session that was entered
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_memory
     * @lwmsg_code{SECURITY, the peer does not have permission to enter the given session}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*enter_session) (
        LWMsgSessionManager* manager,
        const LWMsgSessionID* rsmid,
        LWMsgSecurityToken* rtoken,
        LWMsgSessionConstructor construct,
        LWMsgSessionDestructor destruct,
        void* construct_data,
        LWMsgSession** session
        );

    /**
     * @brief Leave session
     *
     * Called to indicate that an existing session has been left.
     * When a session has been left as many times as it has been
     * entered, it is destroyed.
     *
     * @param manager the session manager
     * @param session the session handle
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*leave_session) (
        LWMsgSessionManager* manager,
        LWMsgSession* session
        );

    /**
     * @brief Register local handle
     *
     * Registers a local handle on the given session.
     *
     * @param[in] manager the session manager
     * @param[in,out] handle the session handle
     * @param[in] type the name of the handle type
     * @param[in] ptr the handle pointer
     * @param[in] cleanup an optional cleanup function for the handle
     * @param[out] hid a unique integer which identifies the handle within the session
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_memory
     * @lwmsg_code{INVALID_HANDLE, the handle is already registered}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*register_handle_local) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        const char* type,
        void* ptr,
        void (*cleanup)(void* ptr),
        LWMsgHandleID* hid
        );

    /**
     * @brief Register remote handle
     *
     * Registers a remote handle on the given session.
     *
     * @param[in] manager the session manager
     * @param[in,out] handle the session handle
     * @param[in] type the name of the handle type
     * @param[in] hid the internal ID of the handle
     * @param[in] cleanup an optional cleanup function for the handle
     * @param[out] ptr a pointer to a unique proxy object which represents the handle locally
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_memory
     * @lwmsg_code{INVALID_HANDLE, the handle is already registered}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*register_handle_remote) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        const char* type,
        LWMsgHandleID hid,
        void (*cleanup)(void* ptr),
        void** ptr
        );

    /**
     * @brief Remap handle pointer
     *
     * Replaces one handle with another in the session.  The internal handle ID
     * will remain the same.
     *
     * @param[in] manager the session manager
     * @param[in,out] handle the session handle
     * @param[in] ptr the old handle pointer
     * @param[in] newptr the new handle pointer
     * @param[in] cleanup an optional new cleanup function for the handle
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{INVALID_HANDLE, the pointer is not registered or the new one already is}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*remap_handle) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        void* ptr,
        void* newptr,
        void (*cleanup)(void* ptr)
        );

    /**
     * @brief Retain handle
     *
     * Retains a reference to the given handle, increasing its reference count by
     * one.  Even if a handle is unregistered, it will not be cleaned up until
     * all references are released.
     *
     * @param[in] manager the session manager
     * @param[in,out] session
     * @param[in] ptr the handle pointer
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{INVALID_HANDLE, the pointer is not registered as a handle}
     */
    LWMsgStatus
    (*retain_handle) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        void* ptr
        );

    /**
     * @brief Release handle
     *
     * Releases a reference to the given handle, decreasing its reference count by
     * one.  When the handle reaches zero references, the cleanup function passed
     * the register function will be invoked.
     *
     * @param[in] manager the session manager
     * @param[in,out] session
     * @param[in] ptr the handle pointer
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{INVALID_HANDLE, the pointer is not registered as a handle}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*release_handle) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        void* ptr
        );

    /**
     * @brief Unregister handle
     *
     * Releases a reference to the given handle, decreasing its reference count by
     * one, and marks the handle as stale.  Any further attempts to use the handle's
     * internal ID will fail, but release and retain operations will continue to
     * succeed until the reference count reaches 0.
     *
     * @param[in] manager the session manager
     * @param[in,out] session
     * @param[in] ptr the handle pointer
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{INVALID_HANDLE, the pointer is not registered as a handle}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*unregister_handle) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        void* ptr
        );

    /**
     * @brief Map pointer to internal handle ID
     *
     * Maps a handle pointer to the internal ID within the session.
     *
     * @param[in] manager the session manager
     * @param[in,out] session
     * @param[in] ptr the handle pointer
     * @param[out] type the name of the handle type in the type specification
     * @param[out] htype the type of the handle -- local or remote
     * @param[out] hid the internal ID of the handle
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{INVALID_HANDLE, the pointer is not registered as a handle}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*handle_pointer_to_id) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        void* ptr,
        const char** type,
        LWMsgHandleType* htype,
        LWMsgHandleID* hid
        );

    /**
     * @brief Map internal handle ID to pointer
     *
     * Maps an internal handle ID to a pointer
     *
     * @param[in] manager the session manager
     * @param[in,out] session the session handle
     * @param[in] type the name of the handle type in the type specification
     * @param[in] htype the type of the handle -- local or remote
     * @param[out] ptr the handle pointer
     * @lwmsg_status
     * @lwmsg_success
     * @lwmsg_code{INVALID_HANDLE, the given ID, type name, and type is not registered as a handle}
     * @lwmsg_endstatus
     */
    LWMsgStatus
    (*handle_id_to_pointer) (
        LWMsgSessionManager* manager,
        LWMsgSession* session,
        const char* type,
        LWMsgHandleType htype,
        LWMsgHandleID hid,
        void** ptr
        );

    /**
     * @brief Get security token for peer
     *
     * Gets a security token which represents the identity of the peer
     * for the given session.
     *
     * @param[in] manager the session manager
     * @param[in] session the session handle
     * @return a security token representing the peer identity, or NULL if unauthenticated
     */
    LWMsgSecurityToken*
    (*get_session_peer_security_token) (
        LWMsgSessionManager* manager,
        LWMsgSession* session
        );

    /**
     * @brief Get custom session data
     *
     * Gets a custom data pointer for the given session.  The data
     * pointer is set by the construtor function passed to the session enter
     * function.
     *
     * @param[in] manager the session manager
     * @param[in] session the session handle
     * @return the session data pointer
     */
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

struct LWMsgSession
{
    LWMsgSessionManager* manager;
};

struct LWMsgSessionManager
{
    LWMsgSessionManagerClass* mclass;
    LWMsgSessionID smid;
};

LWMsgStatus
lwmsg_session_manager_enter_session (
    LWMsgSessionManager* manager,
    const LWMsgSessionID* rsmid,
    LWMsgSecurityToken* rtoken,
    LWMsgSessionConstructor construct,
    LWMsgSessionDestructor destruct,
    void* construct_data,
    LWMsgSession** session
    );

LWMsgStatus 
lwmsg_session_manager_leave_session (
    LWMsgSessionManager* manager,
    LWMsgSession* session
    );

LWMsgStatus
lwmsg_session_manager_register_handle_local (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    const char* type,
    void* ptr,
    void (*cleanup)(void* ptr),
    LWMsgHandleID* hid
    );

LWMsgStatus
lwmsg_session_manager_register_handle_remote (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    const char* type,
    LWMsgHandleID hid,
    void (*cleanup)(void* ptr),
    void** ptr
    );

LWMsgStatus
lwmsg_session_manager_remap_handle (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* ptr,
    void* newptr,
    void (*cleanup) (void* ptr)
    );

LWMsgStatus
lwmsg_session_manager_unregister_handle(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* ptr
    );

LWMsgStatus
lwmsg_session_manager_retain_handle (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* ptr
    );

LWMsgStatus
lwmsg_session_manager_release_handle (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* ptr
    );

LWMsgStatus
lwmsg_session_manager_handle_pointer_to_id (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* ptr,
    const char** type,
    LWMsgHandleType* htype,
    LWMsgHandleID* hid
    );

LWMsgStatus
lwmsg_session_manager_handle_id_to_pointer (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    const char* type,
    LWMsgHandleType htype,
    LWMsgHandleID hid,
    void** ptr
    );

void*
lwmsg_session_manager_get_session_data (
    LWMsgSessionManager* manager,
    LWMsgSession* session
    );

const LWMsgSessionID*
lwmsg_session_manager_get_session_id(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    );

LWMsgStatus
lwmsg_default_session_manager_new(
    LWMsgSessionManager** out_manager
    );

LWMsgStatus
lwmsg_session_manager_init(
    LWMsgSessionManager* manager,
    LWMsgSessionManagerClass* mclass
    );

#define LWMSG_SESSION_MANAGER(obj) ((LWMsgSessionManager*) (obj))
#define LWMSG_SESSION(obj) ((LWMsgSession*) (obj))

/*@}*/

#endif
