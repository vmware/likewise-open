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

NTSTATUS
PvfsSecurityInitMapSecurityCtx(
    PLW_MAP_SECURITY_CONTEXT *ppContext
    )
{
    return LwMapSecurityCreateContext(ppContext);
}

/***********************************************************************
 **********************************************************************/

VOID
PvfsSecurityShutdownMapSecurityCtx(
    PLW_MAP_SECURITY_CONTEXT *ppContext
    )
{
    return LwMapSecurityFreeContext(ppContext);
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

    // Group

    ntError = PvfsSecuritySidMapFromGid(&pGroupSid, pStat->s_gid);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlSetGroupSecurityDescriptor(pSecDesc, pGroupSid, FALSE);
    BAIL_ON_NT_STATUS(ntError);

    // DACL

    ntError = PvfsSecurityAclGetDacl(&pDacl, pStat);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlSetDaclSecurityDescriptor(pSecDesc, TRUE, pDacl, FALSE);
    BAIL_ON_NT_STATUS(ntError);


    // We don't do SACLs here

    // All done

    ntError = RtlAbsoluteToSelfRelativeSD(
                  pSecDesc,
                  pSecDescRelative,
                  pSecDescRelativeLen);
    BAIL_ON_NT_STATUS(ntError);


cleanup:
    if (pSecDesc)
    {
        PvfsFreeAbsoluteSecurityDescriptor(&pSecDesc);
    }

    return ntError;

error:
    LW_RTL_FREE(&pOwnerSid);
    LW_RTL_FREE(&pGroupSid);
    LW_RTL_FREE(&pDacl);
    LW_RTL_FREE(&pSecDesc);

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
    BOOLEAN  bInLock = FALSE;
    UINT32 Key = Uid % PVFS_MAX_MRU_SIZE;

    BAIL_ON_INVALID_PTR(ppUserSid, ntError);

    LWIO_LOCK_MUTEX(bInLock, &gUidMruCacheMutex);

    if (gUidMruCache[Key] != NULL && (gUidMruCache[Key]->UnixId.Uid == Uid))
    {
        ntError = RtlDuplicateSid(ppUserSid, gUidMruCache[Key]->pSid);
        BAIL_ON_NT_STATUS(ntError);

        goto cleanup;
    }

    LWIO_UNLOCK_MUTEX(bInLock, &gUidMruCacheMutex);

    ntError = LwMapSecurityGetSidFromId(
                  gpPvfsLwMapSecurityCtx,
                  ppUserSid,
                  TRUE,
                  Uid);
    BAIL_ON_NT_STATUS(ntError);

    /* Try to cache the SID/UID pair but don't fail if we can't since
       we have already got the data the caller asked for */

    LWIO_LOCK_MUTEX(bInLock, &gUidMruCacheMutex);

    if (gUidMruCache[Key] != NULL)
    {
        PVFS_FREE(&gUidMruCache[Key]);
    }

    ntError = PvfsAllocateMemory(
                  (PVOID*)&gUidMruCache[Key],
                  sizeof(PVFS_ID_CACHE),
                  TRUE);
    if (ntError != STATUS_SUCCESS)
    {
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

    gUidMruCache[Key]->UnixId.Uid = Uid;
    ntError = RtlDuplicateSid(&gUidMruCache[Key]->pSid, *ppUserSid);
    if (ntError != STATUS_SUCCESS)
    {
        PVFS_FREE(&gUidMruCache[Key]);

        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &gUidMruCacheMutex);

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
    BOOLEAN  bInLock = FALSE;
    UINT32 Key = Gid % PVFS_MAX_MRU_SIZE;

    BAIL_ON_INVALID_PTR(ppGroupSid, ntError);


    LWIO_LOCK_MUTEX(bInLock, &gGidMruCacheMutex);

    if (gGidMruCache[Key] != NULL && (gGidMruCache[Key]->UnixId.Gid == Gid))
    {
        ntError = RtlDuplicateSid(ppGroupSid, gGidMruCache[Key]->pSid);
        BAIL_ON_NT_STATUS(ntError);

        goto cleanup;
    }

    LWIO_UNLOCK_MUTEX(bInLock, &gGidMruCacheMutex);

    ntError = LwMapSecurityGetSidFromId(
                  gpPvfsLwMapSecurityCtx,
                  ppGroupSid,
                  FALSE,
                  Gid);
    BAIL_ON_NT_STATUS(ntError);

    LWIO_LOCK_MUTEX(bInLock, &gGidMruCacheMutex);

    /* Try to cache the SID/GID pair but don't fail if we can't since
       we have already got the data the caller asked for */

    if (gGidMruCache[Key] != NULL)
    {
        PVFS_FREE(&gGidMruCache[Key]);
    }

    ntError = PvfsAllocateMemory(
                  (PVOID*)&gGidMruCache[Key],
                  sizeof(PVFS_ID_CACHE),
                  TRUE);
    if (ntError != STATUS_SUCCESS)
    {
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

    gGidMruCache[Key]->UnixId.Gid = Gid;
    ntError = RtlDuplicateSid(&gGidMruCache[Key]->pSid, *ppGroupSid);
    if (ntError != STATUS_SUCCESS)
    {
        PVFS_FREE(&gGidMruCache[Key]);

        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &gGidMruCacheMutex);

    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsSecuritySidMapToId(
    OUT PULONG pId,
    OUT PBOOLEAN pbIsUser,
    IN  PSID pSid
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(pId, ntError);
    BAIL_ON_INVALID_PTR(pbIsUser, ntError);

    ntError = LwMapSecurityGetIdFromSid(
                  gpPvfsLwMapSecurityCtx,
                  pbIsUser,
                  pId,
                  pSid);
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
    IN OUT PACCESS_MASK pAccess,
    IN     BOOLEAN bIsDirectory,
    IN     mode_t Mode,
    IN     mode_t Read,
    IN     mode_t Write,
    IN     mode_t Execute
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
    BOOLEAN bIsDir = S_ISDIR(pStat->s_mode);

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
        bIsDir,
        pStat->s_mode,
        S_IRUSR,
        S_IWUSR,
        S_IXUSR);
    AccessMask |= (READ_CONTROL|WRITE_DAC|WRITE_OWNER);
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
        bIsDir,
        pStat->s_mode,
        S_IRGRP,
        S_IWGRP,
        S_IXGRP);
    AccessMask |= (pStat->s_mode & S_ISGID) ? WRITE_DAC : 0;
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
        bIsDir,
        pStat->s_mode,
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
    IN OUT PACCESS_MASK pAccess,
    IN     BOOLEAN bIsDirectory,
    IN     mode_t Mode,
    IN     mode_t Read,
    IN     mode_t Write,
    IN     mode_t Execute
    )
{
    ACCESS_MASK Access = *pAccess;

    if (Mode & Read)
    {
        Access |= FILE_GENERIC_READ;

        if (bIsDirectory)
        {
            Access |= FILE_LIST_DIRECTORY;
        }
    }


    if (Mode & Write)
    {
        Access |= (FILE_GENERIC_WRITE|DELETE);

        if (bIsDirectory)
        {
            Access |= (FILE_DELETE_CHILD|FILE_ADD_SUBDIRECTORY);
        }
    }

    if (Mode & Execute)
    {
        Access |= FILE_GENERIC_EXECUTE;

        if (bIsDirectory)
        {
            Access |= FILE_TRAVERSE;
        }
    }

    if ((Mode & Read) && (Mode & Write) && (Mode & Execute))
    {
        /* Force the caller to decide if we need to include
           WRITE_OWNER|WRITE_DAC */

        Access = FILE_ALL_ACCESS & ~(WRITE_OWNER|WRITE_DAC);
    }


    *pAccess = Access;

    return;
}

/***********************************************************************
 **********************************************************************/

static
VOID
PvfsSecurityAccessMapToPosix(
    IN OUT mode_t *pMode,
    IN     ACCESS_MASK Access,
    IN     mode_t Read,
    IN     mode_t Write,
    IN     mode_t Execute
    )
{
    mode_t Mode = *pMode;

    if (Access & FILE_READ_DATA)
    {
        Mode |= Read;
    }

    if (Access & FILE_WRITE_DATA)
    {
        Mode |= Write;
    }

    if (Access & FILE_EXECUTE)
    {
        Mode |= Execute;
    }

    *pMode = Mode;

    return;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsSecurityPosixSetOwner(
    PPVFS_CCB pCcb,
    PPVFS_STAT pStat,
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc
    );

static
NTSTATUS
PvfsSecurityPosixSetGroup(
    PPVFS_CCB pCcb,
    PPVFS_STAT pStat,
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc
    );

static
NTSTATUS
PvfsSecurityPosixSetDacl(
    PPVFS_CCB pCcb,
    PPVFS_STAT pStat,
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc
    );

static
NTSTATUS
PvfsSecurityPosixSetSacl(
    PPVFS_CCB pCcb,
    PPVFS_STAT pStat,
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc
    );


NTSTATUS
PvfsSetSecurityDescriptorPosix(
    IN PPVFS_CCB pCcb,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRelative,
    IN ULONG SecDescLen
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDescAbs = NULL;
    PVFS_STAT Stat = {0};

    ntError = PvfsSysFstat(pCcb->fd, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSecurityAclSelfRelativeToAbsoluteSD(
                  &pSecDescAbs,
                  pSecDescRelative);
    BAIL_ON_NT_STATUS(ntError);

    /* The first two are sanity checks mainly and make no changes */

    ntError = PvfsSecurityPosixSetOwner(
                  pCcb,
                  &Stat,
                  pSecDescAbs);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSecurityPosixSetSacl(
                  pCcb,
                  &Stat,
                  pSecDescAbs);
    BAIL_ON_NT_STATUS(ntError);

    /* These can actually change the file/directory permissions */

    ntError = PvfsSecurityPosixSetGroup(
                  pCcb,
                  &Stat,
                  pSecDescAbs);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSecurityPosixSetDacl(
                  pCcb,
                  &Stat,
                  pSecDescAbs);
    BAIL_ON_NT_STATUS(ntError);

    /* Change group - User must be a member of that group */


cleanup:

    if (pSecDescAbs)
    {
        PvfsFreeAbsoluteSecurityDescriptor(&pSecDescAbs);
    }

    return ntError;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsSecurityPosixSetOwner(
    PPVFS_CCB pCcb,
    PPVFS_STAT pStat,
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc
    )
{
    NTSTATUS ntError = STATUS_ACCESS_DENIED;
    PSID pOwner = NULL;
    BOOLEAN bDefaulted = FALSE;
    BOOLEAN bIsUserSid = FALSE;
    ULONG Id = 0;

    BAIL_ON_INVALID_PTR(pSecDesc, ntError);

    ntError = RtlGetOwnerSecurityDescriptor(pSecDesc, &pOwner, &bDefaulted);
    BAIL_ON_NT_STATUS(ntError);

    if (pOwner)
    {
        ntError = PvfsSecuritySidMapToId(&Id, &bIsUserSid, pOwner);
        BAIL_ON_NT_STATUS(ntError);

        /* Owner SID has to be a user and has to match current owner.
           I don't support changing owner rights or take ownership
           permission here */

        if (!bIsUserSid || (Id != pStat->s_uid))
        {
            ntError = STATUS_ACCESS_DENIED;
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsSecurityPosixSetGroup(
    PPVFS_CCB pCcb,
    PPVFS_STAT pStat,
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc
    )
{
    NTSTATUS ntError = STATUS_ACCESS_DENIED;
    PSID pGroup = NULL;
    BOOLEAN bDefaulted = FALSE;
    BOOLEAN bIsUserSid = FALSE;
    ULONG Id = 0;

    BAIL_ON_INVALID_PTR(pSecDesc, ntError);

    ntError = RtlGetGroupSecurityDescriptor(pSecDesc, &pGroup, &bDefaulted);
    BAIL_ON_NT_STATUS(ntError);

    if (pGroup == NULL)
    {
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

    ntError = PvfsSecuritySidMapToId(&Id, &bIsUserSid, pGroup);
    BAIL_ON_NT_STATUS(ntError);

    /* Has to be a valid group.
       TODO - Allow group ownership changes if the user is a member of
       that group. */

    if (bIsUserSid)
    {
        ntError = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntError);
    }

    // Don't allow Unix ownership changes on virtual owner systems

    if (gPvfsDriverConfig.VirtualUid != (uid_t)-1)
    {
        ntError = PvfsSysChown(pCcb, pStat->s_uid, (gid_t)Id);
        BAIL_ON_NT_STATUS(ntError);

        pStat->s_gid = (gid_t)Id;
    }

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsSecurityPosixSetDacl(
    PPVFS_CCB pCcb,
    PPVFS_STAT pStat,
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc
    )
{
    NTSTATUS ntError = STATUS_ACCESS_DENIED;
    PACL pDacl = NULL;
    BOOLEAN bPresent = FALSE;
    PVOID pAce = NULL;
    ULONG AceIndex = 0;
    mode_t Mode = 0;
    BOOLEAN bDefaulted = FALSE;
    BOOLEAN bIsUserSid = FALSE;
    ULONG Id = 0;
    PSID pEveryoneSid = NULL;

    BAIL_ON_INVALID_PTR(pSecDesc, ntError);

    ntError = RtlGetDaclSecurityDescriptor(
                  pSecDesc,
                  &bPresent,
                  &pDacl,
                  &bDefaulted);
    BAIL_ON_NT_STATUS(ntError);

    if (!bPresent || !pDacl)
    {
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

    /* Set up the Everyone SID so we can check for it in the ACL */

    ntError = RtlAllocateSidFromCString(&pEveryoneSid, "S-1-1-0");
    BAIL_ON_NT_STATUS(ntError);

    for (ntError = RtlGetAce(pDacl, AceIndex, &pAce);
         (ntError == STATUS_SUCCESS) && pAce;
         ntError = RtlGetAce(pDacl, AceIndex++, &pAce))
    {
        PACE_HEADER pAceHeader = (PACE_HEADER)pAce;
        PACCESS_ALLOWED_ACE pAllowedAce = NULL;
        PSID pSid = NULL;

        if (pAceHeader->AceType != ACCESS_ALLOWED_ACE_TYPE)
        {
            ntError = STATUS_ACCESS_DENIED;
            BAIL_ON_NT_STATUS(ntError);
        }

        pAllowedAce = (PACCESS_ALLOWED_ACE)pAce;
        pSid = (PSID)&pAllowedAce->SidStart;

        /* First check for "Everyone" */

        if (RtlEqualSid(pSid, pEveryoneSid))
        {
            PvfsSecurityAccessMapToPosix(
                &Mode,
                pAllowedAce->Mask,
                S_IROTH,
                S_IWOTH,
                S_IXOTH);

            continue;
        }

        /* Deal with User/Group SIDs */

        ntError = PvfsSecuritySidMapToId(&Id, &bIsUserSid, pSid);
        BAIL_ON_NT_STATUS(ntError);

        if (bIsUserSid)
        {
            if (Id != pStat->s_uid)
            {
                ntError = STATUS_INVALID_ACL;
                BAIL_ON_NT_STATUS(ntError);
            }

            PvfsSecurityAccessMapToPosix(
                &Mode,
                pAllowedAce->Mask,
                S_IRUSR,
                S_IWUSR,
                S_IXUSR);
        }
        else
        {
            /* Group SID */
            if (Id != pStat->s_gid)
            {
                ntError = STATUS_INVALID_ACL;
                BAIL_ON_NT_STATUS(ntError);
            }

            PvfsSecurityAccessMapToPosix(
                &Mode,
                pAllowedAce->Mask,
                S_IRGRP,
                S_IWGRP,
                S_IXGRP);

            if (pAllowedAce->Mask & WRITE_DAC)
            {
                Mode |= S_ISGID;
            }

        }
    }

    if (Mode == 0)
    {
        ntError = STATUS_INVALID_ACL;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsSysFchmod(pCcb, Mode);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    LW_RTL_FREE(&pEveryoneSid);

    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsSecurityPosixSetSacl(
    PPVFS_CCB pCcb,
    PPVFS_STAT pStat,
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc
    )
{
    NTSTATUS ntError = STATUS_ACCESS_DENIED;
    PACL pSacl = NULL;
    BOOLEAN bPresent = FALSE;
    BOOLEAN bDefaulted = FALSE;

    BAIL_ON_INVALID_PTR(pSecDesc, ntError);

    ntError = RtlGetSaclSecurityDescriptor(
                  pSecDesc,
                  &bPresent,
                  &pSacl,
                  &bDefaulted);
    BAIL_ON_NT_STATUS(ntError);

    if (bPresent || pSacl)
    {
        ntError = STATUS_PRIVILEGE_NOT_HELD;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_SUCCESS;

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

