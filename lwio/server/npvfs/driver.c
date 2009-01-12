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
 *        createnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CreateNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "npvfs.h"

NTSTATUS
DriverEntry(
    PDRIVER_OBJECT pDriverObject,
    PIRP pIrp
	)
{
	NTSTATUS ntStatus = 0;

    pDriverObject->MajorFunction[IRP_MJ_CREATE] = NpfsCreateFile;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE] = NpfsCloseFile;
    pDriverObject->MajorFunction[IRP_MJ_READ] = NpfsReadFile;
    pDriverObject->MajorFunction[IRP_MJ_WRITE] = NpfsWriteFile;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = NpfsDeviceControl;
    pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = NpfsCleanup;
    
    ntStatus = IoCreateDevice(
                    pDriverObject,
                    0,
                    DeviceName,
                    FILE_DEVICE_DISK_FILE_SYSTEM,
                    0,
                    &NpfsGlobalData.pDeviceObject,
                    );
    BAIL_ON_NT_STATUS(ntStatus);
error:    
    
                        
	return ntStatus;
}



