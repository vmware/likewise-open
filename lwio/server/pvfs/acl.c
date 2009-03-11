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
 *        acl.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Supporting ACL routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


/* File Globals */



/* Code */


/****************************************************************
 ***************************************************************/

VOID
PvfsFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc
    )
{
    /* Fixme:  Actually free the SD */
    return;
}


/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsCreateDefaultSecDescFile(
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    DWORD dwSizeDacl = 0;
    PSID pAdministratorsSid = NULL;
    PSID pUsersSid = NULL;
    PSID pEveryoneSid = NULL;
    DWORD dwSidCount = 0;
    PACL pDacl = NULL;

    /* Build SIDs */

    ntError = RtlAllocateSidFromCString(&pAdministratorsSid, "S-1-5-32-544");
    BAIL_ON_NT_STATUS(ntError);
    dwSidCount++;

    ntError = RtlAllocateSidFromCString(&pUsersSid, "S-1-5-32-545");
    BAIL_ON_NT_STATUS(ntError);
    dwSidCount++;

    ntError = RtlAllocateSidFromCString(&pEveryoneSid, "S-1-1-0");
    BAIL_ON_NT_STATUS(ntError);
    dwSidCount++;

    /* Build the DACL */

    /* 4096 should just be sizeof(ACL) */

    dwSizeDacl = 4096 +
        dwSidCount * sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(pAdministratorsSid) +
        RtlLengthSid(pUsersSid) +
        RtlLengthSid(pEveryoneSid) -
        dwSidCount * sizeof(ULONG);

    ntError= PvfsAllocateMemory((PVOID*)&pDacl, dwSizeDacl);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCreateAcl(pDacl, dwSizeDacl, ACL_REVISION);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlAddAccessAllowedAceEx(pDacl,
                                       ACL_REVISION,
                                       0,
                                       FILE_ALL_ACCESS,
                                       pAdministratorsSid);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlAddAccessAllowedAceEx(pDacl,
                                       ACL_REVISION,
                                       0,
                                       FILE_GENERIC_WRITE,
                                       pUsersSid);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlAddAccessAllowedAceEx(pDacl,
                                       ACL_REVISION,
                                       0,
                                       FILE_GENERIC_READ,
                                       pEveryoneSid);
    BAIL_ON_NT_STATUS(ntError);

    /* Now build the security descriptor */

    ntError= PvfsAllocateMemory((PVOID*)&pSecDesc,
                                SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCreateSecurityDescriptorAbsolute(pSecDesc,
                                                  SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlSetOwnerSecurityDescriptor(pSecDesc,
                                           pAdministratorsSid,
                                           FALSE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlSetDaclSecurityDescriptor(pSecDesc,
                                           TRUE,
                                           pDacl,
                                           FALSE);
    BAIL_ON_NT_STATUS(ntError);

    /* All done */

    *ppSecDesc = pSecDesc;
    ntError = STATUS_SUCCESS;

cleanup:
    /* The pAdministratorsSid was set for the owner so don't
       free it */

    PVFS_SAFE_FREE_MEMORY(pUsersSid);
    PVFS_SAFE_FREE_MEMORY(pEveryoneSid);

    return ntError;

error:
    PVFS_SAFE_FREE_MEMORY(pAdministratorsSid);
    PVFS_SAFE_FREE_MEMORY(pDacl);
    PVFS_SAFE_FREE_MEMORY(pSecDesc);

    goto cleanup;
}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsCreateDefaultSecDescDir(
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    BAIL_ON_NT_STATUS(ntError);

cleanup:
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

