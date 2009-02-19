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

#include "iop.h"

typedef struct _IRP_INTERNAL {
    IRP Irp;
    LW_LIST_LINKS FileObjectLinks;
} IRP_INTERNAL, *PIRP_INTERNAL;

// TODO -- make inline -- just want type-safe macro.
PIRP_INTERNAL
IopIrpGetInternal(
    IN PIRP pIrp
    )
{
    return (PIRP_INTERNAL) pIrp;
}

NTSTATUS
IopIrpCreate(
    OUT PIRP* ppIrp,
    IN IRP_TYPE Type,
    IN PIO_FILE_OBJECT pFileObject
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;

    // Note that we allocate enough space for the internal fields.
    status = IO_ALLOCATE(&pIrp, IRP, sizeof(IRP_INTERNAL));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Type = Type;
    pIrp->FileHandle = pFileObject;
    pIrp->DeviceHandle = pFileObject->pDevice;
    pIrp->DriverHandle = pFileObject->pDevice->Driver;

    LwListInsertTail(&pFileObject->IrpList,
                     &IopIrpGetInternal(pIrp)->FileObjectLinks);
cleanup:
    if (status)
    {
        IopIrpFree(&pIrp);
    }

    *ppIrp = pIrp;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

VOID
IopIrpFree(
    IN OUT PIRP* ppIrp
    )
{
    PIRP pIrp = *ppIrp;

    if (pIrp)
    {
        LwListRemove(&IopIrpGetInternal(pIrp)->FileObjectLinks);
        IoMemoryFree(pIrp);
        *ppIrp = NULL;
    }
}

