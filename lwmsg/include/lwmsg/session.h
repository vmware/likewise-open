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

/**
 * @file session.h
 * @brief Session API
 */

/**
 * @defgroup session Sessions
 * @ingroup public
 * @brief Session abstraction
 *
 * A session encapsulates all state in an association between two peers.  Associations
 * delegate session tracking to a session manager which controls creation and state
 * management.  A session contains the following:
 *
 * - A user data pointer for storing custom information about the session
 * - A security token which identifies the peer
 * - A table of handles which allows passing and receiving opaque
 *   object references between peers.
 */

/*@{*/

#ifndef DOXYGEN
typedef struct LWMsgSessionID
{
    unsigned char bytes[8];
} LWMsgSessionID;

typedef char LWMsgSessionString[33];
#endif

/**
 * @brief A session
 *
 * An opaque session structure
 */
typedef struct LWMsgSession LWMsgSession;

/**
 * @brief Handle cleanup callback
 *
 * A callback used to clean up a handle after it is no longer in use.
 * A cleanup callback can be registered as part of lwmsg_sesion_register_handle().
 * The cleanup callback will be invoked when the last reference to the
 * handle is dropped, or when the session containing the handle is
 * torn down.
 *
 * @param[in] handle the handle to clean up
 */
typedef void (*LWMsgHandleCleanupFunction) (void* handle);

/**
 * @brief Session constructor callback
 *
 * A callback function which is invoked by the session manager
 * when a session is first created.  It may set a session data pointer
 * which will be attached to the session to track custom per-session
 * information.  It may also perform authentication by inspecting the
 * provided security token.
 *
 * @param[in] token a security token representing the identity of the peer
 * @param[in] data a user data pointer
 * @param[out] session_data a session data pointer which contains custom per-session information
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{SECURITY, the peer is denied access}
 * @lwmsg_endstatus
 */
typedef LWMsgStatus
(*LWMsgSessionConstructor) (
    LWMsgSecurityToken* token,
    void* data,
    void** session_data
    );

/**
 * @brief Session destructor callback
 *
 * A callback function which is invoked by the session manager
 * when a session is no longer in use.  It should clean up the
 * session data created by the constructor function.
 *
 * @param[in] token a security token representing the identity of the peer
 * @param[in,out] session_data the custom session data created by the constructor
 */
typedef void
(*LWMsgSessionDestructor) (
    LWMsgSecurityToken* token,
    void* session_data
    );

/**
 * @brief Handle type
 *
 * Specifies the type of an opaque handle within a session.  Only
 * handles which are local may be safely dereferenced.  Handles
 * marked as remote are proxies for objects created by the peer
 * and have undefined contents.
 */
typedef enum LWMsgHandleType
{
    /** The handle is NULL
     * @hideinitializer
     */
    LWMSG_HANDLE_NULL = 0,
    /**
     * The handle is local
     * @hideinitializer
     */
    LWMSG_HANDLE_LOCAL = 1,
    /**
     * The handle is remote
     * @hideinitializer
     */
    LWMSG_HANDLE_REMOTE = 2
} LWMsgHandleType;

#ifndef DOXYGEN
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
#endif

/**
 * @brief Get association count
 *
 * Returns a count of the number of associations that
 * have entered the given session.
 *
 * @param session the session
 * @return the number of associations currently inside the session
 */
size_t
lwmsg_session_get_assoc_count(
    LWMsgSession* session
    );

/**
 * @brief Get handle count
 *
 * Returns a count of the number of handles contained
 * in the given session.
 *
 * @param session the session
 * @return the number of handles current contained in the session
 */
size_t
lwmsg_session_get_handle_count(
    LWMsgSession* session
    );

/**
 * @brief Register local handle
 *
 * Registers a local handle so that it may be passed to the peer in
 * subsequent calls.
 *
 * @param session the session
 * @param typename a string constant describing the type of the handle.
 * This should be the same as the type name given to the #LWMSG_HANDLE()
 * or #LWMSG_MEMBER_HANDLE() macro in the type specification.
 * @param handle the physical handle object
 * @param cleanup a cleanup function which should be invoked when the
 * handle is no longer used or the session goes away
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_memory
 * @lwmsg_code{INVALID_HANDLE, the handle is already registered}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_session_register_handle(
    LWMsgSession* session,
    const char* typename,
    void* handle,
    LWMsgHandleCleanupFunction cleanup
    );

/**
 * @brief Increase handle reference count
 *
 * Takes an extra reference to the given handle.
 * When the last reference to a handle is released, the handle
 * will be cleaned up using the function passed to #lwmsg_session_register_handle().
 *
 * @param session the session
 * @param handle the handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_HANDLE, the handle was not previously registered with the session}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_session_retain_handle(
    LWMsgSession* session,
    void* handle
    );

/**
 * @brief Decrease handle reference count
 *
 * Releases a reference to the given handle.
 * When the last reference to a handle is released, the handle
 * will be cleaned up using the function passed to #lwmsg_session_register_handle().
 *
 * @param session the session
 * @param handle the handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_HANDLE, the handle was not previously registered with the session}
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_session_release_handle(
    LWMsgSession* session,
    void* handle
    );

/**
 * @brief Unregister handle
 *
 * Unregisters a handle.  In addition to releasing a reference as by
 * #lwmsg_session_release_handle(), this also marks the handle as invalid
 * and will not allow it to appear in any subsequent messages between
 * peers in the session.  Although it may no longer be used in the session,
 * the cleanup function for the handle will not be invoked until the
 * last reference is released.
 *
 * Both local and remote handles should be explicitly unregistered when
 * no longer in use to avoid resource leaks.
 *
 * @param session the session
 * @param handle the handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_HANDLE, the handle was not previously registered}
 * with the session, or was already unregistered
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_session_unregister_handle(
    LWMsgSession* session,
    void* handle
    );

/**
 * @brief Query handle type
 *
 * Queries the type of a given handle: local or remote.
 *
 * @param[in] session the session
 * @param[in] handle the handle
 * @param[out] location the location of the handle
 * @lwmsg_status
 * @lwmsg_success
 * @lwmsg_code{INVALID_HANDLE, the handle was not previously registered}
 * with the session, or was already unregistered
 * @lwmsg_endstatus
 */
LWMsgStatus
lwmsg_session_get_handle_location(
    LWMsgSession* session,
    void* handle,
    LWMsgHandleType* location
    );

/**
 * @brief Get custom session data
 *
 * Retrieves the custom session data pointer created by the session's construtor
 * function.
 *
 * @param[in] session the session
 * @return the session data pointer
 */
void*
lwmsg_session_get_data(
    LWMsgSession* session
    );

/**
 * @brief Get peer security token
 *
 * Retrives the security token for the session peer.  The token's
 * lifetime is that of the session and should be copied if it needs
 * to be kept longer.
 *
 * @param[in] session the session
 * @return the security token
 */
LWMsgSecurityToken*
lwmsg_session_get_peer_security_token(
    LWMsgSession* session
    );

#ifndef DOXYGEN
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

/*@}*/

#endif
