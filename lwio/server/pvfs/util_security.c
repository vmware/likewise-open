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
 *        security.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Security related routines such as ACLs, acces, checks, etc...
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


/* File Globals */



/* Code */

/***********************************************************
 **********************************************************/

NTSTATUS
PvfsAcquireAccessToken(
    PPVFS_CCB pCcb,
    PIO_CREATE_SECURITY_CONTEXT pIoSecCtx
    )
{
    NTSTATUS ntError= STATUS_UNSUCCESSFUL;

    pCcb->pUserToken = IoSecurityGetAccessToken(pIoSecCtx);
    if (pCcb->pUserToken == NULL) {
        ntError = STATUS_NO_TOKEN;
        BAIL_ON_NT_STATUS(ntError);
    }

    RtlReferenceAccessToken(pCcb->pUserToken);
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/***********************************************************
 **********************************************************/

NTSTATUS
PvfsAccessCheckFileHandle(
    PPVFS_CCB pCcb,
    ACCESS_MASK AccessRequired
    )
{
    NTSTATUS ntError = STATUS_ACCESS_DENIED;

    if ((pCcb->AccessGranted & AccessRequired) == AccessRequired)
    {
        ntError = STATUS_SUCCESS;
    }

    return ntError;
}

/***********************************************************
 **********************************************************/

NTSTATUS
PvfsAccessCheckAnyFileHandle(
    PPVFS_CCB pCcb,
    ACCESS_MASK AccessRequired
    )
{
    NTSTATUS ntError = STATUS_ACCESS_DENIED;

    /* Any of the permissions are ok */

    if ((pCcb->AccessGranted & AccessRequired) != 0)
    {
        ntError = STATUS_SUCCESS;
    }

    return ntError;
}

/***********************************************************
 **********************************************************/

NTSTATUS
PvfsAccessCheckDir(
    PACCESS_TOKEN pToken,
    PCSTR pszDirectory,
    ACCESS_MASK Desired,
    ACCESS_MASK *pGranted)
{
    /* For now this is just the same as file access */

    return PvfsAccessCheckFile(pToken, pszDirectory, Desired, pGranted);
}

/***********************************************************
 **********************************************************/

NTSTATUS
PvfsAccessCheckFile(
    PACCESS_TOKEN pToken,
    PCSTR pszFilename,
    ACCESS_MASK Desired,
    ACCESS_MASK *pGranted)
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    ACCESS_MASK AccessMask = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    ULONG SecDescRelLen = 1024;
    SECURITY_INFORMATION SecInfo = (OWNER_SECURITY_INFORMATION |
                                    GROUP_SECURITY_INFORMATION |
                                    DACL_SECURITY_INFORMATION);
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    ULONG SecDescLen = 0;
    PACL pDacl = NULL;
    ULONG DaclLen = 0;
    PACL pSacl = NULL;
    ULONG SaclLen = 0;
    PSID pOwner = NULL;
    ULONG OwnerLen = 0;
    PSID pGroup = NULL;
    ULONG GroupLen = 0;

    BAIL_ON_INVALID_PTR(pToken, ntError);
    BAIL_ON_INVALID_PTR(pGranted, ntError);

    do
    {
        ntError = PvfsReallocateMemory((PVOID*)&pSecDescRel, SecDescRelLen);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsGetSecurityDescriptorFilename(pszFilename,
                                                    SecInfo,
                                                    pSecDescRel,
                                                    &SecDescRelLen);
        if (ntError == STATUS_BUFFER_TOO_SMALL) {
            SecDescRelLen *= 2;
        } else {
            BAIL_ON_NT_STATUS(ntError);
        }

    } while ((ntError != STATUS_SUCCESS) &&
             (SecDescRelLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE));
    BAIL_ON_NT_STATUS(ntError);

    /* Get sizes */

    ntError = RtlSelfRelativeToAbsoluteSD(pSecDescRel,
                                          pSecDesc, &SecDescLen,
                                          pDacl, &DaclLen,
                                          pSacl, &SaclLen,
                                          pOwner, &OwnerLen,
                                          pGroup, &GroupLen);
    if (ntError == STATUS_BUFFER_TOO_SMALL) {
        ntError = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntError);

    /* Allocate -- Always use RTL routines for Absolute SDs */

    ntError = RTL_ALLOCATE(&pSecDesc, VOID, SecDescLen);
    BAIL_ON_NT_STATUS(ntError);

    if (OwnerLen) {
        ntError = RTL_ALLOCATE(&pOwner, SID, OwnerLen);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (GroupLen) {
        ntError = RTL_ALLOCATE(&pGroup, SID, GroupLen);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (DaclLen) {
        ntError = RTL_ALLOCATE(&pDacl, VOID, DaclLen);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (SaclLen) {
        ntError = RTL_ALLOCATE(&pSacl, VOID, SaclLen);
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Translate the SD */

    ntError = RtlSelfRelativeToAbsoluteSD(pSecDescRel,
                                          pSecDesc, &SecDescLen,
                                          pDacl, &DaclLen,
                                          pSacl, &SaclLen,
                                          pOwner, &OwnerLen,
                                          pGroup, &GroupLen);
    BAIL_ON_NT_STATUS(ntError);

    /* Now check access */

    /* Remove the SACL bit */

    Desired &= ~ACCESS_SYSTEM_SECURITY;

    if (!RtlAccessCheck(pSecDesc,
                        pToken,
                        Desired,
                        0,
                        &gPvfsFileGenericMapping,
                        &AccessMask,
                        &ntError))
    {
        BAIL_ON_NT_STATUS(ntError);
    }

    *pGranted = AccessMask;
    ntError = STATUS_SUCCESS;

cleanup:
    PVFS_FREE(&pSecDescRel);
    PvfsFreeAbsoluteSecurityDescriptor(&pSecDesc);

    return ntError;

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

