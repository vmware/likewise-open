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
 *        acl_posix.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Standard Unix perms & POSIX acls NTFS ACL implementation
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsSecurityInitMapSecurityCtx(
    PLW_MAP_SECURITY_CONTEXT *ppContext
    )
{
    return LwMapSecurityCreateContext(ppContext);
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsSecurityAclMapFromPosix(
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    IN OUT PULONG pSecDescLen,
    IN     PPVFS_STAT pStat
    );

NTSTATUS
PvfsGetSecurityDescriptorPosix(
    IN PPVFS_CCB pCcb,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    IN OUT PULONG pSecDescLen
    )
{
    NTSTATUS ntError = STATUS_NO_SECURITY_ON_OBJECT;
    PVFS_STAT Stat = {0};

    ntError = PvfsSysFstat(pCcb->fd, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSecurityAclMapFromPosix(pSecDesc, pSecDescLen, &Stat);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsGetSecurityDescriptorFilenamePosix(
    IN PCSTR pszFilename,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    IN OUT PULONG pSecDescLen
    )
{
    NTSTATUS ntError = STATUS_NO_SECURITY_ON_OBJECT;
    PVFS_STAT Stat = {0};

    ntError = PvfsSysStat(pszFilename, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSecurityAclMapFromPosix(pSecDesc, pSecDescLen, &Stat);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsSecuritySidMapFromUid(
    OUT PSID *ppUserSid,
    IN  uid_t Uid
    );

static
NTSTATUS
PvfsSecuritySidMapFromGid(
    OUT PSID *ppGroupSid,
    IN  gid_t Gid
    );

static
NTSTATUS
PvfsSecurityAclGetDacl(
    OUT PACL *ppDacl,
    IN  PPVFS_STAT pStat
    );


static
NTSTATUS
PvfsSecurityAclMapFromPosix(
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecDescRelative,
    IN OUT PULONG pSecDescRelativeLen,
    IN     PPVFS_STAT pStat
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PACL pDacl = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;

    BAIL_ON_INVALID_PTR(pStat, ntError);
    BAIL_ON_INVALID_PTR(pSecDescRelativeLen, ntError);

    ntError= LW_RTL_ALLOCATE(
                 &pSecDesc,
                 VOID,
                 SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCreateSecurityDescriptorAbsolute(
                  pSecDesc,
                  SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntError);

    // Owner

    ntError = PvfsSecuritySidMapFromUid(&pOwnerSid, pStat->s_uid);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlSetOwnerSecurityDescriptor(pSecDesc, pOwnerSid, FALSE);
    BAIL_ON_NT_STATUS(ntError);

    pOwnerSid = NULL;

    // Group

    ntError = PvfsSecuritySidMapFromGid(&pGroupSid, pStat->s_gid);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlSetGroupSecurityDescriptor(pSecDesc, pGroupSid, FALSE);
    BAIL_ON_NT_STATUS(ntError);

    pGroupSid = NULL;

    // DACL

    ntError = PvfsSecurityAclGetDacl(&pDacl, pStat);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlSetDaclSecurityDescriptor(pSecDesc, TRUE, pDacl, FALSE);
    BAIL_ON_NT_STATUS(ntError);

    pDacl = NULL;


    // We don't do SACLs here

    // All done

    ntError = RtlAbsoluteToSelfRelativeSD(
                  pSecDesc,
                  pSecDescRelative,
                  pSecDescRelativeLen);
    BAIL_ON_NT_STATUS(ntError);


cleanup:
    LW_RTL_FREE(&pOwnerSid);
    LW_RTL_FREE(&pGroupSid);
    LW_RTL_FREE(&pDacl);

    PvfsFreeAbsoluteSecurityDescriptor(&pSecDesc);

    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsSecuritySidMapFromUid(
    OUT PSID *ppUserSid,
    IN  uid_t Uid
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(ppUserSid, ntError);

    if (!gpPvfsLwMapSecurityCtx)
    {
        ntError = PvfsSecurityInitMapSecurityCtx(&gpPvfsLwMapSecurityCtx);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = LwMapSecurityGetSidFromId(
                  gpPvfsLwMapSecurityCtx,
                  ppUserSid,
                  TRUE,
                  Uid);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsSecuritySidMapFromGid(
    OUT PSID *ppGroupSid,
    IN  gid_t Gid
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(ppGroupSid, ntError);

    if (!gpPvfsLwMapSecurityCtx)
    {
        ntError = PvfsSecurityInitMapSecurityCtx(&gpPvfsLwMapSecurityCtx);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = LwMapSecurityGetSidFromId(
                  gpPvfsLwMapSecurityCtx,
                  ppGroupSid,
                  FALSE,
                  Gid);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}



/***********************************************************************
 **********************************************************************/

static
VOID
PvfsSecurityAccessMapFromPosix(
    IN PACCESS_MASK pAccess,
    PPVFS_STAT pStat,
    mode_t Read,
    mode_t Write,
    mode_t Execute
    );

static
NTSTATUS
PvfsSecurityAclGetDacl(
    OUT PACL *ppDacl,
    IN  PPVFS_STAT pStat
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    DWORD dwSizeDacl = 0;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PSID pEveryoneSid = NULL;
    DWORD dwSidCount = 0;
    PACL pDacl = NULL;
    ACCESS_MASK AccessMask = 0;

    /* Build SIDs */

    ntError = PvfsSecuritySidMapFromUid(&pOwnerSid, pStat->s_uid);
    BAIL_ON_NT_STATUS(ntError);
    dwSidCount++;

    ntError = PvfsSecuritySidMapFromGid(&pGroupSid, pStat->s_gid);
    BAIL_ON_NT_STATUS(ntError);
    dwSidCount++;

    ntError = RtlAllocateSidFromCString(&pEveryoneSid, "S-1-1-0");
    BAIL_ON_NT_STATUS(ntError);
    dwSidCount++;

    dwSizeDacl = ACL_HEADER_SIZE +
        dwSidCount * sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(pOwnerSid) +
        RtlLengthSid(pGroupSid) +
        RtlLengthSid(pEveryoneSid) -
        dwSidCount * sizeof(ULONG);

    ntError= RTL_ALLOCATE(&pDacl, VOID, dwSizeDacl);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCreateAcl(pDacl, dwSizeDacl, ACL_REVISION);
    BAIL_ON_NT_STATUS(ntError);

    AccessMask = 0;
    PvfsSecurityAccessMapFromPosix(
        &AccessMask,
        pStat,
        S_IRUSR,
        S_IWUSR,
        S_IXUSR);
    ntError = RtlAddAccessAllowedAceEx(
                  pDacl,
                  ACL_REVISION,
                  0,
                  AccessMask,
                  pOwnerSid);
    BAIL_ON_NT_STATUS(ntError);

    AccessMask = 0;
    PvfsSecurityAccessMapFromPosix(
        &AccessMask,
        pStat,
        S_IRGRP,
        S_IWGRP,
        S_IXGRP);
    ntError = RtlAddAccessAllowedAceEx(
                  pDacl,
                  ACL_REVISION,
                  0,
                  AccessMask,
                  pGroupSid);
    BAIL_ON_NT_STATUS(ntError);

    AccessMask = 0;
    PvfsSecurityAccessMapFromPosix(
        &AccessMask,
        pStat,
        S_IROTH,
        S_IWOTH,
        S_IXOTH);
    ntError = RtlAddAccessAllowedAceEx(
                  pDacl,
                  ACL_REVISION,
                  0,
                  AccessMask,
                  pEveryoneSid);
    BAIL_ON_NT_STATUS(ntError);

    *ppDacl = pDacl;
    pDacl = NULL;

cleanup:
    /* Use RtlFree for SIDs */

    LW_RTL_FREE(&pOwnerSid);
    LW_RTL_FREE(&pGroupSid);
    LW_RTL_FREE(&pEveryoneSid);

    LW_RTL_FREE(&pDacl);

    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
VOID
PvfsSecurityAccessMapFromPosix(
    IN PACCESS_MASK pAccess,
    PPVFS_STAT pStat,
    mode_t Read,
    mode_t Write,
    mode_t Execute
    )
{
    ACCESS_MASK Access = 0;

    if (pStat->s_mode & Read)
    {
        Access |= FILE_GENERIC_READ;
    }

    if (pStat->s_mode & Write)
    {
        Access |= (FILE_GENERIC_WRITE|DELETE);
    }

    if (pStat->s_mode & Execute)
    {
        Access |= FILE_GENERIC_EXECUTE;
    }

    *pAccess = Access;

    return;
}


/***********************************************************************
 **********************************************************************/

#if 0   // FIX ME - Needs to be finished

static
NTSTATUS
PvfsSecurityAclMapToPosix(
    IN OUT PPVFS_STAT pStat,
    IN     PSECURITY_DESCRIPTOR_RELATIVE pSecDescRelative,
    IN     PULONG pSecDescLen
    );

NTSTATUS
PvfsSetSecurityDescriptorPosix(
    IN PPVFS_CCB pCcb,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    IN ULONG SecDescLen
    )
{
    return STATUS_NOT_IMPLEMENTED;
}


static
NTSTATUS
PvfsSecurityAclMapToPosix(
    IN OUT PPVFS_STAT pStat,
    IN     PSECURITY_DESCRIPTOR_RELATIVE pSecDescRelative,
    IN     PULONG pSecDescLen
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

#endif



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

