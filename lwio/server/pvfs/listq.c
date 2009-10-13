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
 *        listq.c
 *
 * Abstract:
 *
 *        Bounded FIFO List data structure with some List properties
 *
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Structures */

typedef struct _PVFS_LIST {
    LONG MaxSize;
    LONG CurrentSize;

    LW_LIST_LINKS DataList;

    PPVFS_LIST_FREE_DATA_FN pfnFreeData;

} PVFS_LIST;


/* Functions */

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsListInit(
    PPVFS_LIST *ppNewList,
    DWORD dwMaxSize,
    PPVFS_LIST_FREE_DATA_FN pfnFreeData
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_LIST pList = NULL;

    BAIL_ON_INVALID_PTR(ppNewList, ntError);

    ntError = RTL_ALLOCATE(&pList, PVFS_LIST, sizeof(PVFS_LIST));
    BAIL_ON_NT_STATUS(ntError);

    pList->MaxSize = dwMaxSize;
    pList->CurrentSize = 0;

    pList->pfnFreeData = pfnFreeData;

    LwListInit(&pList->DataList);

    *ppNewList = pList;
    pList = NULL;

    ntError = STATUS_SUCCESS;

cleanup:
    RTL_FREE(&pList);

    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsListSetMaxSize(
    PPVFS_LIST pList,
    DWORD dwNewLength
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(pList, ntError);

    /* We can set the List to an unlimited size
       or larger that the current size */

    if ((dwNewLength != 0) && (dwNewLength < pList->CurrentSize))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pList->MaxSize = dwNewLength;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsListAddTail(
    PPVFS_LIST pList,
    PLW_LIST_LINKS pItem
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(pList, ntError);
    BAIL_ON_INVALID_PTR(pItem, ntError);

    /* Using >= here to be safe.  Technically, == should
       be fine */

    if (PvfsListIsFull(pList)) {
        ntError = STATUS_INSUFFICIENT_RESOURCES;
        BAIL_ON_NT_STATUS(ntError);
    }

    LwListInsertTail(&pList->DataList, pItem);
    pList->CurrentSize++;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsListRemoveHead(
    PPVFS_LIST pList,
    PLW_LIST_LINKS *ppItem
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(pList, ntError);
    BAIL_ON_INVALID_PTR(ppItem, ntError);

    if (PvfsListIsEmpty(pList)) {
        ntError = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntError);
    }

    *ppItem = LwListRemoveHead(&pList->DataList);
    pList->CurrentSize--;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsListRemoveItem(
    PPVFS_LIST pList,
    PLW_LIST_LINKS pItem
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(pList, ntError);
    BAIL_ON_INVALID_PTR(pItem, ntError);

    PVFS_ASSERT(!PvfsListIsEmpty(pList));

    LwListRemove(pItem);
    pList->CurrentSize--;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

BOOL
PvfsListIsEmpty(
    PPVFS_LIST pList
    )
{
    BOOLEAN bIsEmpty = FALSE;

    if (pList->CurrentSize == 0)
    {
        bIsEmpty = TRUE;
        PVFS_ASSERT(LwListIsEmpty(&pList->DataList));
    }

    return bIsEmpty;
}


/***********************************************************************
 **********************************************************************/

BOOL
PvfsListIsFull(
    PPVFS_LIST pList
    )
{
    if (pList->MaxSize == 0) {
        return FALSE;
    }

    return (pList->CurrentSize >= pList->MaxSize);
}


/***********************************************************************
 **********************************************************************/

PLW_LIST_LINKS
PvfsListTraverse(
    PPVFS_LIST pList,
    PLW_LIST_LINKS pCursor
    )
{
    return LwListTraverse(&pList->DataList, pCursor);
}



/***********************************************************************
 **********************************************************************/

LONG
PvfsListLength(
    PPVFS_LIST pList
    )
{
    return pList->CurrentSize;
}


/***********************************************************************
 **********************************************************************/

VOID
PvfsListDestroy(
    PPVFS_LIST *ppList
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_LIST pList = NULL;
    PLW_LIST_LINKS pData = NULL;

    if (ppList && *ppList)
    {
        pList = *ppList;

        while (!PvfsListIsEmpty(pList))
        {
            pData = NULL;

            ntError = PvfsListRemoveHead(pList, &pData);

            /* Avoid an infinite loop in the case of
               an error.  This means we may leak memory. */
            BAIL_ON_NT_STATUS(ntError);

            if (pList->pfnFreeData)
            {
                pList->pfnFreeData((PVOID*)&pData);
            }
            else
            {
                PVFS_FREE(&pData);
            }
        }

        RTL_FREE(ppList);
    }

cleanup:
    return;

error:
    goto cleanup;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
