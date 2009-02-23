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

struct LWMsgSessionManager
{
    LWMsgSessionManagerClass* mclass;
    LWMsgSessionID smid;

    unsigned char private_data[] __attribute__((aligned));
};

LWMsgStatus
lwmsg_session_manager_enter_session (
    LWMsgSessionManager* manager,
    const LWMsgSessionID* rsmid,
    LWMsgSecurityToken* rtoken,
    LWMsgSession** session,
    size_t* assoc_count
    );

LWMsgStatus 
lwmsg_session_manager_leave_session (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    size_t* assoc_count
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

LWMsgStatus
lwmsg_session_manager_set_session_data (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* data,
    LWMsgSessionDataCleanupFunction cleanup
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

#ifndef LWMSG_NO_THREADS
LWMsgStatus
lwmsg_shared_session_manager_new(
    LWMsgSessionManager** out_manager
    );
#endif

#endif
