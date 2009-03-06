#include <lw/security-api.h>
#include "security-api2.h"
#include "security-token-create-info.h"
#include "security-types-internal.h"
#include <lw/rtlgoto.h>
#include <lw/rtlmemory.h>
#include <lw/swab.h>
#include <lw/safeint.h>
#include <assert.h>

//
// SID Functions
//

NTSTATUS
RtlInitializeSid(
    OUT PSID Sid,
    IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    IN UCHAR SubAuthorityCount
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!Sid || !IdentifierAuthority)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (SubAuthorityCount > SID_MAX_SUB_AUTHORITIES)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    Sid->Revision = SID_REVISION;
    Sid->SubAuthorityCount = SubAuthorityCount;
    memcpy(&Sid->IdentifierAuthority, IdentifierAuthority, sizeof(*IdentifierAuthority));
    memset(Sid->SubAuthority, 0, sizeof(Sid->SubAuthority[0]) * SubAuthorityCount);

cleanup:
    return status;
}

ULONG
RtlLengthRequiredSid(
    IN ULONG SubAuthorityCount
    )
{
    return (LW_FIELD_OFFSET(SID, SubAuthority) +
            (LW_FIELD_SIZE(SID, SubAuthority[0]) * SubAuthorityCount));
}

ULONG
RtlLengthSid(
    IN PSID Sid
    )
{
    return RtlLengthRequiredSid(Sid->SubAuthorityCount);
}

BOOLEAN
RtlValidSid(
    IN PSID Sid
    )
{
    return ((Sid != NULL) &&
            (Sid->Revision == SID_REVISION) &&
            (Sid->SubAuthorityCount <= SID_MAX_SUB_AUTHORITIES));
}

BOOLEAN
RtlEqualSid(
    IN PSID Sid1,
    IN PSID Sid2
    )
{
    return ((Sid1->SubAuthorityCount == Sid2->SubAuthorityCount) &&
            RtlEqualMemory(Sid1, Sid2, RtlLengthSid(Sid1)));
}

BOOLEAN
RtlEqualPrefixSid(
    IN PSID Sid1,
    IN PSID Sid2
    )
{
    BOOLEAN isEqual = FALSE;
    if (Sid1->SubAuthorityCount == Sid2->SubAuthorityCount)
    {
        UCHAR count = Sid1->SubAuthorityCount;
        if (count > 0)
        {
            count--;
        }
        isEqual = RtlEqualMemory(Sid1, Sid2, RtlLengthRequiredSid(count));
    }
    return isEqual;
}

BOOLEAN
RtlIsPrefixSid(
    IN PSID Prefix,
    IN PSID Sid
    )
{
    return ((Prefix->SubAuthorityCount <= Sid->SubAuthorityCount) &&
            RtlEqualMemory(Prefix, Sid, RtlLengthSid(Prefix)));
}

NTSTATUS
RtlCopySid(
    IN ULONG DestinationSidLength,
    OUT PSID DestinationSid,
    IN PSID SourceSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG length = RtlLengthSid(SourceSid);

    if (DestinationSidLength < length)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    RtlCopyMemory(DestinationSid, SourceSid, length);

cleanup:
    return status;
}

//
// ACE Functions (API2)
//

inline
static
BOOLEAN
RtlpValidAceHeader(
    IN PACE_HEADER Ace
    )
{
    // Note that we currently only support up to V2 ACEs.
    // TODO-Perhaps also support label-type ACEs.
    // TODO-Is it ok to explicitly disallow object/callback-type ACEs?
    // TODO-Should even bother checking the ACE type?
    return (Ace &&
            (Ace->AceSize >= sizeof(ACE_HEADER)) &&
            LW_IS_VALID_FLAGS(Ace->AceFlags, VALID_ACE_FLAGS_MASK) &&
#if 0 // Currently disabled as the compiler complains about always true
            (Ace->AceType >= ACCESS_MIN_MS_ACE_TYPE) &&
#endif
            (Ace->AceType <= ACCESS_MAX_MS_V2_ACE_TYPE));
}

inline
static
PSID
RtlpGetSidAccessAllowedAce(
    IN PACCESS_ALLOWED_ACE Ace
    )
{
    return (PSID) &Ace->SidStart;
}

static
BOOLEAN
RtlpValidAccessAllowedAce(
    IN USHORT AceSize,
    IN PSID Sid,
    OUT OPTIONAL PNTSTATUS Status
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT sizeRequired = 0;

    sizeRequired = RtlLengthAccessAllowedAce(Sid);
    if (sizeRequired != AceSize)
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    if (!RtlValidSid(Sid))
    {
        status = STATUS_INVALID_SID;
        GOTO_CLEANUP();
    }

    status = STATUS_SUCCESS;

cleanup:
    if (Status)
    {
        *Status = status;
    }

    return NT_SUCCESS(status) ? TRUE : FALSE;
}

static
NTSTATUS
RtlpVerifyAceEx(
    IN PACE_HEADER Ace,
    IN BOOLEAN VerifyAceType
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!RtlpValidAceHeader(Ace))
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    if (!VerifyAceType)
    {
        status = STATUS_SUCCESS;
        GOTO_CLEANUP();
    }

    // TODO - Perhaps support additional ACE types?

    switch (Ace->AceType)
    {
        case ACCESS_ALLOWED_ACE_TYPE:
        case ACCESS_DENIED_ACE_TYPE:
        case SYSTEM_AUDIT_ACE_TYPE:
        {
            // These are all isomorphic.
            RtlpValidAccessAllowedAce(
                    Ace->AceSize,
                    RtlpGetSidAccessAllowedAce((PACCESS_ALLOWED_ACE) Ace),
                    &status);
            GOTO_CLEANUP_ON_STATUS(status);
            break;
        }
        default:
            status = STATUS_INVALID_ACL;
            GOTO_CLEANUP();
    }

    status = STATUS_SUCCESS;

cleanup:
    return status;
}

inline
static
BOOLEAN
RtlpValidAce(
    IN PACE_HEADER Ace
    )
{
    return NT_SUCCESS(RtlpVerifyAceEx(Ace, FALSE));
}

USHORT
RtlLengthAccessAllowedAce(
    IN PSID Sid
    )
{
    return (LW_FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart) +
            RtlLengthSid(Sid));
}

USHORT
RtlLengthAccessDeniedAce(
    IN PSID Sid
    )
{
    return (LW_FIELD_OFFSET(ACCESS_DENIED_ACE, SidStart) +
            RtlLengthSid(Sid));
}

NTSTATUS
RtlInitializeAccessAllowedAce(
    OUT PACCESS_ALLOWED_ACE Ace,
    IN ULONG AceLength,
    IN USHORT AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT size = RtlLengthAccessAllowedAce(Sid);

    if (!LW_IS_VALID_FLAGS(AceFlags, VALID_ACE_FLAGS_MASK) ||
        !LW_IS_VALID_FLAGS(AccessMask, VALID_DACL_ACCESS_MASK) ||
        !RtlValidSid(Sid))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (AceLength < size)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    Ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    Ace->Header.AceFlags = AceFlags;
    Ace->Header.AceSize = size;
    Ace->Mask = AccessMask;
    // We already know the size is sufficient
    RtlCopyMemory(&Ace->SidStart, Sid, RtlLengthSid(Sid));

cleanup:
    return status;
}

NTSTATUS
RtlInitializeAccessDeniedAce(
    OUT PACCESS_DENIED_ACE Ace,
    IN ULONG AceLength,
    IN USHORT AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
    )
{
    // ACCESS_DENIED_ACE is isomorphic wrt ACCESS_ALLOWED_ACE
    return RtlInitializeAccessAllowedAce(
                (PACCESS_ALLOWED_ACE) Ace,
                AceLength,
                AceFlags,
                AccessMask,
                Sid);
}

//
// ACL Functions (API2)
//

inline
static
BOOLEAN
RtlpValidAclRevision(
    IN ULONG AclRevision
    )
{
    return ((AclRevision == ACL_REVISION) ||
            (AclRevision == ACL_REVISION_DS));
}

inline
static
BOOLEAN
RtlpValidAclHeader(
    IN PACL Acl
    )
{
    return (Acl &&
            (Acl->AclSize >= sizeof(ACL)) &&
            RtlpValidAclRevision(Acl->AclRevision) &&
            (Acl->Sbz1 == 0) &&
            (Acl->Sbz2 == 0) &&
            (Acl->AceCount <= ((((USHORT)-1) - sizeof(ACL)) / sizeof(ACE_HEADER))));
}

NTSTATUS
RtlInitializeAcl(
    OUT PACL Acl,
    OUT PUSHORT AclSizeUsed,
    IN ULONG AclLength,
    IN ULONG AclRevision
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if ((AclRevision != ACL_REVISION) &&
        (AclRevision != ACL_REVISION_DS))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (AclLength > ((USHORT)-1))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (AclLength < sizeof(ACL))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    Acl->AclRevision = AclRevision;
    Acl->AclSize = AclLength;
    Acl->AceCount = 0;
    Acl->Sbz1 = 0;
    Acl->Sbz2 = 0;

    // ISSUE-Do we need this?
    // Zero out memory in case this somehow gets serialized without
    // going through self-relative code.
    RtlZeroMemory(LW_PTR_ADD(Acl, sizeof(ACL)), AclLength - sizeof(ACL));

cleanup:
    *AclSizeUsed = sizeof(ACL);

    return status;
}

static
NTSTATUS
RtlpGetAceLocationFromOffset(
    IN PACL Acl,
    IN USHORT AclSizeUsed,
    IN USHORT AceOffset,
    OUT PVOID* AceLocation
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT useOffset = 0;
    USHORT endOffset = 0;
    PVOID aceLocation = NULL;

    if (!RtlpValidAclHeader(Acl) ||
        (AclSizeUsed < sizeof(ACL)) ||
        (AclSizeUsed > Acl->AclSize))
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    endOffset = AclSizeUsed - sizeof(ACL);
    if (AceOffset == ACL_END_ACE_OFFSET)
    {
        useOffset = endOffset;
    }
    else if (AceOffset >= endOffset)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    aceLocation = LW_PTR_ADD(Acl, sizeof(ACL) + useOffset);

cleanup:
    if (!NT_SUCCESS(status))
    {
        aceLocation = NULL;
    }

    *AceLocation = aceLocation;

    return status;
}

inline
static
NTSTATUS
RtlpCheckEnoughAclBuffer(
    IN USHORT AclSize,
    IN USHORT AclSizeUsed,
    IN USHORT AceSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG sizeNeeded = AclSizeUsed + AceSize;

    // Check for overflow.
    if (sizeNeeded > ((USHORT)-1))
    {
        // TODO-Perhaps use STATUS_INVALID_PARAMETER or some other code?
        status = STATUS_INTEGER_OVERFLOW;
        GOTO_CLEANUP();
    }

    if (AclSize < sizeNeeded)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

cleanup:
    return status;
}

static
inline
NTSTATUS
RtlpMakeRoomForAceAtLocation(
    IN OUT PACL Acl,
    IN USHORT AclSizeUsed,
    IN PVOID AceLocation,
    IN USHORT AceSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID aceEnd = NULL;
    PVOID aclEnd = NULL;

    status = RtlpCheckEnoughAclBuffer(Acl->AclSize, AclSizeUsed, AceSize);
    GOTO_CLEANUP_ON_STATUS(status);

    aceEnd = LW_PTR_ADD(AceLocation, AceSize);
    aclEnd = LW_PTR_ADD(Acl, AclSizeUsed);

    // Make enough room as needed
    RtlMoveMemory(aceEnd, AceLocation, LW_PTR_OFFSET(AceLocation, aclEnd));

cleanup:
    return status;
}

NTSTATUS
RtlInsertAce(
    IN OUT PACL Acl,
    IN OUT PUSHORT AclSizeUsed,
    IN USHORT AceOffset,
    IN PACE_HEADER Ace
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT sizeUsed = *AclSizeUsed;
    PVOID aceLocation = NULL;

    if (!RtlpValidAce(Ace))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    // TODO-Check ACE type vs ACL revision

    status = RtlpGetAceLocationFromOffset(
                    Acl,
                    sizeUsed,
                    AceOffset,
                    &aceLocation);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpMakeRoomForAceAtLocation(Acl, sizeUsed, aceLocation, Ace->AceSize);
    GOTO_CLEANUP_ON_STATUS(status);

    // Copy in the ACE
    RtlCopyMemory(aceLocation, Ace, Ace->AceSize);
    sizeUsed += Ace->AceSize;
    Acl->AceCount++;

cleanup:
    if (NT_SUCCESS(status))
    {
        *AclSizeUsed = sizeUsed;
    }

    return status;
}

NTSTATUS
RtlInsertAccessAllowedAce(
    IN OUT PACL Acl,
    IN OUT PUSHORT AclSizeUsed,
    IN USHORT AceOffset,
    IN USHORT AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid,
    OUT OPTIONAL PACCESS_ALLOWED_ACE* Ace
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT sizeUsed = *AclSizeUsed;
    PACCESS_ALLOWED_ACE aceLocation = NULL;
    USHORT aceSize = 0;

    if (!RtlValidSid(Sid))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    status = RtlpGetAceLocationFromOffset(
                    Acl,
                    sizeUsed,
                    AceOffset,
                    OUT_PPVOID(&aceLocation));
    GOTO_CLEANUP_ON_STATUS(status);

    aceSize = RtlLengthAccessAllowedAce(Sid);

    status = RtlpMakeRoomForAceAtLocation(Acl, sizeUsed, aceLocation, aceSize);
    GOTO_CLEANUP_ON_STATUS(status);

    // We know we have at least aceSize bytes available.
    status = RtlInitializeAccessAllowedAce(
                    aceLocation,
                    aceSize,
                    AceFlags,
                    AccessMask,
                    Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    assert(aceSize == aceLocation->Header.AceSize);
    sizeUsed += aceSize;
    Acl->AceCount++;

cleanup:
    if (!NT_SUCCESS(status))
    {
        aceLocation = NULL;
    }
    else
    {
        *AclSizeUsed = sizeUsed;
    }

    *Ace = aceLocation;

    return status;
}

NTSTATUS
RtlInsertAccessDeniedAce(
    IN OUT PACL Acl,
    IN OUT PUSHORT AclSizeUsed,
    IN USHORT AceOffset,
    IN USHORT AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid,
    OUT OPTIONAL PACCESS_DENIED_ACE* Ace
    )
{
    // ACCESS_DENIED_ACE is isomorphic wrt ACCESS_ALLOWED_ACE
    return RtlInsertAccessAllowedAce(
                Acl,
                AclSizeUsed,
                AceOffset,
                AceFlags,
                AccessMask,
                Sid,
                (PACCESS_ALLOWED_ACE*) Ace);
}

static
NTSTATUS
RtlpRemoveAceFromLocation(
    IN OUT PACL Acl,
    IN OUT PUSHORT AclSizeUsed,
    IN OUT PACE_HEADER AceLocation
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT sizeUsed = *AclSizeUsed;
    PACE_HEADER aceLocation = AceLocation;
    PVOID aceEnd = NULL;
    PVOID aclEnd = NULL;
    USHORT removedAceSize = 0;

    if (aceLocation->AceSize < sizeof(ACE_HEADER))
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    aceEnd = LW_PTR_ADD(aceLocation, aceLocation->AceSize);
    aclEnd = LW_PTR_ADD(Acl, sizeUsed);
    if (aceEnd > aclEnd)
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    removedAceSize = aceLocation->AceSize;

    // Remove the ACE.
    RtlCopyMemory(aceLocation, aceEnd, LW_PTR_OFFSET(aceEnd, aclEnd));
    RtlZeroMemory(LW_PTR_ADD(aclEnd, -removedAceSize), removedAceSize);
    Acl->AceCount--;
    sizeUsed -= removedAceSize;

cleanup:
    if (NT_SUCCESS(status))
    {
        *AclSizeUsed = sizeUsed;
    }

    return status;
}

NTSTATUS
RtlRemoveAce(
    IN OUT PACL Acl,
    IN OUT PUSHORT AclSizeUsed,
    IN USHORT AceOffset
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT sizeUsed = *AclSizeUsed;
    PACE_HEADER aceLocation = NULL;

    if (AceOffset == ACL_END_ACE_OFFSET)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    status = RtlpGetAceLocationFromOffset(
                    Acl,
                    sizeUsed,
                    AceOffset,
                    OUT_PPVOID(&aceLocation));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpRemoveAceFromLocation(Acl, &sizeUsed, aceLocation);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (NT_SUCCESS(status))
    {
        *AclSizeUsed = sizeUsed;
    }

    return status;
}

NTSTATUS
RtlIterateAce(
    IN PACL Acl,
    IN USHORT AclSizeUsed,
    IN OUT PUSHORT AceOffset,
    OUT PACE_HEADER* AceHeader
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT aceOffset = *AceOffset;
    PACE_HEADER aceLocation = NULL;
    PVOID aceEnd = NULL;
    PVOID aclEnd = NULL;

    if (aceOffset == ACL_END_ACE_OFFSET)
    {
        status = STATUS_NO_MORE_ENTRIES;
        GOTO_CLEANUP();
    }

    status = RtlpGetAceLocationFromOffset(
                    Acl,
                    AclSizeUsed,
                    aceOffset,
                    OUT_PPVOID(&aceLocation));
    GOTO_CLEANUP_ON_STATUS(status);

    if ((aceOffset == 0) &&
        (Acl->AceCount == 0))
    {
        status = STATUS_NO_MORE_ENTRIES;
        GOTO_CLEANUP();
    }

    if (aceLocation->AceSize < sizeof(ACE_HEADER))
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    aceEnd = LW_PTR_ADD(aceLocation, aceLocation->AceSize);
    aclEnd = LW_PTR_ADD(Acl, AclSizeUsed);
    if (aceEnd > aclEnd)
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    if (aceEnd == aclEnd)
    {
        aceOffset = ACL_END_ACE_OFFSET;
    }
    else
    {
        aceOffset += aceLocation->AceSize;
    }

cleanup:
    if (!NT_SUCCESS(status))
    {
        aceLocation = NULL;
    }
    else
    {
        *AceOffset = aceOffset;
    }

    *AceHeader = aceLocation;

    return status;
}

UCHAR
RtlGetAclRevision(
    IN PACL Acl
    )
{
    return Acl->AclRevision;
}

USHORT
RtlGetAclSize(
    IN PACL Acl
    )
{
    return Acl->AclSize;
}

USHORT
RtlGetAclAceCount(
    IN PACL Acl
    )
{
    return Acl->AceCount;
}

USHORT
RtlGetAclSizeUsed(
    IN PACL Acl
    )
{
    USHORT sizeUsed = sizeof(ACL);
    USHORT aceIndex = 0;

    for (aceIndex = 0; aceIndex < Acl->AceCount; aceIndex++)
    {
        PACE_HEADER ace = (PACE_HEADER) LW_PTR_ADD(Acl, sizeUsed);
        // TODO-Overflow protection
        sizeUsed += ace->AceSize;
    }

    return sizeUsed;
}

BOOLEAN
RtlValidAcl(
    IN PACL Acl,
    OUT OPTIONAL PUSHORT AclSizeUsed
    )
{
    BOOLEAN isValid = TRUE;
    USHORT sizeUsed = sizeof(ACL);
    USHORT aceIndex = 0;

    if (!RtlpValidAclHeader(Acl))
    {
        isValid = FALSE;
        GOTO_CLEANUP();
    }

    for (aceIndex = 0; aceIndex < Acl->AceCount; aceIndex++)
    {
        PACE_HEADER ace = (PACE_HEADER) LW_PTR_ADD(Acl, sizeUsed);
        if (!RtlpValidAce(ace))
        {
            isValid = FALSE;
            GOTO_CLEANUP();
        }
        sizeUsed += ace->AceSize;
        // Check for integer overflow
        if (sizeUsed < ace->AceSize)
        {
            isValid = FALSE;
            GOTO_CLEANUP();
        }
        // Check that not past the end of the ACL
        if (sizeUsed > Acl->AclSize)
        {
            isValid = FALSE;
            GOTO_CLEANUP();
        }
    }

cleanup:
    if (AclSizeUsed)
    {
        *AclSizeUsed = isValid ? sizeUsed : 0;
    }

    return isValid;
}

BOOLEAN
RtlValidAceOffset(
    IN PACL Acl,
    IN USHORT AceOffset
    )
{
    BOOLEAN isValid = FALSE;
    USHORT sizeUsed = sizeof(ACL);
    USHORT aceIndex = 0;

    if (!RtlpValidAclHeader(Acl))
    {
        isValid = FALSE;
        GOTO_CLEANUP();
    }

    for (aceIndex = 0; aceIndex < Acl->AceCount; aceIndex++)
    {
        PACE_HEADER ace = (PACE_HEADER) LW_PTR_ADD(Acl, sizeUsed);
        if (AceOffset == (sizeUsed - sizeof(ACL)))
        {
            isValid = TRUE;
            break;
        }
        sizeUsed += ace->AceSize;
        // Check for integer overflow
        if (sizeUsed < ace->AceSize)
        {
            isValid = FALSE;
            GOTO_CLEANUP();
        }
        // Check that not past the end of the ACL
        if (sizeUsed > Acl->AclSize)
        {
            isValid = FALSE;
            GOTO_CLEANUP();
        }
    }

cleanup:
    return isValid;
}

//
// ACL Functions
//

NTSTATUS
RtlCreateAcl(
    OUT PACL Acl,
    IN ULONG AclLength,
    IN ULONG AclRevision
    )
{
    USHORT aclSizeUsed = 0;
    return RtlInitializeAcl(Acl, &aclSizeUsed, AclLength, AclRevision);
}

static
NTSTATUS
RtlpGetAceLocationFromIndex(
    IN PACL Acl,
    IN ULONG AceIndex,
    OUT PUSHORT AclSizeUsed,
    OUT PUSHORT AceOffset,
    OUT PVOID* AceLocation
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT sizeUsed = sizeof(ACL);
    PVOID aceLocation = NULL;
    USHORT aceIndex = 0;
    USHORT aceOffset = 0;

    if ((AceIndex > Acl->AceCount) &&
        (AceIndex != ((ULONG)-1)))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!RtlpValidAclHeader(Acl))
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    for (aceIndex = 0; aceIndex < Acl->AceCount; aceIndex++)
    {
        PACE_HEADER ace = (PACE_HEADER) LW_PTR_ADD(Acl, sizeUsed);
        if (!RtlpValidAce(ace))
        {
            status = STATUS_INVALID_ACL;
            GOTO_CLEANUP();
        }
        if (aceIndex == AceIndex)
        {
            aceOffset = sizeUsed - sizeof(ACL);
        }
        sizeUsed += ace->AceSize;
        // Check for integer overflow
        if (sizeUsed < ace->AceSize)
        {
            status = STATUS_INVALID_ACL;
            GOTO_CLEANUP();
        }
        // Check that not past the end of the ACL
        if (sizeUsed > Acl->AclSize)
        {
            status = STATUS_INVALID_ACL;
            GOTO_CLEANUP();
        }
    }

    // We would not find the offset *iff* we are looking for the end.
    if (0 == aceOffset)
    {
        if (((ULONG)-1) == AceIndex)
        {
            aceOffset = sizeUsed - sizeof(ACL);
        }
        else if (0 != AceIndex)
        {
            status = STATUS_ASSERTION_FAILURE;
            GOTO_CLEANUP();
        }
    }

    if (sizeUsed > Acl->AclSize)
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    aceLocation = LW_PTR_ADD(Acl, sizeof(ACL) + aceOffset);

cleanup:
    if (!NT_SUCCESS(status))
    {
        sizeUsed = 0;
        aceOffset = 0;
        aceLocation = NULL;
    }

    *AclSizeUsed = sizeUsed;
    *AceOffset = aceOffset;
    *AceLocation = aceLocation;

    return status;
}

static
NTSTATUS
RtlpValidAceList(
    IN ULONG AceRevision,
    IN PVOID AceList,
    IN ULONG AceListLength,
    OUT PUSHORT AceCount
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG aceOffset = 0;
    USHORT aceCount = 0;

    if ((0 == AceListLength) ||
        (AceListLength > (((USHORT)-1) - sizeof(ACL))))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    // Validate ACE list
    while (aceOffset < AceListLength)
    {
        PACE_HEADER aceHeader = (PACE_HEADER) LW_PTR_ADD(AceList, aceOffset);

        if ((sizeof(ACE_HEADER) + aceOffset) > AceListLength)
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP();
        }

        if (((ULONG) aceOffset + aceHeader->AceSize) > AceListLength)
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP();
        }

        if (!RtlpValidAce(aceHeader))
        {
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP();
        }

        // TODO-Check ACE type vs ACL revision

        aceCount++;
        aceOffset += aceHeader->AceSize;
    }

    // The list length should match.
    if (aceOffset != AceListLength)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

cleanup:
    if (!NT_SUCCESS(status))
    {
        aceCount = 0;
    }

    *AceCount = aceCount;

    return status;
}

NTSTATUS
RtlAddAce(
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ULONG StartingAceIndex,
    IN PVOID AceList,
    IN ULONG AceListLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT sizeUsed = 0;
    USHORT aceOffset = 0;
    PVOID aceLocation = NULL;
    USHORT aceListCount = 0;

    if (AceListLength == 0)
    {
        status = STATUS_SUCCESS;
        GOTO_CLEANUP();
    }

    if (!AceList)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!RtlpValidAclHeader(Acl))
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    if (!RtlpValidAclRevision(AceRevision) ||
        (AceRevision > Acl->AclRevision))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    // Quick check to bail early if we can.
    if (AceListLength > (Acl->AclSize - sizeof(ACL)))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    status = RtlpValidAceList(
                    AceRevision,
                    AceList,
                    AceListLength,
                    &aceListCount);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpGetAceLocationFromIndex(
                    Acl,
                    StartingAceIndex,
                    &sizeUsed,
                    &aceOffset,
                    &aceLocation);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpMakeRoomForAceAtLocation(Acl, sizeUsed, aceLocation, AceListLength);
    GOTO_CLEANUP_ON_STATUS(status);

    // Copy in the ACE list
    RtlCopyMemory(aceLocation, AceList, AceListLength);
    sizeUsed += AceListLength;
    Acl->AceCount += aceListCount;

cleanup:
    return status;
}

NTSTATUS
RtlDeleteAce(
    IN OUT PACL Acl,
    IN ULONG AceIndex
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT sizeUsed = 0;
    USHORT aceOffset = 0;
    PACE_HEADER aceLocation = NULL;

    if (AceIndex == ((ULONG)-1))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    status = RtlpGetAceLocationFromIndex(
                    Acl,
                    AceIndex,
                    &sizeUsed,
                    &aceOffset,
                    OUT_PPVOID(&aceLocation));
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpRemoveAceFromLocation(Acl, &sizeUsed, aceLocation);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    return status;
}


NTSTATUS
RtlGetAce(
    IN PACL Acl,
    IN ULONG AceIndex,
    OUT PVOID* Ace
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT sizeUsed = 0;
    USHORT aceOffset = 0;
    PACE_HEADER aceLocation = NULL;

    if (AceIndex == ((ULONG)-1))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    status = RtlpGetAceLocationFromIndex(
                    Acl,
                    AceIndex,
                    &sizeUsed,
                    &aceOffset,
                    OUT_PPVOID(&aceLocation));
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        aceLocation = NULL;
    }

    *Ace = aceLocation;

    return status;
}

NTSTATUS
RtlAddAccessAllowedAceEx(
    IN PACL Acl,
    IN ULONG AceRevision,
    IN ULONG AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT sizeUsed = 0;
    USHORT aceOffset = 0;
    PACCESS_ALLOWED_ACE aceLocation = NULL;
    USHORT aceSize = 0;

    if (!RtlpValidAclHeader(Acl))
    {
        status = STATUS_INVALID_ACL;
        GOTO_CLEANUP();
    }

    if (!RtlpValidAclRevision(AceRevision) ||
        (AceRevision > Acl->AclRevision))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!RtlValidSid(Sid))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    status = RtlpGetAceLocationFromIndex(
                    Acl,
                    ((ULONG)-1),
                    &sizeUsed,
                    &aceOffset,
                    OUT_PPVOID(&aceLocation));
    GOTO_CLEANUP_ON_STATUS(status);

    aceSize = RtlLengthAccessAllowedAce(Sid);

    status = RtlpMakeRoomForAceAtLocation(Acl, sizeUsed, aceLocation, aceSize);
    GOTO_CLEANUP_ON_STATUS(status);

    // We know we have at least aceSize bytes available.
    status = RtlInitializeAccessAllowedAce(
                    aceLocation,
                    aceSize,
                    AceFlags,
                    AccessMask,
                    Sid);
    GOTO_CLEANUP_ON_STATUS(status);

    assert(aceSize == aceLocation->Header.AceSize);
    sizeUsed += aceSize;
    Acl->AceCount++;

cleanup:
    return status;
}

NTSTATUS
RtlAddAccessDeniedAceEx(
    IN PACL Acl,
    IN ULONG AceRevision,
    IN ULONG AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
    )
{
    // ACCESS_DENIED_ACE is isomorphic wrt ACCESS_ALLOWED_ACE
    return RtlAddAccessAllowedAceEx(
                Acl,
                AceRevision,
                AceFlags,
                AccessMask,
                Sid);
}

//
// SD Functions
//

NTSTATUS
RtlCreateSecurityDescriptorAbsolute(
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN ULONG Revision
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (Revision != SECURITY_DESCRIPTOR_REVISION)
    {
        status = STATUS_UNKNOWN_REVISION;
        GOTO_CLEANUP();
    }

    RtlZeroMemory(SecurityDescriptor, sizeof(*SecurityDescriptor));
    SecurityDescriptor->Revision = Revision;

    status = STATUS_SUCCESS;

cleanup:
    return status;
}

static
NTSTATUS
RtlpVerifySecurityDescriptorHeader(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
    {
        status = STATUS_UNKNOWN_REVISION;
        GOTO_CLEANUP();
    }

    if ((SecurityDescriptor->Sbz1 != 0) &&
        !IsSetFlag(SecurityDescriptor->Control, SE_RM_CONTROL_VALID))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    // We never handle self-relative through this function.
    if (IsSetFlag(SecurityDescriptor->Control, SE_SELF_RELATIVE))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    if (!SecurityDescriptor->Owner &&
        IsSetFlag(SecurityDescriptor->Control, SE_OWNER_DEFAULTED))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    if (!SecurityDescriptor->Group &&
        IsSetFlag(SecurityDescriptor->Control, SE_GROUP_DEFAULTED))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    // If present bit is not set, ACL and defaulted bit cannot be set.
    if (!IsSetFlag(SecurityDescriptor->Control, SE_DACL_PRESENT) &&
        (SecurityDescriptor->Dacl ||
         IsSetFlag(SecurityDescriptor->Control, SE_DACL_DEFAULTED)))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    // If present bit is not set, ACL and defaulted bit cannot be set.
    if (!IsSetFlag(SecurityDescriptor->Control, SE_SACL_PRESENT) &&
        (SecurityDescriptor->Sacl ||
         IsSetFlag(SecurityDescriptor->Control, SE_SACL_DEFAULTED)))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    // TODO-Add more SD control validity checks.

    status = STATUS_SUCCESS;

cleanup:
    return status;
}

BOOLEAN
RtlValidSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor
    )
{
    BOOLEAN isValid = FALSE;

    if (!NT_SUCCESS(RtlpVerifySecurityDescriptorHeader(SecurityDescriptor)))
    {
        GOTO_CLEANUP();
    }

    // Check non-control fields

    if (SecurityDescriptor->Owner &&
        !RtlValidSid(SecurityDescriptor->Owner))
    {
        GOTO_CLEANUP();
    }

    if (SecurityDescriptor->Group &&
        !RtlValidSid(SecurityDescriptor->Group))
    {
        GOTO_CLEANUP();
    }

    if (SecurityDescriptor->Dacl &&
        !RtlValidAcl(SecurityDescriptor->Dacl, NULL))
    {
        GOTO_CLEANUP();
    }

    if (SecurityDescriptor->Sacl &&
        !RtlValidAcl(SecurityDescriptor->Sacl, NULL))
    {
        GOTO_CLEANUP();
    }

    isValid = TRUE;

cleanup:
    return isValid;
}

inline
static
BOOLEAN
RtlpIsBufferAvailable(
    IN ULONG MaximumSize,
    IN ULONG Offset,
    IN ULONG Size
    )
{
    BOOLEAN isAvailable = TRUE;

    // Check for overflow.
    if ((Offset + Size) < Offset)
    {
        isAvailable = FALSE;
        GOTO_CLEANUP();
    }

    if ((Offset + Size) > MaximumSize)
    {
        isAvailable = FALSE;
        GOTO_CLEANUP();
    }

    isAvailable = TRUE;

cleanup:
    return isAvailable;
}

typedef ULONG (*RTLP_GET_SIZE_FROM_HEADER_IN_RELATIVE_SD_CALLBACK)(IN PVOID Data);
typedef NTSTATUS (*RTLP_VERIFY_FROM_HEADER_IN_RELATIVE_SD_CALLBACK)(IN PVOID Data);

static
NTSTATUS
RtlpVerifyRelativeSecurityDescriptorOffset(
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    IN OUT PULONG SizeUsed,
    IN ULONG Offset,
    IN ULONG MinimumSize,
    IN RTLP_GET_SIZE_FROM_HEADER_IN_RELATIVE_SD_CALLBACK GetSizeCallback,
    IN RTLP_VERIFY_FROM_HEADER_IN_RELATIVE_SD_CALLBACK VerifyCallback
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG sizeUsed = *SizeUsed;

    if (Offset)
    {
        ULONG size = 0;

        if (Offset < sizeUsed)
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            GOTO_CLEANUP();
        }

        // Validate data header size
        if (!RtlpIsBufferAvailable(
                SecurityDescriptorLength,
                Offset,
                MinimumSize))
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            GOTO_CLEANUP();
        }

        // Get size from header
        size = GetSizeCallback(LW_PTR_ADD(SecurityDescriptor, Offset));

        // Validate data size
        if (!RtlpIsBufferAvailable(
                SecurityDescriptorLength,
                Offset,
                size))
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            GOTO_CLEANUP();
        }

        // Validate data
        status = VerifyCallback(LW_PTR_ADD(SecurityDescriptor, Offset));
        GOTO_CLEANUP_ON_STATUS(status);

        *SizeUsed = sizeUsed + size;
    }

cleanup:
    return status;
}

static
ULONG
RtlpSidGetSizeFromHeaderInRelativeSecurityDescriptor(
    IN PVOID Data
    )
{
    PSID littleEndianSid = (PSID) Data;
    return RtlLengthRequiredSid(LW_LTOH8(littleEndianSid->SubAuthorityCount));
}

static
ULONG
RtlpAclGetSizeFromHeaderInRelativeSecurityDescriptor(
    IN PVOID Data
    )
{
    PACL littleEndianAcl = (PACL) Data;
    return LW_LTOH16(littleEndianAcl->AclSize);
}

static
NTSTATUS
RtlpSidVerifyFromHeaderInRelativeSecurityDescriptor(
    IN PVOID Data
    )
{
    PSID littleEndianSid = (PSID) Data;
    // This is ok as long since it only looks at 1-byte fields:
    return RtlValidSid(littleEndianSid) ? STATUS_SUCCESS : STATUS_INVALID_SID;
}

static
NTSTATUS
RtlpAclVerifyFromHeaderInRelativeSecurityDescriptor(
    IN PVOID Data
    )
{
#if 0
    PACL littleEndianAcl = (PACL) Data;
#endif
    // TODO--Need to implement this.
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
RtlpVerifyRelativeSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SECURITY_DESCRIPTOR_RELATIVE relHeader = { 0 };
    SECURITY_DESCRIPTOR_ABSOLUTE absHeader = { 0 };
    ULONG sizeUsed = 0;

    if (!LW_IS_VALID_FLAGS(RequiredInformation, VALID_SECURITY_INFORMATION_MASK))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (SecurityDescriptorLength < sizeof(*SecurityDescriptor))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    relHeader.Revision = LW_LTOH8(SecurityDescriptor->Revision);
    relHeader.Sbz1 = LW_LTOH8(SecurityDescriptor->Sbz1);
    relHeader.Control = LW_LTOH16(SecurityDescriptor->Control);
    relHeader.Owner = LW_LTOH32(SecurityDescriptor->Owner);
    relHeader.Group = LW_LTOH32(SecurityDescriptor->Group);
    relHeader.Sacl = LW_LTOH32(SecurityDescriptor->Sacl);
    relHeader.Dacl = LW_LTOH32(SecurityDescriptor->Dacl);

    //
    // Check that required information is present.
    //

    if (IsSetFlag(RequiredInformation, OWNER_SECURITY_INFORMATION) &&
        !relHeader.Owner)
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    if (IsSetFlag(RequiredInformation, GROUP_SECURITY_INFORMATION) &&
        !relHeader.Group)
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    // Note that SACL can be present but NULL.
    if (IsSetFlag(RequiredInformation, SACL_SECURITY_INFORMATION) &&
        !IsSetFlag(relHeader.Control, SE_SACL_PRESENT))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    // Note that DACL can be present but NULL.
    if (IsSetFlag(RequiredInformation, DACL_SECURITY_INFORMATION) &&
        !IsSetFlag(relHeader.Control, SE_DACL_PRESENT))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    //
    // Verify header information
    //

    absHeader.Revision = relHeader.Revision;
    absHeader.Sbz1 = relHeader.Sbz1;
    absHeader.Control = relHeader.Control;
    // SID and ACL pointers are to denote existence and must not be dereferenced.
    absHeader.Owner = relHeader.Owner ? (PSID) LW_PTR_ADD(SecurityDescriptor, relHeader.Owner) : NULL;
    absHeader.Group = relHeader.Group ? (PSID) LW_PTR_ADD(SecurityDescriptor, relHeader.Group) : NULL;
    absHeader.Sacl = relHeader.Sacl ? (PACL) LW_PTR_ADD(SecurityDescriptor, relHeader.Sacl) : NULL;
    absHeader.Dacl = relHeader.Dacl ? (PACL) LW_PTR_ADD(SecurityDescriptor, relHeader.Dacl) : NULL;

    status = RtlpVerifySecurityDescriptorHeader(&absHeader);
    GOTO_CLEANUP_ON_STATUS(status);

    //
    // Check validity of offsets.
    //

    sizeUsed = sizeof(*SecurityDescriptor);

    status = RtlpVerifyRelativeSecurityDescriptorOffset(
                    SecurityDescriptor,
                    SecurityDescriptorLength,
                    &sizeUsed,
                    relHeader.Owner,
                    SID_MIN_SIZE,
                    RtlpSidGetSizeFromHeaderInRelativeSecurityDescriptor,
                    RtlpSidVerifyFromHeaderInRelativeSecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpVerifyRelativeSecurityDescriptorOffset(
                    SecurityDescriptor,
                    SecurityDescriptorLength,
                    &sizeUsed,
                    relHeader.Group,
                    SID_MIN_SIZE,
                    RtlpSidGetSizeFromHeaderInRelativeSecurityDescriptor,
                    RtlpSidVerifyFromHeaderInRelativeSecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpVerifyRelativeSecurityDescriptorOffset(
                    SecurityDescriptor,
                    SecurityDescriptorLength,
                    &sizeUsed,
                    relHeader.Sacl,
                    ACL_HEADER_SIZE,
                    RtlpAclGetSizeFromHeaderInRelativeSecurityDescriptor,
                    RtlpAclVerifyFromHeaderInRelativeSecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpVerifyRelativeSecurityDescriptorOffset(
                    SecurityDescriptor,
                    SecurityDescriptorLength,
                    &sizeUsed,
                    relHeader.Dacl,
                    ACL_HEADER_SIZE,
                    RtlpAclGetSizeFromHeaderInRelativeSecurityDescriptor,
                    RtlpAclVerifyFromHeaderInRelativeSecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    if (sizeUsed > SecurityDescriptorLength)
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = STATUS_SUCCESS;

cleanup:
    return status;
}

BOOLEAN
RtlValidRelativeSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    IN SECURITY_INFORMATION RequiredInformation
    )
{
    NTSTATUS status = RtlpVerifyRelativeSecurityDescriptor(
                            SecurityDescriptor,
                            SecurityDescriptorLength,
                            RequiredInformation);
    return NT_SUCCESS(status);
}

ULONG
RtlLengthSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor
    )
{
    ULONG size = SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE;

    if (SecurityDescriptor->Owner)
    {
        size += RtlLengthSid(SecurityDescriptor->Owner);
    }

    if (SecurityDescriptor->Group)
    {
        size += RtlLengthSid(SecurityDescriptor->Group);
    }

    if (SecurityDescriptor->Dacl)
    {
        size += SecurityDescriptor->Dacl->AclSize;
    }

    if (SecurityDescriptor->Sacl)
    {
        size += SecurityDescriptor->Sacl->AclSize;
    }

    return size;
}

NTSTATUS
RtlGetSecurityDescriptorControl(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_CONTROL Control,
    OUT OPTIONAL PUCHAR Revision
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SECURITY_DESCRIPTOR_CONTROL control = 0;
    UCHAR revision = 0;

    status = RtlpVerifySecurityDescriptorHeader(SecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    control = SecurityDescriptor->Control;
    revision = SecurityDescriptor->Revision;

cleanup:
    if (!NT_SUCCESS(status))
    {
        control = 0;
        revision = 0;
    }

    if (Control)
    {
        *Control = control;
    }

    if (Revision)
    {
        *Revision = revision;
    }

    return status;
}

NTSTATUS
RtlSetSecurityDescriptorControl(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL BitsToChange,
    IN SECURITY_DESCRIPTOR_CONTROL BitsToSet
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SECURITY_DESCRIPTOR_CONTROL bitsToClear = 0;

    status = RtlpVerifySecurityDescriptorHeader(SecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    if (!LW_IS_VALID_FLAGS(BitsToChange, SE_SET_SECURITY_DESCRIPTOR_CONTROL_MASK) ||
        IsSetFlag(BitsToChange, ~BitsToSet))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    SetFlag(SecurityDescriptor->Control, BitsToSet);
    bitsToClear = (BitsToChange & ~BitsToSet);
    ClearFlag(SecurityDescriptor->Control, bitsToClear);

cleanup:
    return status;
}

static
NTSTATUS
RtlpGetSidSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN PSID* SidLocation,
    IN SECURITY_DESCRIPTOR_CONTROL DefaultedBit,
    OUT PSID* Sid,
    OUT PBOOLEAN IsSidDefaulted
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;
    BOOLEAN isSidDefaulted = FALSE;

    status = RtlpVerifySecurityDescriptorHeader(SecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    sid = *SidLocation;
    isSidDefaulted = IsSetFlag(SecurityDescriptor->Control, DefaultedBit);

cleanup:
    if (!NT_SUCCESS(status))
    {
        sid = NULL;
        isSidDefaulted = FALSE;
    }

    *Sid  = sid;
    *IsSidDefaulted = isSidDefaulted;

    return status;
}

NTSTATUS
RtlGetOwnerSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PSID* Owner,
    OUT PBOOLEAN IsOwnerDefaulted
    )
{
    return RtlpGetSidSecurityDescriptor(
                SecurityDescriptor,
                &SecurityDescriptor->Owner,
                SE_OWNER_DEFAULTED,
                Owner,
                IsOwnerDefaulted);
}

NTSTATUS
RtlGetGroupSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PSID* Group,
    OUT PBOOLEAN IsGroupDefaulted
    )
{
    return RtlpGetSidSecurityDescriptor(
                SecurityDescriptor,
                &SecurityDescriptor->Group,
                SE_GROUP_DEFAULTED,
                Group,
                IsGroupDefaulted);
}

static
NTSTATUS
RtlpGetAclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL PresentBit,
    IN PACL* AclLocation, // use address in case security descriptor was NULL
    IN SECURITY_DESCRIPTOR_CONTROL DefaultedBit,
    OUT PBOOLEAN IsAclPresent,
    OUT PACL* Acl,
    OUT PBOOLEAN IsAclDefaulted
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN isAclPresent = FALSE;
    PACL acl = NULL;
    BOOLEAN isAclDefaulted = FALSE;

    status = RtlpVerifySecurityDescriptorHeader(SecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    isAclPresent = IsSetFlag(SecurityDescriptor->Control, PresentBit);
    acl = *AclLocation;
    isAclDefaulted = IsSetFlag(SecurityDescriptor->Control, DefaultedBit);

cleanup:
    if (!NT_SUCCESS(status))
    {
        isAclPresent = FALSE;
        acl = NULL;
        isAclDefaulted = FALSE;
    }

    *IsAclPresent= isAclPresent;
    *Acl = acl;
    *IsAclDefaulted = isAclDefaulted;

    return status;
}

NTSTATUS
RtlGetSaclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PBOOLEAN IsSaclPresent,
    OUT PACL* Sacl,
    OUT PBOOLEAN IsSaclDefaulted
    )
{
    return RtlpGetAclSecurityDescriptor(
                SecurityDescriptor,
                SE_SACL_PRESENT,
                &SecurityDescriptor->Sacl,
                SE_SACL_DEFAULTED,
                IsSaclPresent,
                Sacl,
                IsSaclDefaulted);
}

NTSTATUS
RtlGetDaclSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PBOOLEAN IsDaclPresent,
    OUT PACL* Dacl,
    OUT PBOOLEAN IsDaclDefaulted
    )
{
    return RtlpGetAclSecurityDescriptor(
                SecurityDescriptor,
                SE_DACL_PRESENT,
                &SecurityDescriptor->Dacl,
                SE_DACL_DEFAULTED,
                IsDaclPresent,
                Dacl,
                IsDaclDefaulted);
}

static
NTSTATUS
RtlpSetSidSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN PSID* SidLocation,
    IN SECURITY_DESCRIPTOR_CONTROL DefaultedBit,
    IN PSID Sid,
    IN BOOLEAN IsSidDefaulted
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if (!Sid && IsSidDefaulted)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    status = RtlpVerifySecurityDescriptorHeader(SecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    *SidLocation = Sid;
    if (IsSidDefaulted)
    {
        SetFlag(SecurityDescriptor->Control, DefaultedBit);
    }
    else
    {
        ClearFlag(SecurityDescriptor->Control, DefaultedBit);
    }

cleanup:
    return status;
}

NTSTATUS
RtlSetOwnerSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN PSID Owner,
    IN BOOLEAN IsOwnerDefaulted
    )
{
    return RtlpSetSidSecurityDescriptor(
                SecurityDescriptor,
                &SecurityDescriptor->Owner,
                SE_OWNER_DEFAULTED,
                Owner,
                IsOwnerDefaulted);
}

NTSTATUS
RtlSetGroupSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN OPTIONAL PSID Group,
    IN BOOLEAN IsGroupDefaulted
    )
{
    return RtlpSetSidSecurityDescriptor(
                SecurityDescriptor,
                &SecurityDescriptor->Group,
                SE_GROUP_DEFAULTED,
                Group,
                IsGroupDefaulted);
}

static
NTSTATUS
RtlpSetAclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL PresentBit,
    IN PACL* AclLocation,
    IN SECURITY_DESCRIPTOR_CONTROL DefaultedBit,
    IN BOOLEAN IsAclPresent,
    IN OPTIONAL PACL Acl,
    IN OPTIONAL BOOLEAN IsAclDefaulted
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PACL acl = NULL;
    BOOLEAN isAclDefaulted = FALSE;

    status = RtlpVerifySecurityDescriptorHeader(SecurityDescriptor);
    GOTO_CLEANUP_ON_STATUS(status);

    if (IsAclPresent)
    {
        SetFlag(SecurityDescriptor->Control, PresentBit);
        acl = *AclLocation;
        isAclDefaulted = IsAclDefaulted;
    }
    else
    {
        ClearFlag(SecurityDescriptor->Control, PresentBit);
        acl = NULL;
        isAclDefaulted = FALSE;
    }

    *AclLocation = acl;
    if (isAclDefaulted)
    {
        SetFlag(SecurityDescriptor->Control, DefaultedBit);
    }
    else
    {
        ClearFlag(SecurityDescriptor->Control, DefaultedBit);
    }

cleanup:
    return status;
}

NTSTATUS
RtlSetSaclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN BOOLEAN IsSaclPresent,
    IN OPTIONAL PACL Sacl,
    IN OPTIONAL BOOLEAN IsSaclDefaulted
    )
{
    return RtlpSetAclSecurityDescriptor(
                SecurityDescriptor,
                SE_SACL_PRESENT,
                &SecurityDescriptor->Sacl,
                SE_SACL_DEFAULTED,
                IsSaclPresent,
                Sacl,
                IsSaclDefaulted);
}

NTSTATUS
RtlSetDaclSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    IN BOOLEAN IsDaclPresent,
    IN OPTIONAL PACL Dacl,
    IN OPTIONAL BOOLEAN IsDaclDefaulted
    )
{
    return RtlpSetAclSecurityDescriptor(
                SecurityDescriptor,
                SE_DACL_PRESENT,
                &SecurityDescriptor->Dacl,
                SE_DACL_DEFAULTED,
                IsDaclPresent,
                Dacl,
                IsDaclDefaulted);
}

NTSTATUS
RtlAbsoluteToSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE AbsoluteSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR_RELATIVE SelfRelativeSecurityDescriptor,
    IN OUT PULONG BufferLength
    );

NTSTATUS
RtlSelfRelativeToAbsoluteSD(
    IN PSECURITY_DESCRIPTOR_RELATIVE SelfRelativeSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR_ABSOLUTE AbsoluteSecurityDescriptor,
    IN OUT PULONG AbsoluteSecurityDescriptorSize,
    OUT PACL Dacl,
    IN OUT PULONG DaclSize,
    OUT PACL Sacl,
    IN OUT PULONG SaclSize,
    OUT PSID Owner,
    IN OUT PULONG OwnerSize,
    OUT PSID PrimaryGroup,
    IN OUT PULONG PrimaryGroupSize
    );

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
        !Owner || !Owner->Owner ||
        !PrimaryGroup || !PrimaryGroup->PrimaryGroup ||
        !DefaultDacl)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!RtlValidSid(User->User.Sid) ||
        !RtlValidSid(Owner->Owner) ||
        !RtlValidSid(PrimaryGroup->PrimaryGroup))
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

    size = RtlLengthSid(Owner->Owner);
    status = RtlSafeAddULONG(&requiredSize, requiredSize, size);
    GOTO_CLEANUP_ON_STATUS(status);

    size = RtlLengthSid(PrimaryGroup->PrimaryGroup);
    status = RtlSafeAddULONG(&requiredSize, requiredSize, size);
    GOTO_CLEANUP_ON_STATUS(status);

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

    token->Owner.Owner = (PSID) location;
    location = RtlpAppendData(location,
                              Owner->Owner,
                              RtlLengthSid(Owner->Owner));

    token->PrimaryGroup.PrimaryGroup = (PSID) location;
    location = RtlpAppendData(location,
                              PrimaryGroup->PrimaryGroup,
                              RtlLengthSid(PrimaryGroup->PrimaryGroup));

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

//
// SID <-> String Conversion Functions
//

//
// A string SID is represented as:
//
//   "S-" REV "-" AUTH ("-" SUB_AUTH ) * SubAuthorityCount
//
// where:
//
//   - REV is a decimal UCHAR (max 3 characters).
//
//   - AUTH is either a decimal ULONG (max 10 characters) or
//     "0x" followed by a 6-byte hex value (max 2 + 12 = 14 characters).
//
//   - SUB_AUTH is a decimal ULONG (max 10 characters).
//

#define RTLP_STRING_SID_MAX_CHARS(SubAuthorityCount) \
    (2 + 3 + 1 + 14 + (1 + 10) * (SubAuthorityCount) + 1)

NTSTATUS
RtlAllocateUnicodeStringFromSid(
    OUT PUNICODE_STRING StringSid,
    IN PSID Sid
    );

NTSTATUS
RtlAllocateAnsiStringFromSid(
    OUT PANSI_STRING StringSid,
    IN PSID Sid
    );

NTSTATUS
RtlAllocateWC16StringFromSid(
    OUT PWSTR* StringSid,
    IN PSID Sid
    );

NTSTATUS
RtlAllocateCStringFromSid(
    OUT PSTR* StringSid,
    IN PSID Sid
    );

static
NTSTATUS
RtlpConvertUnicodeStringSidToSidEx(
    IN PUNICODE_STRING StringSid,
    OUT OPTIONAL PSID* AllocateSid,
    OUT OPTIONAL PSID SidBuffer,
    IN OUT OPTIONAL PULONG SidBufferSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR sidBuffer[SID_MAX_SIZE] = { 0 };
    PSID sid = (PSID) sidBuffer;
    BOOLEAN haveRevision = FALSE;
    BOOLEAN haveAuthority = FALSE;
    UNICODE_STRING remaining = { 0 };
    PSID newSid = NULL;
    ULONG sizeRequired = 0;

    if (!StringSid ||
        (AllocateSid && (SidBuffer || SidBufferSize)) ||
        !(AllocateSid || SidBufferSize))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    // Must have at least 2 characters and they must be "S-"
    if (!((StringSid->Length > (2 * sizeof(StringSid->Buffer[0]))) &&
          ((StringSid->Buffer[0] == 'S') || (StringSid->Buffer[0] == 's')) &&
          (StringSid->Buffer[1] == '-')))
    {
        status = STATUS_INVALID_SID;
        GOTO_CLEANUP();
    }

    // Skip the "S-" prefix
    remaining.Buffer = &StringSid->Buffer[2];
    remaining.Length = StringSid->Length - (2 * sizeof(StringSid->Buffer[0]));
    remaining.MaximumLength = remaining.Length;

    // TODO-Handle S-1-0xHEX-... for more than 4 bytes in IdentifierAuth.
    for (;;)
    {
        ULONG value = 0;

        status = LwRtlUnicodeStringParseULONG(&value, &remaining, &remaining);
        if (!NT_SUCCESS(status))
        {
            break;
        }

        if (remaining.Length)
        {
            if (LW_RTL_STRING_LAST_CHAR(&remaining) != '-')
            {
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
            }
            remaining.Buffer++;
            remaining.Length -= sizeof(remaining.Buffer[0]);
            remaining.MaximumLength = remaining.Length;
        }

        if (!haveRevision)
        {
            if (value > MAXUCHAR)
            {
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
            }
            sid->Revision = (UCHAR) value;
            haveRevision = TRUE;
        }
        else if (!haveAuthority)
        {
            // Authority is represented as a 32-bit number.
            sid->IdentifierAuthority.Value[5] = (value & 0x000000FF);
            sid->IdentifierAuthority.Value[4] = (value & 0x0000FF00) >> 8;
            sid->IdentifierAuthority.Value[3] = (value & 0x00FF0000) >> 16;
            sid->IdentifierAuthority.Value[2] = (value & 0xFF000000) >> 24;
            haveAuthority = TRUE;
        }
        else
        {
            if (sid->SubAuthorityCount >= SID_MAX_SUB_AUTHORITIES)
            {
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
            }
            sid->SubAuthority[sid->SubAuthorityCount] = value;
            sid->SubAuthorityCount++;
        }
    }

    if (!haveAuthority || remaining.Length || !RtlValidSid(sid))
    {
        status = STATUS_INVALID_SID;
        GOTO_CLEANUP();
    }

    sizeRequired = RtlLengthSid(sid);

    if (AllocateSid)
    {
        status = RTL_ALLOCATE(&newSid, SID, sizeRequired);
        GOTO_CLEANUP_ON_STATUS(status);

        RtlCopyMemory(newSid, sid, sizeRequired);
    }
    else if (SidBufferSize)
    {
        if (*SidBufferSize < sizeRequired)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            GOTO_CLEANUP();
        }
        if (SidBuffer)
        {
            RtlCopyMemory(SidBuffer, sid, sizeRequired);
        }
    }
    else
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&newSid);
        sid = NULL;
    }

    if (AllocateSid)
    {
        *AllocateSid = newSid;
    }

    if (SidBufferSize)
    {
        *SidBufferSize = sizeRequired;
    }

    return status;
}

static
NTSTATUS
RtlpConvertAnsiStringSidToSidEx(
    IN PANSI_STRING StringSid,
    OUT OPTIONAL PSID* AllocateSid,
    OUT OPTIONAL PSID SidBuffer,
    IN OUT OPTIONAL PULONG SidBufferSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR sidBuffer[SID_MAX_SIZE] = { 0 };
    PSID sid = (PSID) sidBuffer;
    BOOLEAN haveRevision = FALSE;
    BOOLEAN haveAuthority = FALSE;
    ANSI_STRING remaining = { 0 };
    PSID newSid = NULL;
    ULONG sizeRequired = 0;

    if (!StringSid ||
        (AllocateSid && (SidBuffer || SidBufferSize)) ||
        !(AllocateSid || SidBufferSize))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    // Must have at least 2 characters and they must be "S-"
    if (!((StringSid->Length > (2 * sizeof(StringSid->Buffer[0]))) &&
          ((StringSid->Buffer[0] == 'S') || (StringSid->Buffer[0] == 's')) &&
          (StringSid->Buffer[1] == '-')))
    {
        status = STATUS_INVALID_SID;
        GOTO_CLEANUP();
    }

    // Skip the "S-" prefix
    remaining.Buffer = &StringSid->Buffer[2];
    remaining.Length = StringSid->Length - (2 * sizeof(StringSid->Buffer[0]));
    remaining.MaximumLength = remaining.Length;

    // TODO-Handle S-1-0xHEX-... for more than 4 bytes in IdentifierAuth.
    for (;;)
    {
        ULONG value = 0;

        status = LwRtlAnsiStringParseULONG(&value, &remaining, &remaining);
        if (!NT_SUCCESS(status))
        {
            break;
        }

        if (remaining.Length)
        {
            if (LW_RTL_STRING_LAST_CHAR(&remaining) != '-')
            {
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
            }
            remaining.Buffer++;
            remaining.Length -= sizeof(remaining.Buffer[0]);
            remaining.MaximumLength = remaining.Length;
        }

        if (!haveRevision)
        {
            if (value > MAXUCHAR)
            {
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
            }
            sid->Revision = (UCHAR) value;
            haveRevision = TRUE;
        }
        else if (!haveAuthority)
        {
            // Authority is represented as a 32-bit number.
            sid->IdentifierAuthority.Value[5] = (value & 0x000000FF);
            sid->IdentifierAuthority.Value[4] = (value & 0x0000FF00) >> 8;
            sid->IdentifierAuthority.Value[3] = (value & 0x00FF0000) >> 16;
            sid->IdentifierAuthority.Value[2] = (value & 0xFF000000) >> 24;
            haveAuthority = TRUE;
        }
        else
        {
            if (sid->SubAuthorityCount >= SID_MAX_SUB_AUTHORITIES)
            {
                status = STATUS_INVALID_SID;
                GOTO_CLEANUP();
            }
            sid->SubAuthority[sid->SubAuthorityCount] = value;
            sid->SubAuthorityCount++;
        }
    }

    if (!haveAuthority || remaining.Length || !RtlValidSid(sid))
    {
        status = STATUS_INVALID_SID;
        GOTO_CLEANUP();
    }

    sizeRequired = RtlLengthSid(sid);

    if (AllocateSid)
    {
        status = RTL_ALLOCATE(&newSid, SID, sizeRequired);
        GOTO_CLEANUP_ON_STATUS(status);

        RtlCopyMemory(newSid, sid, sizeRequired);
    }
    else if (SidBufferSize)
    {
        if (*SidBufferSize < sizeRequired)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            GOTO_CLEANUP();
        }
        if (SidBuffer)
        {
            RtlCopyMemory(SidBuffer, sid, sizeRequired);
        }
    }
    else
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    status = STATUS_SUCCESS;

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&newSid);
        sid = NULL;
    }

    if (AllocateSid)
    {
        *AllocateSid = newSid;
    }

    if (SidBufferSize)
    {
        *SidBufferSize = sizeRequired;
    }

    return status;
}

NTSTATUS
RtlAllocateSidFromUnicodeString(
    OUT PSID* Sid,
    IN PUNICODE_STRING StringSid
    )
{
    return RtlpConvertUnicodeStringSidToSidEx(StringSid, Sid, NULL, NULL);
}

NTSTATUS
RtlAllocateSidFromAnsiString(
    OUT PSID* Sid,
    IN PANSI_STRING StringSid
    )
{
    return RtlpConvertAnsiStringSidToSidEx(StringSid, Sid, NULL, NULL);
}

NTSTATUS
RtlAllocateSidFromCString(
    OUT PSID* Sid,
    IN PCSTR StringSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;
    ANSI_STRING stringSid = { 0 };

    status = RtlAnsiStringInitEx(&stringSid, StringSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpConvertAnsiStringSidToSidEx(&stringSid, &sid, NULL, NULL);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&sid);
    }

    *Sid = sid;

    return status;
}

NTSTATUS
RtlAllocateSidFromWC16String(
    OUT PSID* Sid,
    IN PCWSTR StringSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;
    UNICODE_STRING stringSid = { 0 };

    status = RtlUnicodeStringInitEx(&stringSid, StringSid);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlAllocateSidFromUnicodeString(&sid, &stringSid);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (!NT_SUCCESS(status))
    {
        RTL_FREE(&sid);
    }

    *Sid = sid;

    return status;
}

//
// Well-Known SID Functions
//

NTSTATUS
RtlCreateWellKnownSid(
    IN WELL_KNOWN_SID_TYPE WellKnownSidType,
    IN OPTIONAL PSID DomainOrComputerSid,
    OUT PSID* Sid,
    IN OUT PULONG SidSize
    );
