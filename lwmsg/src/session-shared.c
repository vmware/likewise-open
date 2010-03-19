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
#include <limits.h>

typedef struct SharedHandleKey
{
    LWMsgHandleType type;
    LWMsgHandleID id;
} SharedHandleKey;

typedef struct SharedSession
{
    LWMsgSession base;
    /* Remote session identifier */
    LWMsgSessionID rsmid;
    /* Security token of session creator */
    LWMsgSecurityToken* sec_token;
    /* Reference count */
    size_t volatile refs;
    /* Hash of handles by ptr (local handles only) */
    LWMsgHashTable handle_by_ptr;
    /* Hash of handles by id */
    LWMsgHashTable handle_by_id;
    /* Lock */
    pthread_mutex_t lock;
    /* Next handle ID */
    unsigned long volatile next_hid;
    /* User data pointer */
    void* data;
    /* Destruct function */
    LWMsgSessionDestructFunction destruct;
    /* Link in hash table by rsmid */
    LWMsgRing ring;
} SharedSession;

typedef struct SharedHandle
{
    /* Key */
    SharedHandleKey key;
    /* Validity bit */
    LWMsgBool volatile valid;
    /* Reference count */
    size_t volatile refs;
    /* Handle type */
    const char* type;
    /* Handle pointer */
    void* pointer;
    /* Handle cleanup function */
    void (*cleanup)(void*);
    /* Link in hash table by ptr */
    LWMsgRing ptr_ring;
    /* Link in hash table by id */
    LWMsgRing id_ring;
} SharedHandle;

typedef struct SharedManager
{
    LWMsgSessionManager base;
    LWMsgHashTable sessions;
    pthread_mutex_t lock;
} SharedManager;

#define SHARED_MANAGER(obj) ((SharedManager*) (obj))
#define SHARED_SESSION(obj) ((SharedSession*) (obj))

static
void*
shared_session_get_key(
    const void* entry
    )
{
    return &((SharedSession*) entry)->rsmid;
}

static
size_t
shared_session_digest(
    const void* key
    )
{
    const LWMsgSessionID* rsmid = key;
    size_t hash = 0;
    int i = 0;

    for (i = 0; i < sizeof(rsmid->bytes); i++)
    {
        hash = hash * 31 + rsmid->bytes[i];
    }

    return hash;
}

static
LWMsgBool
shared_session_equal(
    const void* key1,
    const void* key2
    )
{
    return memcmp(key1, key2, sizeof(LWMsgSessionID)) == 0;
}

static
void*
shared_handle_get_key_ptr(
    const void* entry
    )
{
    return ((SharedHandle*) entry)->pointer;
}

static
size_t
shared_handle_digest_ptr(
    const void* key
    )
{
    return (size_t) key;
}

static
LWMsgBool
shared_handle_equal_ptr(
    const void* key1,
    const void* key2
    )
{
    return key1 == key2;
}

static
void*
shared_handle_get_key_id(
    const void* entry
    )
{
    return &((SharedHandle*) entry)->key;
}

static
size_t
shared_handle_digest_id(
    const void* key
    )
{
    const SharedHandleKey* hkey = key;

    return hkey->type == LWMSG_HANDLE_LOCAL ? hkey->id : ~hkey->id;
}

static
LWMsgBool
shared_handle_equal_id(
    const void* key1,
    const void* key2
    )
{
    const SharedHandleKey* hkey1 = key1;
    const SharedHandleKey* hkey2 = key2;

    return
        hkey1->type == hkey2->type &&
        hkey1->id == hkey2->id;
}

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
    SharedSession* session,
    SharedHandle* entry
    )
{
    lwmsg_hash_remove_entry(&session->handle_by_ptr, entry);
    lwmsg_hash_remove_entry(&session->handle_by_id, entry);

    if (entry->cleanup)
    {
        entry->cleanup(entry->pointer);
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

    BAIL_ON_ERROR(status = LWMSG_ALLOC(&handle));

    handle->type = type;
    handle->valid = LWMSG_TRUE;
    handle->refs = 1;
    handle->pointer = pointer ? pointer : handle;
    handle->cleanup = cleanup;
    handle->key.id = hid;
    handle->key.type = locality;

    lwmsg_ring_init(&handle->ptr_ring);
    lwmsg_ring_init(&handle->id_ring);

    if (pointer)
    {
        lwmsg_hash_insert_entry(&my_session->handle_by_ptr, handle);
    }

    lwmsg_hash_insert_entry(&my_session->handle_by_id, handle);

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
    return lwmsg_hash_find_key(&priv->sessions, rsmid);
}

static
void
shared_free_session(
    SharedManager* manager,
    SharedSession* session
    )
{
    SharedHandle* handle = NULL;
    LWMsgHashIter iter = {0};

    lwmsg_hash_remove_entry(&manager->sessions, session);

    lwmsg_hash_iter_begin(&session->handle_by_id, &iter);
    while ((handle = lwmsg_hash_iter_next(&session->handle_by_id, &iter)))
    {
        shared_free_handle(session, handle);
    }
    lwmsg_hash_iter_end(&session->handle_by_id, &iter);

    lwmsg_hash_destroy(&session->handle_by_id);
    lwmsg_hash_destroy(&session->handle_by_ptr);

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
    LWMsgHashIter iter = {0};
    SharedSession* entry = NULL;

    lwmsg_hash_iter_begin(&priv->sessions, &iter);
    while ((entry = lwmsg_hash_iter_next(&priv->sessions, &iter)))
    {
        shared_free_session(priv, entry);
    }
    lwmsg_hash_iter_end(&priv->sessions, &iter);

    lwmsg_hash_destroy(&priv->sessions);

    pthread_mutex_destroy(&priv->lock);

    free(priv);
}

static
LWMsgStatus
shared_enter_session(
    LWMsgSessionManager* manager,
    const LWMsgSessionID* rsmid,
    LWMsgSecurityToken* rtoken,
    LWMsgSessionConstructFunction construct,
    LWMsgSessionDestructFunction destruct,
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
        BAIL_ON_ERROR(status = LWMSG_ALLOC(&session));

        session->base.manager = manager;

        BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_mutex_init(&session->lock, NULL)));

        memcpy(session->rsmid.bytes, rsmid->bytes, sizeof(rsmid->bytes));

        if (rtoken)
        {
            BAIL_ON_ERROR(status = lwmsg_security_token_copy(rtoken, &session->sec_token));
        }

        lwmsg_ring_init(&session->ring);

        BAIL_ON_ERROR(status = lwmsg_hash_init(
                          &session->handle_by_id,
                          31,
                          shared_handle_get_key_id,
                          shared_handle_digest_id,
                          shared_handle_equal_id,
                          offsetof(SharedHandle, id_ring)));

        BAIL_ON_ERROR(status = lwmsg_hash_init(
                          &session->handle_by_ptr,
                          31,
                          shared_handle_get_key_ptr,
                          shared_handle_digest_ptr,
                          shared_handle_equal_ptr,
                          offsetof(SharedHandle, ptr_ring)));

        session->refs = 1;
        session->destruct = destruct;

        if (construct)
        {
            BAIL_ON_ERROR(status = construct(
                              session->sec_token,
                              construct_data,
                              &session->data));
        }

        lwmsg_hash_insert_entry(&priv->sessions, session);
    }

    *out_session = LWMSG_SESSION(session);

done:

    shared_unlock(priv);

    return status;

error:

    if (session)
    {
        shared_free_session(priv, session);
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
        shared_free_session(priv, my_session);
    }

    shared_unlock(priv);

    return status;
}

static
void
shared_retain_session(
    LWMsgSessionManager* manager,
    LWMsgSession* session
    )
{
    SharedManager* priv = SHARED_MANAGER(manager);
    SharedSession* my_session = SHARED_SESSION(session);

    shared_lock(priv);

    my_session->refs++;

    shared_unlock(priv);
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
SharedHandle*
shared_find_handle_by_id(
    SharedSession* session,
    LWMsgHandleType type,
    LWMsgHandleID id
    )
{
    SharedHandleKey key = {0};

    key.type = type;
    key.id = id;

    return lwmsg_hash_find_key(&session->handle_by_id, &key);
}

static
SharedHandle*
shared_find_handle_by_ptr(
    SharedSession* session,
    void* ptr
    )
{
    SharedHandle* handle = lwmsg_hash_find_key(&session->handle_by_ptr, ptr);

    /* If the pointer was not found in the table, it must be a proxy
       that we can cast directly */
    return handle ? handle : (SharedHandle*) ptr;
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
    SharedHandleKey key = {0};

    session_lock(my_session);

    if (lwmsg_hash_get_count(&my_session->handle_by_id) == UINT32_MAX)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_RESOURCE_LIMIT);
    }

    do
    {
        key.type = LWMSG_HANDLE_LOCAL;
        key.id = my_session->next_hid++;
    } while (lwmsg_hash_find_key(&my_session->handle_by_id, &key));

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
        *hid = handle->key.id;
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

    handle = shared_find_handle_by_ptr(my_session, ptr);
    handle->refs++;

    session_unlock(my_session);

    return status;
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

    handle = shared_find_handle_by_ptr(my_session, ptr);
    if (--handle->refs == 0)
    {
        shared_free_handle(my_session, handle);
    }

    session_unlock(my_session);

    return status;
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

    handle = shared_find_handle_by_ptr(my_session, ptr);

    if (!handle->valid)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_INVALID_HANDLE);
    }

    handle->valid = LWMSG_FALSE;

    if (--handle->refs == 0)
    {
        shared_free_handle(my_session, handle);
    }

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

    handle = shared_find_handle_by_ptr(my_session, pointer);

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
        *htype = handle->key.type;
    }
    if (hid)
    {
        *hid = handle->key.id;
    }

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

    handle = shared_find_handle_by_id(my_session, htype, hid);

    if (!handle || !handle->valid)
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    if (type && strcmp(type, handle->type))
    {
        BAIL_ON_ERROR(status = LWMSG_STATUS_NOT_FOUND);
    }

    *pointer = handle->pointer;
    handle->refs++;

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
    handles = lwmsg_hash_get_count(&my_session->handle_by_id);
    session_unlock(my_session);
    return handles;
}

static LWMsgSessionManagerClass shared_class = 
{
    .delete = shared_delete,
    .enter_session = shared_enter_session,
    .leave_session = shared_leave_session,
    .retain_session = shared_retain_session,
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

    BAIL_ON_ERROR(status = lwmsg_hash_init(
                      &my_manager->sessions,
                      31,
                      shared_session_get_key,
                      shared_session_digest,
                      shared_session_equal,
                      offsetof(SharedSession, ring)));

    BAIL_ON_ERROR(status = lwmsg_session_manager_init(&my_manager->base, &shared_class));

    BAIL_ON_ERROR(status = lwmsg_error_map_errno(pthread_mutex_init(&my_manager->lock, NULL)));

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
