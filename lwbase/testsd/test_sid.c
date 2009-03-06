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

#include <stdio.h>

#include <config.h>
#include <lw/base.h>
#include <secdesc/secapi.h>

#include "params.h"
#include "test.h"


#define SECURITY_NT_AUTHORITY          ((SID_IDENTIFIER_AUTHORITY) {{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x5 }})
#define SECURITY_CREATOR_SID_AUTHORITY ((SID_IDENTIFIER_AUTHORITY) {{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x3 }})


int TestSidInitialize(struct test *t,
                      struct parameter *options, int optcount);
int TestSidAllocateAndInit(struct test *t,
                           struct parameter *options, int optcount);
int TestSidCopyAlloc(struct test *t,
                     struct parameter *options, int optcount);
int TestSidCopy(struct test *t,
                struct parameter *options, int optcount);
int TestSidString(struct test *t,
                  struct parameter *options, int optcount);
int TestSidAppendRid(struct test *t,
                     struct parameter *options, int optcount);



int TestSidInitialize(struct test *t,
                      struct parameter *options, int optcount)
{
    const int def_subauth_count = 4;
    const char *def_authid_name = "nt";

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    unsigned int subauth_count = 0;
    char *authid_name = NULL;
    PSID pSid = NULL;
    size_t sid_size = 0;
    SID_IDENTIFIER_AUTHORITY authid;

    TESTINFO(t);

    perr = fetch_value(options, optcount, "subauth_count", pt_uint32,
                       &subauth_count, &def_subauth_count);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "authid", pt_string, &authid_name,
                       &def_authid_name);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO_START;
    PARAM_INFO("subauth_count", pt_uint32, subauth_count);
    PARAM_INFO("authid", pt_string, authid_name);
    PARAM_INFO_END;

    if (strcmp(authid_name, "nt") == 0) {
        authid = SECURITY_NT_AUTHORITY;

    } else if (strcmp(authid_name, "creator") == 0) {
        authid = SECURITY_CREATOR_SID_AUTHORITY;

    } else {
        status = STATUS_INVALID_PARAMETER;
        goto done;
    }

    sid_size = SidGetRequiredSize((UINT8)subauth_count);

    pSid = (PSID) malloc(sid_size);
    BAIL_ON_NULL_PTR(pSid);

    status = RtlSidInitialize(pSid, &authid, subauth_count);
    if (status != STATUS_SUCCESS) goto done;

    memset(&pSid->SubAuthority, 0, sizeof(pSid->SubAuthority[0]) * subauth_count);

    if (IsValidSid(pSid)) {
        status = STATUS_INVALID_SID;
        goto done;
    }

done:
    SAFE_FREE(pSid);

    SAFE_FREE(authid_name);

    return (status == STATUS_SUCCESS);
}


int TestSidAllocateAndInit(struct test *t,
                           struct parameter *options, int optcount)
{
    const char *def_subauth = "[21:1:2:3:4]";
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    PSID pSid = NULL;
    unsigned int *subauth = NULL;
    DWORD dwSubAuthCount = 0;

    TESTINFO(t);

    perr = fetch_value(options, optcount, "subauth", pt_uint32_list,
                       &subauth, &def_subauth);
    if (!perr_is_ok(perr)) perr_fail(perr);

    while (subauth[dwSubAuthCount]) dwSubAuthCount++;

    if (dwSubAuthCount == 1) {
        status = RtlSidAllocateAndInitialize(&pSid,
                                             SECURITY_NT_AUTHORITY,
                                             dwSubAuthCount,
                                             subauth[0]);
    } else if (dwSubAuthCount == 2) {
        status = RtlSidAllocateAndInitialize(&pSid,
                                             SECURITY_NT_AUTHORITY,
                                             dwSubAuthCount,
                                             subauth[0],
                                             subauth[1]);
    } else if (dwSubAuthCount == 3) {
        status = RtlSidAllocateAndInitialize(&pSid,
                                             SECURITY_NT_AUTHORITY,
                                             dwSubAuthCount,
                                             subauth[0],
                                             subauth[1],
                                             subauth[2]);
    } else if (dwSubAuthCount == 4) {
        status = RtlSidAllocateAndInitialize(&pSid,
                                             SECURITY_NT_AUTHORITY,
                                             dwSubAuthCount,
                                             subauth[0],
                                             subauth[1],
                                             subauth[2],
                                             subauth[3]);
    } else if (dwSubAuthCount == 5) {
        status = RtlSidAllocateAndInitialize(&pSid,
                                             SECURITY_NT_AUTHORITY,
                                             dwSubAuthCount,
                                             subauth[0],
                                             subauth[1],
                                             subauth[2],
                                             subauth[3],
                                             subauth[4]);
    } else {
        /* At the moment we test only sids with 1-5 subauth ids.
           It's not a problem to raise the number anytime. */
        status = STATUS_NOT_SUPPORTED;
        goto done;
    }

    if (status != STATUS_SUCCESS) goto done; 

    if (!IsValidSid(pSid)) {
        status = STATUS_INVALID_SID;
        goto done;
    }

done:
    if (pSid) {
        SidFree(pSid);
    }

    if (subauth) {
        free(subauth);
    }

    return (status == STATUS_SUCCESS);
}


int TestSidCopyAlloc(struct test *t,
                     struct parameter *options, int optcount)
{
    const char *def_sid = "S-1-5-32-512";

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    PSID pDstSid = NULL;
    PSID pSrcSid = NULL;

    TESTINFO(t);

    perr = fetch_value(options, optcount, "sid", pt_sid,
                       &pSrcSid, &def_sid);
    if (!perr_is_ok(perr)) perr_fail(perr);

    status = RtlSidCopyAlloc(&pDstSid, pSrcSid);
    if (status != STATUS_SUCCESS) goto done;

    if (!IsEqualSid(pSrcSid, pDstSid)) {
        status = STATUS_UNSUCCESSFUL;
    }

done:
    if (pSrcSid) {
        SidFree(pSrcSid);
    }

    if (pDstSid) {
        SidFree(pDstSid);
    }

    return (status == STATUS_SUCCESS);
}


int TestSidCopy(struct test *t,
                struct parameter *options, int optcount)
{
    const char *def_sid = "S-1-5-32-512";

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    PSID pDstSid = NULL;
    PSID pSrcSid = NULL;
    UINT8 SubAuthCount = 0;

    TESTINFO(t);

    perr = fetch_value(options, optcount, "sid", pt_sid,
                       &pSrcSid, &def_sid);
    if (!perr_is_ok(perr)) perr_fail(perr);

    SubAuthCount = SidGetSubAuthorityCount(pSrcSid);
    status = RtlSidAllocate(&pDstSid, SubAuthCount);
    if (status != STATUS_SUCCESS) goto done;

    SidCopy(pDstSid, pSrcSid);

    if (!IsEqualSid(pSrcSid, pDstSid)) {
        status = STATUS_UNSUCCESSFUL;
    }

done:
    if (pSrcSid) {
        SidFree(pSrcSid);
    }

    if (pDstSid) {
        SidFree(pDstSid);
    }

    return (status == STATUS_SUCCESS);
}


int TestSidString(struct test *t,
                  struct parameter *options, int optcount)
{
    const char *def_sidstr = "S-1-5-32-512";

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    PSTR pszSidStr = NULL;
    PSTR pszSidStrOut = NULL;
    PWSTR pwszSidStr = NULL;
    PWSTR pwszSidStrOut = NULL;
    PSID pSid1 = NULL;
    PSID pSid2 = NULL;

    perr = fetch_value(options, optcount, "sidstr", pt_string,
                       &pszSidStr, &def_sidstr);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "sidstrw", pt_w16string,
                       &pwszSidStr, &def_sidstr);
    if (!perr_is_ok(perr)) perr_fail(perr);

    if (pszSidStr) {
        status = ParseSidStringA(&pSid1, pszSidStr);
        if (status != STATUS_SUCCESS) goto done;

        if (!IsValidSid(pSid1)) {
            status = STATUS_INVALID_SID;
            goto done;
        }

        status = SidToStringA(pSid1, &pszSidStrOut);
        if (status != STATUS_SUCCESS) goto done;

        if (strcmp(pszSidStr, pszSidStrOut) != 0) {
            status = STATUS_UNSUCCESSFUL;
            goto done;
        }

    }

    if (pwszSidStr) {
        status = ParseSidStringW(&pSid2, pwszSidStr);
        if (status != STATUS_SUCCESS) goto done;

        if (!IsValidSid(pSid2)) {
            status = STATUS_INVALID_SID;
            goto done;
        }

        status = SidToStringW(pSid2, &pwszSidStrOut);
        if (status != STATUS_SUCCESS) goto done;

        if (wc16scasecmp(pwszSidStr, pwszSidStrOut) != 0) {
            status = STATUS_UNSUCCESSFUL;
            goto done;
        }
    }

done:
    if (pszSidStr) {
        free(pszSidStr);
    }

    if (pwszSidStr) {
        free(pwszSidStr);
    }

    if (pSid1) {
        SidFree(pSid1);
    }

    if (pSid2) {
        SidFree(pSid2);
    }

    if (pszSidStrOut) {
        RtlMemoryFree((void*)pszSidStrOut);
    }

    if (pwszSidStrOut) {
        RtlMemoryFree((void*)pwszSidStrOut);
    }

    return (status == STATUS_SUCCESS);
}


int TestSidAppendRid(struct test *t,
                     struct parameter *options, int optcount)
{
    const char *def_sidstr = "S-1-5-32-512";
    const int def_rid = 513;

    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    PSID pSid = NULL;
    PSID pSidApp = NULL;
    DWORD dwRid = 0;

    perr = fetch_value(options, optcount, "sid", pt_sid,
                       &pSid, &def_sidstr);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "rid", pt_uint32,
                       &dwRid, &def_rid);
    if (!perr_is_ok(perr)) perr_fail(perr);

    status = RtlSidAppendRid(&pSidApp, dwRid, pSid);
    if (status != STATUS_SUCCESS) goto done;

    if (!IsValidSid(pSidApp)) {
        status = STATUS_INVALID_SID;
        goto done;
    }

    if (pSidApp->SubAuthority[pSidApp->SubAuthorityCount - 1] != dwRid) {
        status = STATUS_UNSUCCESSFUL;
    }

done:
    if (pSid) {
        SidFree(pSid);
    }

    if (pSidApp) {
        SidFree(pSidApp);
    }

    return (status == STATUS_SUCCESS);
}


void SetupSidTests(struct test *t)
{
    AddTest(t, "SID-INITIALIZE", TestSidInitialize);
    AddTest(t, "SID-ALLOC-INIT", TestSidAllocateAndInit);
    AddTest(t, "SID-COPY-ALLOC", TestSidCopyAlloc);
    AddTest(t, "SID-COPY", TestSidCopy);
    AddTest(t, "SID-STRING", TestSidString);
    AddTest(t, "SID-APPEND-RID", TestSidAppendRid);
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
