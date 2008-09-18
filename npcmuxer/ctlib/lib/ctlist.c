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

#include <ctlist.h>

void
CtListInit(
    OUT CT_LIST_LINKS* Head
    )
{
    Head->Next = Head->Prev = Head;
}

bool
CtListIsEmpty(
    IN CT_LIST_LINKS* Head
    )
{
    return (Head->Next == Head);
}

void
CtListInsertAfter(
    IN CT_LIST_LINKS* Head,
    IN CT_LIST_LINKS* Element
    )
{
    Element->Next = Head->Next;
    Element->Prev = Head;
    Head->Next->Prev = Element;
    Head->Next = Element;
}

void
CtListInsertBefore(
    IN CT_LIST_LINKS* Head,
    IN CT_LIST_LINKS* Element
    )
{
    Element->Next = Head;
    Element->Prev = Head->Prev;
    Head->Prev->Next = Element;
    Head->Prev = Element;
}

void
CtListRemove(
    IN CT_LIST_LINKS* Element
    )
{
    Element->Prev->Next = Element->Next;
    Element->Next->Prev = Element->Prev;
}

CT_LIST_LINKS*
CtListRemoveAfter(
    IN CT_LIST_LINKS* Head
    )
{
    CT_LIST_LINKS* element = Head->Next;
    CtListRemove(element);
    return element;
}

CT_LIST_LINKS*
CtListRemoveBefore(
    IN CT_LIST_LINKS* Head
    )
{
    CT_LIST_LINKS* element = Head->Prev;
    CtListRemove(element);
    return element;
}

