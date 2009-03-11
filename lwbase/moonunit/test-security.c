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

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
