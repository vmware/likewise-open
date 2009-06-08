/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        srv/protocol.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols
 *
 *        Definitions
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __PROTOCOL_API_H__
#define __PROTOCOL_API_H__


typedef struct _LWIO_SRV_FILE
{
    pthread_rwlock_t        mutex;
    pthread_rwlock_t*       pMutex;

    LONG                    refcount;

    USHORT                  fid;

    IO_FILE_HANDLE          hFile;
    PIO_FILE_NAME           pFilename; // physical path on server
    PWSTR                   pwszFilename; // requested path
    ACCESS_MASK             desiredAccess;
    LONG64                  allocationSize;
    FILE_ATTRIBUTES         fileAttributes;
    FILE_SHARE_FLAGS        shareAccess;
    FILE_CREATE_DISPOSITION createDisposition;
    FILE_CREATE_OPTIONS     createOptions;

} LWIO_SRV_FILE, *PLWIO_SRV_FILE;

typedef struct _LWIO_SRV_TREE
{
    LONG                   refcount;

    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    USHORT            tid;

    PSHARE_DB_INFO    pShareInfo;

    PLWRTL_RB_TREE    pFileCollection;

    USHORT            nextAvailableFid;

} LWIO_SRV_TREE, *PLWIO_SRV_TREE;

typedef struct _LWIO_SRV_SESSION
{
    LONG              refcount;

    pthread_rwlock_t   mutex;
    pthread_rwlock_t*  pMutex;

    USHORT            uid;

    PLWRTL_RB_TREE    pTreeCollection;

    HANDLE            hFinderRepository;

    USHORT            nextAvailableTid;

    PSTR              pszClientPrincipalName;

    PIO_CREATE_SECURITY_CONTEXT   pIoSecurityContext;

} LWIO_SRV_SESSION, *PLWIO_SRV_SESSION;

#endif /* __PROTOCOL_API_H__ */
