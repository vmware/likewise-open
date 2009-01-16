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


DWORD
GetLengthSid(
    SID *pSid
    )
{
    return SidGetSize(pSid);
}


DWORD
GetSidLengthRequired(
    UINT8 SubAuthorityCount
    )
{
    return SidGetRequiredSize(SubAuthorityCount);
}


size_t
SidGetRequiredSize(
    UINT8 SubAuthorityCount
    )
{
    return (CT_FIELD_OFFSET(SID, subauth) +
            CT_FIELD_SIZE(SID, subauth[0]) * SubAuthorityCount);
}


size_t
SidGetSize(
    const SID *pSid
    )
{
    if (pSid == NULL) return 0;

    return SidGetRequiredSize(pSid->subauth_count);
}


UINT8
SidGetSubAuthorityCount(
    const SID *pSid
    )
{
    if (pSid == NULL) return 0;

    return (pSid->subauth_count);
}


DWORD
SidGetSubAuthority(
    SID *pSid,
    UINT8 SubAuthorityIndex
    )
{
    UINT8 i = SubAuthorityIndex;
    if (pSid == NULL) return 0;

    return (i < pSid->subauth_count) ? pSid->subauth[i] : 0;
}


void
SidFree(
    SID *pSid
    )
{
    SdFreeMemory((void*)pSid);
}


void*
FreeSid(
    SID *pSid
    )
{
    SdFreeMemory((void*)pSid);
    return NULL;
}


UINT8*
GetSidSubAuthorityCount(
    SID *pSid
    )
{
    if (pSid == NULL) return NULL;

    return (&pSid->subauth_count);
}


BOOL
AllocateAndInitializeSid(
    SID_IDENTIFIER_AUTHORITY *pAuthority,
    UINT8 SubAuthorityCount,
    DWORD dwSubAuthority0,
    DWORD dwSubAuthority1,
    DWORD dwSubAuthority2,
    DWORD dwSubAuthority3,
    DWORD dwSubAuthority4,
    DWORD dwSubAuthority5,
    DWORD dwSubAuthority6,
    DWORD dwSubAuthority7,
    PSID *ppSid
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SID *pSid = NULL;
    SID_IDENTIFIER_AUTHORITY Auth;

    BAIL_ON_NULL_PTR_PARAM(pAuthority);
    BAIL_ON_NULL_PTR_PARAM(ppSid);

    memcpy(&Auth.Octet, pAuthority->Octet, sizeof(Auth.Octet));

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
    SID *pSidDst,
    const SID *pSidSrc
    )
{
    size_t SidSize = 0;
    if (pSidDst == NULL || pSidSrc == NULL) return;

    SidSize = SidGetSize(pSidSrc);
    memcpy(pSidDst, pSidSrc, SidSize);
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
    SID *pSid
    )
{
    SID_IDENTIFIER_AUTHORITY nt_authid = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY creator_authid = SECURITY_CREATOR_SID_AUTHORITY;
    int i = 0;
    BOOL valid = TRUE;

    SID_TEST(pSid == NULL);

    /* check revision number */
    SID_TEST(pSid->revision != 1);

    /* check security authority id */
    SID_TEST(memcmp(pSid->authid, nt_authid.Octet, sizeof(pSid->authid)) &&
             memcmp(pSid->authid, creator_authid.Octet, sizeof(pSid->authid)));

    /* check subauthority ids */
    SID_TEST(pSid->subauth_count > MAXIMUM_SUBAUTHORITY_COUNT)

    for (i = 0; i < pSid->subauth_count; i++) {
        SID_TEST(pSid->subauth[i] == 0);
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
    SID *pS1,
    SID *pS2
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
    SID_CMP(pS1->revision == pS2->revision);

    /* autority id */
    SID_CMP(memcmp(pS1->authid, pS2->authid, sizeof(pS1->authid)) == 0);

    /* subauth id count */
    SID_CMP(pS1->subauth_count == pS2->subauth_count);

    for (i = 0; i < pS1->subauth_count; i++) {
        SID_CMP(pS1->subauth[i] == pS2->subauth[i]);
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
