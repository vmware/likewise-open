/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
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
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 *          Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"


DWORD
GetLengthSid(
    IN PSID pSid
    )
{
    return SidGetSize(pSid);
}


DWORD
GetSidLengthRequired(
    IN UINT8 SubAuthorityCount
    )
{
    return SidGetRequiredSize(SubAuthorityCount);
}


size_t
SidGetRequiredSize(
    IN UINT8 SubAuthorityCount
    )
{
    return (LW_FIELD_OFFSET(SID, SubAuthority) +
            LW_FIELD_SIZE(SID, SubAuthority[0]) * SubAuthorityCount);
}


size_t
SidGetSize(
    IN PSID pSid
    )
{
    if (pSid == NULL) return 0;

    return SidGetRequiredSize(pSid->SubAuthorityCount);
}


UINT8
SidGetSubAuthorityCount(
    IN PSID pSid
    )
{
    if (pSid == NULL) return 0;

    return pSid->SubAuthorityCount;
}

DWORD
SidGetSubAuthority(
    IN PSID pSid,
    IN UINT8 SubAuthorityIndex
    )
{
    UINT8 i = SubAuthorityIndex;
    if (pSid == NULL) return 0;

    return (i < pSid->SubAuthorityCount) ? pSid->SubAuthority[i] : 0;
}

void
SidFree(
    IN OUT PSID pSid
    )
{
    RtlMemoryFree(pSid);
}

UINT8*
GetSidSubAuthorityCount(
    IN PSID pSid
    )
{
    if (pSid == NULL) return NULL;

    return &pSid->SubAuthorityCount;
}


BOOL
AllocateAndInitializeSid(
    IN PSID_IDENTIFIER_AUTHORITY pAuthority,
    IN UINT8 SubAuthorityCount,
    IN DWORD dwSubAuthority0,
    IN DWORD dwSubAuthority1,
    IN DWORD dwSubAuthority2,
    IN DWORD dwSubAuthority3,
    IN DWORD dwSubAuthority4,
    IN DWORD dwSubAuthority5,
    IN DWORD dwSubAuthority6,
    IN DWORD dwSubAuthority7,
    OUT PSID* ppSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSID pSid = NULL;
    SID_IDENTIFIER_AUTHORITY Auth;

    BAIL_ON_NULL_PTR_PARAM(pAuthority);
    BAIL_ON_NULL_PTR_PARAM(ppSid);

    Auth = *pAuthority;

    if (SubAuthorityCount == 1) {
        status = RtlSidAllocateAndInitialize(&pSid, Auth, SubAuthorityCount,
                                             dwSubAuthority0);
    } else if (SubAuthorityCount == 2) {
        status = RtlSidAllocateAndInitialize(&pSid, Auth, SubAuthorityCount,
                                             dwSubAuthority0,
                                             dwSubAuthority1);
    } else if (SubAuthorityCount == 3) {
        status = RtlSidAllocateAndInitialize(&pSid, Auth, SubAuthorityCount,
                                             dwSubAuthority0,
                                             dwSubAuthority1,
                                             dwSubAuthority2);
    } else if (SubAuthorityCount == 4) {
        status = RtlSidAllocateAndInitialize(&pSid, Auth, SubAuthorityCount,
                                             dwSubAuthority0,
                                             dwSubAuthority1,
                                             dwSubAuthority2,
                                             dwSubAuthority3);
    } else if (SubAuthorityCount == 5) {
        status = RtlSidAllocateAndInitialize(&pSid, Auth, SubAuthorityCount,
                                             dwSubAuthority0,
                                             dwSubAuthority1,
                                             dwSubAuthority2,
                                             dwSubAuthority3,
                                             dwSubAuthority4);
    } else if (SubAuthorityCount == 6) {
        status = RtlSidAllocateAndInitialize(&pSid, Auth, SubAuthorityCount,
                                             dwSubAuthority0,
                                             dwSubAuthority1,
                                             dwSubAuthority2,
                                             dwSubAuthority3,
                                             dwSubAuthority4,
                                             dwSubAuthority5);
    } else if (SubAuthorityCount == 7) {
        status = RtlSidAllocateAndInitialize(&pSid, Auth, SubAuthorityCount,
                                             dwSubAuthority0,
                                             dwSubAuthority1,
                                             dwSubAuthority2,
                                             dwSubAuthority3,
                                             dwSubAuthority4,
                                             dwSubAuthority5,
                                             dwSubAuthority6);
    } else if (SubAuthorityCount == 8) {
        status = RtlSidAllocateAndInitialize(&pSid, Auth, SubAuthorityCount,
                                             dwSubAuthority0,
                                             dwSubAuthority1,
                                             dwSubAuthority2,
                                             dwSubAuthority3,
                                             dwSubAuthority4,
                                             dwSubAuthority5,
                                             dwSubAuthority6,
                                             dwSubAuthority7);
    } else {
        status = STATUS_INVALID_PARAMETER;
        goto error;
    }

    *ppSid = pSid;

cleanup:
    return status;

error:
    if (pSid) {
        SidFree(pSid);
    }

    pSid = NULL;
    goto cleanup;
}


void
SidCopy(
    OUT PSID pDstSid,
    IN PSID pSrcSid
    )
{
    size_t SidSize = 0;
    if (pDstSid == NULL || pSrcSid == NULL) return;

    SidSize = SidGetSize(pSrcSid);
    memcpy(pDstSid, pSrcSid, SidSize);
}


#ifdef SID_TEST
#undefine SID_TEST
#endif

#define SID_TEST(test)              \
    if ((test)) {                   \
        valid = FALSE;              \
        goto done;                  \
    }


BOOL
IsValidSid(
    IN PSID pSid
    )
{
    SID_IDENTIFIER_AUTHORITY nt_authid = { SECURITY_NT_AUTHORITY };
    SID_IDENTIFIER_AUTHORITY creator_authid = { SECURITY_CREATOR_SID_AUTHORITY };
    int i = 0;
    BOOL valid = TRUE;

    SID_TEST(pSid == NULL);

    /* check revision number */
    SID_TEST(pSid->Revision != SID_REVISION);

    /* check security authority id */
    SID_TEST(memcmp(&pSid->IdentifierAuthority, &nt_authid, sizeof(pSid->IdentifierAuthority)) &&
             memcmp(&pSid->IdentifierAuthority, &creator_authid, sizeof(pSid->IdentifierAuthority)));

    /* check subauthority ids */
    SID_TEST(pSid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES)

    for (i = 0; i < pSid->SubAuthorityCount; i++) {
        SID_TEST(pSid->SubAuthority[i] == 0);
    }

done:
    return valid;
}


#ifdef SID_CMP
#undefine SID_CMP
#endif

#define SID_CMP(test)     \
    if (!(test)) {        \
        equal = FALSE;    \
        goto done;        \
    }

BOOL
IsEqualSid(
    IN PSID pS1,
    IN PSID pS2
    )
{
    BOOL equal = TRUE;
    UINT8 i = 0;

    if (pS1 == pS2) {
        goto done;
    }

    /* at this point both pointers have to be non-null */
    SID_CMP(pS1 != NULL);
    SID_CMP(pS2 != NULL);

    /* revision number */
    SID_CMP(pS1->Revision == pS2->Revision);

    /* autority id */
    SID_CMP(memcmp(&pS1->IdentifierAuthority, &pS2->IdentifierAuthority, sizeof(pS1->IdentifierAuthority)) == 0);

    /* subauth id count */
    SID_CMP(pS1->SubAuthorityCount == pS2->SubAuthorityCount);

    for (i = 0; i < pS1->SubAuthorityCount; i++) {
        SID_CMP(pS1->SubAuthority[i] == pS2->SubAuthority[i]);
    }

done:
    return equal;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
