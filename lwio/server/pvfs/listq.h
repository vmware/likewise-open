/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        listq.h
 *
 * Abstract:
 *
 *        Bounded FIFO List data structure
 *
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */


#ifndef _PVFS_LIST_H_
#define _PVFS_LIST_H_

typedef struct _PVFS_LIST *PPVFS_LIST;

typedef VOID  (*PPVFS_LIST_FREE_DATA_FN)(PVOID *ppData);

NTSTATUS
PvfsListInit(
    PPVFS_LIST *ppNewList,
    DWORD dwMaxSize,
    PPVFS_LIST_FREE_DATA_FN pfnFreeData
    );

NTSTATUS
PvfsListSetMaxSize(
    PPVFS_LIST pList,
    DWORD dwNewLength
    );

BOOL
PvfsListIsEmpty(
    PPVFS_LIST pList
    );

BOOL
PvfsListIsFull(
    PPVFS_LIST pList
    );

NTSTATUS
PvfsListAddTail(
    PPVFS_LIST pList,
    PLW_LIST_LINKS pItem
    );

NTSTATUS
PvfsListRemoveHead(
    PPVFS_LIST pList,
    PLW_LIST_LINKS *ppItem
    );

NTSTATUS
PvfsListRemoveItem(
    PPVFS_LIST pList,
    PLW_LIST_LINKS pItem
    );

VOID
PvfsListDestroy(
    PPVFS_LIST *ppList
    );

PLW_LIST_LINKS
PvfsListTraverse(
    PPVFS_LIST pList,
    PLW_LIST_LINKS pCursor
    );

LONG
PvfsListLength(
    PPVFS_LIST pList
    );


#endif    /* _PVFS_LIST_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
