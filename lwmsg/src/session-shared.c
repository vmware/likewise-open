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
 *        session-shared.c
 *
 * Abstract:
 *
 *        Session management API
 *        Shared (thread-safe) session manager implementation
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */
#include <config.h>
#include "session-private.h"
#include "util-private.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

typedef struct LWMsgSession
{
    /* Remote session identifier */
    LWMsgSessionID rsmid;
    /* Security token of session creator */
    LWMsgSecurityToken* sec_token;
    /* Reference count */
    size_t volatile refs;
    /* Pointer to linked list of handles */
    struct HandleEntry* volatile handles;
    /* Number of handles */
    size_t num_handles;
    /* Links to other sessions in the manager */
    struct LWMsgSession * volatile next, * volatile prev;
    /* Lock */
    pthread_mutex_t lock;
    /* Next handle ID */
    unsigned long volatile next_hid;
    /* User data pointer */
    void* data;
    /* Data pointer cleanup function */
    LWMsgSessionDataCleanupFunction cleanup;
} SessionEntry;

typedef struct HandleEntry
{
    /* Pointer to associated session */
    SessionEntry* session;
    /* Handle type */
    const char* type;
    /* Handle pointer */
    void* pointer;
    /* Handle locality */
    LWMsgHandleLocation locality;
    /* Handle id */
    unsigned long hid;
    /* Handle cleanup function */
    void (*cleanup)(void*);
    /* Links to other handles in the session */
    struct HandleEntry* next, *prev;
} HandleEntry;

typedef struct SharedPrivate
{
    SessionEntry* volatile sessions;
    pthread_mutex_t lock;
} SharedPrivate;

static inline
void
shared_lock(
    SharedPrivate* priv
    )
{
    pthread_mutex_lock(&priv->lock);
}

static inline
void
shared_unlock(
    SharedPrivate* priv
    )
{
    pthread_mutex_unlock(&priv->lock);
}

static inline 
void
session_lock(
    LWMsgSession* session
    )
{
    pthread_mutex_lock(&session->lock);
}

static inline
void
session_unlock(
    LWMsgSession* session
    )
{
    pthread_mutex_unlock(&session->lock);
}

static void
shared_free_handle(
    HandleEntry* entry,
    LWMsgBool do_cleanup
    )
{
    if (entry->cleanup && do_cleanup)
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
shared_add_handle(
    SessionEntry* session,
    const char* type,
    LWMsgHandleLocation locality,
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
shared_find_session(
    SharedPrivate* priv,
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
shared_free_session(
    SessionEntry* session
    )
{
    HandleEntry* handle, *next;

    for (handle = session->handles; handle; handle = next)
    {
        next = handle->next;
        shared_free_handle(handle, LWMSG_TRUE);
    }

    if (session->sec_token)
    {
        lwmsg_security_token_delete(session->sec_token);
    }

    if (session->cleanup)
    {
        session->cleanup(session->data);
    }

    pthread_mutex_destroy(&session->lock);
    free(session);
}

static
LWMsgStatus
shared_construct(
    LWMsgSessionManager* manager
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SharedPrivate* priv = lwmsg_session_manager_get_private(manager);

    if (pthread_mutex_init(&priv->lock, NULL))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

error:

    return status;
}

static
void
shared_destruct(
    LWMsgSessionManager* manager
    )
{
    SharedPrivate* priv = lwmsg_session_manager_get_private(manager);
    SessionEntry* entry, *next;

    for (entry = priv->sessions; entry; entry = next)
    {
        next = entry->next;

        shared_free_session(entry);
    }

    pthread_mutex_destroy(&priv->lock);
}

static
LWMsgStatus
shared_enter_session(
    LWMsgSessionManager* manager,
    const LWMsgSessionID* rsmid,
    LWMsgSecurityToken* rtoken,
    LWMsgSession** out_session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SharedPrivate* priv = lwmsg_session_manager_get_private(manager);
    SessionEntry* session;

    shared_lock(priv);

    session = shared_find_session(priv, rsmid);
    
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

        if (pthread_mutex_init(&session->lock, NULL))
        {
            BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
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

error:

    shared_unlock(priv);

    return status;
}

static
LWMsgStatus 
shared_leave_session(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SharedPrivate* priv = lwmsg_session_manager_get_private(manager);

    shared_lock(priv);

    session->refs--;

    if (session->refs == 0)
    {
        if (priv->sessions == session)
        {
            priv->sessions = priv->sessions->next;
        }
        
        if (session->next)
        {
            session->next->prev = session->prev;
        }
        
        if (session->prev)
        {
            session->prev->next = session->next;
        }

        shared_free_session(session);
    }

    shared_unlock(priv);

    return status;
}

static
LWMsgStatus
shared_register_handle(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    const char* type,
    void* pointer,
    void (*cleanup)(void*)
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    HandleEntry* handle = NULL;

    session_lock(session);

    BAIL_ON_ERROR(status = shared_add_handle(
                      session,
                      type,
                      LWMSG_HANDLE_LOCAL,
                      pointer,
                      session->next_hid++,
                      cleanup,
                      &handle));

error:

    session_unlock(session);

    return status;
}

static
LWMsgStatus
shared_unregister_handle(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* ptr,
    LWMsgBool do_cleanup
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    HandleEntry* handle = NULL;

    session_lock(session);

    for (handle = session->handles; handle; handle = handle->next)
    {
        if (handle->pointer == ptr)
        {
            if (handle == session->handles)
            {
                session->handles = session->handles->next;
            }

            shared_free_handle(handle, do_cleanup);
            session->num_handles--;
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);

done:

    session_unlock(session);

    return status;

error:
    
    goto done;
}


static
LWMsgStatus
shared_handle_pointer_to_id(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    const char* type,
    void* pointer,
    LWMsgBool autoreg,
    LWMsgHandleLocation* out_location,
    unsigned long* out_hid
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    HandleEntry* handle = NULL;

    session_lock(session);

    for (handle = session->handles; handle; handle = handle->next)
    {
        if (handle->pointer == pointer)
        {
            if (type && strcmp(type, handle->type))
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
            }

            if (out_location)
            {
                *out_location = handle->locality;
            }
            if (out_hid)
            {
                *out_hid = handle->hid;
            }
            goto done;
        }
    }

    if (autoreg)
    {
        BAIL_ON_ERROR(status = shared_add_handle(
                          session,
                          type,
                          LWMSG_HANDLE_LOCAL,
                          pointer,
                          session->next_hid++,
                          NULL,
                          &handle));
    
        *out_location = handle->locality;
        *out_hid = handle->hid;
    }
    else
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

done:

    session_unlock(session);

    return status;

error:

    goto done;
}

static
LWMsgStatus
shared_handle_id_to_pointer(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    const char* type,
    LWMsgHandleLocation location,
    unsigned long hid,
    LWMsgBool autoreg,
    void** out_ptr
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    HandleEntry* handle = NULL;

    session_lock(session);

    for (handle = session->handles; handle; handle = handle->next)
    {
        if (handle->hid == hid && handle->locality == location)
        {
            if (type && strcmp(type, handle->type))
            {
                BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
            }

            *out_ptr = handle->pointer;
            goto done;
        }
    }

    if (autoreg)
    {
        BAIL_ON_ERROR(status = shared_add_handle(
                          session,
                          type,
                          location,
                          NULL,
                          hid,
                          NULL,
                          &handle));
        
        BAIL_ON_ERROR(status);

        *out_ptr = handle->pointer;
    }
    else
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

done:

    session_unlock(session);

    return status;

error:

    goto done;
}

LWMsgStatus
shared_set_session_data (
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* data,
    LWMsgSessionDataCleanupFunction cleanup
    )
{
    session_lock(session);

    if (session->cleanup)
    {
        session->cleanup(session->data);
    }

    session->data = data;
    session->cleanup = cleanup;

    session_unlock(session);

    return LWMSG_STATUS_SUCCESS;
}

void*
shared_get_session_data (
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    void* data = NULL;

    session_lock(session);

    data = session->data;

    session_unlock(session);

    return data;
}

const LWMsgSessionID*
shared_get_session_id(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    return &session->rsmid;
}

size_t
shared_get_session_assoc_count(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    size_t refs;

    session_lock(session);
    refs = session->refs;
    session_unlock(session);
    return refs;
}

size_t
shared_get_session_handle_count(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    size_t handles;

    session_lock(session);
    handles = session->num_handles;
    session_unlock(session);
    return handles;
}

static LWMsgSessionManagerClass shared_class = 
{
    .private_size = sizeof(SharedPrivate),
    .construct = shared_construct,
    .destruct = shared_destruct,
    .enter_session = shared_enter_session,
    .leave_session = shared_leave_session,
    .register_handle = shared_register_handle,
    .unregister_handle = shared_unregister_handle,
    .handle_pointer_to_id = shared_handle_pointer_to_id,
    .handle_id_to_pointer = shared_handle_id_to_pointer,
    .set_session_data = shared_set_session_data,
    .get_session_data = shared_get_session_data,
    .get_session_id = shared_get_session_id,
    .get_session_assoc_count = shared_get_session_assoc_count,
    .get_session_handle_count = shared_get_session_handle_count
};
                                         
LWMsgStatus
lwmsg_shared_session_manager_new(
    LWMsgSessionManager** out_manager
    )
{
    return lwmsg_session_manager_new(&shared_class, out_manager);
}
