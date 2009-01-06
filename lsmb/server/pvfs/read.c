/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        driver.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Driver Entry Function
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "pvfs.h"

NTSTATUS
PvfsReadFile(
    PDEVICE_OBJECT pDriverObject,
    PIRP pIrp
	)
{
    NTSTATUS ntStatus = 0;
    

    PtrFileObject = PtrIoStackLocation->FileObject;
    PtrCCB = (PtrSFsdCCB)(PtrFileObject->FsContext2);

    ByteOffset = PtrIoStackLocation->Parameters.Read.ByteOffset;
    ReadLength = PtrIoStackLocation->Parameters.Read.Length;

    ntStatus = PvfsGetUnixdfFromCCB(
                    PtrCCB
                    );
    BAIL_ON_NT_STATUS(ntStatus);


    return ntStatus;
}



