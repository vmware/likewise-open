/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise Distributed File System Driver (DFS)
 *
 *        Dfs Driver internal structures
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#ifndef _DFS_STRUCTS_H
#define _DFS_STRUCTS_H

typedef struct _DFS_OBJECT_COUNTER
{
    LONG IrpContextCount;
    LONG FcbCount;
    LONG CcbCount;

} DFS_OBJECT_COUNTER, *PDFS_OBJECT_COUNTER;

typedef struct _DFS_FCB_TABLE
{
    pthread_rwlock_t rwLock;

    PLWRTL_RB_TREE pFcbTree;

} DFS_FCB_TABLE, *PDFS_FCB_TABLE;

typedef struct _DFS_IRP_CONTEXT DFS_IRP_CONTEXT, *PDFS_IRP_CONTEXT;

#define DFS_IRP_CTX_FLAG_NONE             0x0000
#define DFS_IRP_CTX_FLAG_CANCELLED        0x0001
#define DFS_IRP_CTX_FLAG_PENDED           0x0002
#define DFS_IRP_CTX_FLAG_ACTIVE           0x0004
#define DFS_IRP_CTX_FLAG_COMPLETE         0x0008
#define DFS_IRP_CTX_FLAG_REQUEST_CANCEL   0x0010

struct _DFS_IRP_CONTEXT
{
    pthread_mutex_t Mutex;
    LONG RefCount;

    USHORT Flags;

    PIRP pIrp;
};

typedef struct _DFS_FCB
{
    LONG RefCount;

    /* ControlBlock */
    pthread_mutex_t ControlBlock;   /* For ensuring atomic operations
                                       on an individual FCB */
    PSTR pszPathname;
    BOOLEAN bRemoved;
    /* End of ControlBlock */

    /* rwCcbLock */
    pthread_rwlock_t rwCcbLock;     /* For managing the CCB list */
    PDFS_LIST pCcbList;
    /* End rwCcbLock */

} DFS_FCB, *PDFS_FCB;

typedef struct _DFS_CCB
{
    LW_LIST_LINKS FcbList;

    pthread_mutex_t ControlBlock;   /* Use for CCB SetFileInfo operations */

    LONG RefCount;
    PDFS_FCB pFcb;

    PSTR pszPathname;

} DFS_CCB, *PDFS_CCB;


#endif    /* _DFS_STRUCTS_H */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
