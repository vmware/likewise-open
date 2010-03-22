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

#include "dfs.h"

/* Structures */

typedef struct _DFS_LIST {
    LONG MaxSize;
    LONG CurrentSize;

    LW_LIST_LINKS DataList;

    PDFS_LIST_FREE_DATA_FN pfnFreeData;

} DFS_LIST;


/* Functions */

/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsListInit(
    PDFS_LIST *ppNewList,
    DWORD dwMaxSize,
    PDFS_LIST_FREE_DATA_FN pfnFreeData
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PDFS_LIST pList = NULL;

    BAIL_ON_INVALID_PTR(ppNewList, ntStatus);

    ntStatus = RTL_ALLOCATE(&pList, DFS_LIST, sizeof(DFS_LIST));
    BAIL_ON_NT_STATUS(ntStatus);

    pList->MaxSize = dwMaxSize;
    pList->CurrentSize = 0;

    pList->pfnFreeData = pfnFreeData;

    LwListInit(&pList->DataList);

    *ppNewList = pList;
    pList = NULL;

    ntStatus = STATUS_SUCCESS;

cleanup:
    RTL_FREE(&pList);

    return ntStatus;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsListSetMaxSize(
    PDFS_LIST pList,
    DWORD dwNewLength
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(pList, ntStatus);

    /* We can set the List to an unlimited size
       or larger that the current size */

    if ((dwNewLength != 0) && (dwNewLength < pList->CurrentSize))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pList->MaxSize = dwNewLength;

    ntStatus = STATUS_SUCCESS;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsListAddTail(
    PDFS_LIST pList,
    PLW_LIST_LINKS pItem
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(pList, ntStatus);
    BAIL_ON_INVALID_PTR(pItem, ntStatus);

    /* Using >= here to be safe.  Technically, == should
       be fine */

    if (DfsListIsFull(pList)) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LwListInsertTail(&pList->DataList, pItem);
    pList->CurrentSize++;

    ntStatus = STATUS_SUCCESS;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsListRemoveHead(
    PDFS_LIST pList,
    PLW_LIST_LINKS *ppItem
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(pList, ntStatus);
    BAIL_ON_INVALID_PTR(ppItem, ntStatus);

    if (DfsListIsEmpty(pList)) {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppItem = LwListRemoveHead(&pList->DataList);
    pList->CurrentSize--;

    ntStatus = STATUS_SUCCESS;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsListRemoveItem(
    PDFS_LIST pList,
    PLW_LIST_LINKS pItem
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(pList, ntStatus);
    BAIL_ON_INVALID_PTR(pItem, ntStatus);

    if (DfsListIsEmpty(pList))
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LwListRemove(pItem);
    pList->CurrentSize--;

    ntStatus = STATUS_SUCCESS;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

BOOL
DfsListIsEmpty(
    PDFS_LIST pList
    )
{
    BOOLEAN bIsEmpty = FALSE;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pList->CurrentSize == 0)
    {
        bIsEmpty = TRUE;
        if (!LwListIsEmpty(&pList->DataList))
        {
            /* This would be a logic error */
            ntStatus = STATUS_INTERNAL_ERROR;
        }
    }

    return bIsEmpty;
}


/***********************************************************************
 **********************************************************************/

BOOL
DfsListIsFull(
    PDFS_LIST pList
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
DfsListTraverse(
    PDFS_LIST pList,
    PLW_LIST_LINKS pCursor
    )
{
    return LwListTraverse(&pList->DataList, pCursor);
}



/***********************************************************************
 **********************************************************************/

LONG
DfsListLength(
    PDFS_LIST pList
    )
{
    return pList->CurrentSize;
}


/***********************************************************************
 **********************************************************************/

VOID
DfsListDestroy(
    PDFS_LIST *ppList
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PDFS_LIST pList = NULL;
    PLW_LIST_LINKS pData = NULL;

    if (ppList && *ppList)
    {
        pList = *ppList;

        while (!DfsListIsEmpty(pList))
        {
            pData = NULL;

            ntStatus = DfsListRemoveHead(pList, &pData);

            /* Avoid an infinite loop in the case of
               an error.  This means we may leak memory. */
            BAIL_ON_NT_STATUS(ntStatus);

            if (pList->pfnFreeData)
            {
                pList->pfnFreeData((PVOID*)&pData);
            }
            else
            {
                DfsFreeMemory((PVOID*)&pData);
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
