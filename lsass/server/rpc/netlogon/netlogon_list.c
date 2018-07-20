/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2018
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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        netlogon_list.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Netlogon listed list manager
 *
 * Authors: Adam Bernstein (abernstein@vmware.com)
 */
#include <includes.h>


struct _listnode {
    struct _listnode *next;
    unsigned char v[];
};


int list_new(
    listhead **new_list,
    void (*delete_node)(listnode *))
{
    listhead *head_list = NULL;
    int sts = 0;

    if (new_list && *new_list)
    {
        /* Already initialized, do nothing */
        goto cleanup;
    }

    head_list = calloc(1, sizeof(listhead));
    if (!head_list)
    {
        sts = STATUS_NO_MEMORY;
        goto cleanup;
    }

    head_list->delete_node = delete_node;
    *new_list = head_list;

cleanup:
    return sts;
}

int list_free(
    listhead *list)
{
    int status = 0;

    if (!list || list->count)
    {
        goto cleanup;
    }
    free(list);

cleanup:
    return status;
}


int list_insert_node(
    listhead *list,
    listnode *node)
{
    int status = 0;

    if (list->head)
    {
        node->next = list->head;
        list->head = node;
    }
    else
    {
        list->head = node;
    }
    list->count++;

    return status;
}


int list_remove_node(
    listhead *list,
    listnode *node)
{
    listnode *prev_node = NULL;

    /* Empty list, do nothing */
    if (!list->head || !node)
    {
        return 0;
    }

    /* Node to delete is head of list */
    if (list->head == node)
    {
        list->delete_node(list->head);
        list->head = NULL;
        list->count--;
    }

    /* Node to delete is in the list, search for it */
    prev_node = list->head;
    while (prev_node && prev_node->next && prev_node->next != node)
    {
        prev_node = prev_node->next;
    }

    /* Holding pointer to previous entry in list, unlink node and delete */
    if (prev_node && prev_node->next == node)
    {
        prev_node->next = node->next;
        list->delete_node(node);
        list->count--;
    }
    return 0;
}

int list_iterate_func(
    listhead *list,
    int (*func)(listnode *, listnode *),
    listnode *find_node,
    listnode **ret_node)
{
    int sts = 0;
    int found = 0;
    listnode *entry = NULL;

    if (!list || !list->head)
    {
        sts = ENOENT;
        goto cleanup;
    }

    entry = list->head;
    while (entry)
    {
        if (func(find_node, entry))
        {
            found = 1;
            break;
        }
        entry = entry->next;
    }

    if (found)
    {
        *ret_node = entry;
    }
    else
    {
        sts = ENOENT;
        goto cleanup;
    }

cleanup:
    return sts;
}

