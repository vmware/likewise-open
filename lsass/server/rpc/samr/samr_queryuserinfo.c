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


static
NTSTATUS
SamrFillUserInfo4(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo5(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo6(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo7(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo8(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo9(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo10(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo11(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo12(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo13(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo14(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo16(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo17(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    );


static
NTSTATUS
SamrFillUserInfo20(
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
    wchar_t wszFilterFmt[] = L"%ws='%ws'";
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PWSTR pwszBase = NULL;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PWSTR wszAttributes[] = { NULL } ;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    UserInfo *pUserInfo = NULL;

    pAcctCtx = (PACCOUNT_CONTEXT)hUser;

    if (pAcctCtx == NULL || pAcctCtx->Type != SamrContextAccount) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    pDomCtx  = pAcctCtx->pDomCtx;
    pConnCtx = pDomCtx->pConnCtx;

    pwszBase = pDomCtx->pwszDn;

    dwFilterLen = ((sizeof(wszAttrDn)/sizeof(WCHAR)) - 1) +
                  wc16slen(pAcctCtx->pwszDn) +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    status = SamrSrvAllocateMemory((void**)&pwszFilter,
                                   dwFilterLen * sizeof(WCHAR),
                                   pAcctCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                wszAttrDn, pAcctCtx->pwszDn);

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
                                   pAcctCtx);
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

    case 4:
        status = SamrFillUserInfo4(pEntry, pUserInfo);
        break;

    case 5:
        status = SamrFillUserInfo5(pEntry, pUserInfo);
        break;

    case 6:
        status = SamrFillUserInfo6(pEntry, pUserInfo);
        break;

    case 7:
        status = SamrFillUserInfo7(pEntry, pUserInfo);
        break;

    case 8:
        status = SamrFillUserInfo8(pEntry, pUserInfo);
        break;

    case 9:
        status = SamrFillUserInfo9(pEntry, pUserInfo);
        break;

    case 10:
        status = SamrFillUserInfo10(pEntry, pUserInfo);
        break;

    case 11:
        status = SamrFillUserInfo11(pEntry, pUserInfo);
        break;

    case 12:
        status = SamrFillUserInfo12(pEntry, pUserInfo);
        break;

    case 13:
        status = SamrFillUserInfo13(pEntry, pUserInfo);
        break;

    case 14:
        status = SamrFillUserInfo14(pEntry, pUserInfo);
        break;

    case 16:
        status = SamrFillUserInfo16(pEntry, pUserInfo);
        break;

    case 17:
        status = SamrFillUserInfo17(pEntry, pUserInfo);
        break;

    case 20:
        status = SamrFillUserInfo20(pEntry, pUserInfo);
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

    if (pEntry) {
        DirectoryFreeEntries(pEntry, dwEntriesNum);
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
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo1 *pInfo1 = NULL;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    PWSTR pwszUsername = NULL;
    PWSTR pwszFullName = NULL;
    DWORD dwPrimaryGid = 0;
    PWSTR pwszDescription = NULL;
    PWSTR pwszComment = NULL;

    pInfo1 = &(pInfo->info1);

    /* account_name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo1->account_name, pwszUsername,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* full name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrFullName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszFullName);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo1->full_name, pwszFullName,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* primary_gid */
    pInfo1->primary_gid = dwPrimaryGid;

    /* description */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDescription,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDescription);
    BAIL_ON_LSA_ERROR(dwError);
    status = SamrSrvInitUnicodeString(&pInfo1->description, pwszDescription,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* comment */
    status = SamrSrvInitUnicodeString(&pInfo1->comment, pwszComment, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


cleanup:
    return status;

error:
    memset(pInfo1, 0, sizeof(*pInfo1));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo2(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UserInfo2 *pInfo2 = NULL;
    PWSTR pwszComment = NULL;
    PWSTR pwszUnknown1 = NULL;

    pInfo2 = &(pInfo->info2);

    /* comment */
    status = SamrSrvInitUnicodeString(&pInfo2->comment, pwszComment, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* unknown1 */
    status = SamrSrvInitUnicodeString(&pInfo2->unknown1, pwszUnknown1, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* country_code */
    pInfo2->country_code = 0;

    /* code_page */
    pInfo2->code_page = 0;

cleanup:
    return status;

error:
    memset(pInfo2, 0, sizeof(*pInfo2));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo3(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo3 *pInfo3 = NULL;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    WCHAR wszAttrHomeDirectory[] = DS_ATTR_HOME_DIR;
    PWSTR pwszUsername = NULL;
    PWSTR pwszFullName = NULL;
    PWSTR pwszSid = NULL;
    PSID pSid = NULL;
    DWORD dwPrimaryGid = 0;
    PWSTR pwszHomeDirectory = NULL;
    PWSTR pwszHomeDrive = NULL;
    PWSTR pwszLogonScript = NULL;
    PWSTR pwszProfilePath = NULL;
    PWSTR pwszWorkstations = NULL;

    pInfo3 = &(pInfo->info3);

    /* account_name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo3->account_name, pwszUsername,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* full_name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrFullName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszFullName);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo3->full_name, pwszFullName,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* rid */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrObjectSid,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszSid);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvAllocateSidFromWC16String(&pSid, pwszSid, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    pInfo3->rid = pSid->SubAuthority[pSid->SubAuthorityCount - 1];


    /* primary_gid */
    pInfo3->primary_gid = dwPrimaryGid;

    /* home_directory */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrHomeDirectory,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszHomeDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo3->home_directory, pwszHomeDirectory,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* home_drive */
    status = SamrSrvInitUnicodeString(&pInfo3->home_drive, pwszHomeDrive,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* logon_script */
    status = SamrSrvInitUnicodeString(&pInfo3->logon_script, pwszLogonScript,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* profile_path */
    status = SamrSrvInitUnicodeString(&pInfo3->profile_path, pwszProfilePath,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* workstations */
    status = SamrSrvInitUnicodeString(&pInfo3->workstations, pwszWorkstations,
                                      pInfo);
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
    if (pSid) {
        SamrSrvFreeMemory(pSid);
    }

    return status;

error:
    memset(pInfo3, 0, sizeof(*pInfo3));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo4(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UserInfo4 *pInfo4 = NULL;

    pInfo4 = &(pInfo->info4);

    pInfo4->logon_hours.units_per_week = 0;
    pInfo4->logon_hours.units          = NULL;

    return status;
}


static
NTSTATUS
SamrFillUserInfo5(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo5 *pInfo5 = NULL;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    WCHAR wszAttrHomeDirectory[] = DS_ATTR_HOME_DIR;
    PWSTR pwszUsername = NULL;
    PWSTR pwszFullName = NULL;
    PWSTR pwszSid = NULL;
    PSID pSid = NULL;
    DWORD dwPrimaryGid = 0;
    PWSTR pwszHomeDirectory = NULL;
    PWSTR pwszHomeDrive = NULL;
    PWSTR pwszLogonScript = NULL;
    PWSTR pwszProfilePath = NULL;
    PWSTR pwszWorkstations = NULL;
    PWSTR pwszDescription = NULL;

    pInfo5 = &(pInfo->info5);

    /* account_name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo5->account_name, pwszUsername,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* full_name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrFullName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszFullName);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo5->full_name, pwszFullName,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* rid */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrObjectSid,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszSid);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvAllocateSidFromWC16String(&pSid, pwszSid, pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    pInfo5->rid = pSid->SubAuthority[pSid->SubAuthorityCount - 1];


    /* primary_gid */
    pInfo5->primary_gid = dwPrimaryGid;


    /* home_directory */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrHomeDirectory,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszHomeDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo5->home_directory, pwszHomeDirectory,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* home_drive */
    status = SamrSrvInitUnicodeString(&pInfo5->home_drive, pwszHomeDrive,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* logon_script */
    status = SamrSrvInitUnicodeString(&pInfo5->logon_script, pwszLogonScript,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* profile_path */
    status = SamrSrvInitUnicodeString(&pInfo5->profile_path, pwszProfilePath,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* description */
    status = SamrSrvInitUnicodeString(&pInfo5->description, pwszDescription,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* workstations */
    status = SamrSrvInitUnicodeString(&pInfo5->workstations, pwszWorkstations,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* last_logon */
    /* last_logoff */

    /* logon hours */
    pInfo5->logon_hours.units_per_week = 0;
    pInfo5->logon_hours.units          = NULL;

    /* bad_password_count */
    /* logon_count */
    /* last_password_change */
    /* account_expiry */

    /* account_flags */
    pInfo5->account_flags = ACB_NORMAL;

cleanup:
    if (pSid) {
        SamrSrvFreeMemory(pSid);
    }

    return status;

error:
    memset(pInfo5, 0, sizeof(*pInfo5));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo6(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo6 *pInfo6 = NULL;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    PWSTR pwszUsername = NULL;
    PWSTR pwszFullName = NULL;

    pInfo6 = &(pInfo->info6);

    /* account_name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo6->account_name, pwszUsername,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* full_name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrFullName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszFullName);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo6->full_name, pwszFullName,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    memset(pInfo6, 0, sizeof(*pInfo6));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo7(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo7 *pInfo7 = NULL;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    PWSTR pwszUsername = NULL;

    pInfo7 = &(pInfo->info7);

    /* account_name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrSamAccountName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo7->account_name, pwszUsername,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    memset(pInfo7, 0, sizeof(*pInfo7));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo8(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo8 *pInfo8 = NULL;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    PWSTR pwszFullName = NULL;

    pInfo8 = &(pInfo->info8);

    /* full_name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrFullName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszFullName);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo8->full_name, pwszFullName,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    memset(pInfo8, 0, sizeof(*pInfo8));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo9(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UserInfo9 *pInfo9 = NULL;
    DWORD dwPrimaryGid = 0;

    pInfo9 = &(pInfo->info9);

    /* primary_gid */
    pInfo9->primary_gid = dwPrimaryGid;

    return status;
}


static
NTSTATUS
SamrFillUserInfo10(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo10 *pInfo10 = NULL;
    WCHAR wszAttrHomeDirectory[] = DS_ATTR_HOME_DIR;
    PWSTR pwszHomeDirectory = NULL;
    PWSTR pwszHomeDrive = NULL;

    pInfo10 = &(pInfo->info10);

    /* home_directory */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrHomeDirectory,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszHomeDirectory);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo10->home_directory, pwszHomeDirectory,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* home_drive */
    status = SamrSrvInitUnicodeString(&pInfo10->home_drive, pwszHomeDrive,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    memset(pInfo10, 0, sizeof(*pInfo10));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo11(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UserInfo11 *pInfo11 = NULL;
    PWSTR pwszLogonScript = NULL;

    pInfo11 = &(pInfo->info11);

    /* logon_script */
    status = SamrSrvInitUnicodeString(&pInfo11->logon_script, pwszLogonScript,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    memset(pInfo11, 0, sizeof(*pInfo11));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo12(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UserInfo12 *pInfo12 = NULL;
    PWSTR pwszProfilePath = NULL;

    pInfo12 = &(pInfo->info12);

    /* profile_path */
    status = SamrSrvInitUnicodeString(&pInfo12->profile_path, pwszProfilePath,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    memset(pInfo12, 0, sizeof(*pInfo12));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo13(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UserInfo13 *pInfo13 = NULL;
    PWSTR pwszDescription = NULL;

    pInfo13 = &(pInfo->info13);

    /* description */
    status = SamrSrvInitUnicodeString(&pInfo13->description, pwszDescription,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    memset(pInfo13, 0, sizeof(*pInfo13));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo14(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UserInfo14 *pInfo14 = NULL;
    PWSTR pwszWorkstations = NULL;

    pInfo14 = &(pInfo->info14);

    /* workstations */
    status = SamrSrvInitUnicodeString(&pInfo14->workstations, pwszWorkstations,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    memset(pInfo14, 0, sizeof(*pInfo14));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo16(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UserInfo16 *pInfo16 = NULL;

    pInfo16 = &(pInfo->info16);

    /* account_flags */
    pInfo16->account_flags |= ACB_NORMAL;

    return status;
}


static
NTSTATUS
SamrFillUserInfo17(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UserInfo17 *pInfo17 = NULL;

    pInfo17 = &(pInfo->info17);

    /* account_expiry */
    pInfo17->account_expiry = 0;

    return status;
}


static
NTSTATUS
SamrFillUserInfo20(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UserInfo20 *pInfo20 = NULL;
    PWSTR pwszParameters = NULL;

    pInfo20 = &(pInfo->info20);

    /* parameters */
    status = SamrSrvInitUnicodeString(&pInfo20->parameters, pwszParameters,
                                      pInfo);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    memset(pInfo20, 0, sizeof(*pInfo20));
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
