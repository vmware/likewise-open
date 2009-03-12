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
 *        security-token.c
 *
 * Abstract:
 *
 *        Token/Access Functions in Security Module.
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#include "security-includes.h"

//
// ACCESS_MASK Functions
//

static
inline
VOID
RtlpMapGenericMaskSingleFlag(
    IN OUT PACCESS_MASK AccessMask,
    IN ACCESS_MASK GenericFlag,
    IN ACCESS_MASK SpecificFlags
    )
{
    if (IsSetFlag(*AccessMask, GenericFlag))
    {
        ClearFlag(*AccessMask, GenericFlag);
        SetFlag(*AccessMask, SpecificFlags);
    }
}

VOID
RtlMapGenericMask(
    IN OUT PACCESS_MASK AccessMask,
    IN PGENERIC_MAPPING GenericMapping
    )
{
    RtlpMapGenericMaskSingleFlag(AccessMask, GENERIC_READ, GenericMapping->GenericRead);
    RtlpMapGenericMaskSingleFlag(AccessMask, GENERIC_WRITE, GenericMapping->GenericWrite);
    RtlpMapGenericMaskSingleFlag(AccessMask, GENERIC_EXECUTE, GenericMapping->GenericExecute);
    RtlpMapGenericMaskSingleFlag(AccessMask, GENERIC_ALL, GenericMapping->GenericAll);
}

//
// Access Token Functions
//

inline
static
PVOID
RtlpAppendData(
    IN PVOID Location,
    IN PVOID Data,
    IN ULONG DataSize
    )
{
    RtlCopyMemory(Location, Data, DataSize);
    return LW_PTR_ADD(Location, DataSize);
}

NTSTATUS
RtlCreateAccessToken(
    OUT PACCESS_TOKEN* AccessToken,
    IN PTOKEN_USER User,
    IN PTOKEN_GROUPS Groups,
    IN PTOKEN_OWNER Owner,
    IN PTOKEN_PRIMARY_GROUP PrimaryGroup,
    IN PTOKEN_DEFAULT_DACL DefaultDacl,
    IN OPTIONAL PTOKEN_UNIX Unix
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG requiredSize = 0;
    PACCESS_TOKEN token = NULL;
    ULONG i = 0;
    ULONG size = 0;
    PVOID location = NULL;

    if (!User || !User->User.Sid ||
        !Groups ||
        !Owner ||
        !PrimaryGroup ||
        !DefaultDacl)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!RtlValidSid(User->User.Sid) ||
        (Owner->Owner && !RtlValidSid(Owner->Owner)) ||
        (PrimaryGroup->PrimaryGroup && !RtlValidSid(PrimaryGroup->PrimaryGroup)))
    {
        status = STATUS_INVALID_SID;
        GOTO_CLEANUP();
    }

    // No user attributes currently exist.
    if (User->User.Attributes != 0)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    for (i = 0; i < Groups->GroupCount; i++)
    {
        // TODO-Perhaps validate Group attributes
        if (!Groups->Groups[i].Sid)
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP();
        }
        if (!RtlValidSid(Groups->Groups[i].Sid))
        {
            status = STATUS_INVALID_SID;
            GOTO_CLEANUP();
        }
    }

    if (DefaultDacl->DefaultDacl &&
        !RtlValidAcl(DefaultDacl->DefaultDacl, NULL))
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    // Compute size required

    requiredSize = sizeof(*token);

    size = RtlLengthSid(User->User.Sid);
    status = RtlSafeAddULONG(&requiredSize, requiredSize, size);
    GOTO_CLEANUP_ON_STATUS(status);

    if (Owner->Owner)
    {
        size = RtlLengthSid(Owner->Owner);
        status = RtlSafeAddULONG(&requiredSize, requiredSize, size);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (PrimaryGroup->PrimaryGroup)
    {
        size = RtlLengthSid(PrimaryGroup->PrimaryGroup);
        status = RtlSafeAddULONG(&requiredSize, requiredSize, size);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (DefaultDacl->DefaultDacl)
    {
        status = RtlSafeAddULONG(&requiredSize, requiredSize,
                                 DefaultDacl->DefaultDacl->AclSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RtlSafeMultiplyULONG(&size, sizeof(Groups->Groups[0]), Groups->GroupCount);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlSafeAddULONG(&requiredSize, requiredSize, size);
    GOTO_CLEANUP_ON_STATUS(status);

    for (i = 0; i < Groups->GroupCount; i++)
    {
        size = RtlLengthSid(Groups->Groups[i].Sid);

        status = RtlSafeAddULONG(&requiredSize, requiredSize, size);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RTL_ALLOCATE(&token, ACCESS_TOKEN, requiredSize);
    GOTO_CLEANUP_ON_STATUS(status);

    location = LW_PTR_ADD(token, sizeof(*token));

    // Initialize

    token->Flags = 0;

    token->User.User.Attributes = User->User.Attributes;
    token->User.User.Sid = (PSID) location;
    location = RtlpAppendData(location,
                              User->User.Sid,
                              RtlLengthSid(User->User.Sid));

    token->Groups.GroupCount = Groups->GroupCount;
    for (i = 0; i < Groups->GroupCount; i++)
    {
        token->Groups.Groups[i].Attributes = Groups->Groups[i].Attributes;
        token->Groups.Groups[i].Sid = (PSID) location;
        location = RtlpAppendData(location,
                                  Groups->Groups[i].Sid,
                                  RtlLengthSid(Groups->Groups[i].Sid));
    }

    if (Owner->Owner)
    {
        token->Owner.Owner = (PSID) location;
        location = RtlpAppendData(location,
                                  Owner->Owner,
                                  RtlLengthSid(Owner->Owner));
    }

    if (PrimaryGroup->PrimaryGroup)
    {
        token->PrimaryGroup.PrimaryGroup = (PSID) location;
        location = RtlpAppendData(location,
                                  PrimaryGroup->PrimaryGroup,
                                  RtlLengthSid(PrimaryGroup->PrimaryGroup));
    }

    if (DefaultDacl->DefaultDacl)
    {
        token->DefaultDacl.DefaultDacl = (PACL) location;
        location = RtlpAppendData(location,
                                  DefaultDacl->DefaultDacl,
                                  DefaultDacl->DefaultDacl->AclSize);
    }

    if (Unix)
    {
        SetFlag(token->Flags, ACCESS_TOKEN_FLAG_UNIX_PRESENT);
        token->Unix = *Unix;
    }

    if (location != LW_PTR_ADD(token, requiredSize))
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        RtlReleaseAccessToken(&token);
    }

    *AccessToken = token;

    return status;
}

VOID
RtlReleaseAccessToken(
    IN OUT PACCESS_TOKEN* AccessToken
    )
{
    RTL_FREE(AccessToken);
}

NTSTATUS
RtlQueryAccessTokenInformation(
    IN PACCESS_TOKEN AccessToken,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT OPTIONAL PVOID TokenInformation,
    IN ULONG TokenInformationLength,
    OUT PULONG ReturnedLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG requiredSize = 0;
    ULONG i = 0;
    PVOID location = NULL;

    if (!AccessToken || !ReturnedLength)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!TokenInformation && (TokenInformationLength != 0))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    // Check required size
    switch (TokenInformationClass)
    {
        case TokenUser:
            status = RtlSafeAddULONG(
                            &requiredSize,
                            sizeof(TOKEN_USER),
                            RtlLengthSid(AccessToken->User.User.Sid));
            GOTO_CLEANUP_ON_STATUS(status);
            break;
        case TokenGroups:
            status = RtlSafeMultiplyULONG(
                        &requiredSize,
                        sizeof(AccessToken->Groups.Groups[0]),
                        AccessToken->Groups.GroupCount);
            GOTO_CLEANUP_ON_STATUS(status);

            status = RtlSafeAddULONG(&requiredSize, requiredSize, sizeof(TOKEN_GROUPS));
            GOTO_CLEANUP_ON_STATUS(status);

            for (i = 0; i < AccessToken->Groups.GroupCount; i++)
            {
                status = RtlSafeAddULONG(
                                &requiredSize,
                                requiredSize,
                                RtlLengthSid(AccessToken->Groups.Groups[i].Sid));
                GOTO_CLEANUP_ON_STATUS(status);
            }
            break;
        case TokenOwner:
            status = RtlSafeAddULONG(
                            &requiredSize,
                            sizeof(TOKEN_OWNER),
                            RtlLengthSid(AccessToken->Owner.Owner));
            GOTO_CLEANUP_ON_STATUS(status);
            break;
        case TokenPrimaryGroup:
            status = RtlSafeAddULONG(
                            &requiredSize,
                            sizeof(TOKEN_PRIMARY_GROUP),
                            RtlLengthSid(AccessToken->PrimaryGroup.PrimaryGroup));
            GOTO_CLEANUP_ON_STATUS(status);
            break;
        case TokenDefaultDacl:
            status = RtlSafeAddULONG(
                            &requiredSize,
                            sizeof(TOKEN_DEFAULT_DACL),
                            (AccessToken->DefaultDacl.DefaultDacl ?
                             AccessToken->DefaultDacl.DefaultDacl->AclSize :
                             0));
            GOTO_CLEANUP_ON_STATUS(status);
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP();
    }

    if (requiredSize > TokenInformationLength)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    if (!TokenInformation)
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    // Copy data
    switch (TokenInformationClass)
    {
        case TokenUser:
        {
            PTOKEN_USER user = (PTOKEN_USER) TokenInformation;
            location = LW_PTR_ADD(TokenInformation, sizeof(TOKEN_USER));
            user->User.Attributes = AccessToken->User.User.Attributes;
            user->User.Sid = (PSID) location;
            location = RtlpAppendData(location,
                                      AccessToken->User.User.Sid,
                                      RtlLengthSid(AccessToken->User.User.Sid));
            break;
        }
        case TokenGroups:
        {
            PTOKEN_GROUPS groups = (PTOKEN_GROUPS) TokenInformation;
            location = LW_PTR_ADD(TokenInformation, sizeof(TOKEN_GROUPS));
            groups->GroupCount = AccessToken->Groups.GroupCount;
            for (i = 0; i < AccessToken->Groups.GroupCount; i++)
            {
                groups->Groups[i].Attributes = AccessToken->Groups.Groups[i].Attributes;
                groups->Groups[i].Sid = (PSID) location;
                location = RtlpAppendData(location,
                                          AccessToken->Groups.Groups[i].Sid,
                                          RtlLengthSid(AccessToken->Groups.Groups[i].Sid));
            }
            break;
        }
        case TokenOwner:
        {
            PTOKEN_OWNER owner = (PTOKEN_OWNER) TokenInformation;
            location = LW_PTR_ADD(TokenInformation, sizeof(TOKEN_OWNER));
            owner->Owner = (PSID) location;
            location = RtlpAppendData(location,
                                      AccessToken->Owner.Owner,
                                      RtlLengthSid(AccessToken->Owner.Owner));
            break;
        }
        case TokenPrimaryGroup:
        {
            PTOKEN_PRIMARY_GROUP primaryGroup = (PTOKEN_PRIMARY_GROUP) TokenInformation;
            location = LW_PTR_ADD(TokenInformation, sizeof(TOKEN_PRIMARY_GROUP));
            primaryGroup->PrimaryGroup = (PSID) location;
            location = RtlpAppendData(location,
                                      AccessToken->PrimaryGroup.PrimaryGroup,
                                      RtlLengthSid(AccessToken->PrimaryGroup.PrimaryGroup));
            break;
        }
        case TokenDefaultDacl:
        {
            PTOKEN_DEFAULT_DACL defaultDacl = (PTOKEN_DEFAULT_DACL) TokenInformation;
            location = LW_PTR_ADD(TokenInformation, sizeof(TOKEN_DEFAULT_DACL));
            if (AccessToken->DefaultDacl.DefaultDacl)
            {
                defaultDacl->DefaultDacl = (PACL) location;
                location = RtlpAppendData(location,
                                          AccessToken->DefaultDacl.DefaultDacl,
                                          AccessToken->DefaultDacl.DefaultDacl->AclSize);
            }
            break;
        }
        default:
            // We should have already checked.
            status = STATUS_ASSERTION_FAILURE;
            GOTO_CLEANUP();
    }

    if (location != LW_PTR_ADD(TokenInformation, requiredSize))
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    status = STATUS_SUCCESS;

cleanup:
    *ReturnedLength = requiredSize;

    return status;
}

NTSTATUS
RtlQueryAccessTokenUnixInformation(
    IN PACCESS_TOKEN AccessToken,
    OUT PTOKEN_UNIX TokenInformation
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!AccessToken)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!IsSetFlag(AccessToken->Flags, ACCESS_TOKEN_FLAG_UNIX_PRESENT))
    {
        status = STATUS_NOT_FOUND;
        GOTO_CLEANUP();
    }

    *TokenInformation = AccessToken->Unix;
    status = STATUS_SUCCESS;

cleanup:
    return status;
}

static
BOOLEAN
RtlpIsSidMemberOfToken(
    IN PACCESS_TOKEN AccessToken,
    IN PSID Sid
    )
{
    BOOLEAN isMember = FALSE;
    ULONG i = 0;

    if (RtlEqualSid(Sid, AccessToken->User.User.Sid))
    {
        isMember = TRUE;
        GOTO_CLEANUP();
    }

    for (i = 0; i < AccessToken->Groups.GroupCount; i++)
    {
        PSID_AND_ATTRIBUTES sidInfo = &AccessToken->Groups.Groups[i];
        if (IsSetFlag(sidInfo->Attributes, SE_GROUP_ENABLED) &&
            RtlEqualSid(Sid, sidInfo->Sid))
        {
            isMember = TRUE;
            GOTO_CLEANUP();
        }
    }

    isMember = FALSE;

cleanup:
    return isMember;
}

BOOLEAN
RtlAccessCheck(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN PACCESS_TOKEN AccessToken,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK PreviouslyGrantedAccess,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
    )
{
    NTSTATUS status = STATUS_ACCESS_DENIED;
    ACCESS_MASK grantedAccess = PreviouslyGrantedAccess;
    ACCESS_MASK desiredAccess = DesiredAccess;
    BOOLEAN wantMaxAllowed = FALSE;
    USHORT aclSizeUsed = 0;
    USHORT aceOffset = 0;
    PACE_HEADER aceHeader = NULL;

    if (!SecurityDescriptor || !AccessToken || !GenericMapping)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!LW_IS_VALID_FLAGS(DesiredAccess, VALID_DESIRED_ACCESS_MASK))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!LW_IS_VALID_FLAGS(PreviouslyGrantedAccess, VALID_GRANTED_ACCESS_MASK))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    wantMaxAllowed = IsSetFlag(desiredAccess, MAXIMUM_ALLOWED);
    ClearFlag(desiredAccess, MAXIMUM_ALLOWED);

    RtlMapGenericMask(&desiredAccess, GenericMapping);

    if (IsSetFlag(desiredAccess, ACCESS_SYSTEM_SECURITY))
    {
        // TODO-Handle ACCESS_SYSTEM_SECURITY by checking SE_SECURITY_NAME
        // privilege.  For now, requesting ACCESS_SYSTEM_SECURITY is not
        // allowed.
        status = STATUS_ACCESS_DENIED;
        GOTO_CLEANUP();
    }

    if (IsSetFlag(desiredAccess, WRITE_OWNER))
    {
        // TODO-Allow WRITE_OWNER if have SE_TAKE_OWNERSHIP_NAME regardless
        // of DACL.
    }

    // TODO-MAXIMUM_ALLOWED wrt privileges and WRITE_OWNER and
    // ACCESS_SYSTEM_SECURITY above.

    if (!SecurityDescriptor->Dacl)
    {
        // TODO-Interplay with special bits above?
        if (wantMaxAllowed)
        {
            SetFlag(desiredAccess, STANDARD_RIGHTS_ALL);
            SetFlag(desiredAccess, GENERIC_ALL);
            RtlMapGenericMask(&desiredAccess, GenericMapping);
        }
        SetFlag(grantedAccess, desiredAccess);
        desiredAccess = 0;

        status = STATUS_SUCCESS;
        GOTO_CLEANUP();
    }

    if (!RtlValidAcl(SecurityDescriptor->Dacl, &aclSizeUsed))
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    while (wantMaxAllowed || desiredAccess)
    {
        status = RtlIterateAce(SecurityDescriptor->Dacl,
                               aclSizeUsed,
                               &aceOffset,
                               &aceHeader);
        if (STATUS_NO_MORE_ENTRIES == status)
        {
            break;
        }
        GOTO_CLEANUP_ON_STATUS(status);

        // Check ACE
        switch (aceHeader->AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
            {
                PACCESS_ALLOWED_ACE ace = (PACCESS_ALLOWED_ACE) aceHeader;
                if (wantMaxAllowed || IsSetFlag(desiredAccess, ace->Mask))
                {
                    // SID in token => add bits to granted bits
                    PSID sid = RtlpGetSidAccessAllowedAce(ace);
                    if (RtlpIsSidMemberOfToken(AccessToken, sid))
                    {
                        if (wantMaxAllowed)
                        {
                            SetFlag(grantedAccess, ace->Mask);
                        }
                        else
                        {
                            SetFlag(grantedAccess, ace->Mask & desiredAccess);
                        }
                        ClearFlag(desiredAccess, grantedAccess);
                    }
                }
                break;
            }
            case ACCESS_DENIED_ACE_TYPE:
            {
                // Allowed and deny ACEs are isomorphic.
                PACCESS_ALLOWED_ACE ace = (PACCESS_ALLOWED_ACE) aceHeader;
                if (IsSetFlag(desiredAccess, ace->Mask))
                {
                    // SID in token => exit with STATUS_ACCESS_DENIED
                    PSID sid = RtlpGetSidAccessAllowedAce(ace);
                    if (RtlpIsSidMemberOfToken(AccessToken, sid))
                    {
                        status = STATUS_ACCESS_DENIED;
                        GOTO_CLEANUP();
                    }
                }
                break;
            }
            default:
                // ignore
                break;
        }
    }

    status = desiredAccess ? STATUS_ACCESS_DENIED : STATUS_SUCCESS;

cleanup:
    if (NT_SUCCESS(status) &&
        !LW_IS_VALID_FLAGS(grantedAccess, VALID_GRANTED_ACCESS_MASK))
    {
        status = STATUS_ASSERTION_FAILURE;
    }
    if (!NT_SUCCESS(status))
    {
        grantedAccess = PreviouslyGrantedAccess;
    }

    *GrantedAccess = grantedAccess;
    *AccessStatus = status;

    return NT_SUCCESS(status) ? TRUE : FALSE;
}
