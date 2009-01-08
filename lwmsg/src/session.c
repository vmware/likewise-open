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
 *        session.c
 *
 * Abstract:
 *
 *        Session management API
 *        Primary entry points
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "util-private.h"
#include "session-private.h"
#include "mt19937ar.h"
#include <lwmsg/time.h>

#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

void
lwmsg_session_id_to_string(
    const LWMsgSessionID* smid,
    char buffer[sizeof(smid->bytes) * 2 + 1]
    )
{
    sprintf(buffer, "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
            (unsigned int) smid->bytes[0],
            (unsigned int) smid->bytes[1],
            (unsigned int) smid->bytes[2],
            (unsigned int) smid->bytes[3],
            (unsigned int) smid->bytes[4],
            (unsigned int) smid->bytes[5],
            (unsigned int) smid->bytes[6],
            (unsigned int) smid->bytes[7]);
}

static void
lwmsg_session_manager_generate_smid(
    LWMsgSessionID* smid
    )
{
    mt m;
    uint32_t seed[3];
    uint32_t s;
    int i;
    LWMsgTime now;

    lwmsg_time_now(&now);

    /* Add in 32 bits of data from the address of the smid */
    seed[0] = (uint32_t) (size_t) smid;
    /* Add in 32 bits of data from the current pid */
    seed[1] = (uint32_t) getpid();
    /* Add in 32 bits of data from the current time */
    seed[2] = (uint32_t) now.microseconds;
        
    mt_init_by_array(&m, seed, sizeof(seed) / sizeof(*seed));
        
    for (i = 0; i < sizeof(smid->bytes); i += sizeof(s))
    {
        s = mt_genrand_int32(&m);
        
        memcpy(smid->bytes + i, &s, sizeof(s));
    }
}

LWMsgStatus
lwmsg_session_manager_new(
    LWMsgSessionManagerClass* mclass,
    LWMsgSessionManager** out_manager
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    LWMsgSessionManager* manager = NULL;

    manager = calloc(1, offsetof(LWMsgSessionManager, private_data) + mclass->private_size);
    
    if (!manager)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    BAIL_ON_ERROR(status = mclass->construct(manager));

    manager->mclass = mclass;

    lwmsg_session_manager_generate_smid(&manager->smid);

    *out_manager = manager;

error:

    return status;
}

void
lwmsg_session_manager_delete(
    LWMsgSessionManager* manager
    )
{
    manager->mclass->destruct(manager);

    free(manager);
}

void*
lwmsg_session_manager_get_private(
    LWMsgSessionManager* manager
    )
{
    return manager->private_data;
}

const LWMsgSessionID*
lwmsg_session_manager_get_id(
    LWMsgSessionManager* manager
    )
{
    return &manager->smid;
}

LWMsgStatus
lwmsg_session_manager_enter_session (
    LWMsgSessionManager* manager,
    const LWMsgSessionID* rsmid,
    LWMsgSecurityToken* rtoken,
    LWMsgSession** session
    )
{
    return manager->mclass->enter_session(manager, rsmid, rtoken, session);
}

LWMsgStatus 
lwmsg_session_manager_leave_session (
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    return manager->mclass->leave_session(manager, session);
}

LWMsgStatus
lwmsg_session_manager_register_handle (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    const char* type,
    void* ptr,
    void (*cleanup)(void* ptr)
    )
{
    return manager->mclass->register_handle(manager, session, type, ptr, cleanup);
}

LWMsgStatus
lwmsg_session_manager_unregister_handle (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* ptr,
    LWMsgBool do_cleanup
    )
{
    return manager->mclass->unregister_handle(manager, session, ptr, do_cleanup);
}

LWMsgStatus
lwmsg_session_manager_handle_pointer_to_id (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    const char* type,
    void* ptr,
    LWMsgBool autoreg,
    LWMsgHandleLocation* out_location,
    unsigned long* out_hid
    )
{
    return manager->mclass->handle_pointer_to_id(manager, session, type, ptr, autoreg, out_location, out_hid);
}

LWMsgStatus
lwmsg_session_manager_handle_id_to_pointer (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    const char* type,
    LWMsgHandleLocation location,
    unsigned long hid,
    LWMsgBool autoreg,
    void** out_ptr
    )
{
    return manager->mclass->handle_id_to_pointer(manager, session, type, location, hid, autoreg, out_ptr);
}

LWMsgStatus
lwmsg_session_manager_set_session_data (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* data,
    LWMsgSessionDataCleanupFunction cleanup
    )
{
    return manager->mclass->set_session_data(manager, session, data, cleanup);
}

void*
lwmsg_session_manager_get_session_data (
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    return manager->mclass->get_session_data(manager, session);
}

const LWMsgSessionID*
lwmsg_session_manager_get_session_id(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    return manager->mclass->get_session_id(manager, session);
}

size_t
lwmsg_session_manager_get_session_assoc_count(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    return manager->mclass->get_session_assoc_count(manager, session);
}

size_t
lwmsg_session_manager_get_session_handle_count(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    return manager->mclass->get_session_handle_count(manager, session);
}
