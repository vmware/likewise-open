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
 *        security-sd.c
 *
 * Abstract:
 *
 *        Security Descriptor (SD) Functions in Security Module.
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#include "security-includes.h"

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

typedef BOOLEAN (*RTLP_IS_VALID_BUFFER_CALLBACK)(
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BufferUsed
    );

static
NTSTATUS
RtlpVerifyRelativeSecurityDescriptorOffset(
    IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    IN OUT PULONG SizeUsed,
    IN ULONG Offset,
    IN RTLP_IS_VALID_BUFFER_CALLBACK IsValidBufferCallback
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

        if (Offset > SecurityDescriptorLength)
        {
            status = STATUS_ASSERTION_FAILURE;
            GOTO_CLEANUP();
        }

        // Validate data
        if (!IsValidBufferCallback(
                    LW_PTR_ADD(SecurityDescriptor, Offset),
                    SecurityDescriptorLength - Offset,
                    &size))
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            GOTO_CLEANUP();
        }

        *SizeUsed = sizeUsed + size;
    }

    status = STATUS_SUCCESS;

cleanup:
    return status;
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
    // The self-relative bit must be set.
    //

    if (!IsSetFlag(relHeader.Control, SE_SELF_RELATIVE))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

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

    // Clear the self-relative flag since it cannot be present in the
    // absolute header.
    ClearFlag(absHeader.Control, SE_SELF_RELATIVE);

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
                    RtlpIsValidLittleEndianSidBuffer);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpVerifyRelativeSecurityDescriptorOffset(
                    SecurityDescriptor,
                    SecurityDescriptorLength,
                    &sizeUsed,
                    relHeader.Group,
                    RtlpIsValidLittleEndianSidBuffer);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpVerifyRelativeSecurityDescriptorOffset(
                    SecurityDescriptor,
                    SecurityDescriptorLength,
                    &sizeUsed,
                    relHeader.Sacl,
                    RtlpIsValidLittleEndianAclBuffer);
    GOTO_CLEANUP_ON_STATUS(status);

    status = RtlpVerifyRelativeSecurityDescriptorOffset(
                    SecurityDescriptor,
                    SecurityDescriptorLength,
                    &sizeUsed,
                    relHeader.Dacl,
                    RtlpIsValidLittleEndianAclBuffer);
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
        acl = Acl;
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

static
NTSTATUS
RtlpGetSizeUsedSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE SecurityDescriptor,
    OUT PULONG SizeUsed
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG size = SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE;
    USHORT aclSizeUsed = 0;

    if (!NT_SUCCESS(RtlpVerifySecurityDescriptorHeader(SecurityDescriptor)))
    {
        GOTO_CLEANUP();
    }

    // Check non-control fields

    if (SecurityDescriptor->Owner)
    {
        if (!RtlValidSid(SecurityDescriptor->Owner))
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            GOTO_CLEANUP();
        }
        size += RtlLengthSid(SecurityDescriptor->Owner);
    }

    if (SecurityDescriptor->Group)
    {
        if (!RtlValidSid(SecurityDescriptor->Group))
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            GOTO_CLEANUP();
        }
        size += RtlLengthSid(SecurityDescriptor->Group);
    }

    if (SecurityDescriptor->Dacl)
    {
        if (!RtlValidAcl(SecurityDescriptor->Dacl, &aclSizeUsed))
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            GOTO_CLEANUP();
        }
        size += aclSizeUsed;
    }

    if (SecurityDescriptor->Sacl)
    {
        if (!RtlValidAcl(SecurityDescriptor->Sacl, &aclSizeUsed))
        {
            status = STATUS_INVALID_SECURITY_DESCR;
            GOTO_CLEANUP();
        }
        size += aclSizeUsed;
    }

    status = STATUS_SUCCESS;

cleanup:
    *SizeUsed = NT_SUCCESS(status) ? size : 0;
    return status;
}

NTSTATUS
RtlAbsoluteToSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR_ABSOLUTE AbsoluteSecurityDescriptor,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE SelfRelativeSecurityDescriptor,
    IN OUT PULONG BufferLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG sizeRequired = 0;
    ULONG offset = 0;
    ULONG size = 0;

    if (!AbsoluteSecurityDescriptor ||
        !BufferLength)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    size = *BufferLength;

    if ((size > 0) && !SelfRelativeSecurityDescriptor)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    if (!RtlValidSecurityDescriptor(AbsoluteSecurityDescriptor))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        GOTO_CLEANUP();
    }

    status = RtlpGetSizeUsedSecurityDescriptor(
                    AbsoluteSecurityDescriptor,
                    &sizeRequired);
    GOTO_CLEANUP_ON_STATUS(status);

    // Use self-relative header size
    sizeRequired -= SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE;
    sizeRequired += SECURITY_DESCRIPTOR_RELATIVE_MIN_SIZE;

    if (sizeRequired > size)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    SelfRelativeSecurityDescriptor->Revision = LW_HTOL8(AbsoluteSecurityDescriptor->Revision);
    SelfRelativeSecurityDescriptor->Sbz1 = LW_HTOL8(AbsoluteSecurityDescriptor->Sbz1);
    SelfRelativeSecurityDescriptor->Control = LW_HTOL16(AbsoluteSecurityDescriptor->Control | SE_SELF_RELATIVE);

    offset = SECURITY_DESCRIPTOR_RELATIVE_MIN_SIZE;

    if (AbsoluteSecurityDescriptor->Owner)
    {
        ULONG used = 0;
        status = RtlpEncodeLittleEndianSid(
                        AbsoluteSecurityDescriptor->Owner,
                        LwRtlOffsetToPointer(SelfRelativeSecurityDescriptor, offset),
                        size - offset,
                        &used);
        GOTO_CLEANUP_ON_STATUS(status);
        SelfRelativeSecurityDescriptor->Owner = LW_HTOL32(offset);
        offset += used;
    }

    if (AbsoluteSecurityDescriptor->Group)
    {
        ULONG used = 0;
        status = RtlpEncodeLittleEndianSid(
                        AbsoluteSecurityDescriptor->Group,
                        LwRtlOffsetToPointer(SelfRelativeSecurityDescriptor, offset),
                        size - offset,
                        &used);
        GOTO_CLEANUP_ON_STATUS(status);
        SelfRelativeSecurityDescriptor->Group = LW_HTOL32(offset);
        offset += used;
    }

    if (AbsoluteSecurityDescriptor->Dacl)
    {
        ULONG used = 0;
        status = RtlpEncodeLittleEndianAcl(
                        AbsoluteSecurityDescriptor->Dacl,
                        LwRtlOffsetToPointer(SelfRelativeSecurityDescriptor, offset),
                        size - offset,
                        &used);
        GOTO_CLEANUP_ON_STATUS(status);
        SelfRelativeSecurityDescriptor->Dacl = LW_HTOL32(offset);
        offset += used;
    }

    if (AbsoluteSecurityDescriptor->Sacl)
    {
        ULONG used = 0;
        status = RtlpEncodeLittleEndianAcl(
                        AbsoluteSecurityDescriptor->Sacl,
                        LwRtlOffsetToPointer(SelfRelativeSecurityDescriptor, offset),
                        size - offset,
                        &used);
        GOTO_CLEANUP_ON_STATUS(status);
        SelfRelativeSecurityDescriptor->Sacl = LW_HTOL32(offset);
        offset += used;
    }

    status = STATUS_SUCCESS;

cleanup:
    if (BufferLength)
    {
        *BufferLength = sizeRequired;
    }

    return status;
}

NTSTATUS
RtlSelfRelativeToAbsoluteSD(
    IN PSECURITY_DESCRIPTOR_RELATIVE SelfRelativeSecurityDescriptor,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE AbsoluteSecurityDescriptor,
    IN OUT PULONG AbsoluteSecurityDescriptorSize,
    OUT OPTIONAL PACL Dacl,
    IN OUT PULONG DaclSize,
    OUT OPTIONAL PACL Sacl,
    IN OUT PULONG SaclSize,
    OUT OPTIONAL PSID Owner,
    IN OUT PULONG OwnerSize,
    OUT OPTIONAL PSID PrimaryGroup,
    IN OUT PULONG PrimaryGroupSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SECURITY_DESCRIPTOR_RELATIVE relHeader = { 0 };
    SECURITY_DESCRIPTOR_ABSOLUTE absHeader = { 0 };
    ULONG securityDescriptorSize = 0;
    ULONG daclSize = 0;
    ULONG saclSize = 0;
    ULONG ownerSize = 0;
    ULONG groupSize = 0;
    ULONG securityDescriptorSizeRequired = 0;
    ULONG daclSizeRequired = 0;
    ULONG saclSizeRequired = 0;
    ULONG ownerSizeRequired = 0;
    ULONG groupSizeRequired = 0;

    if (!AbsoluteSecurityDescriptorSize ||
        !DaclSize ||
        !SaclSize ||
        !OwnerSize ||
        !PrimaryGroupSize)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    securityDescriptorSize = *AbsoluteSecurityDescriptorSize;
    daclSize = *DaclSize;
    saclSize = *SaclSize;
    ownerSize = *OwnerSize;
    groupSize = *PrimaryGroupSize;

    if (((securityDescriptorSize > 0) && !AbsoluteSecurityDescriptor) ||
        ((daclSize > 0) && !Dacl) ||
        ((saclSize > 0) && !Sacl) ||
        ((ownerSize > 0) && !Owner) ||
        ((groupSize > 0) && !PrimaryGroup))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    //
    // Extract header information
    //

    relHeader.Revision = LW_LTOH8(SelfRelativeSecurityDescriptor->Revision);
    relHeader.Sbz1 = LW_LTOH8(SelfRelativeSecurityDescriptor->Sbz1);
    relHeader.Control = LW_LTOH16(SelfRelativeSecurityDescriptor->Control);
    relHeader.Owner = LW_LTOH32(SelfRelativeSecurityDescriptor->Owner);
    relHeader.Group = LW_LTOH32(SelfRelativeSecurityDescriptor->Group);
    relHeader.Sacl = LW_LTOH32(SelfRelativeSecurityDescriptor->Sacl);
    relHeader.Dacl = LW_LTOH32(SelfRelativeSecurityDescriptor->Dacl);

    //
    // The self-relative bit must be set.
    //
    // The caller is required to have already checked this via
    // RtlValidRelativeSecurityDescriptor().  Otherwise, the caller
    // is violating an invariant and this code will return
    // STATUS_ASSERTION_FAILURE.
    //

    if (!IsSetFlag(relHeader.Control, SE_SELF_RELATIVE))
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    //
    // Verify header information
    //

    absHeader.Revision = relHeader.Revision;
    absHeader.Sbz1 = relHeader.Sbz1;
    absHeader.Control = relHeader.Control;
    // SID and ACL pointers are to denote existence and must not be dereferenced.
    absHeader.Owner = relHeader.Owner ? (PSID) LW_PTR_ADD(SelfRelativeSecurityDescriptor, relHeader.Owner) : NULL;
    absHeader.Group = relHeader.Group ? (PSID) LW_PTR_ADD(SelfRelativeSecurityDescriptor, relHeader.Group) : NULL;
    absHeader.Sacl = relHeader.Sacl ? (PACL) LW_PTR_ADD(SelfRelativeSecurityDescriptor, relHeader.Sacl) : NULL;
    absHeader.Dacl = relHeader.Dacl ? (PACL) LW_PTR_ADD(SelfRelativeSecurityDescriptor, relHeader.Dacl) : NULL;

    // Clear the self-relative flag since it cannot be present in the
    // absolute header.
    ClearFlag(absHeader.Control, SE_SELF_RELATIVE);

    //
    // The security descriptor header must be valid.
    //
    // The caller is required to have already checked this via
    // RtlValidRelativeSecurityDescriptor().  Otherwise, the caller
    // is violating an invariant and this code will return
    // STATUS_ASSERTION_FAILURE.
    //

    status = RtlpVerifySecurityDescriptorHeader(&absHeader);
    if (!NT_SUCCESS(status))
    {
        status = STATUS_ASSERTION_FAILURE;
        GOTO_CLEANUP();
    }

    //
    // Get size requirements
    //

    securityDescriptorSizeRequired = SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE;

    if (absHeader.Owner)
    {
        ownerSizeRequired = RtlLengthRequiredSid(LW_LTOH8(absHeader.Owner->SubAuthorityCount));
    }

    if (absHeader.Group)
    {
        groupSizeRequired = RtlLengthRequiredSid(LW_LTOH8(absHeader.Group->SubAuthorityCount));
    }

    if (absHeader.Sacl)
    {
        saclSizeRequired = LW_LTOH16(absHeader.Sacl->AclSize);
    }

    if (absHeader.Dacl)
    {
        daclSizeRequired = LW_LTOH16(absHeader.Dacl->AclSize);
    }

    //
    // Check sizes
    //

    if ((securityDescriptorSize < securityDescriptorSizeRequired) ||
        (ownerSize < ownerSizeRequired) ||
        (groupSize < groupSizeRequired) ||
        (saclSize < saclSizeRequired) ||
        (daclSize < daclSizeRequired))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        GOTO_CLEANUP();
    }

    //
    // Now convert
    //

    if (AbsoluteSecurityDescriptor)
    {
        RtlCopyMemory(AbsoluteSecurityDescriptor, &absHeader, securityDescriptorSizeRequired);
    }

    if (Owner && absHeader.Owner)
    {
        RtlpDecodeLittleEndianSid(absHeader.Owner, Owner);
        if (AbsoluteSecurityDescriptor)
        {
            AbsoluteSecurityDescriptor->Owner = Owner;
        }
    }

    if (PrimaryGroup && absHeader.Group)
    {
        RtlpDecodeLittleEndianSid(absHeader.Group, PrimaryGroup);
        if (AbsoluteSecurityDescriptor)
        {
            AbsoluteSecurityDescriptor->Group = PrimaryGroup;
        }
    }

    if (Sacl && absHeader.Sacl)
    {
        RtlpDecodeLittleEndianAcl(absHeader.Sacl, Sacl);
        if (AbsoluteSecurityDescriptor)
        {
            AbsoluteSecurityDescriptor->Sacl = Sacl;
        }
    }

    if (Dacl && absHeader.Dacl)
    {
        RtlpDecodeLittleEndianAcl(absHeader.Dacl, Dacl);
        if (AbsoluteSecurityDescriptor)
        {
            AbsoluteSecurityDescriptor->Dacl = Dacl;
        }
    }

    status = STATUS_SUCCESS;

cleanup:
    if (AbsoluteSecurityDescriptorSize)
    {
        *AbsoluteSecurityDescriptorSize = securityDescriptorSizeRequired;
    }
    if (DaclSize)
    {
        *DaclSize = daclSizeRequired;
    }
    if (SaclSize)
    {
        *SaclSize = saclSizeRequired;
    }
    if (OwnerSize)
    {
        *OwnerSize = ownerSizeRequired;
    }
    if (PrimaryGroupSize)
    {
        *PrimaryGroupSize = groupSizeRequired;
    }

    return status;
}

NTSTATUS
RtlQuerySecurityDescriptorInfo(
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    IN OUT PULONG Length,
    IN PSECURITY_DESCRIPTOR_RELATIVE ObjectSecurityDescriptor
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDescAbs = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    ULONG SecDescAbsSize = 0;
    ULONG OwnerSize = 0;
    ULONG GroupSize = 0;
    ULONG DaclSize = 0;
    ULONG SaclSize = 0;

    /* Sanity checks */

    if (SecurityInformation == 0) {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    /* Get the necessary sizes */

    status = RtlSelfRelativeToAbsoluteSD(ObjectSecurityDescriptor,
                                         pSecDescAbs,
                                         &SecDescAbsSize,
                                         pDacl,
                                         &DaclSize,
                                         pSacl,
                                         &SaclSize,
                                         pOwnerSid,
                                         &OwnerSize,
                                         pGroupSid,
                                         &GroupSize);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (SecDescAbsSize) {
        status = RTL_ALLOCATE(&pSecDescAbs,
                              SECURITY_DESCRIPTOR_ABSOLUTE,
                              SecDescAbsSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (DaclSize) {
        status = RTL_ALLOCATE(&pDacl, ACL, DaclSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (SaclSize) {
        status = RTL_ALLOCATE(&pSacl, ACL, SaclSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (OwnerSize) {
        status = RTL_ALLOCATE(&pOwnerSid, SID,OwnerSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (GroupSize) {
        status = RTL_ALLOCATE(&pGroupSid, SID, GroupSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    /* Once for with feeling...This one should succeed. */

    status = RtlSelfRelativeToAbsoluteSD(ObjectSecurityDescriptor,
                                         pSecDescAbs,
                                         &SecDescAbsSize,
                                         pDacl,
                                         &DaclSize,
                                         pSacl,
                                         &SaclSize,
                                         pOwnerSid,
                                         &OwnerSize,
                                         pGroupSid,
                                         &GroupSize);
    GOTO_CLEANUP_ON_STATUS(status);

    if (!(SecurityInformation & OWNER_SECURITY_INFORMATION)) {
        status = RtlSetOwnerSecurityDescriptor(pSecDescAbs, NULL, FALSE);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (!(SecurityInformation & GROUP_SECURITY_INFORMATION)) {
        status = RtlSetGroupSecurityDescriptor(pSecDescAbs, NULL, FALSE);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (!(SecurityInformation & DACL_SECURITY_INFORMATION)) {
        status = RtlSetDaclSecurityDescriptor(pSecDescAbs, FALSE, NULL, FALSE);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (!(SecurityInformation & SACL_SECURITY_INFORMATION)) {
        status = RtlSetSaclSecurityDescriptor(pSecDescAbs, FALSE, NULL, FALSE);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    /* Convert back to relative form */

    status = RtlAbsoluteToSelfRelativeSD(pSecDescAbs,
                                         SecurityDescriptor,
                                         Length);
    GOTO_CLEANUP_ON_STATUS(status);


cleanup:
    RTL_FREE(&pOwnerSid);
    RTL_FREE(&pGroupSid);
    RTL_FREE(&pDacl);
    RTL_FREE(&pSacl);
    RTL_FREE(&pSecDescAbs);

    return status;
}

NTSTATUS
RtlSetSecurityDescriptorInfo(
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR_RELATIVE InputSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR_RELATIVE ObjectSecurityDescriptor,
    OUT OPTIONAL PSECURITY_DESCRIPTOR_RELATIVE NewObjectSecurityDescriptor,
    IN OUT PULONG NewObjectSecurityDescriptorLength,
    IN PGENERIC_MAPPING GenericMapping
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSECURITY_DESCRIPTOR_ABSOLUTE pObjSecDescAbs = NULL;
    PSID pObjOwnerSid = NULL;
    PSID pObjGroupSid = NULL;
    PACL pObjDacl = NULL;
    PACL pObjSacl = NULL;
    ULONG ObjSecDescAbsSize = 0;
    ULONG ObjOwnerSize = 0;
    ULONG ObjGroupSize = 0;
    ULONG ObjDaclSize = 0;
    ULONG ObjSaclSize = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pIncSecDescAbs = NULL;
    PSID pIncOwnerSid = NULL;
    PSID pIncGroupSid = NULL;
    PACL pIncDacl = NULL;
    PACL pIncSacl = NULL;
    ULONG IncSecDescAbsSize = 0;
    ULONG IncOwnerSize = 0;
    ULONG IncGroupSize = 0;
    ULONG IncDaclSize = 0;
    ULONG IncSaclSize = 0;
    SECURITY_DESCRIPTOR_CONTROL Control = 0;
    UCHAR Revision = 0;

    /* Sanity checks */

    if (SecurityInformation == 0) {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    /* Object SD: Get the necessary sizes */

    status = RtlSelfRelativeToAbsoluteSD(ObjectSecurityDescriptor,
                                         pObjSecDescAbs,
                                         &ObjSecDescAbsSize,
                                         pObjDacl,
                                         &ObjDaclSize,
                                         pObjSacl,
                                         &ObjSaclSize,
                                         pObjOwnerSid,
                                         &ObjOwnerSize,
                                         pObjGroupSid,
                                         &ObjGroupSize);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (ObjSecDescAbsSize) {
        status = RTL_ALLOCATE(&pObjSecDescAbs,
                              SECURITY_DESCRIPTOR_ABSOLUTE,
                              ObjSecDescAbsSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (ObjDaclSize) {
        status = RTL_ALLOCATE(&pObjDacl, ACL, ObjDaclSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (ObjSaclSize) {
        status = RTL_ALLOCATE(&pObjSacl, ACL, ObjSaclSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (ObjOwnerSize) {
        status = RTL_ALLOCATE(&pObjOwnerSid, SID, ObjOwnerSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (ObjGroupSize) {
        status = RTL_ALLOCATE(&pObjGroupSid, SID, ObjGroupSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RtlSelfRelativeToAbsoluteSD(ObjectSecurityDescriptor,
                                         pObjSecDescAbs,
                                         &ObjSecDescAbsSize,
                                         pObjDacl,
                                         &ObjDaclSize,
                                         pObjSacl,
                                         &ObjSaclSize,
                                         pObjOwnerSid,
                                         &ObjOwnerSize,
                                         pObjGroupSid,
                                         &ObjGroupSize);
    GOTO_CLEANUP_ON_STATUS(status);

    /* Incoming SD: Get the necessary sizes */

    status = RtlSelfRelativeToAbsoluteSD(InputSecurityDescriptor,
                                         pIncSecDescAbs,
                                         &IncSecDescAbsSize,
                                         pIncDacl,
                                         &IncDaclSize,
                                         pIncSacl,
                                         &IncSaclSize,
                                         pIncOwnerSid,
                                         &IncOwnerSize,
                                         pIncGroupSid,
                                         &IncGroupSize);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (IncSecDescAbsSize) {
        status = RTL_ALLOCATE(&pIncSecDescAbs,
                              SECURITY_DESCRIPTOR_ABSOLUTE,
                              IncSecDescAbsSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (IncDaclSize) {
        status = RTL_ALLOCATE(&pIncDacl, ACL, IncDaclSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (IncSaclSize) {
        status = RTL_ALLOCATE(&pIncSacl, ACL, IncSaclSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (IncOwnerSize) {
        status = RTL_ALLOCATE(&pIncOwnerSid, SID, IncOwnerSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (IncGroupSize) {
        status = RTL_ALLOCATE(&pIncGroupSid, SID, IncGroupSize);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = RtlSelfRelativeToAbsoluteSD(InputSecurityDescriptor,
                                         pIncSecDescAbs,
                                         &IncSecDescAbsSize,
                                         pIncDacl,
                                         &IncDaclSize,
                                         pIncSacl,
                                         &IncSaclSize,
                                         pIncOwnerSid,
                                         &IncOwnerSize,
                                         pIncGroupSid,
                                         &IncGroupSize);
    GOTO_CLEANUP_ON_STATUS(status);

    /* Merge */

    status = RtlGetSecurityDescriptorControl(pIncSecDescAbs, &Control, &Revision);
    GOTO_CLEANUP_ON_STATUS(status);

    if (SecurityInformation & OWNER_SECURITY_INFORMATION) {
        BOOLEAN bOwnerDefaulted = Control & SE_OWNER_DEFAULTED;

        status = RtlSetOwnerSecurityDescriptor(pObjSecDescAbs,
                                               pIncOwnerSid,
                                               bOwnerDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (SecurityInformation & GROUP_SECURITY_INFORMATION) {
        BOOLEAN bGroupDefaulted = Control & SE_GROUP_DEFAULTED;

        status = RtlSetGroupSecurityDescriptor(pObjSecDescAbs,
                                               pIncGroupSid,
                                               bGroupDefaulted);

        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION) {
        BOOLEAN bDaclDefaulted = Control & SE_DACL_DEFAULTED;

        status = RtlSetDaclSecurityDescriptor(pObjSecDescAbs,
                                              TRUE,
                                              pIncDacl,
                                              bDaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION) {
        BOOLEAN bSaclDefaulted = Control & SE_SACL_DEFAULTED;

        status = RtlSetSaclSecurityDescriptor(pObjSecDescAbs,
                                              TRUE,
                                              pIncSacl,
                                              bSaclDefaulted);
        GOTO_CLEANUP_ON_STATUS(status);
    }

    /* Convert back to relative form */

    status = RtlAbsoluteToSelfRelativeSD(pObjSecDescAbs,
                                         NewObjectSecurityDescriptor,
                                         NewObjectSecurityDescriptorLength);
    GOTO_CLEANUP_ON_STATUS(status);


cleanup:
    RTL_FREE(&pObjOwnerSid);
    RTL_FREE(&pObjGroupSid);
    RTL_FREE(&pObjDacl);
    RTL_FREE(&pObjSacl);
    RTL_FREE(&pObjSecDescAbs);

    RTL_FREE(&pIncOwnerSid);
    RTL_FREE(&pIncGroupSid);
    RTL_FREE(&pIncDacl);
    RTL_FREE(&pIncSacl);
    RTL_FREE(&pIncSecDescAbs);

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

