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
 *        shareapi.h
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        Share API
 *
 * Authors: Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */

#ifndef __SHAREAPI_H__
#define __SHAREAPI_H__

#define LWIO_SRV_FILE_SYSTEM_PREFIX_A "C:\\"
#define LWIO_SRV_FILE_SYSTEM_PREFIX_W { 'C', ':', '\\', 0 }

#define LWIO_SRV_DEFAULT_SHARE_PATH_A "\\lwcifs"
#define LWIO_SRV_DEFAULT_SHARE_PATH_W { '\\', 'l', 'w', 'c', 'i', 'f', 's', 0 }

#define LWIO_SRV_FILE_SYSTEM_ROOT_A   "\\pvfs"
#define LWIO_SRV_FILE_SYSTEM_ROOT_W   { '\\', 'p', 'v', 'f', 's', 0 }

#define LWIO_SRV_PIPE_SYSTEM_ROOT_A   "\\npfs"
#define LWIO_SRV_PIPE_SYSTEM_ROOT_W   { '\\', 'n', 'p', 'f', 's', 0 }

#define LWIO_SRV_SHARE_STRING_ID_ANY_A "????"
#define LWIO_SRV_SHARE_STRING_ID_ANY_W {'?','?','?','?',0}

#define LWIO_SRV_SHARE_STRING_ID_IPC_A "IPC"
#define LWIO_SRV_SHARE_STRING_ID_IPC_W {'I','P','C',0}

#define LWIO_SRV_SHARE_STRING_ID_COMM_A "COMM"
#define LWIO_SRV_SHARE_STRING_ID_COMM_W {'C','O','M','M',0}

#define LWIO_SRV_SHARE_STRING_ID_PRINTER_A "LPT1:"
#define LWIO_SRV_SHARE_STRING_ID_PRINTER_W {'L','P','T','1',':',0}

#define LWIO_SRV_SHARE_STRING_ID_DISK_A "A:"
#define LWIO_SRV_SHARE_STRING_ID_DISK_W {'A',':',0}

typedef enum
{
    SHARE_SERVICE_DISK_SHARE = 0,
    SHARE_SERVICE_PRINTER,
    SHARE_SERVICE_COMM_DEVICE,
    SHARE_SERVICE_NAMED_PIPE,
    SHARE_SERVICE_ANY,
    SHARE_SERVICE_UNKNOWN

} SHARE_SERVICE;

typedef struct _SRV_SHARE_INFO
{
    LONG refcount;

    pthread_rwlock_t  mutex;
    pthread_rwlock_t* pMutex;

    PWSTR pwszName;
    PWSTR pwszPath;
    PWSTR pwszComment;

    PSECURITY_DESCRIPTOR_RELATIVE pSecDesc;
    ULONG ulSecDescLen;

    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsSecDesc;

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

    PLWRTL_RB_TREE    pShareCollection;

} LWIO_SRV_SHARE_ENTRY_LIST, *PLWIO_SRV_SHARE_ENTRY_LIST;

NTSTATUS
SrvShareInit(
    VOID
    );

NTSTATUS
SrvShareMapIdToServiceStringW(
    IN  SHARE_SERVICE  service,
    OUT PWSTR*         ppwszService
    );

NTSTATUS
SrvShareMapIdToServiceStringA(
    IN  SHARE_SERVICE  service,
    OUT PSTR*          ppszService
    );

NTSTATUS
SrvShareMapServiceStringToIdA(
    IN     PCSTR          pszService,
    IN OUT SHARE_SERVICE* pService
    );

NTSTATUS
SrvShareMapServiceStringToIdW(
    IN     PWSTR          pwszService,
    IN OUT SHARE_SERVICE* pService
    );

NTSTATUS
SrvShareMapFromWindowsPath(
    IN  PWSTR  pwszInputPath,
    OUT PWSTR* ppwszPath
    );

NTSTATUS
SrvShareMapToWindowsPath(
    IN  PWSTR  pwszInputPath,
    OUT PWSTR* ppwszPath
    );

NTSTATUS
SrvGetShareName(
    IN  PCSTR  pszHostname,
    IN  PCSTR  pszDomain,
    IN  PWSTR  pwszPath,
    OUT PWSTR* ppwszSharename
    );

NTSTATUS
SrvGetMaximalShareAccessMask(
    PSRV_SHARE_INFO pShareInfo,
    ACCESS_MASK*   pMask
    );

NTSTATUS
SrvGetGuestShareAccessMask(
    PSRV_SHARE_INFO pShareInfo,
    ACCESS_MASK*   pMask
    );

VOID
SrvShareFreeSecurity(
    IN PSRV_SHARE_INFO pShareInfo
    );

NTSTATUS
SrvShareAccessCheck(
    PSRV_SHARE_INFO pShareInfo,
    PACCESS_TOKEN pToken,
    ACCESS_MASK DesiredAccess,
    PGENERIC_MAPPING pGenericMap,
    PACCESS_MASK pGrantedAccess
    );

NTSTATUS
SrvShareSetSecurity(
    IN  PSRV_SHARE_INFO pShareInfo,
    IN  PSECURITY_DESCRIPTOR_RELATIVE pIncRelSecDesc,
    IN  ULONG ulIncRelSecDescLen
    );

NTSTATUS
SrvShareInitList(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList
    );

NTSTATUS
SrvShareFindByName(
    IN  PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN  PWSTR                      pwszShareName,
    OUT PSRV_SHARE_INFO*           ppShareInfo
    );

NTSTATUS
SrvShareAdd(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName,
    IN     PWSTR                      pwszPath,
    IN     PWSTR                      pwszComment,
    IN     PBYTE                      pSecDesc,
    IN     ULONG                      ulSecDescLen,
    IN     PWSTR                      pwszShareType
    );

NTSTATUS
SrvShareUpdate(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN     PSRV_SHARE_INFO            pShareInfo
    );

NTSTATUS
SrvShareDelete(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    IN     PWSTR                      pwszShareName
    );

NTSTATUS
SrvShareEnum(
    IN     PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    OUT    PSRV_SHARE_INFO**          pppShareInfo,
    IN OUT PULONG                     pulNumEntries
    );

NTSTATUS
SrvShareDuplicateInfo(
    PSRV_SHARE_INFO  pShareInfo,
    PSRV_SHARE_INFO* ppShareInfo
    );

VOID
SrvShareFreeListContents(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList
    );

VOID
SrvShareFreeEntry(
    IN PSRV_SHARE_ENTRY pShareEntry
    );

VOID
SrvShareFreeInfoList(
    PSRV_SHARE_INFO* ppInfoList,
    ULONG            ulNumInfos
    );

PSRV_SHARE_INFO
SrvShareAcquireInfo(
    IN PSRV_SHARE_INFO pShareInfo
    );

VOID
SrvShareReleaseInfo(
    IN PSRV_SHARE_INFO pShareInfo
    );

NTSTATUS
SrvShareShutdown(
    VOID
    );

#endif /* __SHAREAPI_H__ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
