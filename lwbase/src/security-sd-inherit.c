/*
 * Copyright (c) Likewise Software.  All rights Reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Module Name:
 *
 *        security-sd-inherit.c
 *
 * Abstract:
 *
 *        Security Descriptor Inheritance (SD) Functions in
 *        Security Module.
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *
 */

#include "security-includes.h"


static
NTSTATUS
RtlpCreateAbsSecDescFromRelative(
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppAbsSecDesc,
    IN  PSECURITY_DESCRIPTOR_RELATIVE pRelSecDesc
    );

static
VOID
RtlpFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    );


static
NTSTATUS
RtlpObjectSetOwner(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken
    );

static
NTSTATUS
RtlpObjectSetGroup(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken
    );

static
NTSTATUS
RtlpObjectSetDacl(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN BOOLEAN bIsContainerObject,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken,
    IN PGENERIC_MAPPING pGenericMap
    );

static
NTSTATUS
RtlpObjectSetSacl(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN BOOLEAN bIsContainerObject,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken,
    IN PGENERIC_MAPPING pGenericMap
    );

NTSTATUS
RtlCreatePrivateObjectSecurityEx(
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE pCreatorSecDesc,
    OUT PSECURITY_DESCRIPTOR_RELATIVE *ppNewSecDesc,
    OUT PULONG pNewSecDescLength,
    IN OPTIONAL PVOID pObjectType,  // Unused
    IN BOOLEAN bIsContainerObject,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken,
    IN PGENERIC_MAPPING pGenericMap
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_RELATIVE pNewSecDesc = NULL;
    ULONG ulNewSecDescLength = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsParentSecDesc = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsCreatorSecDesc = NULL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsNewSecDesc = NULL;

    if (!ppNewSecDesc || !pNewSecDescLength || !pGenericMap)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    *ppNewSecDesc = NULL;
    *pNewSecDescLength = 0;

    // pUserToken can only be NULL if bother the following flags
    // Are set in the AutoInheritFlags
    //      (SEF_AVOID_OWNER_CHECK|SEF_AVOID_PRIVILEGE_CHECK)

    if (!((AutoInheritFlags & (SEF_AVOID_OWNER_CHECK|SEF_AVOID_PRIVILEGE_CHECK))
          != (SEF_AVOID_OWNER_CHECK|SEF_AVOID_PRIVILEGE_CHECK)) &&
        (pUserToken == NULL))
    {
        status = STATUS_NO_TOKEN;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (pParentSecDesc)
    {
        status = RtlpCreateAbsSecDescFromRelative(
                     &pAbsParentSecDesc,
                     pParentSecDesc);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (pCreatorSecDesc)
    {
        status = RtlpCreateAbsSecDescFromRelative(
                     &pAbsCreatorSecDesc,
                     pParentSecDesc);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // Create new Absolute SecDesc

    status= LW_RTL_ALLOCATE(
                 &pAbsNewSecDesc,
                 SECURITY_DESCRIPTOR_ABSOLUTE,
                 SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCreateSecurityDescriptorAbsolute(
                  pAbsNewSecDesc,
                  SECURITY_DESCRIPTOR_REVISION);
    GOTO_CLEANUP_ON_STATUS(status);

    // Set Owner

    status = RtlpObjectSetOwner(
                 pAbsNewSecDesc,
                 pAbsParentSecDesc,
                 pAbsCreatorSecDesc,
                 AutoInheritFlags,
                 pUserToken);
    GOTO_CLEANUP_ON_STATUS(status);

    // Set Group

    status = RtlpObjectSetGroup(
                 pAbsNewSecDesc,
                 pAbsParentSecDesc,
                 pAbsCreatorSecDesc,
                 AutoInheritFlags,
                 pUserToken);
    GOTO_CLEANUP_ON_STATUS(status);

    // Set DACL

    status = RtlpObjectSetDacl(
                 pAbsNewSecDesc,
                 pAbsParentSecDesc,
                 pAbsCreatorSecDesc,
                 bIsContainerObject,
                 AutoInheritFlags,
                 pUserToken,
                 pGenericMap);
    GOTO_CLEANUP_ON_STATUS(status);

    // Set SACL

    status = RtlpObjectSetSacl(
                 pAbsNewSecDesc,
                 pAbsParentSecDesc,
                 pAbsCreatorSecDesc,
                 bIsContainerObject,
                 AutoInheritFlags,
                 pUserToken,
                 pGenericMap);
    GOTO_CLEANUP_ON_STATUS(status);

    // All done - convert to Self-Relative form and return

    status = RtlAbsoluteToSelfRelativeSD(
                 pAbsNewSecDesc,
                 NULL,
                 &ulNewSecDescLength);
    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        status = LW_RTL_ALLOCATE(
                     (PVOID*)&pNewSecDesc,
                     SECURITY_DESCRIPTOR_RELATIVE,
                     ulNewSecDescLength);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlAbsoluteToSelfRelativeSD(
                     pAbsNewSecDesc,
                     pNewSecDesc,
                     &ulNewSecDescLength);
    }
    GOTO_CLEANUP_ON_STATUS(status);

    *ppNewSecDesc = pNewSecDesc;
    *pNewSecDescLength = ulNewSecDescLength;

cleanup:
    // Normal cleanup

    if (pAbsParentSecDesc)
    {
        RtlpFreeAbsoluteSecurityDescriptor(&pAbsParentSecDesc);
    }

    if (pAbsCreatorSecDesc)
    {
        RtlpFreeAbsoluteSecurityDescriptor(&pAbsCreatorSecDesc);
    }

    if (pAbsNewSecDesc)
    {
        RtlpFreeAbsoluteSecurityDescriptor(&pAbsNewSecDesc);
    }

    // Failure cleanup

    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&pNewSecDesc);
    }


    return status;
}


static
NTSTATUS
RtlpCreateAbsSecDescFromRelative(
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppAbsSecDesc,
    IN PSECURITY_DESCRIPTOR_RELATIVE pRelSecDesc
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pAbsSecDesc = NULL;
    ULONG ulAbsSecDescLen = 0;
    PACL pDacl = NULL;
    ULONG ulDaclLen = 0;
    PACL pSacl = NULL;
    ULONG ulSaclLen = 0;
    PSID pOwner = NULL;
    ULONG ulOwnerLen = 0;
    PSID pGroup = NULL;
    ULONG ulGroupLen = 0;

    // Get sizes

    status = RtlSelfRelativeToAbsoluteSD(
                   pRelSecDesc,
                   pAbsSecDesc,
                   &ulAbsSecDescLen,
                   pDacl,
                   &ulDaclLen,
                   pSacl,
                   &ulSaclLen,
                   pOwner,
                   &ulOwnerLen,
                   pGroup,
                   &ulGroupLen);
    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        status = STATUS_SUCCESS;
    }
    GOTO_CLEANUP_ON_STATUS(status);

    // Allocate

    if (ulOwnerLen)
    {
        status = RTL_ALLOCATE(&pOwner, SID, ulOwnerLen);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (ulGroupLen)
    {
        status = RTL_ALLOCATE(&pGroup, SID, ulGroupLen);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (ulDaclLen)
    {
        status = RTL_ALLOCATE(&pDacl, VOID, ulDaclLen);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (ulSaclLen)
    {
        status = RTL_ALLOCATE(&pSacl, VOID, ulSaclLen);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RTL_ALLOCATE(&pAbsSecDesc, VOID, ulAbsSecDescLen);
    GOTO_CLEANUP_ON_STATUS(status);

    // Translate

    status = RtlSelfRelativeToAbsoluteSD(
                   pRelSecDesc,
                   pAbsSecDesc,
                   &ulAbsSecDescLen,
                   pDacl,
                   &ulDaclLen,
                   pSacl,
                   &ulSaclLen,
                   pOwner,
                   &ulOwnerLen,
                   pGroup,
                   &ulGroupLen);
    GOTO_CLEANUP_ON_STATUS(status);

    *ppAbsSecDesc = pAbsSecDesc;

cleanup:
    // Failure cleanup

    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&pAbsSecDesc);
        LW_RTL_FREE(&pOwner);
        LW_RTL_FREE(&pGroup);
        LW_RTL_FREE(&pSacl);
        LW_RTL_FREE(&pDacl);
    }

    return status;
}

static
VOID
RtlpFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID pOwner = NULL;
    PSID pGroup = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    BOOLEAN bDefaulted = FALSE;
    BOOLEAN bPresent = FALSE;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;

    if ((ppSecDesc == NULL) || (*ppSecDesc == NULL))
    {
        return;
    }

    pSecDesc = *ppSecDesc;

    status = RtlGetOwnerSecurityDescriptor(pSecDesc, &pOwner, &bDefaulted);
    status = RtlGetGroupSecurityDescriptor(pSecDesc, &pGroup, &bDefaulted);

    status = RtlGetDaclSecurityDescriptor(pSecDesc, &bPresent, &pDacl, &bDefaulted);
    status = RtlGetSaclSecurityDescriptor(pSecDesc, &bPresent, &pSacl, &bDefaulted);

    RTL_FREE(&pSecDesc);
    RTL_FREE(&pOwner);
    RTL_FREE(&pGroup);
    RTL_FREE(&pDacl);
    RTL_FREE(&pSacl);

    *ppSecDesc = NULL;

    return;
}


static
NTSTATUS
RtlpObjectSetOwner(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken
    )
{
    NTSTATUS status = STATUS_SUCCESS;
#if 0
    PSID pCreatorSecDescOwner = NULL;
    PSID pParentSecDescOwner = NULL;
    BOOLEAN bDefaulted = FALSE;
#endif
    PSID pOwner = NULL;
    union {
        TOKEN_OWNER TokeUserInfo;
        BYTE Buffer[SID_MAX_SIZE];
    } TokenOwnerBuffer;
    PTOKEN_OWNER pTokenOwnerInformation = (PTOKEN_OWNER)&TokenOwnerBuffer;
    ULONG ulTokenOwnerLength = 0;

#if 0
    // Check is disabled until we have an LsaPrivilege model
    // support SeRestorePrivilege

    // Always use the CreatorSecDesc Owner if present

    if (pCreatorSecDesc)
    {
        status = RtlGetOwnerSecurityDescriptor(
                     pCreatorSecDesc,
                     &pCreatorSecDescOwner,
                     &bDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        if (pCreatorSecDescOwner)
        {
            status = RtlDuplicateSid(&pOwner, pCreatorSecDescOwner);
            GOTO_CLEANUP_ON_STATUS(status);

            status = RtlSetOwnerSecurityDescriptor(
                         pSecurityDescriptor,
                         pOwner,
                         FALSE);
            GOTO_CLEANUP_ON_STATUS(status);

            goto cleanup;
        }
    }

    // Check for Defaulting the owner from the parent Sec Desc

    if (AutoInheritFlags & SEF_DEFAULT_OWNER_FROM_PARENT)
    {
        status = RtlGetOwnerSecurityDescriptor(
                     pParentSecDesc,
                     &pParentSecDescOwner,
                     &bDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        if (pParentSecDescOwner)
        {
            status = RtlDuplicateSid(&pOwner, pCreatorSecDescOwner);
            GOTO_CLEANUP_ON_STATUS(status);

            status = RtlSetOwnerSecurityDescriptor(
                         pSecurityDescriptor,
                         pOwner,
                         TRUE);
            GOTO_CLEANUP_ON_STATUS(status);

            goto cleanup;
        }
    }
#endif   // End of disabled code

    // Copy the owner of the ACCESS_TOKEN

    status = RtlQueryAccessTokenInformation(
                 pUserToken,
                 TokenOwner,
                 (PVOID)pTokenOwnerInformation,
                 sizeof(TokenOwnerBuffer),
                 &ulTokenOwnerLength);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlDuplicateSid(&pOwner, pTokenOwnerInformation->Owner);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSetOwnerSecurityDescriptor(
                 pSecurityDescriptor,
                 pOwner,
                 TRUE);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    // Failure cleanup

    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&pOwner);
    }


    return status;
}


static
NTSTATUS
RtlpObjectSetGroup(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken
    )
{
    NTSTATUS status = STATUS_SUCCESS;
#if 0
    PSID pCreatorSecDescGroup = NULL;
    PSID pParentSecDescGroup = NULL;
    BOOLEAN bDefaulted = FALSE;
#endif
    PSID pGroup = NULL;
    struct {
        TOKEN_USER TokeUserInfo;
        BYTE Buffer[SID_MAX_SIZE];
    } TokenPrimaryGroupBuffer;
    PTOKEN_PRIMARY_GROUP pTokenPrimaryGroupInfo = (PTOKEN_PRIMARY_GROUP)&TokenPrimaryGroupBuffer;
    ULONG ulTokenPrimaryGroupLength = 0;

#if 0
    // Check is disabled until we have an LsaPrivilege model
    // support SeRestorePrivilege

    // Always use the CreatorSecDesc Group if present

    if (pCreatorSecDesc)
    {
        status = RtlGetGroupSecurityDescriptor(
                     pCreatorSecDesc,
                     &pCreatorSecDescGroup,
                     &bDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        if (pCreatorSecDescGroup)
        {
            status = RtlDuplicateSid(&pGroup, pCreatorSecDescGroup);
            GOTO_CLEANUP_ON_STATUS(status);

            status = RtlSetGroupSecurityDescriptor(
                         pSecurityDescriptor,
                         pGroup,
                         FALSE);
            GOTO_CLEANUP_ON_STATUS(status);

            goto cleanup;
        }
    }

    // Check for Defaulting the owner from the parent Sec Desc

    if (AutoInheritFlags & SEF_DEFAULT_GROUP_FROM_PARENT)
    {
        status = RtlGetGroupSecurityDescriptor(
                     pParentSecDesc,
                     &pParentSecDescGroup,
                     &bDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        if (pParentSecDescGroup)
        {
            status = RtlDuplicateSid(&pGroup, pCreatorSecDescGroup);
            GOTO_CLEANUP_ON_STATUS(status);

            status = RtlSetGroupSecurityDescriptor(
                         pSecurityDescriptor,
                         pGroup,
                         TRUE);
            GOTO_CLEANUP_ON_STATUS(status);

            goto cleanup;
        }
    }
#endif   // End of disabled code

    // Copy the Group SID from the Token

    status = RtlQueryAccessTokenInformation(
                 pUserToken,
                 TokenPrimaryGroup,
                 (PVOID)pTokenPrimaryGroupInfo,
                 sizeof(TokenPrimaryGroupBuffer),
                 &ulTokenPrimaryGroupLength);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlDuplicateSid(&pGroup, pTokenPrimaryGroupInfo->PrimaryGroup);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSetGroupSecurityDescriptor(
                 pSecurityDescriptor,
                 pGroup,
                 TRUE);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    // Failure cleanup

    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&pGroup);
    }

    return status;
}

static
NTSTATUS
RtlpDuplicateDacl(
    PACL *ppNewDacl,
    PACL pSrcDacl
    );

static
NTSTATUS
RtlpObjectInheritSecurity(
    OUT PACL *ppNewDacl,
    OUT PBOOLEAN pbIsNewDaclDefaulted,
    IN OPTIONAL PACL pParentDacl,
    IN BOOLEAN bParentIsDaclDefaulted,
    IN OPTIONAL PACL pCreatorDacl,
    IN BOOLEAN bCreatorIsDaclDefaulted,
    IN BOOLEAN bIsContainerObject,
    IN PGENERIC_MAPPING pGenericMap
    );

static
NTSTATUS
RtlpObjectSetDacl(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN BOOLEAN bIsContainerObject,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken,
    IN PGENERIC_MAPPING pGenericMap
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACL pParentDacl = NULL;
    PACL pCreatorDacl = NULL;
    PACL pFinalDacl = NULL;
    BOOLEAN bCreatorIsDaclPresent = FALSE;
    BOOLEAN bCreatorIsDaclDefaulted = FALSE;
    BOOLEAN bParentIsDaclPresent = FALSE;
    BOOLEAN bParentIsDaclDefaulted = FALSE;
    BOOLEAN bFinalIsDaclDefaulted = FALSE;
    SECURITY_DESCRIPTOR_CONTROL CreatorSecDescControl = 0;

    // Sanity check - ACCESS_TOKEN does not have a default DACL currently

    if (!pParentSecDesc && !pCreatorSecDesc)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // Pull the DACLs so we have something to work with

    if (pParentSecDesc)
    {
        status = RtlGetDaclSecurityDescriptor(
                     pParentSecDesc,
                     &bParentIsDaclPresent,
                     &pParentDacl,
                     &bParentIsDaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (pCreatorSecDesc)
    {
        status = RtlGetDaclSecurityDescriptor(
                     pCreatorSecDesc,
                     &bCreatorIsDaclPresent,
                     &pCreatorDacl,
                     &bCreatorIsDaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlGetSecurityDescriptorControl(
                     pCreatorSecDesc,
                     &CreatorSecDescControl,
                     NULL);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    // If the creator wants to block inheritance, we are done

    if (bCreatorIsDaclPresent &&
        (CreatorSecDescControl & SE_DACL_PROTECTED))
    {
        status = RtlpDuplicateDacl(&pFinalDacl, pCreatorDacl);
        GOTO_CLEANUP_ON_STATUS(status);

        bFinalIsDaclDefaulted = FALSE;
    }
    else  // Do the inheritance
    {
        status = RtlpObjectInheritSecurity(
                     &pFinalDacl,
                     &bFinalIsDaclDefaulted,
                     bParentIsDaclPresent ? pParentDacl : NULL,
                     bParentIsDaclDefaulted,
                     bCreatorIsDaclPresent ? pCreatorDacl : NULL,
                     bCreatorIsDaclDefaulted,
                     bIsContainerObject,
                     pGenericMap);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RtlSetDaclSecurityDescriptor(
                 pSecurityDescriptor,
                 pFinalDacl ? TRUE : FALSE,
                 pFinalDacl,
                 bFinalIsDaclDefaulted);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    // Failure cleanup

    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&pFinalDacl);
    }

    return status;
}

static
NTSTATUS
RtlpDuplicateDacl(
    PACL *ppNewDacl,
    PACL pSrcDacl
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACL pDacl = NULL;
    USHORT usDaclSize = 0;

    usDaclSize = RtlGetAclSize(pSrcDacl);

    status = LW_RTL_ALLOCATE(&pDacl, VOID, usDaclSize);
    GOTO_CLEANUP_ON_STATUS(status);

    RtlCopyMemory(pDacl, pSrcDacl, usDaclSize);

    *ppNewDacl = pDacl;

cleanup:
    return status;
}

static
NTSTATUS
RtlpObjectInheritSecurity(
    OUT PACL *ppNewDacl,
    OUT PBOOLEAN pbIsNewDaclDefaulted,
    IN OPTIONAL PACL pParentDacl,
    IN BOOLEAN bParentIsDaclDefaulted,
    IN OPTIONAL PACL pCreatorDacl,
    IN BOOLEAN bCreatorIsDaclDefaulted,
    IN BOOLEAN bIsContainerObject,
    IN PGENERIC_MAPPING pGenericMap
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACL pDacl = NULL;
    USHORT usDaclSize = 0;
    USHORT usCreatorNumAces = 0;
    USHORT usParentNumAces = 0;
    USHORT i = 0;
    PACE_HEADER pAceHeader = NULL;
    PACCESS_ALLOWED_ACE pAllowAce = NULL;
    PACCESS_DENIED_ACE pDenyAce = NULL;

    // Inheritance Algorithm:
    //
    // (a) If defaulted creator DACL and no parent, just use defaulted
    //     creator DACL,
    //
    // else
    //
    // (b) Add in all creator DACL entries
    // (c)  Add in inheritable ACEs from parent
    //      i.   Skip if is a container and is NOT a container inherit
    //           ace, or if is NOT a container and is NOT a object
    //           (non-container) inherit ace
    //      ii.  For containers, add propagatable inheritance entries
    //      iii. Add inherited ace, applying generic map
    //

    // Case (a)

    if (!pParentDacl && bCreatorIsDaclDefaulted)
    {
        status = RtlpDuplicateDacl(&pDacl, pCreatorDacl);
        GOTO_CLEANUP_ON_STATUS(status);

        *pbIsNewDaclDefaulted = TRUE;
        *ppNewDacl = pDacl;

        goto cleanup;
    }

    if (pCreatorDacl)
    {
        usDaclSize = RtlGetAclSizeUsed(pCreatorDacl);
    }

    if (pParentDacl)
    {
        usDaclSize += (RtlGetAclSizeUsed(pParentDacl) * 2);
    }

    if (usDaclSize <= 0)
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = LW_RTL_ALLOCATE(&pDacl, VOID, usDaclSize);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlCreateAcl(pDacl, usDaclSize, ACL_REVISION);
    GOTO_CLEANUP_ON_STATUS(status);

    // Case (b) - Direct ACEs first

    if (pCreatorDacl)
    {
        usCreatorNumAces = RtlGetAclAceCount(pCreatorDacl);
    }

    for (i=0; i<usCreatorNumAces; i++)
    {
        status = RtlGetAce(pCreatorDacl, i, (PVOID*)&pAceHeader);
        GOTO_CLEANUP_ON_STATUS(status);

        switch(pAceHeader->AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            pAllowAce = (PACCESS_ALLOWED_ACE)pAceHeader;
            status = RtlAddAccessAllowedAceEx(
                         pDacl,
                         ACL_REVISION,
                         pAllowAce->Header.AceFlags,
                         pAllowAce->Mask,
                         (PSID)&pAllowAce->SidStart);
            GOTO_CLEANUP_ON_STATUS(status);
            break;

        case ACCESS_DENIED_ACE_TYPE:
            pDenyAce = (PACCESS_DENIED_ACE)pAceHeader;
            status = RtlAddAccessDeniedAceEx(
                         pDacl,
                         ACL_REVISION,
                         pDenyAce->Header.AceFlags,
                         pDenyAce->Mask,
                         (PSID)&pDenyAce->SidStart);
            GOTO_CLEANUP_ON_STATUS(status);
            break;

        default:
            // Skip all other types
            break;
        }
    }

    // Case (c) - Inheritable ACEs

    if (pParentDacl)
    {
        usParentNumAces = RtlGetAclAceCount(pParentDacl);
    }

    for (i=0; i<usParentNumAces; i++)
    {
        ACCESS_MASK Mask;
        UCHAR AceFlags;

        status = RtlGetAce(pParentDacl, i, (PVOID*)&pAceHeader);
        GOTO_CLEANUP_ON_STATUS(status);

        // Skip if no inheritable access rights

        if ((bIsContainerObject &&
             !(pAceHeader->AceFlags & CONTAINER_INHERIT_ACE)) ||
            (!bIsContainerObject &&
             !(pAceHeader->AceFlags & OBJECT_INHERIT_ACE)))
        {
            continue;
        }

        // See if the inherit ACE should continue to be propagated.
        // If so, then copy it

        if (bIsContainerObject &&
            !(pAceHeader->AceFlags & NO_PROPAGATE_INHERIT_ACE))
        {
            switch(pAceHeader->AceType)
            {
            case ACCESS_ALLOWED_ACE_TYPE:
                pAllowAce = (PACCESS_ALLOWED_ACE)pAceHeader;
                Mask = pAllowAce->Mask;
                status = RtlAddAccessAllowedAceEx(
                             pDacl,
                             ACL_REVISION,
                             pAllowAce->Header.AceFlags,
                             pAllowAce->Mask,
                             (PSID)&pAllowAce->SidStart);
                GOTO_CLEANUP_ON_STATUS(status);
                break;

            case ACCESS_DENIED_ACE_TYPE:
                pDenyAce = (PACCESS_DENIED_ACE)pAceHeader;
                Mask = pDenyAce->Mask;
                status = RtlAddAccessDeniedAceEx(
                             pDacl,
                             ACL_REVISION,
                             pDenyAce->Header.AceFlags,
                             pDenyAce->Mask,
                             (PSID)&pDenyAce->SidStart);
                GOTO_CLEANUP_ON_STATUS(status);
                break;

            default:
                // Skip all other types
                continue;
            }
        }

        // Map the generic bits to specific bits and remove inherit flags

        RtlMapGenericMask(&Mask, pGenericMap);
        AceFlags = pAceHeader->AceFlags;
        AceFlags &= ~(CONTAINER_INHERIT_ACE|
                      OBJECT_INHERIT_ACE|
                      NO_PROPAGATE_INHERIT_ACE);
        AceFlags |= INHERITED_ACE;

        // Set the inherited direct ACE

        switch(pAceHeader->AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            pAllowAce = (PACCESS_ALLOWED_ACE)pAceHeader;
            status = RtlAddAccessAllowedAceEx(
                         pDacl,
                         ACL_REVISION,
                         AceFlags,
                         Mask,
                         (PSID)&pAllowAce->SidStart);
                GOTO_CLEANUP_ON_STATUS(status);
                break;

        case ACCESS_DENIED_ACE_TYPE:
            pDenyAce = (PACCESS_DENIED_ACE)pAceHeader;
            status = RtlAddAccessDeniedAceEx(
                         pDacl,
                         ACL_REVISION,
                         AceFlags,
                         Mask,
                         (PSID)&pDenyAce->SidStart);
            GOTO_CLEANUP_ON_STATUS(status);
            break;

        default:
            // Skip all other types
            continue;
        }
    }

    *pbIsNewDaclDefaulted = FALSE;
    *ppNewDacl = pDacl;

    status = STATUS_SUCCESS;

cleanup:
    // Failure cleanup

    if (!NT_SUCCESS(status))
    {
        LW_RTL_FREE(&pDacl);
    }

    return status;
}


static
NTSTATUS
RtlpObjectSetSacl(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pParentSecDesc,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pCreatorSecDesc,
    IN BOOLEAN bIsContainerObject,
    IN ULONG AutoInheritFlags,
    IN OPTIONAL PACCESS_TOKEN pUserToken,
    IN PGENERIC_MAPPING pGenericMap
    )
{
    // No SACL support currently

    return STATUS_SUCCESS;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

