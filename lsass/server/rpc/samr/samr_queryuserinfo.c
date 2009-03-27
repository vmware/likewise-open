/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        samr_queryuserinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrQueryUserInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
SamrFillUserInfo1(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo2(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo3(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


NTSTATUS
SamrSrvQueryUserInfo(
    /* [in] */ handle_t hBinding,
    /* [in] */ ACCOUNT_HANDLE hUser,
    /* [in] */ uint16 level,
    /* [out] */ UserInfo **info
    )
{
    CHAR szFilterFmt[] = "(&(objectclass=user)(uid=%d))";
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PACCOUNT_CONTEXT pAccCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PWSTR wszAttributes[] = { NULL } ;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    UserInfo *pUserInfo = NULL;

    pAccCtx = (PACCOUNT_CONTEXT)hUser;

    if (pAccCtx == NULL || pAccCtx->Type == SamrContextAccount) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    pDomCtx  = pAccCtx->pDomCtx;
    pConnCtx = pDomCtx->pConnCtx;

    pwszBase = pDomCtx->pwszDn;

    dwFilterLen = sizeof(szFilterFmt) + 10;
    status = SamrSrvAllocateMemory((void**)&pwszFilter,
                                   dwFilterLen * sizeof(WCHAR),
                                   NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    sw16printf(pwszFilter, szFilterFmt, pAccCtx->dwRid);

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              wszAttributes,
                              FALSE,
                              &pEntry,
                              &dwEntriesNum);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwEntriesNum == 0) {
        status = STATUS_INVALID_HANDLE;

    } else if (dwEntriesNum > 1) {
        status = STATUS_INTERNAL_ERROR;
    }

    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAllocateMemory((void**)&pUserInfo,
                                   sizeof(*pUserInfo),
                                   pAccCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    switch (level) {
    case 1:
        status = SamrFillUserInfo1(pEntry, pUserInfo);
        break;

    case 2:
        status = SamrFillUserInfo2(pEntry, pUserInfo);
        break;

    case 3:
        status = SamrFillUserInfo3(pEntry, pUserInfo);
        break;

    default:
        status = STATUS_INVALID_INFO_CLASS;
    }

    BAIL_ON_NTSTATUS_ERROR(status);

    *info = pUserInfo;

cleanup:
    if (pwszFilter) {
        SamrSrvFreeMemory(pwszFilter);
    }

    return status;

error:
    if (pUserInfo) {
        SamrSrvFreeMemory(pUserInfo);
    }

    *info = NULL;
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo1(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    WCHAR wszEmptyString[] = { (WCHAR)'\0' };
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo1 *pInfo1 = NULL;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszFullName = NULL;
    DWORD dwPrimaryGid = 0;

    pInfo1 = &(pInfo->info1);

    /* account_name */
    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_USER_NAME,
                                                DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    status = InitUnicodeString(&pInfo1->account_name, pwszUsername);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo1->account_name.string, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* full name */
    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_USER_FULLNAME,
                                                DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                &pwszFullName);
    BAIL_ON_LSA_ERROR(dwError);

    status = InitUnicodeString(&pInfo1->full_name, pwszFullName);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo1->full_name.string, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* primary_gid */
    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_USER_PRIMARY_GROUP_DN,
                                                DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                                &dwPrimaryGid);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo1->primary_gid = dwPrimaryGid;

    /* description */
    status = InitUnicodeString(&pInfo1->description, wszEmptyString);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo1->description.string, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* comment */
    status = InitUnicodeString(&pInfo1->comment, wszEmptyString);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo1->comment.string, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo2(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    WCHAR wszEmptyString[] = { (WCHAR)'\0' };
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo2 *pInfo2 = NULL;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    PWSTR pwszComment = NULL;
    DWORD dwCountryCode = 0;
    DWORD dwCodePage = 0;

    pInfo2 = &(pInfo->info2);

    /* comment */
    status = InitUnicodeString(&pInfo2->comment, wszEmptyString);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo2->comment.string, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* unknown1 */
    status = InitUnicodeString(&pInfo2->unknown1, wszEmptyString);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo2->unknown1.string, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* country_code */
    pInfo2->country_code = 0;

    /* code_page */
    pInfo2->code_page = 0;

cleanup:
    return status;

error:
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo3(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    WCHAR wszEmptyString[] = { (WCHAR)'\0' };
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo3 *pInfo3 = NULL;
    PDIRECTORY_ATTRIBUTE pAttr = NULL;
    PATTRIBUTE_VALUE pAttrVal = NULL;
    PWSTR pwszUsername = NULL;
    PWSTR pwszFullName = NULL;
    DWORD dwRid = 0;
    DWORD dwPrimaryGid = 0;
    PWSTR pwszHomeDirectory = NULL;
    PWSTR pwszHomeDrive = NULL;
    PWSTR pwszLogonScript = NULL;
    PWSTR pwszProfilePath = NULL;
    PWSTR pwszWorkstations = NULL;
    NtTime tLastLogon = 0;
    NtTime tLastLogoff = 0;
    NtTime tLastPassChange = 0;
    NtTime tAllowPassChange = 0;
    NtTime tForcePassChange = 0;
    PWSTR pwszComment = NULL;
    DWORD dwBadPassCount = 0;
    DWORD dwLogonCount = 0;
    DWORD dwAcctFlags = 0;

    pInfo3 = &(pInfo->info3);

    /* account_name */
    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_USER_NAME,
                                                DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    status = InitUnicodeString(&pInfo3->account_name, pwszUsername);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo3->account_name.string, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* full_name */
    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_USER_FULLNAME,
                                                DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    status = InitUnicodeString(&pInfo3->full_name, pwszFullName);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo3->full_name.string, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* rid */
    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_UID,
                                                DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                                &dwRid);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo3->rid = dwRid;


    /* primary_gid */
    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_USER_PRIMARY_GROUP_DN,
                                                DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
                                                &dwPrimaryGid);
    BAIL_ON_LSA_ERROR(dwError);

    pInfo3->primary_gid = dwPrimaryGid;


    /* home_directory */
    dwError = DirectoryGetEntryAttrValueByNameA(pEntry,
                                                DIRECTORY_ATTR_TAG_HOMEDIR,
                                                DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                                &pwszHomeDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    status = InitUnicodeString(&pInfo3->home_directory, pwszHomeDirectory);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo3->home_directory.string, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* home_drive */
    status = InitUnicodeString(&pInfo3->home_drive, wszEmptyString);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo3->home_drive.string, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* logon_script */
    status = InitUnicodeString(&pInfo3->logon_script, wszEmptyString);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo3->logon_script.string, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* profile_path */
    status = InitUnicodeString(&pInfo3->profile_path, wszEmptyString);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo3->profile_path.string, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* workstations */
    status = InitUnicodeString(&pInfo3->workstations, wszEmptyString);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = SamrSrvAddDepMemory(pInfo3->workstations.string, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* last_logon */
    /* last_logoff */
    /* last_password_change */
    /* allow_password_change */
    /* force_password_change */
    /* logon_hours */
    /* bad_password_count */
    /* logon_count */

    /* account_flags */
    pInfo3->account_flags = ACB_NORMAL;

cleanup:
    return status;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
