/*
 * Copyright Likewise Software    2004-2009
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
 *        shares/structs.h
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        Share API
 *
 *        structures
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __SRV_SHARES_STRUCTS_H__
#define __SRV_SHARES_STRUCTS_H__

typedef struct _SRV_SHARE_INFO
{
    LONG refcount;

    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    PWSTR pwszName;
    PWSTR pwszPath;
    PWSTR pwszComment;
    PBYTE pSecDesc;
    ULONG ulSecDescLen;

    SHARE_SERVICE service;

    BOOLEAN bMarkedForDeletion;

} SRV_SHARE_INFO, *PSRV_SHARE_INFO;

typedef struct _SRV_SHARE_ENTRY
{
    PSRV_SHARE_INFO pInfo;

    struct _SRV_SHARE_ENTRY  *pNext;

} SRV_SHARE_ENTRY, *PSRV_SHARE_ENTRY;

typedef struct _LWIO_SRV_SHARE_ENTRY_LIST
{
    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    PSRV_SHARE_ENTRY  pShareEntry;

} LWIO_SRV_SHARE_ENTRY_LIST, *PLWIO_SRV_SHARE_ENTRY_LIST;

#endif /* __SRV_SHARES_STRUCTS_H__ */
