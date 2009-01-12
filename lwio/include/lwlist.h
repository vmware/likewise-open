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

/**
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * @file
 *
 *     lwlist.h
 *
 * @brief
 *
 *     Embedded Linked Lists API
 *
 * @details
 *
 *     This API lets you manipulate embedded linked lists.  These
 *     at linked lists where the links are stored inside the object,
 *     thus allowing efficient (and simple) manipulation of lists
 *     containing the object.  (For example, you can remove an object
 *     from a list only by using a reference to the object since the
 *     object conains the list pointers and w/o doing any searching.
 *     Another example is that you never need to allocate extra
 *     memory when adding an element to a list.)
 *
 * @author Danilo Almeida (dalmeida@likewisesoftware.com)
 *
 */

#ifndef __LWLIST_H__
#define __LWLIST_H__

// ISSUE-2008/07/30-dalmeida -- Pull in proper headers for dependencies.

// ISSUE-2008/07/30-dalmeida -- Put these memory macros elsewhere.
#define LW_PTR_OFFSET(BasePointer, Pointer) \
    ((int)((char*)(Pointer) - (char*)(BasePointer)))

#define LW_PTR_ADD(Pointer, Offset) \
    ((char*)(Pointer) + Offset)

#define LW_FIELD_OFFSET(Type, Field) \
    ((size_t)(&(((Type*)(0))->Field)))

#define LW_STRUCT_FROM_FIELD(Pointer, Type, Field) \
    ((Type*)LW_PTR_ADD(Pointer, -((ssize_t)LW_FIELD_OFFSET(Type, Field))))

#define LW_ARRAY_SIZE(StaticArray) \
    (sizeof(StaticArray)/sizeof((StaticArray)[0]))


///
/// Doubly-linked list structure.
///
typedef struct _LW_LIST_LINKS {
    struct _LW_LIST_LINKS* Next;
    struct _LW_LIST_LINKS* Prev;
} LW_LIST_LINKS, *PLW_LIST_LINKS;

VOID
LwListInit(
    OUT LW_LIST_LINKS* Head
    );

BOOLEAN
LwListIsEmpty(
    IN LW_LIST_LINKS* Head
    );

VOID
LwListInsertAfter(
    IN LW_LIST_LINKS* Head,
    IN LW_LIST_LINKS* Element
    );

VOID
LwListInsertBefore(
    IN LW_LIST_LINKS* Head,
    IN LW_LIST_LINKS* Element
    );

VOID
LwListRemove(
    IN LW_LIST_LINKS* Element
    );

LW_LIST_LINKS*
LwListRemoveAfter(
    IN LW_LIST_LINKS* Head
    );

LW_LIST_LINKS*
LwListRemoveBefore(
    IN LW_LIST_LINKS* Head
    );

#define LwListInsertHead LwListInsertAfter
#define LwListInsertTail LwListInsertBefore

#define LwListRemoveHead LwListRemoveAfter
#define LwListRemoveTail LwListRemoveBefore

#endif /* __LWLIST_H__ */
