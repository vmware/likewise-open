/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include <moonunit/moonunit.h>
#include <lw/base.h>

#define MU_ASSERT_STATUS_SUCCESS(status) \
    MU_ASSERT(STATUS_SUCCESS == (status))

MU_TEST(Security, 0000_SidInitialize)
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID sid = NULL;
    ULONG sidSize = 0;
    SID_IDENTIFIER_AUTHORITY ntAuthority = { SECURITY_NT_AUTHORITY };
    const UCHAR ntSubAuthorityCount = 4;

    sidSize = RtlLengthRequiredSid(ntSubAuthorityCount);
    MU_ASSERT(sidSize >= SID_MIN_SIZE);

    status = RTL_ALLOCATE(&sid, SID, sidSize);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlInitializeSid(sid, &ntAuthority, ntSubAuthorityCount);
    MU_ASSERT_STATUS_SUCCESS(status);

    MU_ASSERT(RtlValidSid(sid));
    MU_ASSERT(RtlLengthSid(sid) == sidSize);
    MU_ASSERT(RtlEqualMemory(&sid->IdentifierAuthority, &ntAuthority, sizeof(ntAuthority)));

    RTL_FREE(&sid);
}

MU_TEST(Security, 0001_SidFromCString)
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR initialSidString = "S-1-5-21-100-200-300-500";
    PSID sid = NULL;
    PSTR parsedSidString = NULL;

    status = RtlAllocateSidFromCString(&sid, initialSidString);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlAllocateCStringFromSid(&parsedSidString, sid);
    MU_ASSERT_STATUS_SUCCESS(status);

    MU_ASSERT(RtlCStringIsEqual(initialSidString, parsedSidString, FALSE));

    RTL_FREE(&parsedSidString);
    RTL_FREE(&sid);
}

MU_TEST(Security, 0001_SidFromWC16String)
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR initialSidStringA = "S-1-5-21-100-200-300-500";
    PWSTR initialSidString = NULL;
    PSID sid = NULL;
    PWSTR parsedSidString = NULL;

    status = RtlWC16StringAllocateFromCString(&initialSidString, initialSidStringA);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlAllocateSidFromWC16String(&sid, initialSidString);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlAllocateWC16StringFromSid(&parsedSidString, sid);
    MU_ASSERT_STATUS_SUCCESS(status);

    MU_ASSERT(RtlWC16StringIsEqual(initialSidString, parsedSidString, FALSE));

    RTL_FREE(&parsedSidString);
    RTL_FREE(&sid);
    RTL_FREE(&initialSidString);
}

MU_TEST(Security, 0002_SidChange)
{
    MU_SKIP("Not implemented");
    // RtlSidAllocate
    // RtlSidCopyAlloc
    // SidCopy
    // RtlSidAppendRid
}

MU_TEST(Security, 0003_AccessCheck)
{
    NTSTATUS status = STATUS_SUCCESS;
    static BYTE buffer[] = {
        0x01, 0x00, 0x04, 0x84, 0x14, 0x00, 0x00, 0x00,
        0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x4c, 0x00, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00,
        0x06, 0x6a, 0xeb, 0x18, 0x5a, 0xbb, 0xfe, 0x46, 0xa8, 0x7f, 0x47, 0x83, 0x0d, 0x07, 0x00, 0x00,
        0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00, 0x06, 0x6a, 0xeb, 0x18,
        0x5a, 0xbb, 0xfe, 0x46, 0xa8, 0x7f, 0x47, 0x83, 0x01, 0x02, 0x00, 0x00, 0x02, 0x00, 0xcc, 0x00,
        0x07, 0x00, 0x00, 0x00, 0x00, 0x03, 0x24, 0x00, 0xff, 0x01, 0x1f, 0x00, 0x01, 0x05, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00, 0x06, 0x6a, 0xeb, 0x18, 0x5a, 0xbb, 0xfe, 0x46,
        0xa8, 0x7f, 0x47, 0x83, 0xed, 0x06, 0x00, 0x00, 0x00, 0x10, 0x24, 0x00, 0xff, 0x01, 0x1f, 0x00,
        0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00, 0x06, 0x6a, 0xeb, 0x18,
        0x5a, 0xbb, 0xfe, 0x46, 0xa8, 0x7f, 0x47, 0x83, 0x0d, 0x07, 0x00, 0x00, 0x00, 0x1b, 0x24, 0x00,
        0x00, 0x00, 0x00, 0x10, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x15, 0x00, 0x00, 0x00,
        0x06, 0x6a, 0xeb, 0x18, 0x5a, 0xbb, 0xfe, 0x46, 0xa8, 0x7f, 0x47, 0x83, 0x0d, 0x07, 0x00, 0x00,
        0x00, 0x10, 0x14, 0x00, 0xff, 0x01, 0x1f, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
        0x12, 0x00, 0x00, 0x00, 0x00, 0x1b, 0x14, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x05, 0x12, 0x00, 0x00, 0x00, 0x00, 0x10, 0x18, 0x00, 0xff, 0x01, 0x1f, 0x00,
        0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x20, 0x00, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00,
        0x00, 0x1b, 0x18, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
        0x20, 0x00, 0x00, 0x00, 0x20, 0x02, 0x00, 0x00
    };
    PSID sid = NULL;
    TOKEN_USER tokenUser = { { 0 } };
    TOKEN_GROUPS tokenGroups = { 0 };
    TOKEN_OWNER tokenOwner = { 0 };
    TOKEN_PRIMARY_GROUP tokenPrimaryGroup = { 0 };
    TOKEN_DEFAULT_DACL tokenDefaultDacl = { 0 };
    PACCESS_TOKEN token = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE relativeSd = (PSECURITY_DESCRIPTOR_RELATIVE) (PBYTE) buffer;
    ULONG relativeSdLength = sizeof(buffer);
    PSECURITY_DESCRIPTOR_ABSOLUTE sd = NULL;
    PACL dacl = NULL;
    PACL sacl = NULL;
    PSID owner = NULL;
    PSID primaryGroup = NULL;
    ULONG sdSize = 0;
    ULONG daclSize = 0;
    ULONG saclSize = 0;
    ULONG ownerSize = 0;
    ULONG primaryGroupSize = 0;
    GENERIC_MAPPING mapping = {
        .GenericRead = 0,
        .GenericWrite = 0,
        .GenericExecute = 0,
        .GenericAll = 0,
    };
    ACCESS_MASK granted = 0;

    status = RtlAllocateSidFromCString(&sid, "S-1-5-21-418081286-1191099226-2202501032-1805");
    MU_ASSERT_STATUS_SUCCESS(status);

    tokenUser.User.Sid = sid;

    status = RtlCreateAccessToken(
                    &token,
                    &tokenUser,
                    &tokenGroups,
                    &tokenOwner,
                    &tokenPrimaryGroup,
                    &tokenDefaultDacl,
                    NULL);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlValidRelativeSecurityDescriptor(
                    relativeSd,
                    relativeSdLength,
                    0);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlSelfRelativeToAbsoluteSD(
                    relativeSd,
                    NULL,
                    &sdSize,
                    NULL,
                    &daclSize,
                    NULL,
                    &saclSize,
                    NULL,
                    &ownerSize,
                    NULL,
                    &primaryGroupSize);
    MU_ASSERT(STATUS_BUFFER_TOO_SMALL == status);

    daclSize = LW_MAX(daclSize, 1);
    saclSize = LW_MAX(saclSize, 1);
    ownerSize = LW_MAX(ownerSize, 1);
    primaryGroupSize = LW_MAX(primaryGroupSize, 1);

    status = RTL_ALLOCATE(&sd, VOID, sdSize);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RTL_ALLOCATE(&dacl, VOID, daclSize);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RTL_ALLOCATE(&sacl, VOID, saclSize);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RTL_ALLOCATE(&owner, VOID, ownerSize);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RTL_ALLOCATE(&primaryGroup, VOID, primaryGroupSize);
    MU_ASSERT_STATUS_SUCCESS(status);

    status = RtlSelfRelativeToAbsoluteSD(
                    relativeSd,
                    sd,
                    &sdSize,
                    dacl,
                    &daclSize,
                    sacl,
                    &saclSize,
                    owner,
                    &ownerSize,
                    primaryGroup,
                    &primaryGroupSize);
    MU_ASSERT_STATUS_SUCCESS(status);

    RtlAccessCheck(
        sd,
        token,
        MAXIMUM_ALLOWED,
        0,
        &mapping,
        &granted,
        &status);
    MU_ASSERT(status == STATUS_SUCCESS || status == STATUS_ACCESS_DENIED);

    MU_INFO("status = 0x%08x, granted = 0x%08x", status, granted);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
