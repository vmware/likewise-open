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

#define DFS_IRP_CTX_FLAG_NONE             0x0000
#define DFS_IRP_CTX_FLAG_CANCELLED        0x0001
#define DFS_IRP_CTX_FLAG_PENDED           0x0002
#define DFS_IRP_CTX_FLAG_ACTIVE           0x0004
#define DFS_IRP_CTX_FLAG_COMPLETE         0x0008
#define DFS_IRP_CTX_FLAG_REQUEST_CANCEL   0x0010

typedef struct _DFS_IRP_CONTEXT
{
    pthread_mutex_t Mutex;
    LONG RefCount;

    USHORT Flags;

    PIRP pIrp;
} DFS_IRP_CONTEXT, *PDFS_IRP_CONTEXT;


typedef struct _DFS_ROOT_CONTROL_BLOCK
{
    pthread_rwlock_t RwLock;

    LONG RefCount;

    PSTR pszRootName;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor;

    // DFS_REFERRAL_TABLE ReferralTable;

} DFS_ROOT_CONTROL_BLOCK, *PDFS_ROOT_CONTROL_BLOCK;

typedef struct _DFS_REFERRAL_CONTROL_BLOCK
{
    pthread_rwlock_t RwLock;

    LONG RefCount;

    PSTR ReferralName;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor;

    LW_LIST_LINKS TargetList;

} DFS_REFERRAL_CONTROL_BLOCK, *PDFS_REFERRAL_CONTROL_BLOCK;

typedef struct _DFS_REFERRAL_TARGET
{
    LW_LIST_LINKS ReferralLink;

    pthread_mutex_t Mutex;

    LONG RefCount;

    ULONG Ttl;

    PSTR pszTargetPath;

} DFS_REFERRAL_TARGET, *PDFS_REFERRAL_TARGET;

typedef struct _DFS_CREATE_CONTROL_BLOCK
{
    pthread_mutex_t Mutex;

    LONG RefCount;

    PSTR pszFilename;
    ACCESS_MASK GrantedAccess;


} DFS_CREATE_CONTROL_BLOCK, *PDFS_CREATE_CONTROL_BLOCK;







#endif    /* _DFS_STRUCTS_H */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
