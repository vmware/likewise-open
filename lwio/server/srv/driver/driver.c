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
 *        driver.c
 *
 * Abstract:
 *
 *        Likewise I/O (LWIO) - SRV
 *
 *        Driver
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
NTSTATUS
SrvDriverDispatch(
    IN IO_DEVICE_HANDLE hDevice,
    IN PIRP pIrp
    );

static NTSTATUS
SrvDriverRefresh(
    IN IO_DRIVER_HANDLE DriverHandle
    );

static
VOID
SrvDriverShutdown(
    IN IO_DRIVER_HANDLE hDriver
    );

static
NTSTATUS
SrvInitialize(
    IO_DEVICE_HANDLE hDevice
    );

static
NTSTATUS
SrvShareBootstrapNamedPipeRoot(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList
    );

static
NTSTATUS
SrvShareBootstrapDiskRoot(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList
    );

static
NTSTATUS
SrvGetDefaultSharePath(
    PSTR* ppszFileSystemRoot
    );

static
NTSTATUS
SrvCreateDefaultSharePath(
    PWSTR pwszDefaultSharePath
    );

static
NTSTATUS
SrvBuildDefaultShareSD(
    PSECURITY_DESCRIPTOR_RELATIVE* ppSecDesc,
    PULONG pulRelSecDescLen
    );

static
NTSTATUS
SrvShutdown(
    VOID
    );

static
VOID
SrvUnblockOneWorker(
    IN PSMB_PROD_CONS_QUEUE pWorkQueue
    );

NTSTATUS
IO_DRIVER_ENTRY(srv)(
    IN IO_DRIVER_HANDLE hDriver,
    IN ULONG ulInterfaceVersion
    )
{
    NTSTATUS ntStatus = 0;
    PCSTR    pszName  = "srv";
    PVOID    pDeviceContext = NULL;
    IO_DEVICE_HANDLE hDevice = NULL;

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != ulInterfaceVersion)
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = IoDriverInitialize(
                    hDriver,
                    NULL,
                    SrvDriverShutdown,
                    SrvDriverDispatch);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoDeviceCreate(
                    &hDevice,
                    hDriver,
                    pszName,
                    pDeviceContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvInitialize(hDevice);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoDriverRegisterRefreshCallback(
                    hDriver,
                    SrvDriverRefresh);
    BAIL_ON_NT_STATUS(ntStatus);

    hDevice = NULL;

cleanup:

    return ntStatus;

error:

    if (hDevice)
    {
        IoDeviceDelete(&hDevice);
    }

    goto cleanup;
}

static
NTSTATUS
SrvDriverDispatch(
    IN IO_DEVICE_HANDLE hDevice,
    IN PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;

    switch (pIrp->Type)
    {
        case IRP_TYPE_CREATE:

            ntStatus = SrvDeviceCreate(
                            hDevice,
                            pIrp);
            break;

        case IRP_TYPE_CLOSE:

            ntStatus = SrvDeviceClose(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_READ:

            ntStatus = SrvDeviceRead(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_WRITE:

            ntStatus = SrvDeviceWrite(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_DEVICE_IO_CONTROL:

            ntStatus = SrvDeviceControlIo(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_FS_CONTROL:

            ntStatus = SrvDeviceControlFS(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_FLUSH_BUFFERS:

            ntStatus = SrvDeviceFlush(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_QUERY_INFORMATION:

            ntStatus = SrvDeviceQueryInfo(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_SET_INFORMATION:

            ntStatus = SrvDeviceSetInfo(
                            hDevice,
                            pIrp);

            break;

        default:

            ntStatus = STATUS_UNSUCCESSFUL;
            BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}

static NTSTATUS
SrvDriverRefresh(
    IN IO_DRIVER_HANDLE DriverHandle
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
VOID
SrvDriverShutdown(
    IN IO_DRIVER_HANDLE hDriver
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = SrvShutdown();
    BAIL_ON_NT_STATUS(ntStatus);

    if (gSMBSrvGlobals.hDevice)
    {
        IoDeviceDelete(&gSMBSrvGlobals.hDevice);
    }

cleanup:

    return;

error:

    if (ntStatus)
    {
        LWIO_LOG_ERROR("[srv] driver failed to stop. [code: %d]", ntStatus);
    }

    goto cleanup;
}

static
NTSTATUS
SrvInitialize(
    IO_DEVICE_HANDLE hDevice
    )
{
    NTSTATUS ntStatus = 0;
    INT      iWorker = 0;

    memset(&gSMBSrvGlobals, 0, sizeof(gSMBSrvGlobals));

    pthread_mutex_init(&gSMBSrvGlobals.mutex, NULL);
    gSMBSrvGlobals.pMutex = &gSMBSrvGlobals.mutex;

    ntStatus = SrvUtilsInitialize();
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvInitConfig(&gSMBSrvGlobals.config);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvReadConfig(&gSMBSrvGlobals.config);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketCreateAllocator(
                    gSMBSrvGlobals.config.ulMaxNumPackets,
                    &gSMBSrvGlobals.hPacketAllocator);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProdConsInitContents(
                    &gSMBSrvGlobals.workQueue,
                    gSMBSrvGlobals.config.ulMaxNumWorkItemsInQueue,
                    &SrvReleaseExecContextHandle);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareInit();
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareInitList(&gSMBSrvGlobals.shareList);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareBootstrapNamedPipeRoot(&gSMBSrvGlobals.shareList);
    BAIL_ON_NT_STATUS(ntStatus);

    if (gSMBSrvGlobals.config.bBootstrapDefaultSharePath)
    {
        ntStatus = SrvShareBootstrapDiskRoot(&gSMBSrvGlobals.shareList);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvOEMInitialize();
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvElementsInit();
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvStatisticsInitialize();
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProtocolInit(
                    &gSMBSrvGlobals.workQueue,
                    gSMBSrvGlobals.hPacketAllocator,
                    &gSMBSrvGlobals.shareList);
    BAIL_ON_NT_STATUS(ntStatus);


    if (gSMBSrvGlobals.config.ulMonitorIntervalMinutes)
    {
        ntStatus = SrvMonitorCreate(
                    gSMBSrvGlobals.config.ulMonitorIntervalMinutes,
                    &gSMBSrvGlobals.pMonitor);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvMonitorStart(gSMBSrvGlobals.pMonitor);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(
                    gSMBSrvGlobals.config.ulNumWorkers * sizeof(LWIO_SRV_WORKER),
                    (PVOID*)&gSMBSrvGlobals.pWorkerArray);
    BAIL_ON_NT_STATUS(ntStatus);

    gSMBSrvGlobals.ulNumWorkers = gSMBSrvGlobals.config.ulNumWorkers;

    for (; iWorker < gSMBSrvGlobals.config.ulNumWorkers; iWorker++)
    {
        PLWIO_SRV_WORKER pWorker = &gSMBSrvGlobals.pWorkerArray[iWorker];

        pWorker->workerId = iWorker + 1;

        ntStatus = SrvWorkerInit(pWorker);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    gSMBSrvGlobals.hDevice = hDevice;

error:

    return ntStatus;
}

static
NTSTATUS
SrvShareBootstrapNamedPipeRoot(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    wchar16_t wszPipeRootName[] = {'I','P','C','$',0};
    PSRV_SHARE_INFO pShareInfo = NULL;

    ntStatus = SrvShareFindByName(
                    pShareList,
                    &wszPipeRootName[0],
                    &pShareInfo);
    if (ntStatus == STATUS_NOT_FOUND)
    {
        wchar16_t wszPipeSystemRoot[] = LWIO_SRV_PIPE_SYSTEM_ROOT_W;
        wchar16_t wszServiceType[] = LWIO_SRV_SHARE_STRING_ID_IPC_W;
        wchar16_t wszDesc[] = {'R','e','m','o','t','e',' ','I','P','C',0};

        ntStatus = SrvShareAdd(
                            pShareList,
                            &wszPipeRootName[0],
                            &wszPipeSystemRoot[0],
                            &wszDesc[0],
                            NULL,
                            0,
                            &wszServiceType[0],
                            0);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (pShareInfo)
    {
        SrvShareReleaseInfo(pShareInfo);
    }

    return ntStatus;

error:

    LWIO_LOG_ERROR("Failed to bootstrap default IPC$ shares. [error code: %d]",
                   ntStatus);

    goto cleanup;
}

static
NTSTATUS
SrvShareBootstrapDiskRoot(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    wchar16_t wszFileRootName[] = {'C','$',0};
    PSTR  pszDefaultSharePath = NULL;
    PWSTR pwszFileSystemRoot = NULL;
    PSRV_SHARE_INFO pShareInfo = NULL;

    ntStatus = SrvShareFindByName(
                        pShareList,
                        &wszFileRootName[0],
                        &pShareInfo);
    if (ntStatus == STATUS_NOT_FOUND)
    {
        wchar16_t wszDesc[] =
                        {'D','e','f','a','u','l','t',' ','S','h','a','r','e',0};
        wchar16_t wszServiceType[] = LWIO_SRV_SHARE_STRING_ID_DISK_W;

        ntStatus = SrvGetDefaultSharePath(&pszDefaultSharePath);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvMbsToWc16s(pszDefaultSharePath, &pwszFileSystemRoot);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvCreateDefaultSharePath(pwszFileSystemRoot);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvShareAdd(
                        pShareList,
                        &wszFileRootName[0],
                        pwszFileSystemRoot,
                        &wszDesc[0],
                        NULL,
                        0,
                        &wszServiceType[0],
                        0);
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SRV_SAFE_FREE_MEMORY(pszDefaultSharePath);
    SRV_SAFE_FREE_MEMORY(pwszFileSystemRoot);

    return ntStatus;

error:

    LWIO_LOG_ERROR("Failed to bootstrap default shares. [error code: %d]",
                   ntStatus);

    goto cleanup;
}

static
NTSTATUS
SrvGetDefaultSharePath(
    PSTR* ppszFileSystemRoot
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR     pszDefaultSharePath = NULL;
    PSTR     pszFileSystemRoot = NULL;
    PSTR     pszCursor = NULL;
    BOOLEAN  bUsePolicy = TRUE;
    CHAR     szTmpFSRoot[] = LWIO_SRV_FILE_SYSTEM_ROOT_A;
    PLWIO_CONFIG_REG pReg = NULL;

    ntStatus = LwIoOpenConfig(
                        "Services\\lwio\\Parameters\\Drivers\\srv",
                        "Policy\\Services\\lwio\\Parameters\\Drivers\\srv",
                        &pReg);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwIoReadConfigString(
                    pReg,
                    "DefaultSharePath",
                    bUsePolicy,
                    &pszDefaultSharePath);
    BAIL_ON_NT_STATUS(ntStatus);

    if (IsNullOrEmptyString(pszDefaultSharePath) ||
            ((*pszDefaultSharePath != '/') &&
             (*pszDefaultSharePath != '\\')))
    {
        LWIO_LOG_ERROR("Error: Default share path configured is not an absolute path");

        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateStringPrintf(
                    &pszFileSystemRoot,
                    "%s%s%s",
                    &szTmpFSRoot[0],
                    (((szTmpFSRoot[strlen(&szTmpFSRoot[0])-1] == '/') ||
                      (szTmpFSRoot[strlen(&szTmpFSRoot[0])-1] == '\\')) ? "" : "\\"),
                    IsNullOrEmptyString(pszDefaultSharePath+1) ? "" : pszDefaultSharePath+1);
    BAIL_ON_NT_STATUS(ntStatus);

    for (pszCursor = pszFileSystemRoot; pszCursor && *pszCursor; pszCursor++)
    {
        if (*pszCursor == '/')
        {
            *pszCursor = '\\';
        }
    }

    *ppszFileSystemRoot = pszFileSystemRoot;

cleanup:

    if (pReg)
    {
        LwIoCloseConfig(pReg);
    }

    RTL_FREE(&pszDefaultSharePath);

    return ntStatus;

error:

    *ppszFileSystemRoot = NULL;

    LWIO_LOG_ERROR("Failed to access device configuration [error code: %u]",
                   ntStatus);

    ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;

    goto cleanup;
}

static
NTSTATUS
SrvCreateDefaultSharePath(
    PWSTR pwszDefaultSharePath
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_RELATIVE pRelSecDesc = NULL;
    ULONG ulRelSecDescLen = 0;
    IO_FILE_HANDLE hFile = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    IO_FILE_NAME filename = {0};
    PIO_CREATE_SECURITY_CONTEXT pSecContext = NULL;
    SECURITY_INFORMATION secInfo = (OWNER_SECURITY_INFORMATION|
                                    GROUP_SECURITY_INFORMATION|
                                    DACL_SECURITY_INFORMATION);

    ntStatus = SrvBuildDefaultShareSD(&pRelSecDesc, &ulRelSecDescLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoSecurityCreateSecurityContextFromUidGid(
                    &pSecContext,
                    0,
                    0,
                    NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    filename.FileName = pwszDefaultSharePath;

    ntStatus = IoCreateFile(
                   &hFile,
                   NULL,
                   &ioStatusBlock,
                   pSecContext,
                   &filename,
                   NULL,                  /* SecDesc */
                   NULL,                  /* Security QOS        */
                   WRITE_OWNER|WRITE_DAC|READ_CONTROL,
                   0,                     /* Allocation Size     */
                   FILE_ATTRIBUTE_NORMAL, /* File Attributes     */
                   0,                     /* No Sharing          */
                   FILE_OPEN_IF,
                   FILE_DIRECTORY_FILE,
                   NULL,                  /* Extended Attributes */
                   0,                     /* EA Length           */
                   NULL);                 /* ECP List            */
    BAIL_ON_NT_STATUS(ntStatus);

    if (ioStatusBlock.CreateResult == FILE_CREATED)
    {
        ntStatus = IoSetSecurityFile(
                       hFile,
                       NULL,
                       &ioStatusBlock,
                       secInfo,
                       pRelSecDesc,
                       ulRelSecDescLen);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    if (pRelSecDesc)
    {
        SrvFreeMemory(pRelSecDesc);
    }

    IoSecurityDereferenceSecurityContext(&pSecContext);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildDefaultShareSD(
    PSECURITY_DESCRIPTOR_RELATIVE* ppSecDesc,
    PULONG pulRelSecDescLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_RELATIVE pRelSecDesc = NULL;
    ULONG ulRelSecDescLen = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsSecDesc = NULL;
    DWORD dwAceCount = 0;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;
    DWORD dwSizeDacl = 0;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } localSystemSid;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } administratorsSid;
    union
    {
        SID sid;
        BYTE buffer[SID_MAX_SIZE];
    } worldSid;
    ULONG ulLocalSystemSidSize = sizeof(localSystemSid.buffer);
    ULONG ulAdministratorsSidSize = sizeof(administratorsSid.buffer);
    ULONG ulWorldSidSize = sizeof(worldSid.buffer);
    ACCESS_MASK worldAccessMask = 0;
    ULONG ulAceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;

    /* Build the new Absolute Security Descriptor */

    ntStatus = RTL_ALLOCATE(
                   &pAbsSecDesc,
                   VOID,
                   SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlCreateSecurityDescriptorAbsolute(
                  pAbsSecDesc,
                  SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Create some SIDs */

    ntStatus = RtlCreateWellKnownSid(
                   WinLocalSystemSid,
                   NULL,
                   (PSID)localSystemSid.buffer,
                   &ulLocalSystemSidSize);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlCreateWellKnownSid(
                   WinBuiltinAdministratorsSid,
                   NULL,
                   (PSID)administratorsSid.buffer,
                   &ulAdministratorsSidSize);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlCreateWellKnownSid(
                    WinWorldSid,
                    NULL,
                    (PSID)worldSid.buffer,
                    &ulWorldSidSize);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Owner: Local System */

    ntStatus = RtlDuplicateSid(&pOwnerSid, &localSystemSid.sid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlSetOwnerSecurityDescriptor(
                   pAbsSecDesc,
                   pOwnerSid,
                   FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Group: Administrators */

    ntStatus = RtlDuplicateSid(&pGroupSid, &administratorsSid.sid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlSetGroupSecurityDescriptor(
                   pAbsSecDesc,
                   pGroupSid,
                   FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /* DACL:
       LocalSystem    - (Full Control)
       Administrators - (Full Control)
       Everyone       - (Read && Execute && List Directory Contents)
     */

    dwAceCount = 3;

    dwSizeDacl = ACL_HEADER_SIZE +
        dwAceCount * sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(&localSystemSid.sid) +
        RtlLengthSid(&administratorsSid.sid) +
        RtlLengthSid(&worldSid.sid) -
        dwAceCount * sizeof(ULONG);

    ntStatus= RTL_ALLOCATE(&pDacl, VOID, dwSizeDacl);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlCreateAcl(pDacl, dwSizeDacl, ACL_REVISION);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlAddAccessAllowedAceEx(
                  pDacl,
                  ACL_REVISION,
                  ulAceFlags,
                  FILE_ALL_ACCESS,
                  &localSystemSid.sid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlAddAccessAllowedAceEx(
                  pDacl,
                  ACL_REVISION,
                  ulAceFlags,
                  FILE_ALL_ACCESS,
                  &administratorsSid.sid);
    BAIL_ON_NT_STATUS(ntStatus);

    worldAccessMask = FILE_GENERIC_READ | FILE_GENERIC_EXECUTE;

    ntStatus = RtlAddAccessAllowedAceEx(
                  pDacl,
                  ACL_REVISION,
                  ulAceFlags,
                  worldAccessMask,
                  &worldSid.sid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlSetDaclSecurityDescriptor(
                  pAbsSecDesc,
                  TRUE,
                  pDacl,
                  FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Create the SelfRelative SD */

    ntStatus = RtlAbsoluteToSelfRelativeSD(
                   pAbsSecDesc,
                   NULL,
                   &ulRelSecDescLen);
    if (ntStatus == STATUS_BUFFER_TOO_SMALL)
    {
        ntStatus = SrvAllocateMemory(ulRelSecDescLen, (PVOID*)&pRelSecDesc);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RtlAbsoluteToSelfRelativeSD(
                       pAbsSecDesc,
                       pRelSecDesc,
                       &ulRelSecDescLen);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSecDesc = pRelSecDesc;
    *pulRelSecDescLen = ulRelSecDescLen;

cleanup:

    RTL_FREE(&pAbsSecDesc);
    RTL_FREE(&pOwnerSid);
    RTL_FREE(&pGroupSid);
    RTL_FREE(&pDacl);

    return ntStatus;

error:

    *ppSecDesc = NULL;

    if (pRelSecDesc)
    {
        SrvFreeMemory(pRelSecDesc);
    }

    goto cleanup;
}

static
NTSTATUS
SrvShutdown(
    VOID
    )
{
    NTSTATUS ntStatus = 0;

    // TODO: All existing requests must be waited on to be completed before
    //       shutting down the worker queues.

    if (gSMBSrvGlobals.pMutex)
    {
        pthread_mutex_lock(gSMBSrvGlobals.pMutex);

        if (gSMBSrvGlobals.pWorkerArray)
        {
            INT iWorker = 0;

            for (iWorker = 0; iWorker < gSMBSrvGlobals.ulNumWorkers; iWorker++)
            {
                PLWIO_SRV_WORKER pWorker = &gSMBSrvGlobals.pWorkerArray[iWorker];

                SrvWorkerIndicateStop(pWorker);
            }

            // Must indicate stop for all workers before queueing the
            // unblocks.
            for (iWorker = 0; iWorker < gSMBSrvGlobals.ulNumWorkers; iWorker++)
            {
                SrvUnblockOneWorker(&gSMBSrvGlobals.workQueue);
            }

            for (iWorker = 0; iWorker < gSMBSrvGlobals.ulNumWorkers; iWorker++)
            {
                PLWIO_SRV_WORKER pWorker = &gSMBSrvGlobals.pWorkerArray[iWorker];

                SrvWorkerFreeContents(pWorker);
            }

            SrvFreeMemory(gSMBSrvGlobals.pWorkerArray);
            gSMBSrvGlobals.pWorkerArray = NULL;
        }

        SrvProtocolShutdown();

        if (gSMBSrvGlobals.pMonitor)
        {
            SrvMonitorIndicateStop(gSMBSrvGlobals.pMonitor);

            SrvMonitorFree(gSMBSrvGlobals.pMonitor);

            gSMBSrvGlobals.pMonitor = NULL;
        }

        SrvStatisticsShutdown();

        SrvElementsShutdown();

        SrvOEMShutdown();

        SrvShareFreeListContents(&gSMBSrvGlobals.shareList);

        SrvShareShutdown();

        SrvProdConsFreeContents(&gSMBSrvGlobals.workQueue);

        if (gSMBSrvGlobals.hPacketAllocator)
        {
            SMBPacketFreeAllocator(gSMBSrvGlobals.hPacketAllocator);
            gSMBSrvGlobals.hPacketAllocator = NULL;
        }

        SrvFreeConfigContents(&gSMBSrvGlobals.config);

        SrvUtilsShutdown();

        pthread_mutex_unlock(gSMBSrvGlobals.pMutex);
        gSMBSrvGlobals.pMutex = NULL;
    }

    return ntStatus;
}

static
VOID
SrvUnblockOneWorker(
    IN PSMB_PROD_CONS_QUEUE pWorkQueue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT pExecContext = NULL;

    ntStatus = SrvBuildEmptyExecContext(&pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProdConsEnqueue(pWorkQueue, pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return;

error:

    if (pExecContext)
    {
        SrvReleaseExecContext(pExecContext);
    }

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

