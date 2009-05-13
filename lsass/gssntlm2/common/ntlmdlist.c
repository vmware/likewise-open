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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ntlmdlist.c
 *
 * Abstract:
 *
 *        List functions
 *
 * Author: Todd Stecher (2007)
 *
 */

#include <ntlmcommon.h>

NTLM_LIST*
NTLMListAppend(
    NTLM_LIST *list,
    NTLM_LIST *new_node
    )
{
    NTLM_LIST *last;

    last = NTLMListLast(list);
    last->next = new_node;
    new_node->prev = last;

    return list;
}

NTLM_LIST*
NTLMListPrepend(
    NTLM_LIST	 *list,
    NTLM_LIST	 *new_node
    )
{
    if (!list)
        return new_node;

    new_node->prev = list->prev;
    if (list->prev)
        list->prev->next = new_node;
    list->prev = new_node;
    new_node->next = list;

  return new_node;
}

NTLM_LIST*
NTLMListFindNode(
    NTLM_LIST         *list,
    NTLM_LIST         *toFind
    )
{
    while (list)
    {
        if (list == toFind)
            break;
        list = list->next;
    }

    return list;
}

NTLM_LIST*
NTLMRemoveLink(
    NTLM_LIST *list,
    NTLM_LIST *link
    )
{
    if (link)
    {
        if (link->prev)
            link->prev->next = link->next;
        if (link->next)
            link->next->prev = link->prev;

        if (link == list)
            list = list->next;

        link->next = NULL;
        link->prev = NULL;
    }

    return list;
}

NTLM_LIST*
NTLMListRemove(
    NTLM_LIST   *list,
    NTLM_LIST   *toRemove
    )
{
    NTLM_LIST *tmp = list;

    while (tmp)
    {
        if (tmp != toRemove)
            tmp = tmp->next;
        else
        {
            list = NTLMRemoveLink(list, tmp);
            break;
        }
    }

    return list;
}


NTLM_LIST*
NTLMListLast(
    NTLM_LIST *list
    )
{
    if (list)
    {
        while (list->next)
            list = list->next;
    }

    return list;
}

NTLM_LIST*
NTLMListFirst (
    NTLM_LIST *list
    )
{
    if (list)
    {
        while (list->prev)
            list = list->prev;
    }

    return list;
}


