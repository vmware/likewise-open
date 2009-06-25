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

typedef struct SharedSession
{
    LWMsgSession base;
    /* Remote session identifier */
    LWMsgSessionID rsmid;
    /* Security token of session creator */
    LWMsgSecurityToken* sec_token;
    /* Reference count */
    size_t volatile refs;
    /* Pointer to linked list of handles */
    struct SharedHandle* volatile handles;
    /* Number of handles */
    size_t num_handles;
    /* Links to other sessions in the manager */
    struct SharedSession * volatile next, * volatile prev;
    /* Lock */
    pthread_mutex_t lock;
    /* Next handle ID */
    unsigned long volatile next_hid;
    /* User data pointer */
    void* data;
    /* Destruct function */
    LWMsgSessionDestructor destruct;
} SharedSession;

typedef struct SharedHandle
{
    /* Pointer to associated session */
    SharedSession* session;
    /* Validity vit */
    LWMsgBool volatile valid;
    /* Reference count */
    size_t volatile refs;
    /* Handle type */
    const char* type;
    /* Handle pointer */
    void* pointer;
    /* Handle locality */
    LWMsgHandleType locality;
    /* Handle id */
    unsigned long hid;
    /* Handle cleanup function */
    void (*cleanup)(void*);
    /* Links to other handles in the session */
    struct SharedHandle* next, *prev;
} SharedHandle;

typedef struct SharedManager
{
    LWMsgSessionManager base;
    SharedSession* volatile sessions;
    pthread_mutex_t lock;
} SharedManager;

#define SHARED_MANAGER(obj) ((SharedManager*) (obj))
#define SHARED_SESSION(obj) ((SharedSession*) (obj))

static inline
void
shared_lock(
    SharedManager* priv
    )
{
    pthread_mutex_lock(&priv->lock);
}

static inline
void
shared_unlock(
    SharedManager* priv
    )
{
    pthread_mutex_unlock(&priv->lock);
}

static inline 
void
session_lock(
    SharedSession* session
    )
{
    pthread_mutex_lock(&session->lock);
}

static inline
void
session_unlock(
    SharedSession* session
    )
{
    pthread_mutex_unlock(&session->lock);
}

static void
shared_free_handle(
    SharedHandle* entry
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
shared_add_handle(
    SharedSession* session,
    const char* type,
    LWMsgHandleType locality,
    void* pointer,
    unsigned long hid,
    void (*cleanup)(void*),
    SharedHandle** out_handle
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SharedHandle* handle = NULL;
    SharedSession* my_session = SHARED_SESSION(session);

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

    handle->next = my_session->handles;

    if (my_session->handles)
    {
        my_session->handles->prev = handle;
    }

    my_session->handles = handle;

    my_session->num_handles++;

    *out_handle = handle;

error:

    return status;
}

static
SharedSession*
shared_find_session(
    SharedManager* priv,
    const LWMsgSessionID* rsmid
    )
{
    SharedSession* entry = NULL;

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
    SharedSession* session
    )
{
    SharedHandle* handle, *next;

    for (handle = session->handles; handle; handle = next)
    {
        next = handle->next;
        shared_free_handle(handle);
    }

    if (session->destruct && session->data)
    {
        session->destruct(session->sec_token, session->data);
    }

    if (session->sec_token)
    {
        lwmsg_security_token_delete(session->sec_token);
    }

    pthread_mutex_destroy(&session->lock);
    free(session);
}

static
void
shared_delete(
    LWMsgSessionManager* manager
    )
{
    SharedManager* priv = SHARED_MANAGER(manager);
    SharedSession* entry, *next;

    for (entry = priv->sessions; entry; entry = next)
    {
        next = entry->next;

        shared_free_session(entry);
    }

    pthread_mutex_destroy(&priv->lock);

    free(priv);
}

static
LWMsgStatus
shared_enter_session(
    LWMsgSessionManager* manager,
    const LWMsgSessionID* rsmid,
    LWMsgSecurityToken* rtoken,
    LWMsgSessionConstructor construct,
    LWMsgSessionDestructor destruct,
    void* construct_data,
    LWMsgSession** out_session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SharedManager* priv = SHARED_MANAGER(manager);
    SharedSession* session;

    shared_lock(priv);

    session = shared_find_session(priv, rsmid);
    
    if (session)
    {
        if (!session->sec_token || !lwmsg_security_token_can_access(session->sec_token, rtoken))
        {
            session = NULL;
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

        session->base.manager = manager;

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
        session->destruct = destruct;

        if (construct)
        {
            BAIL_ON_ERROR(status = construct(
                              session->sec_token,
                              construct_data,
                              &session->data));
        }

        if (priv->sessions)
        {
            priv->sessions->prev = session;
        }
        session->next = priv->sessions;
        priv->sessions = session;

    }

    *out_session = LWMSG_SESSION(session);

done:

    shared_unlock(priv);

    return status;

error:

    if (session)
    {
        shared_free_session(session);
    }

    goto done;
}

static
LWMsgStatus
shared_leave_session(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SharedManager* priv = SHARED_MANAGER(manager);
    SharedSession* my_session = SHARED_SESSION(session);

    shared_lock(priv);

    my_session->refs--;

    if (my_session->refs == 0)
    {
        if (priv->sessions == my_session)
        {
            priv->sessions = priv->sessions->next;
        }
        
        if (my_session->next)
        {
            my_session->next->prev = my_session->prev;
        }
        
        if (my_session->prev)
        {
            my_session->prev->next = my_session->next;
        }

        shared_free_session(my_session);
    }

    shared_unlock(priv);

    return status;
}

static
LWMsgSecurityToken*
shared_get_session_peer_security_token (
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    SharedSession* my_session = SHARED_SESSION(session);

    return my_session->sec_token;
}


static
LWMsgStatus
shared_register_handle_remote(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    const char* type,
    LWMsgHandleID hid,
    void (*cleanup)(void* ptr),
    void** ptr)
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SharedHandle* handle = NULL;
    SharedSession* my_session = SHARED_SESSION(session);

    session_lock(my_session);

    BAIL_ON_ERROR(status = shared_add_handle(
                      my_session,
                      type,
                      LWMSG_HANDLE_REMOTE,
                      NULL,
                      hid,
                      cleanup,
                      &handle));

    if (ptr)
    {
        *ptr = handle->pointer;
    }

error:

    session_unlock(my_session);

    return status;
}

static
LWMsgStatus
shared_register_handle_local(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    const char* type,
    void* pointer,
    void (*cleanup)(void* ptr),
    LWMsgHandleID* hid
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SharedHandle* handle = NULL;
    SharedSession* my_session = SHARED_SESSION(session);

    session_lock(my_session);

    BAIL_ON_ERROR(status = shared_add_handle(
                      my_session,
                      type,
                      LWMSG_HANDLE_LOCAL,
                      pointer,
                      my_session->next_hid++,
                      cleanup,
                      &handle));

    if (hid)
    {
        *hid = handle->hid;
    }

error:

    session_unlock(my_session);

    return status;
}

static
LWMsgStatus
shared_retain_handle(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* ptr
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SharedHandle* handle = NULL;
    SharedSession* my_session = SHARED_SESSION(session);

    session_lock(my_session);

    for (handle = my_session->handles; handle; handle = handle->next)
    {
        if (handle->pointer == ptr)
        {
            handle->refs++;
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);

done:

    session_unlock(my_session);

    return status;

error:

    goto done;
}

static
LWMsgStatus
shared_release_handle(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* ptr
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SharedHandle* handle = NULL;
    SharedSession* my_session = SHARED_SESSION(session);

    session_lock(my_session);

    for (handle = my_session->handles; handle; handle = handle->next)
    {
        if (handle->pointer == ptr)
        {
            if (--handle->refs == 0)
            {
                if (handle == my_session->handles)
                {
                    my_session->handles = my_session->handles->next;
                }

                shared_free_handle(handle);
                my_session->num_handles--;
            }
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);

done:

    session_unlock(my_session);

    return status;

error:

    goto done;
}

static
LWMsgStatus
shared_unregister_handle(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* ptr
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SharedHandle* handle = NULL;
    SharedSession* my_session = SHARED_SESSION(session);

    session_lock(my_session);

    for (handle = my_session->handles; handle; handle = handle->next)
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
                if (handle == my_session->handles)
                {
                    my_session->handles = my_session->handles->next;
                }

                shared_free_handle(handle);
                my_session->num_handles--;
            }
            goto done;
        }
    }

    BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);

done:

    session_unlock(my_session);

    return status;

error:
    
    goto done;
}

static
LWMsgStatus
shared_handle_pointer_to_id(
    LWMsgSessionManager* manager,
    LWMsgSession* session,
    void* pointer,
    const char** type,
    LWMsgHandleType* htype,
    LWMsgHandleID* hid
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SharedHandle* handle = NULL;
    SharedSession* my_session = SHARED_SESSION(session);

    session_lock(my_session);

    for (handle = my_session->handles; handle; handle = handle->next)
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

    session_unlock(my_session);

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
    LWMsgHandleType htype,
    LWMsgHandleID hid,
    void** pointer
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SharedHandle* handle = NULL;
    SharedSession* my_session = SHARED_SESSION(session);

    session_lock(my_session);

    for (handle = my_session->handles; handle; handle = handle->next)
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

    session_unlock(my_session);

    return status;

error:

    goto done;
}

static
void*
shared_get_session_data (
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    void* data = NULL;
    SharedSession* my_session = SHARED_SESSION(session);


    session_lock(my_session);

    data = my_session->data;

    session_unlock(my_session);

    return data;
}

static
const LWMsgSessionID*
shared_get_session_id(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    return &SHARED_SESSION(session)->rsmid;
}

static
size_t
shared_get_session_assoc_count(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    size_t refs;
    SharedSession* my_session = SHARED_SESSION(session);

    session_lock(my_session);
    refs = my_session->refs;
    session_unlock(my_session);
    return refs;
}

static
size_t
shared_get_session_handle_count(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    size_t handles;
    SharedSession* my_session = SHARED_SESSION(session);

    session_lock(my_session);
    handles = my_session->num_handles;
    session_unlock(my_session);
    return handles;
}

static LWMsgSessionManagerClass shared_class = 
{
    .delete = shared_delete,
    .enter_session = shared_enter_session,
    .leave_session = shared_leave_session,
    .register_handle_local = shared_register_handle_local,
    .register_handle_remote = shared_register_handle_remote,
    .retain_handle = shared_retain_handle,
    .release_handle = shared_release_handle,
    .unregister_handle = shared_unregister_handle,
    .handle_pointer_to_id = shared_handle_pointer_to_id,
    .handle_id_to_pointer = shared_handle_id_to_pointer,
    .get_session_data = shared_get_session_data,
    .get_session_id = shared_get_session_id,
    .get_session_assoc_count = shared_get_session_assoc_count,
    .get_session_handle_count = shared_get_session_handle_count,
    .get_session_peer_security_token = shared_get_session_peer_security_token
};
                                         
LWMsgStatus
lwmsg_shared_session_manager_new(
    LWMsgSessionManager** manager
    )
{
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    SharedManager* my_manager = NULL;

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&my_manager));
    BAIL_ON_ERROR(status = lwmsg_session_manager_init(&my_manager->base, &shared_class));

    if (pthread_mutex_init(&my_manager->lock, NULL))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_SYSTEM);
    }

    *manager = LWMSG_SESSION_MANAGER(my_manager);

done:

    return status;

error:

    if (my_manager)
    {
        free(my_manager);
    }

    goto done;
}
