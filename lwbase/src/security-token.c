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
#include <lw/atomic.h>

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

    token->ReferenceCount = 1;
    token->Flags = 0;

    token->User.Attributes = User->User.Attributes;
    token->User.Sid = (PSID) location;
    location = RtlpAppendData(location,
                              User->User.Sid,
                              RtlLengthSid(User->User.Sid));

    token->GroupCount = Groups->GroupCount;
    token->Groups = (PSID_AND_ATTRIBUTES) location;
    location = LwRtlOffsetToPointer(location, sizeof(Groups->Groups[0]) * Groups->GroupCount);
    for (i = 0; i < Groups->GroupCount; i++)
    {
        token->Groups[i].Attributes = Groups->Groups[i].Attributes;
        token->Groups[i].Sid = (PSID) location;
        location = RtlpAppendData(location,
                                  Groups->Groups[i].Sid,
                                  RtlLengthSid(Groups->Groups[i].Sid));
    }

    if (Owner->Owner)
    {
        token->Owner = (PSID) location;
        location = RtlpAppendData(location,
                                  Owner->Owner,
                                  RtlLengthSid(Owner->Owner));
    }

    if (PrimaryGroup->PrimaryGroup)
    {
        token->PrimaryGroup = (PSID) location;
        location = RtlpAppendData(location,
                                  PrimaryGroup->PrimaryGroup,
                                  RtlLengthSid(PrimaryGroup->PrimaryGroup));
    }

    if (DefaultDacl->DefaultDacl)
    {
        token->DefaultDacl = (PACL) location;
        location = RtlpAppendData(location,
                                  DefaultDacl->DefaultDacl,
                                  DefaultDacl->DefaultDacl->AclSize);
    }

    if (Unix)
    {
        SetFlag(token->Flags, ACCESS_TOKEN_FLAG_UNIX_PRESENT);
        token->Uid = Unix->Uid;
        token->Gid = Unix->Gid;
        token->Umask = Unix->Umask;
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
RtlReferenceAccessToken(
    IN PACCESS_TOKEN AccessToken
    )
{
    LwInterlockedIncrement(&AccessToken->ReferenceCount);
}

VOID
RtlReleaseAccessToken(
    IN OUT PACCESS_TOKEN* AccessToken
    )
{
    PACCESS_TOKEN accessToken = *AccessToken;

    if (accessToken)
    {
        LONG count = LwInterlockedDecrement(&accessToken->ReferenceCount);
        assert(count >= 0);
        if (0 == count)
        {
            RTL_FREE(AccessToken);
        }
    }
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
                            RtlLengthSid(AccessToken->User.Sid));
            GOTO_CLEANUP_ON_STATUS(status);
            break;
        case TokenGroups:
            status = RtlSafeMultiplyULONG(
                        &requiredSize,
                        sizeof(AccessToken->Groups[0]),
                        AccessToken->GroupCount);
            GOTO_CLEANUP_ON_STATUS(status);

            status = RtlSafeAddULONG(&requiredSize, requiredSize, sizeof(TOKEN_GROUPS));
            GOTO_CLEANUP_ON_STATUS(status);

            for (i = 0; i < AccessToken->GroupCount; i++)
            {
                status = RtlSafeAddULONG(
                                &requiredSize,
                                requiredSize,
                                RtlLengthSid(AccessToken->Groups[i].Sid));
                GOTO_CLEANUP_ON_STATUS(status);
            }
            break;
        case TokenOwner:
            status = RtlSafeAddULONG(
                            &requiredSize,
                            sizeof(TOKEN_OWNER),
                            RtlLengthSid(AccessToken->Owner));
            GOTO_CLEANUP_ON_STATUS(status);
            break;
        case TokenPrimaryGroup:
            status = RtlSafeAddULONG(
                            &requiredSize,
                            sizeof(TOKEN_PRIMARY_GROUP),
                            RtlLengthSid(AccessToken->PrimaryGroup));
            GOTO_CLEANUP_ON_STATUS(status);
            break;
        case TokenDefaultDacl:
            status = RtlSafeAddULONG(
                            &requiredSize,
                            sizeof(TOKEN_DEFAULT_DACL),
                            (AccessToken->DefaultDacl ?
                             AccessToken->DefaultDacl->AclSize :
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
            PTOKEN_USER tokenInfo = (PTOKEN_USER) TokenInformation;
            location = LW_PTR_ADD(TokenInformation, sizeof(TOKEN_USER));
            tokenInfo->User.Attributes = AccessToken->User.Attributes;
            tokenInfo->User.Sid = (PSID) location;
            location = RtlpAppendData(location,
                                      AccessToken->User.Sid,
                                      RtlLengthSid(AccessToken->User.Sid));
            break;
        }
        case TokenGroups:
        {
            PTOKEN_GROUPS tokenInfo = (PTOKEN_GROUPS) TokenInformation;
            location = LW_PTR_ADD(TokenInformation,
                                  (sizeof(TOKEN_GROUPS) +
                                   (sizeof(AccessToken->Groups[0]) * AccessToken->GroupCount)));
            tokenInfo->GroupCount = AccessToken->GroupCount;
            for (i = 0; i < AccessToken->GroupCount; i++)
            {
                tokenInfo->Groups[i].Attributes = AccessToken->Groups[i].Attributes;
                tokenInfo->Groups[i].Sid = (PSID) location;
                location = RtlpAppendData(location,
                                          AccessToken->Groups[i].Sid,
                                          RtlLengthSid(AccessToken->Groups[i].Sid));
            }
            break;
        }
        case TokenOwner:
        {
            PTOKEN_OWNER tokenInfo = (PTOKEN_OWNER) TokenInformation;
            location = LW_PTR_ADD(TokenInformation, sizeof(TOKEN_OWNER));
            tokenInfo->Owner = (PSID) location;
            location = RtlpAppendData(location,
                                      AccessToken->Owner,
                                      RtlLengthSid(AccessToken->Owner));
            break;
        }
        case TokenPrimaryGroup:
        {
            PTOKEN_PRIMARY_GROUP tokenInfo = (PTOKEN_PRIMARY_GROUP) TokenInformation;
            location = LW_PTR_ADD(TokenInformation, sizeof(TOKEN_PRIMARY_GROUP));
            tokenInfo->PrimaryGroup = (PSID) location;
            location = RtlpAppendData(location,
                                      AccessToken->PrimaryGroup,
                                      RtlLengthSid(AccessToken->PrimaryGroup));
            break;
        }
        case TokenDefaultDacl:
        {
            PTOKEN_DEFAULT_DACL tokenInfo = (PTOKEN_DEFAULT_DACL) TokenInformation;
            location = LW_PTR_ADD(TokenInformation, sizeof(TOKEN_DEFAULT_DACL));
            if (AccessToken->DefaultDacl)
            {
                tokenInfo->DefaultDacl = (PACL) location;
                location = RtlpAppendData(location,
                                          AccessToken->DefaultDacl,
                                          AccessToken->DefaultDacl->AclSize);
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
    TOKEN_UNIX tokenInfo = { 0 };

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

    tokenInfo.Uid = AccessToken->Uid;
    tokenInfo.Gid = AccessToken->Gid;
    tokenInfo.Umask = AccessToken->Umask;

    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        RtlZeroMemory(&tokenInfo, sizeof(tokenInfo));
    }

    *TokenInformation = tokenInfo;
    return status;
}

BOOLEAN
RtlIsSidMemberOfToken(
    IN PACCESS_TOKEN AccessToken,
    IN PSID Sid
    )
{
    BOOLEAN isMember = FALSE;
    ULONG i = 0;

    if (RtlEqualSid(Sid, AccessToken->User.Sid))
    {
        isMember = TRUE;
        GOTO_CLEANUP();
    }

    for (i = 0; i < AccessToken->GroupCount; i++)
    {
        PSID_AND_ATTRIBUTES sidInfo = &AccessToken->Groups[i];
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
    ACCESS_MASK deniedAccess = 0;
    ACCESS_MASK desiredAccess = DesiredAccess;
    BOOLEAN wantMaxAllowed = FALSE;
    USHORT aclSizeUsed = 0;
    USHORT aceOffset = 0;
    PACE_HEADER aceHeader = NULL;
    union {
        SID Sid;
        BYTE Buffer[SID_MAX_SIZE];
    } sidBuffer;
    ULONG ulSidSize = sizeof(sidBuffer);

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

    if ((SecurityDescriptor->Owner == NULL) ||
        (SecurityDescriptor->Group == NULL))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    wantMaxAllowed = IsSetFlag(desiredAccess, MAXIMUM_ALLOWED);
    ClearFlag(desiredAccess, MAXIMUM_ALLOWED);

    RtlMapGenericMask(&desiredAccess, GenericMapping);

    //
    // NT AUTHORITY\SYSTEM is always allowed an access
    //
    status = RtlCreateWellKnownSid(WinLocalSystemSid,
                                   NULL,
                                   &sidBuffer.Sid,
                                   &ulSidSize);
    GOTO_CLEANUP_ON_STATUS(status);

    if (RtlIsSidMemberOfToken(AccessToken, &sidBuffer.Sid))
    {
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

    if (IsSetFlag(desiredAccess, ACCESS_SYSTEM_SECURITY))
    {
        // TODO-Handle ACCESS_SYSTEM_SECURITY by checking SE_SECURITY_NAME
        // privilege.  For now, requesting ACCESS_SYSTEM_SECURITY is not
        // allowed.
        status = STATUS_PRIVILEGE_NOT_HELD;
        GOTO_CLEANUP();
    }

    if (IsSetFlag(desiredAccess, WRITE_OWNER))
    {
        // TODO-Allow WRITE_OWNER if have SE_TAKE_OWNERSHIP_NAME regardless
        // of DACL.

        //
        // BUILTIN\Administrators are always allowed WRITE_OWNER
        //

        ulSidSize = sizeof(sidBuffer);
        status = RtlCreateWellKnownSid(
                     WinBuiltinAdministratorsSid,
                     NULL,
                     &sidBuffer.Sid,
                     &ulSidSize);
        GOTO_CLEANUP_ON_STATUS(status);

        if (RtlIsSidMemberOfToken(AccessToken, &sidBuffer.Sid))
        {
            SetFlag(grantedAccess, WRITE_OWNER);
            ClearFlag(desiredAccess, WRITE_OWNER);
        }
    }

    //
    // Owner can always read the SD and write the DACL.
    //

    if (wantMaxAllowed || IsSetFlag(desiredAccess, READ_CONTROL | WRITE_DAC))
    {
        if (RtlIsSidMemberOfToken(AccessToken, SecurityDescriptor->Owner))
        {
            if (wantMaxAllowed)
            {
                desiredAccess |= (READ_CONTROL | WRITE_DAC);
            }

            SetFlag(grantedAccess, (READ_CONTROL | WRITE_DAC) & desiredAccess);
            ClearFlag(desiredAccess, grantedAccess);
        }
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
                ACCESS_MASK mask = ace->Mask;

                RtlMapGenericMask(&mask, GenericMapping);

                if (wantMaxAllowed || IsSetFlag(desiredAccess, mask))
                {
                    // SID in token => add bits to granted bits
                    PSID sid = RtlpGetSidAccessAllowedAce(ace);

                    if (RtlIsSidMemberOfToken(AccessToken, sid))
                    {
                        if (wantMaxAllowed)
                        {
                            SetFlag(grantedAccess, mask & ~deniedAccess);
                        }
                        else
                        {
                            SetFlag(grantedAccess, mask & desiredAccess);
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
                ACCESS_MASK mask = ace->Mask;

                RtlMapGenericMask(&mask, GenericMapping);

                if (wantMaxAllowed || IsSetFlag(desiredAccess, mask))
                {
                    // SID in token => exit with STATUS_ACCESS_DENIED
                    PSID sid = RtlpGetSidAccessAllowedAce(ace);

                    if (RtlIsSidMemberOfToken(AccessToken, sid))
                    {
                        if (wantMaxAllowed)
                        {
                            SetFlag(deniedAccess, mask);
                        }
                        else
                        {
                            status = STATUS_ACCESS_DENIED;
                            GOTO_CLEANUP();
                        }

                        ClearFlag(desiredAccess, deniedAccess);
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

static
inline
VOID
Align32(
    PULONG ulValue
    )
{
    ULONG ulRem = *ulValue % 32;

    if (ulRem) *ulValue += (32 - ulRem);
}

static
inline
NTSTATUS
CheckOffset(
    ULONG ulOffset,
    ULONG ulSize,
    ULONG ulMaxSize
    )
{
    if (ulMaxSize - ulOffset < ulSize)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

static
inline
NTSTATUS
AdvanceTo(
    PULONG ulValue,
    ULONG ulPosition,
    ULONG ulSize
    )
{
    if (ulPosition >= ulSize)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        *ulValue = ulPosition;
        return STATUS_SUCCESS;
    }
}

static
ULONG
RtlAccessTokenRelativeSize(
    PACCESS_TOKEN pToken
    )
{
    ULONG ulRelativeSize = 0;
    ULONG i = 0;

    ulRelativeSize += sizeof(ACCESS_TOKEN_SELF_RELATIVE);
    Align32(&ulRelativeSize);

    ulRelativeSize += RtlLengthSid(pToken->User.Sid);
    Align32(&ulRelativeSize);

    if (pToken->Groups)
    {
        ulRelativeSize += sizeof(SID_AND_ATTRIBUTES_SELF_RELATIVE) * pToken->GroupCount;
        Align32(&ulRelativeSize);

        for (i = 0; i < pToken->GroupCount; i++)
        {
            ulRelativeSize += RtlLengthSid(pToken->Groups[i].Sid);
            Align32(&ulRelativeSize);
        }
    }

    if (pToken->Owner)
    {
        ulRelativeSize += RtlLengthSid(pToken->Owner);
        Align32(&ulRelativeSize);
    }

    if (pToken->PrimaryGroup)
    {
        ulRelativeSize += RtlLengthSid(pToken->PrimaryGroup);
        Align32(&ulRelativeSize);
    }

    if (pToken->DefaultDacl)
    {
        ulRelativeSize += RtlGetAclSize(pToken->DefaultDacl);
        Align32(&ulRelativeSize);
    }

    return ulRelativeSize;
}

NTSTATUS
RtlAccessTokenToSelfRelativeAccessToken(
    IN PACCESS_TOKEN pToken,
    OUT OPTIONAL PACCESS_TOKEN_SELF_RELATIVE pRelative,
    IN OUT PULONG pulSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG ulRelativeSize = RtlAccessTokenRelativeSize(pToken);
    PSID_AND_ATTRIBUTES_SELF_RELATIVE pGroups = NULL;
    PBYTE pBuffer = NULL;
    ULONG ulOffset = 0;
    ULONG i = 0;

    if (pRelative)
    {
        if (*pulSize < ulRelativeSize)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            GOTO_CLEANUP_ON_STATUS(status);
        }

        pBuffer = (PBYTE) pRelative;

        pRelative->Flags = pToken->Flags;
        pRelative->User.Attributes = pToken->User.Attributes;
        pRelative->GroupCount = pToken->GroupCount;
        pRelative->Uid = pToken->Uid;
        pRelative->Gid = pToken->Gid;
        pRelative->Umask = pToken->Umask;

        ulOffset += sizeof(*pRelative);
        Align32(&ulOffset);

        pRelative->User.SidOffset = ulOffset;
        memcpy(pBuffer + ulOffset, pToken->User.Sid, RtlLengthSid(pToken->User.Sid));
        ulOffset += RtlLengthSid(pToken->User.Sid);
        Align32(&ulOffset);

        if (pToken->Groups)
        {
            pRelative->GroupsOffset = ulOffset;
            pGroups = (PSID_AND_ATTRIBUTES_SELF_RELATIVE) (pBuffer + ulOffset);
            ulOffset += sizeof(SID_AND_ATTRIBUTES_SELF_RELATIVE) * pToken->GroupCount;
            Align32(&ulOffset);

            for (i = 0; i < pToken->GroupCount; i++)
            {
                pGroups[i].Attributes = pToken->Groups[i].Attributes;
                pGroups[i].SidOffset = ulOffset;
                memcpy(pBuffer + ulOffset, pToken->Groups[i].Sid, RtlLengthSid(pToken->Groups[i].Sid));
                ulOffset += RtlLengthSid(pToken->Groups[i].Sid);
                Align32(&ulOffset);
            }
        }
        else
        {
            pRelative->GroupsOffset = 0;
        }

        if (pToken->Owner)
        {
            pRelative->OwnerOffset = ulOffset;
            memcpy(pBuffer + ulOffset, pToken->Owner, RtlLengthSid(pToken->Owner));
            ulOffset += RtlLengthSid(pToken->Owner);
            Align32(&ulOffset);
        }
        else
        {
            pRelative->OwnerOffset = 0;
        }

        if (pToken->PrimaryGroup)
        {
            pRelative->PrimaryGroupOffset = ulOffset;
            memcpy(pBuffer + ulOffset, pToken->PrimaryGroup, RtlLengthSid(pToken->PrimaryGroup));
            ulOffset += RtlLengthSid(pToken->PrimaryGroup);
            Align32(&ulOffset);
        }
        else
        {
            pRelative->PrimaryGroupOffset = 0;
        }

        if (pToken->DefaultDacl)
        {
            pRelative->DefaultDaclOffset = ulOffset;
            memcpy(pBuffer + ulOffset, pToken->DefaultDacl, RtlGetAclSize(pToken->DefaultDacl));
            ulOffset += RtlGetAclSize(pToken->DefaultDacl);
            Align32(&ulOffset);
        }
        else
        {
            pRelative->DefaultDaclOffset = 0;
        }


        assert(ulOffset == ulRelativeSize);
    }

cleanup:

    *pulSize = ulRelativeSize;

    return status;
}

static
NTSTATUS
RtlValidateSelfRelativeSid(
    PSID pSid,
    ULONG ulOffset,
    ULONG ulRelativeSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = CheckOffset(ulOffset, SID_MIN_SIZE, ulRelativeSize);
    GOTO_CLEANUP_ON_STATUS(status);

    status = CheckOffset(ulOffset, RtlLengthSid(pSid), ulRelativeSize);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:

    return status;
}

NTSTATUS
RtlSelfRelativeAccessTokenToAccessToken(
    IN PACCESS_TOKEN_SELF_RELATIVE pRelative,
    IN ULONG ulRelativeSize,
    OUT PACCESS_TOKEN* ppToken
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACCESS_TOKEN pToken = NULL;
    ULONG ulOffset = 0;
    PBYTE pBuffer = (PBYTE) pRelative;
    PSID pSid = NULL;
    PSID_AND_ATTRIBUTES_SELF_RELATIVE pGroups = NULL;
    ULONG ulSize = 0;
    ULONG ulRealSize = 0;
    ULONG i = 0;

    status = RTL_ALLOCATE(&pToken, ACCESS_TOKEN, sizeof(*pToken));
    GOTO_CLEANUP_ON_STATUS(status);

    pToken->ReferenceCount = 1;

    status = CheckOffset(0, sizeof(*pRelative), ulRelativeSize);
    GOTO_CLEANUP_ON_STATUS(status);

    pToken->Flags = pRelative->Flags;
    pToken->User.Attributes = pRelative->User.Attributes;
    pToken->GroupCount = pRelative->GroupCount;
    pToken->Uid = pRelative->Uid;
    pToken->Gid = pRelative->Gid;
    pToken->Umask = pRelative->Umask;

    ulOffset = pRelative->User.SidOffset;
    pSid = (PSID) (pBuffer + ulOffset);
    status = RtlValidateSelfRelativeSid(pSid, ulOffset, ulRelativeSize);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlDuplicateSid(&pToken->User.Sid, pSid);
    GOTO_CLEANUP_ON_STATUS(status);

    ulOffset = pRelative->GroupsOffset;
    if (ulOffset)
    {
        status = LwRtlSafeMultiplyULONG(
            &ulSize,
            sizeof(SID_AND_ATTRIBUTES_SELF_RELATIVE),
            pRelative->GroupCount);
        GOTO_CLEANUP_ON_STATUS(status);

        status = LwRtlSafeMultiplyULONG(
            &ulRealSize,
            sizeof(SID_AND_ATTRIBUTES),
            pRelative->GroupCount);
        GOTO_CLEANUP_ON_STATUS(status);

        status = CheckOffset(ulOffset, ulSize, ulRelativeSize);
        GOTO_CLEANUP_ON_STATUS(status);

        pGroups = (PSID_AND_ATTRIBUTES_SELF_RELATIVE) (pBuffer + ulOffset);

        status = RTL_ALLOCATE(&pToken->Groups, SID_AND_ATTRIBUTES, ulRealSize);
        GOTO_CLEANUP_ON_STATUS(status);

        for (i = 0; i < pRelative->GroupCount; i++)
        {
            pToken->Groups[i].Attributes = pGroups[i].Attributes;

            ulOffset = pGroups[i].SidOffset;
            pSid = (PSID) (pBuffer + ulOffset);
            status = RtlValidateSelfRelativeSid(pSid, ulOffset, ulRelativeSize);
            GOTO_CLEANUP_ON_STATUS(status);

            status = RtlDuplicateSid(&pToken->Groups[i].Sid, pSid);
            GOTO_CLEANUP_ON_STATUS(status);
        }
    }

    ulOffset = pRelative->OwnerOffset;
    if (ulOffset)
    {
        pSid = (PSID) (pBuffer + ulOffset);
        status = RtlValidateSelfRelativeSid(pSid, ulOffset, ulRelativeSize);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlDuplicateSid(&pToken->Owner, pSid);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    ulOffset = pRelative->PrimaryGroupOffset;
    if (ulOffset)
    {
        pSid = (PSID) (pBuffer + ulOffset);
        status = RtlValidateSelfRelativeSid(pSid, ulOffset, ulRelativeSize);
        GOTO_CLEANUP_ON_STATUS(status);

        status = RtlDuplicateSid(&pToken->PrimaryGroup, pSid);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    *ppToken = pToken;

cleanup:

    if (!NT_SUCCESS(status))
    {
        RtlReleaseAccessToken(&pToken);
    }

    return status;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
