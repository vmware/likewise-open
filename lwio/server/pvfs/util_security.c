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
 *        Security related routines such as ACLs, access checks, etc...
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

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsAccessCheckFile(
    PACCESS_TOKEN pToken,
    PCSTR pszFilename,
    ACCESS_MASK Desired,
    ACCESS_MASK *pGranted)
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    ACCESS_MASK AccessMask = 0;
    ACCESS_MASK GrantedAccess = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    BYTE pRelativeSecDescBuffer[SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE] = {0};
    ULONG ulRelativeSecDescLength = SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE;
    PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc = NULL;
    BYTE pParentRelSecDescBuffer[SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE] = {0};
    ULONG ulParentRelSecDescLength = SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE;
    BOOLEAN bGranted = FALSE;
    SECURITY_INFORMATION SecInfo = (OWNER_SECURITY_INFORMATION |
                                    GROUP_SECURITY_INFORMATION |
                                    DACL_SECURITY_INFORMATION);
    PSTR pszParentPath = NULL;
    PSID pOwner = NULL;
    BOOLEAN bOwnerDefaulted = FALSE;

    BAIL_ON_INVALID_PTR(pToken, ntError);
    BAIL_ON_INVALID_PTR(pGranted, ntError);

    /* if asking for DELETE, check and see if the parent directory
       will grant us that */

    if (Desired & (MAXIMUM_ALLOWED | DELETE))
    {
        ntError = PvfsFileDirname(&pszParentPath, pszFilename);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsGetSecurityDescriptorFilename(
                      pszParentPath,
                      SecInfo,
                      (PSECURITY_DESCRIPTOR_RELATIVE)pParentRelSecDescBuffer,
                      &ulParentRelSecDescLength);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSecurityAclSelfRelativeToAbsoluteSD(
                      &pParentSecDesc,
                      (PSECURITY_DESCRIPTOR_RELATIVE)pParentRelSecDescBuffer);
        BAIL_ON_NT_STATUS(ntError);

        bGranted = RtlAccessCheck(
                       pParentSecDesc,
                       pToken,
                       DELETE,
                       0,
                       &gPvfsFileGenericMapping,
                       &GrantedAccess,
                       &ntError);
        if (bGranted)
        {
            ClearFlag(Desired, DELETE);
        }

    }

    /* Check the file object itself */

    ntError = PvfsGetSecurityDescriptorFilename(
                  pszFilename,
                  SecInfo,
                  (PSECURITY_DESCRIPTOR_RELATIVE)pRelativeSecDescBuffer,
                  &ulRelativeSecDescLength);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSecurityAclSelfRelativeToAbsoluteSD(
                  &pSecDesc,
                  (PSECURITY_DESCRIPTOR_RELATIVE)pRelativeSecDescBuffer);
    BAIL_ON_NT_STATUS(ntError);

    /* Tests against NTFS/Win2003R2 show that the file/directory object
       owner is always granted FILE_READ_ATTRIBUTES */

    ntError = RtlGetOwnerSecurityDescriptor(
                  pSecDesc,
                  &pOwner,
                  &bOwnerDefaulted);
    BAIL_ON_NT_STATUS(ntError);

    if (RtlIsSidMemberOfToken(pToken, pOwner))
    {
        ClearFlag(Desired, FILE_READ_ATTRIBUTES);
        SetFlag(GrantedAccess, FILE_READ_ATTRIBUTES);
    }

    /* Now check access */

    bGranted = RtlAccessCheck(
                   pSecDesc,
                   pToken,
                   Desired,
                   GrantedAccess,
                   &gPvfsFileGenericMapping,
                   &AccessMask,
                   &ntError);
    if (!bGranted)
    {
        BAIL_ON_NT_STATUS(ntError);
    }

    *pGranted = AccessMask;
    ntError = STATUS_SUCCESS;

cleanup:
    if (pszParentPath)
    {
        LwRtlCStringFree(&pszParentPath);
    }

    if (pParentSecDesc)
    {
        PvfsFreeAbsoluteSecurityDescriptor(&pParentSecDesc);
    }

    if (pSecDesc)
    {
        PvfsFreeAbsoluteSecurityDescriptor(&pSecDesc);
    }

    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsAccessCheckFileEnumerate(
    PPVFS_CCB pCcb,
    PCSTR pszRelativeFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PACCESS_TOKEN pToken = pCcb->pUserToken;
    PSTR pszFilename = NULL;
    ACCESS_MASK AccessMask = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    BYTE pRelativeSecDescBuffer[SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE] = {0};
    ULONG ulRelativeSecDescLength = SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE;
    BOOLEAN bGranted = FALSE;
    SECURITY_INFORMATION SecInfo = (OWNER_SECURITY_INFORMATION |
                                    GROUP_SECURITY_INFORMATION |
                                    DACL_SECURITY_INFORMATION);

    /* Create the absolute path */

    ntError = LwRtlCStringAllocatePrintf(
                  &pszFilename,
                  "%s/%s",
                  pCcb->pszFilename,
                  pszRelativeFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Check the file object itself */

    ntError = PvfsGetSecurityDescriptorFilename(
                  pszFilename,
                  SecInfo,
                  (PSECURITY_DESCRIPTOR_RELATIVE)pRelativeSecDescBuffer,
                  &ulRelativeSecDescLength);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSecurityAclSelfRelativeToAbsoluteSD(
                  &pSecDesc,
                  (PSECURITY_DESCRIPTOR_RELATIVE)pRelativeSecDescBuffer);
    BAIL_ON_NT_STATUS(ntError);

    /* Now check access */

    bGranted = RtlAccessCheck(
                   pSecDesc,
                   pToken,
                   MAXIMUM_ALLOWED,
                   0,
                   &gPvfsFileGenericMapping,
                   &AccessMask,
                   &ntError);
    if (!bGranted)
    {
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_ACCESS_DENIED;
    if ((AccessMask & FILE_GENERIC_READ) == FILE_GENERIC_READ)
    {
        ntError = STATUS_SUCCESS;
    }

cleanup:
    if (pszFilename)
    {
        LwRtlCStringFree(&pszFilename);
    }

    if (pSecDesc)
    {
        PvfsFreeAbsoluteSecurityDescriptor(&pSecDesc);
    }

    return ntError;

error:
    goto cleanup;
}

/***********************************************************
 **********************************************************/

ACCESS_MASK
PvfsGetGrantedAccessForNewObject(
    ACCESS_MASK DesiredAccess
    )
{
    ACCESS_MASK GrantedAccess = DesiredAccess;

    // TODO: This function probably needs to be more complicated.

    if (IsSetFlag(DesiredAccess, MAXIMUM_ALLOWED))
    {
        GrantedAccess = FILE_ALL_ACCESS;
    }

    RtlMapGenericMask(&GrantedAccess, &gPvfsFileGenericMapping);

    return GrantedAccess;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

