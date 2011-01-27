/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsGetSecurityDescriptorFile(
    IN PPVFS_CCB pCcb,
    IN SECURITY_INFORMATION SecInfo,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    IN OUT PULONG pSecDescLength
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BYTE secDescBufferStatic[PVFS_DEFAULT_SD_RELATIVE_SIZE];
    PBYTE secDescBuffer = secDescBufferStatic;
    ULONG secDescLength = PVFS_DEFAULT_SD_RELATIVE_SIZE;
    ULONG origSecDescLength = PVFS_DEFAULT_SD_RELATIVE_SIZE;
    SECURITY_INFORMATION SecInfoAll = (OWNER_SECURITY_INFORMATION |
                                       GROUP_SECURITY_INFORMATION |
                                       DACL_SECURITY_INFORMATION |
                                       SACL_SECURITY_INFORMATION);
    BOOLEAN eaNotFound = FALSE;

    if (SecInfo == 0)
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_RESOURCE_DATA_NOT_FOUND;

    do
    {
        if (ntError == STATUS_BUFFER_TOO_SMALL)
        {
            if (secDescBuffer == secDescBufferStatic)
            {
                // Have to move to allocating the buffer since
                // static buffer was not large enough)
                secDescBuffer = NULL;
            }

            secDescLength = PVFS_MIN(
                                secDescLength*2,
                                SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE);
            ntError = PvfsReallocateMemory(
                          (PVOID*)&secDescBuffer,
                          secDescLength);
            BAIL_ON_NT_STATUS(ntError);
        }

        if (!eaNotFound)
        {
#ifdef HAVE_EA_SUPPORT
            ntError = PvfsGetSecurityDescriptorFileXattr(
                          pCcb,
                          (PSECURITY_DESCRIPTOR_RELATIVE)secDescBuffer,
                          &secDescLength);
#endif
        }

        /* EA security descriptor support not available or present for
           this object */

        if ((ntError == STATUS_RESOURCE_DATA_NOT_FOUND) ||
            (ntError == STATUS_NOT_SUPPORTED))
        {
            eaNotFound = TRUE;

            secDescLength = origSecDescLength;

            ntError = PvfsGetSecurityDescriptorPosix(
                          pCcb,
                          (PSECURITY_DESCRIPTOR_RELATIVE)secDescBuffer,
                          &secDescLength);
        }

    } while ((ntError == STATUS_BUFFER_TOO_SMALL) &&
             (secDescLength < SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE));
    BAIL_ON_NT_STATUS(ntError);

    /* If the caller wants the complete Security Descriptor, just copy
       the buffer */

    if (SecInfo == SecInfoAll)
    {
        if (*pSecDescLength < secDescLength)
        {
            ntError = STATUS_BUFFER_TOO_SMALL;
            BAIL_ON_NT_STATUS(ntError);
        }

        LwRtlCopyMemory(pSecDesc, secDescBuffer, secDescLength);
        *pSecDescLength = secDescLength;
    }
    else
    {
        ntError = RtlQuerySecurityDescriptorInfo(
                      SecInfo,
                      pSecDesc,
                      pSecDescLength,
                      (PSECURITY_DESCRIPTOR_RELATIVE)secDescBuffer);
        BAIL_ON_NT_STATUS(ntError);
    }

error:
    if (secDescBuffer != secDescBufferStatic)
    {
        PvfsFreeMemory((PVOID*)&secDescBuffer);
    }

    return ntError;
}


/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsGetSecurityDescriptorFilename(
    IN PCSTR pszFilename,
    IN SECURITY_INFORMATION SecInfo,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    IN OUT PULONG pSecDescLength
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BYTE secDescBufferStatic[PVFS_DEFAULT_SD_RELATIVE_SIZE];
    PBYTE secDescBuffer = secDescBufferStatic;
    ULONG secDescLength = PVFS_DEFAULT_SD_RELATIVE_SIZE;
    ULONG origSecDescLength = PVFS_DEFAULT_SD_RELATIVE_SIZE;
    SECURITY_INFORMATION SecInfoAll = (OWNER_SECURITY_INFORMATION |
                                       GROUP_SECURITY_INFORMATION |
                                       DACL_SECURITY_INFORMATION |
                                       SACL_SECURITY_INFORMATION);
    BOOLEAN eaNotFound = FALSE;

    BAIL_ON_INVALID_PTR(pszFilename, ntError);

    if (SecInfo == 0)
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_RESOURCE_DATA_NOT_FOUND;

    do
    {
        if (ntError == STATUS_BUFFER_TOO_SMALL)
        {
            if (secDescBuffer == secDescBufferStatic)
            {
                // Have to move to allocating the buffer since
                // static buffer was not large enough)
                secDescBuffer = NULL;
            }

            secDescLength = PVFS_MIN(
                                secDescLength*2,
                                SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE);
            ntError = PvfsReallocateMemory(
                          (PVOID*)&secDescBuffer,
                          secDescLength);
            BAIL_ON_NT_STATUS(ntError);
        }

        if (!eaNotFound)
        {
#ifdef HAVE_EA_SUPPORT
            ntError = PvfsGetSecurityDescriptorFilenameXattr(
                          pszFilename,
                          (PSECURITY_DESCRIPTOR_RELATIVE)secDescBuffer,
                          &secDescLength);
#endif
        }

        /* EA security descriptor support not available or present for
           this object */

        if ((ntError == STATUS_RESOURCE_DATA_NOT_FOUND) ||
            (ntError == STATUS_NOT_SUPPORTED))
        {
            eaNotFound = TRUE;

            secDescLength = origSecDescLength;

            ntError = PvfsGetSecurityDescriptorFilenamePosix(
                          pszFilename,
                          (PSECURITY_DESCRIPTOR_RELATIVE)secDescBuffer,
                          &secDescLength);
        }

    } while ((ntError == STATUS_BUFFER_TOO_SMALL) &&
             (secDescLength < SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE));
    BAIL_ON_NT_STATUS(ntError);

    /* If the caller wants the complete Security Descriptor, just copy
       the buffer */

    if (SecInfo == SecInfoAll)
    {
        if (*pSecDescLength < secDescLength)
        {
            ntError = STATUS_BUFFER_TOO_SMALL;
            BAIL_ON_NT_STATUS(ntError);
        }

        LwRtlCopyMemory(pSecDesc, secDescBuffer, secDescLength);
        *pSecDescLength = secDescLength;
    }
    else
    {
        ntError = RtlQuerySecurityDescriptorInfo(
                      SecInfo,
                      pSecDesc,
                      pSecDescLength,
                      (PSECURITY_DESCRIPTOR_RELATIVE)secDescBuffer);
        BAIL_ON_NT_STATUS(ntError);
    }

error:
    if (secDescBuffer != secDescBufferStatic)
    {
        PvfsFreeMemory((PVOID*)&secDescBuffer);
    }

    return ntError;
}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsSetSecurityDescriptorFile(
    IN PPVFS_CCB pCcb,
    IN SECURITY_INFORMATION SecInfo,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    IN ULONG SecDescLength
    )
{
    NTSTATUS ntError = STATUS_ACCESS_DENIED;

    BYTE secDescBufferStatic[PVFS_DEFAULT_SD_RELATIVE_SIZE];
    PBYTE secDescBuffer = secDescBufferStatic;
    ULONG secDescLength = PVFS_DEFAULT_SD_RELATIVE_SIZE;
    ULONG origSecDescLength = PVFS_DEFAULT_SD_RELATIVE_SIZE;

    BYTE newSecDescBufferStatic[PVFS_DEFAULT_SD_RELATIVE_SIZE] = {0};
    PBYTE newSecDescBuffer = newSecDescBufferStatic;
    ULONG newSecDescLength = PVFS_DEFAULT_SD_RELATIVE_SIZE;

    PSECURITY_DESCRIPTOR_RELATIVE finalSecDesc = NULL;
    ULONG finalSecDescLength = 0;

    PSECURITY_DESCRIPTOR_ABSOLUTE pIncAbsSecDesc = NULL;

    union {
        TOKEN_OWNER TokenOwnerInfo;
        BYTE Buffer[SID_MAX_SIZE];
    } TokenOwnerBuffer;
    PTOKEN_OWNER pTokenOwnerInformation = (PTOKEN_OWNER)&TokenOwnerBuffer;
    ULONG ulTokenOwnerLength = 0;

    union {
        SID Sid;
        BYTE Buffer[SID_MAX_SIZE];
    } LocalSystemSidBuffer;
    PSID pLocalSystemSid = (PSID)&LocalSystemSidBuffer;
    ULONG ulLocalSystemSidLength = sizeof(LocalSystemSidBuffer);

    SECURITY_INFORMATION SecInfoAll = (OWNER_SECURITY_INFORMATION |
                                       GROUP_SECURITY_INFORMATION |
                                       DACL_SECURITY_INFORMATION |
                                       SACL_SECURITY_INFORMATION);

    if (SecInfo == 0)
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* If the new Security Descriptor contains owner or group SID
       information, berify that the user's ACCESS_TOKEN contains the
       SID as a member */

    if (SecInfo & (OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION))
    {
        PSID pOwner = NULL;
        PSID pGroup = NULL;
        BOOLEAN bDefaulted = FALSE;

        ntError = PvfsSecurityAclSelfRelativeToAbsoluteSD(
                      &pIncAbsSecDesc,
                      pSecDesc);
        BAIL_ON_NT_STATUS(ntError);

        ntError = RtlQueryAccessTokenInformation(
                      pCcb->pUserToken,
                      TokenOwner,
                      (PVOID)pTokenOwnerInformation,
                      sizeof(TokenOwnerBuffer),
                      &ulTokenOwnerLength);
        BAIL_ON_NT_STATUS(ntError);

        ntError = RtlCreateWellKnownSid(
                      WinLocalSystemSid,
                      NULL,
                      pLocalSystemSid,
                      &ulLocalSystemSidLength);
        BAIL_ON_NT_STATUS(ntError);

        if (SecInfo & OWNER_SECURITY_INFORMATION)
        {
            ntError = RtlGetOwnerSecurityDescriptor(
                          pIncAbsSecDesc,
                          &pOwner,
                          &bDefaulted);
            BAIL_ON_NT_STATUS(ntError);

            if (!RtlIsSidMemberOfToken(pCcb->pUserToken, pOwner) &&
                !RtlEqualSid(pLocalSystemSid, pTokenOwnerInformation->Owner))
            {
                ntError = STATUS_ACCESS_DENIED;
                BAIL_ON_NT_STATUS(ntError);
            }
        }

        if (SecInfo & GROUP_SECURITY_INFORMATION)
        {
            ntError = RtlGetGroupSecurityDescriptor(
                          pIncAbsSecDesc,
                          &pGroup,
                          &bDefaulted);
            BAIL_ON_NT_STATUS(ntError);

            if (!RtlIsSidMemberOfToken(pCcb->pUserToken, pGroup) &&
                !RtlEqualSid(pLocalSystemSid, pTokenOwnerInformation->Owner))
            {
                ntError = STATUS_ACCESS_DENIED;
                BAIL_ON_NT_STATUS(ntError);
            }
        }
    }


    if (SecInfo == SecInfoAll)
    {
        /* We already have a fully formed Security Descriptor */

        finalSecDesc = pSecDesc;
        finalSecDescLength = SecDescLength;
    }
    else
    {
        /* Retrieve the existing SD and merge with the incoming one */

        ntError = STATUS_SUCCESS;

        do
        {
            if (ntError == STATUS_BUFFER_TOO_SMALL)
            {
                if (secDescBuffer == secDescBufferStatic)
                {
                    // Have to move to allocating the buffer since
                    // static buffer was not large enough)
                    secDescBuffer = NULL;
                }

                secDescLength = PVFS_MIN(
                                    secDescLength*2,
                                    SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE);
                ntError = PvfsReallocateMemory(
                              (PVOID*)&secDescBuffer,
                              secDescLength);
                BAIL_ON_NT_STATUS(ntError);
            }

            ntError = PvfsGetSecurityDescriptorFile(
                          pCcb,
                          SecInfoAll,
                          (PSECURITY_DESCRIPTOR_RELATIVE)secDescBuffer,
                          &secDescLength);

        } while ((ntError == STATUS_BUFFER_TOO_SMALL) &&
                 (secDescLength < SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE));
        BAIL_ON_NT_STATUS(ntError);

        if (secDescLength > origSecDescLength)
        {
            newSecDescLength = secDescLength;
            ntError = PvfsAllocateMemory(
                          (PVOID*)&newSecDescBuffer,
                          newSecDescLength,
                          TRUE);
            BAIL_ON_NT_STATUS(ntError);
        }

        ntError = RtlSetSecurityDescriptorInfo(
                      SecInfo,
                      pSecDesc,
                      (PSECURITY_DESCRIPTOR_RELATIVE)secDescBuffer,
                      (PSECURITY_DESCRIPTOR_RELATIVE)newSecDescBuffer,
                      &newSecDescLength,
                      &gPvfsFileGenericMapping);
        BAIL_ON_NT_STATUS(ntError);

        finalSecDesc = (PSECURITY_DESCRIPTOR_RELATIVE)newSecDescBuffer;
        finalSecDescLength = newSecDescLength;
    }


    /* Save the combined SD */

#ifdef HAVE_EA_SUPPORT
    ntError = PvfsSetSecurityDescriptorFileXattr(
                  pCcb,
                  finalSecDesc,
                  finalSecDescLength);
#else
    ntError = PvfsSetSecurityDescriptorPosix(
                  pCcb,
                  finalSecDesc,
                  finalSecDescLength);
#endif
    BAIL_ON_NT_STATUS(ntError);

    PvfsNotifyScheduleFullReport(
        pCcb->pScb->pOwnerFcb,
        FILE_NOTIFY_CHANGE_SECURITY,
        FILE_ACTION_MODIFIED,
        pCcb->pszFilename);

error:
    if (pIncAbsSecDesc)
    {
        PvfsFreeAbsoluteSecurityDescriptor(&pIncAbsSecDesc);
    }

    if (secDescBuffer != secDescBufferStatic)
    {
        PvfsFreeMemory((PVOID*)&secDescBuffer);
    }
    if (newSecDescBuffer != newSecDescBufferStatic)
    {
        PvfsFreeMemory((PVOID*)&newSecDescBuffer);
    }

    return ntError;
}

/****************************************************************
 ATTN: Always use RTL routines when allocating memory for
 PSECURITY_DESCRIPTOR_ABSOLUTE.  Else the Free() here will not
 be symmetic.
 ***************************************************************/

VOID
PvfsFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSID pOwner = NULL;
    PSID pGroup = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    BOOLEAN bDefaulted = FALSE;
    BOOLEAN bPresent = FALSE;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;

    if ((ppSecDesc == NULL) || (*ppSecDesc == NULL)) {
        return;
    }

    pSecDesc = *ppSecDesc;

    ntError = RtlGetOwnerSecurityDescriptor(pSecDesc, &pOwner, &bDefaulted);
    ntError = RtlGetGroupSecurityDescriptor(pSecDesc, &pGroup, &bDefaulted);

    ntError = RtlGetDaclSecurityDescriptor(pSecDesc, &bPresent, &pDacl, &bDefaulted);
    ntError = RtlGetSaclSecurityDescriptor(pSecDesc, &bPresent, &pSacl, &bDefaulted);

    RTL_FREE(&pSecDesc);
    RTL_FREE(&pOwner);
    RTL_FREE(&pGroup);
    RTL_FREE(&pDacl);
    RTL_FREE(&pSacl);

    *ppSecDesc = NULL;

    return;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsSecurityAclSelfRelativeToAbsoluteSD(
    PSECURITY_DESCRIPTOR_ABSOLUTE *ppAbsolute,
    PSECURITY_DESCRIPTOR_RELATIVE pRelative
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsolute = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    ULONG SecDescAbsSize = 0;
    ULONG OwnerSize = 0;
    ULONG GroupSize = 0;
    ULONG DaclSize = 0;
    ULONG SaclSize = 0;

    /* Get the necessary sizes */

    ntError = RtlSelfRelativeToAbsoluteSD(
                 pRelative,
                 pAbsolute,
                 &SecDescAbsSize,
                 pDacl,
                 &DaclSize,
                 pSacl,
                 &SaclSize,
                 pOwnerSid,
                 &OwnerSize,
                 pGroupSid,
                 &GroupSize);
    if (ntError != STATUS_BUFFER_TOO_SMALL)
    {
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = LW_RTL_ALLOCATE(
                  &pAbsolute,
                  VOID,
                  SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCreateSecurityDescriptorAbsolute(
                  pAbsolute,
                  SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntError);

    if (DaclSize)
    {
        ntError = LW_RTL_ALLOCATE(&pDacl, VOID, DaclSize);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (SaclSize)
    {
        ntError = LW_RTL_ALLOCATE(&pSacl, VOID, SaclSize);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (OwnerSize)
    {
        ntError = LW_RTL_ALLOCATE(&pOwnerSid, VOID, OwnerSize);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (GroupSize)
    {
        ntError = LW_RTL_ALLOCATE(&pGroupSid, VOID, GroupSize);
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Once more with feeling...This one should succeed. */

    ntError = RtlSelfRelativeToAbsoluteSD(
                 pRelative,
                 pAbsolute,
                 &SecDescAbsSize,
                 pDacl,
                 &DaclSize,
                 pSacl,
                 &SaclSize,
                 pOwnerSid,
                 &OwnerSize,
                 pGroupSid,
                 &GroupSize);
    BAIL_ON_NT_STATUS(ntError);

    *ppAbsolute = pAbsolute;
    pAbsolute = NULL;

cleanup:
    return ntError;

error:
    LW_RTL_FREE(&pOwnerSid);
    LW_RTL_FREE(&pGroupSid);
    LW_RTL_FREE(&pDacl);
    LW_RTL_FREE(&pSacl);
    LW_RTL_FREE(&pAbsolute);

    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsCreateFileSecurity(
    PACCESS_TOKEN pUserToken,
    PPVFS_CCB pCcb,
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    BOOLEAN bIsDirectory
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR pszParentPath = NULL;
    PSTR pszBaseFilename = NULL;

    BYTE parentSecDescBufferStatic[PVFS_DEFAULT_SD_RELATIVE_SIZE];
    PBYTE parentSecDescBuffer = parentSecDescBufferStatic;
    ULONG parentSecDescLength = PVFS_DEFAULT_SD_RELATIVE_SIZE;

    PSECURITY_DESCRIPTOR_RELATIVE finalSecDescBuffer = NULL;
    ULONG finalSecDescLength = 0;

    SECURITY_INFORMATION SecInfoAll = (OWNER_SECURITY_INFORMATION |
                                       GROUP_SECURITY_INFORMATION |
                                       DACL_SECURITY_INFORMATION  |
                                       SACL_SECURITY_INFORMATION);

    ntError = PvfsFileSplitPath(
                  &pszParentPath,
                  &pszBaseFilename,
                  pCcb->pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    do
    {
        if (ntError == STATUS_BUFFER_TOO_SMALL)
        {
            if (parentSecDescBuffer == parentSecDescBufferStatic)
            {
                // Have to move to allocating the buffer since
                // static buffer was not large enough)
                parentSecDescBuffer = NULL;
            }

            parentSecDescLength = PVFS_MIN(
                                      parentSecDescLength*2,
                                      SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE);
            ntError = PvfsReallocateMemory(
                          (PVOID*)&parentSecDescBuffer,
                          parentSecDescLength);
            BAIL_ON_NT_STATUS(ntError);
        }

        ntError = PvfsGetSecurityDescriptorFilename(
                      pszParentPath,
                      SecInfoAll,
                      (PSECURITY_DESCRIPTOR_RELATIVE)parentSecDescBuffer,
                      &parentSecDescLength);

    } while ((ntError == STATUS_BUFFER_TOO_SMALL) &&
             (parentSecDescLength < SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE));
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCreatePrivateObjectSecurityEx(
                  (PSECURITY_DESCRIPTOR_RELATIVE)parentSecDescBuffer,
                  pSecurityDescriptor,
                  &finalSecDescBuffer,
                  &finalSecDescLength,
                  NULL,
                  bIsDirectory,
                  SEF_DACL_AUTO_INHERIT|SEF_SACL_AUTO_INHERIT,
                  pUserToken,
                  &gPvfsFileGenericMapping);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSetSecurityDescriptorFile(
                  pCcb,
                  SecInfoAll,
                  finalSecDescBuffer,
                  finalSecDescLength);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    LW_RTL_FREE(&finalSecDescBuffer);
    if (parentSecDescBuffer != parentSecDescBufferStatic)
    {
        PvfsFreeMemory((PVOID*)&parentSecDescBuffer);
    }

    RtlCStringFree(&pszParentPath);
    RtlCStringFree(&pszBaseFilename);

    return ntError;

error:
    goto cleanup;
}

