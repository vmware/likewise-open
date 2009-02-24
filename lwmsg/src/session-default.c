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
 *        session-default.c
 *
 * Abstract:
 *
 *        Session management API
 *        Default session manager implementation
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include "session-private.h"
#include "util-private.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct LWMsgSession
{
    /* Remote session identifier */
    LWMsgSessionID rsmid;
    /* Security token of session creator */
    LWMsgSecurityToken* sec_token;
    /* Reference count */
    size_t refs;
    /* Pointer to linked list of handles */
    struct HandleEntry* handles;
    /* Number of handles */
    size_t num_handles;
    /* Links to other sessions in the manager */
    struct LWMsgSession* next, *prev;
    /* User data pointer */
    void* data;
    /* Data pointer cleanup function */
    LWMsgSessionDataCleanupFunction cleanup;
} SessionEntry;

typedef struct HandleEntry
{
    /* Handle type */
    const char* type;
    /* Handle refcount */
    size_t refs;
    /* Validity bit */
    LWMsgBool valid;
    /* Handle pointer */
    void* pointer;
    /* Handle locality */
    LWMsgHandleType locality;
    /* Handle id */
    unsigned long hid;
    /* Handle cleanup function */
    void (*cleanup)(void*);
    /* Links to other handles in the session */
    struct HandleEntry* next, *prev;
} HandleEntry;

typedef struct DefaultPrivate
{
    SessionEntry* sessions;
    unsigned long next_hid;
} DefaultPrivate;

static void
default_free_handle(
    HandleEntry* entry
    )
{
    if (entry->cleanup)
    {
        entry->cleanup(entry->pointer);
    }

    if (entry->prev)
    {
        entry->prev->next = entry->next;
    }
    
    if (entry->next)
    {
        entry->next->prev = entry->prev;
    }

    free(entry);
}

static
LWMsgStatus
default_add_handle(
    SessionEntry* session,
    const char* type,
    LWMsgHandleType locality,
    void* pointer,
    unsigned long hid,
    void (*cleanup)(void*),
    HandleEntry** out_handle
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    HandleEntry* handle = NULL;

    handle = calloc(1, sizeof(*handle));

    if (!handle)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
    }

    handle->type = type;
    handle->valid = LWMSG_TRUE;
    handle->refs = 1;
    handle->pointer = pointer ? pointer : handle;
    handle->cleanup = cleanup;
    handle->hid = hid;
    handle->locality = locality;

    handle->next = session->handles;

    if (session->handles)
    {
        session->handles->prev = handle;
    }

    session->handles = handle;
    session->num_handles++;

    *out_handle = handle;

error:

    return status;
}

static
SessionEntry*
default_find_session(
    DefaultPrivate* priv,
    const LWMsgSessionID* rsmid
    )
{
    SessionEntry* entry = NULL;

    for (entry = priv->sessions; entry; entry = entry->next)
    {
        if (!memcmp(rsmid->bytes, entry->rsmid.bytes, sizeof(rsmid->bytes)))
        {
            return entry;
        }
    }

    return NULL;
}

static
void
default_free_session(
    SessionEntry* session
    )
{
    HandleEntry* handle, *next;

    for (handle = session->handles; handle; handle = next)
    {
        next = handle->next;
        default_free_handle(handle);
    }

    if (session->sec_token)
    {
        lwmsg_security_token_delete(session->sec_token);
    }

    if (session->cleanup)
    {
        session->cleanup(session->data);
    }

    free(session);
}

static
LWMsgStatus
default_construct(
    LWMsgSessionManager* manager
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;

    return status;
}

static
void
default_destruct(
    LWMsgSessionManager* manager
    )
{
    DefaultPrivate* priv = lwmsg_session_manager_get_private(manager);
    SessionEntry* entry, *next;

    for (entry = priv->sessions; entry; entry = next)
    {
        next = entry->next;

        default_free_session(entry);
    }
}

static
LWMsgStatus
default_enter_session(
    LWMsgSessionManager* manager,
    const LWMsgSessionID* rsmid,
    LWMsgSecurityToken* rtoken,
    LWMsgSession** out_session,
    size_t* assoc_count
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultPrivate* priv = lwmsg_session_manager_get_private(manager);
    SessionEntry* session = default_find_session(priv, rsmid);
    
    if (session)
    {
        if (!session->sec_token || !lwmsg_security_token_can_access(session->sec_token, rtoken))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_SECURITY);
        }

        session->refs++;
    }
    else
    {
        session = calloc(1, sizeof(*session));
        if (!session)
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_MEMORY);
        }

        memcpy(session->rsmid.bytes, rsmid->bytes, sizeof(rsmid->bytes));

        if (rtoken)
        {
            BAIL_ON_ERROR(status = lwmsg_security_token_copy(rtoken, &session->sec_token));
        }
        
        session->refs = 1;
        session->next = priv->sessions;

        if (priv->sessions)
        {
            priv->sessions->prev = session;
        }

        priv->sessions = session;
    }

    *out_session = session;

    if (assoc_count)
    {
        *assoc_count = session->refs;
    }

error:

    return status;
}

static
LWMsgStatus 
default_leave_session(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    size_t* assoc_count
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultPrivate* priv = lwmsg_session_manager_get_private(manager);
    
    session->refs--;

    if (assoc_count)
    {
        *assoc_count = session->refs;
    }

    if (session->refs == 0)
    {
        if (priv->sessions == session)
        {
            priv->sessions = priv->sessions->next;
        }

        default_free_session(session);
    }

    return status;
}

static
LWMsgStatus
default_register_handle_remote(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    const char* type,
    LWMsgHandleID hid,
    void (*cleanup)(void* ptr),
    void** ptr)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultPrivate* priv = lwmsg_session_manager_get_private(manager);
    HandleEntry* handle = NULL;

    BAIL_ON_ERROR(status = default_add_handle(
                      session,
                      type,
                      LWMSG_HANDLE_REMOTE,
                      NULL,
                      priv->next_hid++,
                      cleanup,
                      &handle));

    if (ptr)
    {
        *ptr = handle->pointer;
    }

error:

    return status;
}

static
LWMsgStatus
default_register_handle_local(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    const char* type,
    void* pointer,
    void (*cleanup)(void* ptr),
    LWMsgHandleID* hid
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    DefaultPrivate* priv = lwmsg_session_manager_get_private(manager);
    HandleEntry* handle = NULL;

    BAIL_ON_ERROR(status = default_add_handle(
                      session,
                      type,
                      LWMSG_HANDLE_LOCAL,
                      pointer,
                      priv->next_hid++,
                      cleanup,
                      &handle));

    if (hid)
    {
        *hid = handle->hid;
    }

error:

    return status;
}

static
LWMsgStatus
default_retain_handle(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* ptr
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    HandleEntry* handle = NULL;

    if (!session)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    for (handle = session->handles; handle; handle = handle->next)
    {
        if (handle->pointer == ptr)
        {
            handle->refs++;
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
default_release_handle(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* ptr
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    HandleEntry* handle = NULL;

    if (!session)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    for (handle = session->handles; handle; handle = handle->next)
    {
        if (handle->pointer == ptr)
        {
            if (--handle->refs == 0)
            {
                if (handle == session->handles)
                {
                    session->handles = session->handles->next;
                }

                default_free_handle(handle);
                session->num_handles--;
            }
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);

done:

    return status;

error:
    
    goto done;
}

static
LWMsgStatus
default_unregister_handle(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* ptr
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    HandleEntry* handle = NULL;

    if (!session)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    for (handle = session->handles; handle; handle = handle->next)
    {
        if (handle->pointer == ptr)
        {
            if (!handle->valid)
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);
            }

            handle->valid = LWMSG_FALSE;

            if (--handle->refs == 0)
            {
                if (handle == session->handles)
                {
                    session->handles = session->handles->next;
                }

                default_free_handle(handle);
                session->num_handles--;
            }
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
default_handle_pointer_to_id(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* pointer,
    const char** type,
    LWMsgHandleType* htype,
    LWMsgHandleID* hid
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    HandleEntry* handle = NULL;

    if (!session)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    for (handle = session->handles; handle; handle = handle->next)
    {
        if (handle->pointer == pointer)
        {
            if (!handle->valid)
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
            }

            if (type)
            {
                *type = handle->type;
            }
            if (htype)
            {
                *htype = handle->locality;
            }
            if (hid)
            {
                *hid = handle->hid;
            }
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
default_handle_id_to_pointer(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    const char* type,
    LWMsgHandleType htype,
    LWMsgHandleID hid,
    void** pointer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    HandleEntry* handle = NULL;

    for (handle = session->handles; handle; handle = handle->next)
    {
        if (handle->hid == hid && handle->locality == htype)
        {
            if (!handle->valid)
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
            }

            if (type && strcmp(type, handle->type))
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
            }

            *pointer = handle->pointer;
            handle->refs++;
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);

done:

    return status;

error:

    goto done;
}

static
LWMsgStatus
default_set_session_data (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* data,
    LWMsgSessionDataCleanupFunction cleanup
    )
{
    if (session->cleanup)
    {
        session->cleanup(session->data);
    }

    session->data = data;
    session->cleanup = cleanup;

    return LWMSG_STATUS_SUCCESS;
}

static
void*
default_get_session_data (
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    return session->data;
}

static
const LWMsgSessionID*
default_get_session_id(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    return &session->rsmid;
}

static
size_t
default_get_session_assoc_count(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    return session->refs;
}

static
size_t
default_get_session_handle_count(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    return session->num_handles;
}

static LWMsgSessionManagerClass default_class = 
{
    .private_size = sizeof(DefaultPrivate),
    .construct = default_construct,
    .destruct = default_destruct,
    .enter_session = default_enter_session,
    .leave_session = default_leave_session,
    .register_handle_local = default_register_handle_local,
    .register_handle_remote = default_register_handle_remote,
    .retain_handle = default_retain_handle,
    .release_handle = default_release_handle,
    .unregister_handle = default_unregister_handle,
    .handle_pointer_to_id = default_handle_pointer_to_id,
    .handle_id_to_pointer = default_handle_id_to_pointer,
    .set_session_data = default_set_session_data,
    .get_session_data = default_get_session_data,
    .get_session_id = default_get_session_id,
    .get_session_assoc_count = default_get_session_assoc_count,
    .get_session_handle_count = default_get_session_handle_count
};
                                         
LWMsgStatus
lwmsg_default_session_manager_new(
    LWMsgSessionManager** out_manager
    )
{
    return lwmsg_session_manager_new(&default_class, out_manager);
}
