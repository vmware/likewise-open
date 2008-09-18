/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
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

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <lwrpc/types.h>
#include <lwrpc/ntstatus.h>
#include <lwrpc/memptr.h>
#include <lwrpc/allocate.h>


/*
 * Status check macros
 */

#define goto_if_invalid_param_ntstatus(p, lbl) \
    if ((p) == NULL) {                       \
        status = STATUS_INVALID_PARAMETER;   \
        goto lbl;                            \
    }

#define goto_if_no_memory_ntstatus(p, lbl)   \
    if ((p) == NULL) {                       \
        status = STATUS_NO_MEMORY;           \
        goto lbl;                            \
    }

#define goto_if_ntstatus_not_success(s, lbl) \
    if ((s) != STATUS_SUCCESS) {             \
        status = s;                          \
        goto lbl;                            \
    }



/*
 * Locking macros
 */

#define PTR_LIST_LOCK(list)                       \
    do {                                          \
        int ret = 0;                              \
        ret = pthread_mutex_lock(&(list)->mutex); \
        if (ret) {                                \
            status = STATUS_UNSUCCESSFUL;         \
            goto error;                           \
                                                  \
        } else {                                  \
            locked = 1;                           \
        }                                         \
    } while (0);


#define PTR_LIST_UNLOCK(list)                       \
    do {                                            \
        int ret = 0;                                \
        if (!locked) break;                         \
        ret = pthread_mutex_unlock(&(list)->mutex); \
        if (ret && status == STATUS_SUCCESS) {      \
            status = STATUS_UNSUCCESSFUL;           \
                                                    \
        } else {                                    \
            locked = 0;                             \
        }                                           \
    } while (0);



static NTSTATUS MemPtrNodeAppend(PtrList *list, PtrNode *node)
{
    NTSTATUS status = STATUS_SUCCESS;
    PtrNode *last = NULL;
    int locked = 0;

    goto_if_invalid_param_ntstatus(list, done);
    goto_if_invalid_param_ntstatus(node, done);

    PTR_LIST_LOCK(list);

    /* Find the last node */
    last = list->p;
    while (last && last->next) last = last->next;

    if (last == NULL) {
        /* This is the very first node */
        list->p = node;

    } else {
        /* Append the new node */
        last->next = node;
    }

    node->next = NULL;

done:
    PTR_LIST_UNLOCK(list); 
    return status;

error:
    goto done;
}


static NTSTATUS MemPtrNodeRemove(PtrList *list, PtrNode *node)
{
    NTSTATUS status = STATUS_SUCCESS;
    PtrNode *prev = NULL;
    int locked = 0;

    goto_if_invalid_param_ntstatus(list, done);
    goto_if_invalid_param_ntstatus(node, done);

    PTR_LIST_LOCK(list);

    /* Simple case - this happens to be the first node */
    if (node == list->p) {
        list->p = node->next;
        goto done;
    }

    /* Find node that is previous to the requested one */
    prev = list->p;
    while (prev && prev->next != node) {
        prev = prev->next;
    }

    if (prev == NULL) {
        status = STATUS_INVALID_PARAMETER;
        goto error;
    }

    prev->next = node->next;

done:
    PTR_LIST_UNLOCK(list);
    return status;

error:
    goto done;
}


NTSTATUS MemPtrListInit(PtrList **out)
{
    NTSTATUS status = STATUS_SUCCESS;
    PtrList *list = NULL;

    goto_if_invalid_param_ntstatus(out, done);

    list = (PtrList*) malloc(sizeof(PtrList));
    goto_if_no_memory_ntstatus(list, error);

    list->p = NULL;

    /* According to pthread_mutex_init(3) documentation
       the function call always succeeds */
    pthread_mutex_init(&list->mutex, NULL);

    *out = list;

done:
    return status;

error:
    SAFE_FREE(list);
    *out = NULL;
    goto done;
}


NTSTATUS MemPtrListDestroy(PtrList **out)
{
    NTSTATUS status = STATUS_SUCCESS;
    PtrList *list = NULL;
    PtrNode *node = NULL;
    int ret = 0;

    goto_if_invalid_param_ntstatus(out, done);

    list = *out;

    node = list->p;
    while (node) {
        PtrNode *rmnode = NULL;

        SAFE_FREE(node->ptr);

        rmnode = node;
        node = node->next;

        SAFE_FREE(rmnode);
    }

    ret = pthread_mutex_destroy(&list->mutex);
    if (ret) status = STATUS_UNSUCCESSFUL;

    SAFE_FREE(list);
    *out = NULL;

done:
    return status;
}


NTSTATUS MemPtrAllocate(PtrList *list, void **out, size_t size, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;
    void *ptr = NULL;
    PtrNode *node = NULL;

    goto_if_invalid_param_ntstatus(out, done);

    /* Allocate new node */
    node = (PtrNode*) malloc(sizeof(PtrNode));
    goto_if_no_memory_ntstatus(node, done);

    node->ptr  = NULL;
    node->dep  = dep;
    node->size = size;

    if (node->size)
    {
        /* Allocate the actual memory */
        node->ptr = malloc(node->size);
        goto_if_no_memory_ntstatus(node->ptr, error);

        /* ... and initialise it to zero */
        memset(node->ptr, 0, node->size);
    }

    status = MemPtrNodeAppend(list, node);
    goto_if_ntstatus_not_success(status, error);

    *out = node->ptr;

done:
    return status;

error:

    if (node && node->ptr) {
        SAFE_FREE(node->ptr);
    }

    SAFE_FREE(node);
    *out = NULL;

    goto done;
}


NTSTATUS MemPtrFree(PtrList *list, void *ptr)
{
    NTSTATUS status = STATUS_SUCCESS;
    PtrNode *node = NULL;

    goto_if_invalid_param_ntstatus(ptr, done);

    /* Free the pointer and all pointer (nodes) depending on it */
    node = list->p;
    while (node) {
        if (node->dep == ptr || node->ptr == ptr) {
            PtrNode *rmnode = NULL;
            
            /* Move to the next node before removing the current one */
            rmnode = node;
            node   = node->next;

            status = MemPtrNodeRemove(list, rmnode);
            goto_if_ntstatus_not_success(status, done);

            free(rmnode->ptr);
            free(rmnode);
            continue;
        }

        node = node->next;
    }

done:
    return status;
}


NTSTATUS MemPtrAddDependant(PtrList *list, void *ptr, void *dep)
{
    NTSTATUS status = STATUS_SUCCESS;
    PtrNode *node = NULL;

    goto_if_invalid_param_ntstatus(ptr, done);

    /* Allocate new node */
    node = (PtrNode*) malloc(sizeof(PtrNode));
    goto_if_no_memory_ntstatus(node, done);

    node->ptr  = ptr;
    node->dep  = dep;
    node->size = 0;    /* size is unknown when adding dependency */

    status = MemPtrNodeAppend(list, node);
    goto_if_ntstatus_not_success(status, error);

done:
    return status;

error:
    /* Only the node should be freed here. node->ptr should
       be left intact */
    SAFE_FREE(node);
    goto done;
}




/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
