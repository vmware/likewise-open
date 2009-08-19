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

#define SET_UNICODE_STRING_VALUE(attr, field)                           \
    {                                                                   \
        WCHAR wszEmpty[] = { '\0' };                                    \
        PWSTR pwszValue = NULL;                                         \
        dwError = DirectoryGetEntryAttrValueByName(                     \
                          pEntry,                                       \
                          (attr),                                       \
                          DIRECTORY_ATTR_TYPE_UNICODE_STRING,           \
                          &pwszValue);                                  \
        BAIL_ON_LSA_ERROR(dwError);                                     \
                                                                        \
        status = SamrSrvInitUnicodeString(                              \
                          &(field),                                     \
                          (pwszValue) ? pwszValue : wszEmpty);          \
        BAIL_ON_NTSTATUS_ERROR(status);                                 \
    }

#define SET_NTTIME_VALUE(attr, field)                                   \
    {                                                                   \
        LONG64 llValue = 0;                                             \
        dwError = DirectoryGetEntryAttrValueByName(                     \
                          pEntry,                                       \
                          (attr),                                       \
                          DIRECTORY_ATTR_TYPE_LARGE_INTEGER,            \
                          &llValue);                                    \
        BAIL_ON_LSA_ERROR(dwError);                                     \
                                                                        \
        field = (NtTime)llValue;                                        \
    }

#define SET_RID_VALUE(attr, field)                                      \
    {                                                                   \
        PWSTR pwszSid = NULL;                                           \
        PSID pSid = NULL;                                               \
        dwError = DirectoryGetEntryAttrValueByName(                     \
                          pEntry,                                       \
                          (attr),                                       \
                          DIRECTORY_ATTR_TYPE_UNICODE_STRING,           \
                          &pwszSid);                                    \
        BAIL_ON_LSA_ERROR(dwError);                                     \
                                                                        \
        status = RtlAllocateSidFromWC16String(                          \
                          &pSid,                                        \
                          pwszSid);                                     \
        BAIL_ON_NTSTATUS_ERROR(status);                                 \
                                                                        \
        field = pSid->SubAuthority[pSid->SubAuthorityCount - 1];        \
                                                                        \
        RTL_FREE(&pSid);                                                \
    }

#define SET_UINT32_VALUE(attr, field)                                   \
    {                                                                   \
        ULONG ulValue = 0;                                              \
        dwError = DirectoryGetEntryAttrValueByName(                     \
                          pEntry,                                       \
                          (attr),                                       \
                          DIRECTORY_ATTR_TYPE_INTEGER,                  \
                          &ulValue);                                    \
        BAIL_ON_LSA_ERROR(dwError);                                     \
                                                                        \
        field = (uint32)ulValue;                                        \
    }

#define SET_UINT16_VALUE(attr, field)                                   \
    {                                                                   \
        ULONG ulValue = 0;                                              \
        dwError = DirectoryGetEntryAttrValueByName(                     \
                          pEntry,                                       \
                          (attr),                                       \
                          DIRECTORY_ATTR_TYPE_INTEGER,                  \
                          &ulValue);                                    \
        BAIL_ON_LSA_ERROR(dwError);                                     \
                                                                        \
        field = (uint16)ulValue;                                        \
    }


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


static
NTSTATUS
SamrFillUserInfo21(
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
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    WCHAR wszAttrPrimaryGroup[] = DS_ATTR_PRIMARY_GROUP;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    WCHAR wszAttrComment[] = DS_ATTR_COMMENT;
    WCHAR wszAttrCountryCode[] = DS_ATTR_COUNTRY_CODE;
    WCHAR wszAttrCodePage[] = DS_ATTR_CODE_PAGE;
    WCHAR wszAttrHomeDirectory[] = DS_ATTR_HOME_DIR;
    WCHAR wszAttrHomeDrive[] = DS_ATTR_HOME_DRIVE;
    WCHAR wszAttrLogonScript[] = DS_ATTR_LOGON_SCRIPT;
    WCHAR wszAttrProfilePath[] = DS_ATTR_PROFILE_PATH;
    WCHAR wszAttrWorkstations[]= DS_ATTR_WORKSTATIONS;
    WCHAR wszAttrParameters[] = DS_ATTR_PARAMETERS;
    WCHAR wszAttrLastLogon[] = DS_ATTR_LAST_LOGON;
    WCHAR wszAttrLastLogoff[] = DS_ATTR_LAST_LOGOFF;
    WCHAR wszAttrPasswordLastSet[] = DS_ATTR_PASSWORD_LAST_SET;
    WCHAR wszAttrAllowPasswordChange[] = DS_ATTR_ALLOW_PASSWORD_CHANGE;
    WCHAR wszAttrForcePasswordChange[] = DS_ATTR_FORCE_PASSWORD_CHANGE;
    WCHAR wszAttrLogonHours[] = DS_ATTR_LOGON_HOURS;
    WCHAR wszAttrBadPasswordCount[] = DS_ATTR_BAD_PASSWORD_COUNT;
    WCHAR wszAttrLogonCount[] = DS_ATTR_LOGON_COUNT;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrAccountExpiry[] = DS_ATTR_ACCOUNT_EXPIRY;
    WCHAR wszAttrLmHash[] = DS_ATTR_LM_HASH;
    WCHAR wszAttrNtHash[] = DS_ATTR_NT_HASH;
    DWORD dwScope = 0;
    PWSTR pwszFilter = NULL;
    DWORD dwFilterLen = 0;
    PDIRECTORY_ENTRY pEntry = NULL;
    DWORD dwEntriesNum = 0;
    UserInfo *pUserInfo = NULL;

    PWSTR wszAttributesLevel1[] = {
        wszAttrSamAccountName,
        wszAttrFullName,
        wszAttrPrimaryGroup,
        wszAttrDescription,
        wszAttrComment,
        NULL
    };

    PWSTR wszAttributesLevel2[] = {
        wszAttrComment,
        wszAttrCountryCode,
        wszAttrCodePage,
        NULL
    };

    PWSTR wszAttributesLevel3[] = {
        wszAttrSamAccountName,
        wszAttrFullName,
        wszAttrObjectSid,
        wszAttrPrimaryGroup,
        wszAttrHomeDirectory,
        wszAttrHomeDrive,
        wszAttrLogonScript,
        wszAttrProfilePath,
        wszAttrWorkstations,
        wszAttrLastLogon,
        wszAttrLastLogoff,
        wszAttrPasswordLastSet,
        wszAttrAllowPasswordChange,
        wszAttrForcePasswordChange,
        wszAttrLogonHours,
        wszAttrBadPasswordCount,
        wszAttrLogonCount,
        wszAttrAccountFlags,
        NULL
    };

    PWSTR wszAttributesLevel4[] = {
        wszAttrLogonHours,
        NULL
    };

    PWSTR wszAttributesLevel5[] = {
        wszAttrSamAccountName,
        wszAttrFullName,
        wszAttrObjectSid,
        wszAttrPrimaryGroup,
        wszAttrHomeDirectory,
        wszAttrHomeDrive,
        wszAttrLogonScript,
        wszAttrProfilePath,
        wszAttrDescription,
        wszAttrWorkstations,
        wszAttrLastLogon,
        wszAttrLastLogoff,
        wszAttrLogonHours,
        wszAttrBadPasswordCount,
        wszAttrLogonCount,
        wszAttrPasswordLastSet,
        wszAttrAccountExpiry,
        wszAttrAccountFlags,
        NULL
    };

    PWSTR wszAttributesLevel6[] = {
        wszAttrSamAccountName,
        wszAttrFullName,
        NULL
    };

    PWSTR wszAttributesLevel7[] = {
        wszAttrSamAccountName,
        NULL
    };

    PWSTR wszAttributesLevel8[] = {
        wszAttrFullName,
        NULL
    };

    PWSTR wszAttributesLevel9[] = {
        wszAttrPrimaryGroup,
        NULL
    };

    PWSTR wszAttributesLevel10[] = {
        wszAttrHomeDirectory,
        wszAttrHomeDrive,
        NULL
    };

    PWSTR wszAttributesLevel11[] = {
        wszAttrLogonScript,
        NULL
    };

    PWSTR wszAttributesLevel12[] = {
        wszAttrProfilePath,
        NULL
    };

    PWSTR wszAttributesLevel13[] = {
        wszAttrDescription,
        NULL
    };

    PWSTR wszAttributesLevel14[] = {
        wszAttrWorkstations,
        NULL
    };

    PWSTR wszAttributesLevel16[] = {
        wszAttrAccountFlags,
        NULL
    };

    PWSTR wszAttributesLevel17[] = {
        wszAttrAccountExpiry,
        NULL
    };

    PWSTR wszAttributesLevel20[] = {
        wszAttrParameters,
        NULL
    };

    PWSTR wszAttributesLevel21[] = {
        wszAttrLastLogon,
        wszAttrLastLogoff,
        wszAttrPasswordLastSet,
        wszAttrAccountExpiry,
        wszAttrAllowPasswordChange,
        wszAttrForcePasswordChange,
        wszAttrSamAccountName,
        wszAttrFullName,
        wszAttrHomeDirectory,
        wszAttrHomeDrive,
        wszAttrLogonScript,
        wszAttrProfilePath,
        wszAttrDescription,
        wszAttrWorkstations,
        wszAttrComment,
        wszAttrParameters,
        wszAttrObjectSid,
        wszAttrPrimaryGroup,
        wszAttrAccountFlags,
        wszAttrLogonHours,
        wszAttrBadPasswordCount,
        wszAttrLogonCount,
        wszAttrCountryCode,
        wszAttrCodePage,
        wszAttrLmHash,
        wszAttrNtHash,
        NULL
    };

    PWSTR *pwszAttributes[] = {
        wszAttributesLevel1,
        wszAttributesLevel2,
        wszAttributesLevel3,
        wszAttributesLevel4,
        wszAttributesLevel5,
        wszAttributesLevel6,
        wszAttributesLevel7,
        wszAttributesLevel8,
        wszAttributesLevel9,
        wszAttributesLevel10,
        wszAttributesLevel11,
        wszAttributesLevel12,
        wszAttributesLevel13,
        wszAttributesLevel14,
        NULL,
        wszAttributesLevel16,
        wszAttributesLevel17,
        NULL,
        NULL,
        wszAttributesLevel20,
        wszAttributesLevel21
    };

    pAcctCtx = (PACCOUNT_CONTEXT)hUser;

    if (pAcctCtx == NULL || pAcctCtx->Type != SamrContextAccount) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    switch (level) {
    case 15:
    case 18:
    case 19:
        status = STATUS_INVALID_INFO_CLASS;

    default:
        status = STATUS_SUCCESS;
    }
    BAIL_ON_NTSTATUS_ERROR(status);

    pDomCtx  = pAcctCtx->pDomCtx;
    pConnCtx = pDomCtx->pConnCtx;

    pwszBase = pDomCtx->pwszDn;

    dwFilterLen = ((sizeof(wszAttrDn)/sizeof(WCHAR)) - 1) +
                  wc16slen(pAcctCtx->pwszDn) +
                  (sizeof(wszFilterFmt)/sizeof(wszFilterFmt[0]));

    status = SamrSrvAllocateMemory((void**)&pwszFilter,
                                   dwFilterLen * sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(status);

    sw16printfw(pwszFilter, dwFilterLen, wszFilterFmt,
                wszAttrDn, pAcctCtx->pwszDn);

    dwError = DirectorySearch(pConnCtx->hDirectory,
                              pwszBase,
                              dwScope,
                              pwszFilter,
                              pwszAttributes[level - 1],
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
                                   sizeof(*pUserInfo));
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

    case 21:
        status = SamrFillUserInfo21(pEntry, pUserInfo);
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

    status = SamrSrvInitUnicodeString(&pInfo1->account_name, pwszUsername);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* full name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrFullName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszFullName);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo1->full_name, pwszFullName);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* primary_gid */
    pInfo1->primary_gid = dwPrimaryGid;

    /* description */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrDescription,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszDescription);
    BAIL_ON_LSA_ERROR(dwError);
    status = SamrSrvInitUnicodeString(&pInfo1->description, pwszDescription);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* comment */
    status = SamrSrvInitUnicodeString(&pInfo1->comment, pwszComment);
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
    status = SamrSrvInitUnicodeString(&pInfo2->comment, pwszComment);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* unknown1 */
    status = SamrSrvInitUnicodeString(&pInfo2->unknown1, pwszUnknown1);
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

    status = SamrSrvInitUnicodeString(&pInfo3->account_name, pwszUsername);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* full_name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrFullName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszFullName);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo3->full_name, pwszFullName);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* rid */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrObjectSid,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszSid);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvAllocateSidFromWC16String(&pSid, pwszSid);
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

    status = SamrSrvInitUnicodeString(&pInfo3->home_directory, pwszHomeDirectory);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* home_drive */
    status = SamrSrvInitUnicodeString(&pInfo3->home_drive, pwszHomeDrive);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* logon_script */
    status = SamrSrvInitUnicodeString(&pInfo3->logon_script, pwszLogonScript);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* profile_path */
    status = SamrSrvInitUnicodeString(&pInfo3->profile_path, pwszProfilePath);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* workstations */
    status = SamrSrvInitUnicodeString(&pInfo3->workstations, pwszWorkstations);
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

    status = SamrSrvInitUnicodeString(&pInfo5->account_name, pwszUsername);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* full_name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrFullName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszFullName);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo5->full_name, pwszFullName);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* rid */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrObjectSid,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszSid);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvAllocateSidFromWC16String(&pSid, pwszSid);
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

    status = SamrSrvInitUnicodeString(&pInfo5->home_directory, pwszHomeDirectory);
    BAIL_ON_NTSTATUS_ERROR(status);


    /* home_drive */
    status = SamrSrvInitUnicodeString(&pInfo5->home_drive, pwszHomeDrive);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* logon_script */
    status = SamrSrvInitUnicodeString(&pInfo5->logon_script, pwszLogonScript);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* profile_path */
    status = SamrSrvInitUnicodeString(&pInfo5->profile_path, pwszProfilePath);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* description */
    status = SamrSrvInitUnicodeString(&pInfo5->description, pwszDescription);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* workstations */
    status = SamrSrvInitUnicodeString(&pInfo5->workstations, pwszWorkstations);
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

    status = SamrSrvInitUnicodeString(&pInfo6->account_name, pwszUsername);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* full_name */
    dwError = DirectoryGetEntryAttrValueByName(pEntry,
                                               wszAttrFullName,
                                               DIRECTORY_ATTR_TYPE_UNICODE_STRING,
                                               &pwszFullName);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrSrvInitUnicodeString(&pInfo6->full_name, pwszFullName);
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

    status = SamrSrvInitUnicodeString(&pInfo7->account_name, pwszUsername);
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

    status = SamrSrvInitUnicodeString(&pInfo8->full_name, pwszFullName);
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

    status = SamrSrvInitUnicodeString(&pInfo10->home_directory, pwszHomeDirectory);
    BAIL_ON_NTSTATUS_ERROR(status);

    /* home_drive */
    status = SamrSrvInitUnicodeString(&pInfo10->home_drive, pwszHomeDrive);
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
    status = SamrSrvInitUnicodeString(&pInfo11->logon_script, pwszLogonScript);
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
    status = SamrSrvInitUnicodeString(&pInfo12->profile_path, pwszProfilePath);
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
    status = SamrSrvInitUnicodeString(&pInfo13->description, pwszDescription);
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
    status = SamrSrvInitUnicodeString(&pInfo14->workstations, pwszWorkstations);
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
    status = SamrSrvInitUnicodeString(&pInfo20->parameters, pwszParameters);
    BAIL_ON_NTSTATUS_ERROR(status);

cleanup:
    return status;

error:
    memset(pInfo20, 0, sizeof(*pInfo20));
    goto cleanup;
}


static
NTSTATUS
SamrFillUserInfo21(
    PDIRECTORY_ENTRY pEntry,
    UserInfo *pInfo
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    UserInfo21 *pInfo21 = NULL;
    WCHAR wszAttrLastLogon[] = DS_ATTR_LAST_LOGON;
    WCHAR wszAttrLastLogoff[] = DS_ATTR_LAST_LOGOFF;
    WCHAR wszAttrLastPasswordChange[] = DS_ATTR_PASSWORD_LAST_SET;
    WCHAR wszAttrAccountExpiry[] = DS_ATTR_ACCOUNT_EXPIRY;
    WCHAR wszAttrAllowPasswordChange[] = DS_ATTR_ALLOW_PASSWORD_CHANGE;
    WCHAR wszAttrForcePasswordChange[] = DS_ATTR_FORCE_PASSWORD_CHANGE;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrFullName[] = DS_ATTR_FULL_NAME;
    WCHAR wszAttrHomeDirectory[] = DS_ATTR_HOME_DIR;
    WCHAR wszAttrHomeDrive[] = DS_ATTR_HOME_DRIVE;
    WCHAR wszAttrLogonScript[] = DS_ATTR_LOGON_SCRIPT;
    WCHAR wszAttrProfilePath[] = DS_ATTR_PROFILE_PATH;
    WCHAR wszAttrDescription[] = DS_ATTR_DESCRIPTION;
    WCHAR wszAttrWorkstations[] = DS_ATTR_WORKSTATIONS;
    WCHAR wszAttrComment[] = DS_ATTR_COMMENT;
    WCHAR wszAttrParameters[] = DS_ATTR_PARAMETERS;
    WCHAR wszAttrObjectSid[] = DS_ATTR_OBJECT_SID;
    WCHAR wszAttrPrimaryGid[] = DS_ATTR_PRIMARY_GROUP;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrBadPasswordCount[] = DS_ATTR_BAD_PASSWORD_COUNT;
    WCHAR wszAttrLogonCount[] = DS_ATTR_LOGON_COUNT;
    WCHAR wszAttrCountryCode[] = DS_ATTR_COUNTRY_CODE;
    WCHAR wszAttrCodePage[] = DS_ATTR_CODE_PAGE;

    pInfo21 = &(pInfo->info21);

    SET_NTTIME_VALUE(wszAttrLastLogon, pInfo21->last_logon);
    pInfo21->fields_present |= SAMR_FIELD_LAST_LOGON;

    SET_NTTIME_VALUE(wszAttrLastLogoff, pInfo21->last_logoff);
    pInfo21->fields_present |= SAMR_FIELD_LAST_LOGOFF;

    SET_NTTIME_VALUE(wszAttrLastPasswordChange, pInfo21->last_password_change);
    pInfo21->fields_present |= SAMR_FIELD_LAST_PWD_CHANGE;

    SET_NTTIME_VALUE(wszAttrAccountExpiry, pInfo21->account_expiry);
    pInfo21->fields_present |= SAMR_FIELD_ACCT_EXPIRY;

    SET_NTTIME_VALUE(wszAttrAllowPasswordChange, pInfo21->allow_password_change);
    pInfo21->fields_present |= SAMR_FIELD_ALLOW_PWD_CHANGE;

    SET_NTTIME_VALUE(wszAttrForcePasswordChange, pInfo21->force_password_change);
    pInfo21->fields_present |= SAMR_FIELD_FORCE_PWD_CHANGE;

    SET_UNICODE_STRING_VALUE(wszAttrSamAccountName, pInfo21->account_name);
    pInfo21->fields_present |= SAMR_FIELD_ACCOUNT_NAME;

    SET_UNICODE_STRING_VALUE(wszAttrFullName, pInfo21->full_name);
    pInfo21->fields_present |= SAMR_FIELD_FULL_NAME;

    SET_UNICODE_STRING_VALUE(wszAttrHomeDirectory, pInfo21->home_directory);
    pInfo21->fields_present |= SAMR_FIELD_HOME_DIRECTORY;

    SET_UNICODE_STRING_VALUE(wszAttrHomeDrive, pInfo21->home_drive);
    pInfo21->fields_present |= SAMR_FIELD_HOME_DRIVE;

    SET_UNICODE_STRING_VALUE(wszAttrLogonScript, pInfo21->logon_script);
    pInfo21->fields_present |= SAMR_FIELD_LOGON_SCRIPT;

    SET_UNICODE_STRING_VALUE(wszAttrProfilePath, pInfo21->profile_path);
    pInfo21->fields_present |= SAMR_FIELD_PROFILE_PATH;

    SET_UNICODE_STRING_VALUE(wszAttrDescription, pInfo21->description);
    pInfo21->fields_present |= SAMR_FIELD_DESCRIPTION;

    SET_UNICODE_STRING_VALUE(wszAttrWorkstations, pInfo21->workstations);
    pInfo21->fields_present |= SAMR_FIELD_WORKSTATIONS;

    SET_UNICODE_STRING_VALUE(wszAttrComment, pInfo21->comment);
    pInfo21->fields_present |= SAMR_FIELD_COMMENT;

    SET_UNICODE_STRING_VALUE(wszAttrParameters, pInfo21->parameters);
    pInfo21->fields_present |= SAMR_FIELD_PARAMETERS;

    /* unknown1 */
    /* unknown2 */
    /* unknown3 */

    SET_RID_VALUE(wszAttrObjectSid, pInfo21->rid);
    pInfo21->fields_present |= SAMR_FIELD_RID;

    SET_UINT32_VALUE(wszAttrPrimaryGid, pInfo21->primary_gid);
    pInfo21->fields_present |= SAMR_FIELD_PRIMARY_GID;

    SET_UINT32_VALUE(wszAttrAccountFlags, pInfo21->account_flags);
    pInfo21->fields_present |= SAMR_FIELD_ACCT_FLAGS;

    SET_UINT16_VALUE(wszAttrBadPasswordCount, pInfo21->bad_password_count);
    pInfo21->fields_present |= SAMR_FIELD_BAD_PWD_COUNT;

    SET_UINT16_VALUE(wszAttrLogonCount, pInfo21->logon_count);
    pInfo21->fields_present |= SAMR_FIELD_NUM_LOGONS;

    SET_UINT16_VALUE(wszAttrCountryCode, pInfo21->country_code);
    pInfo21->fields_present |= SAMR_FIELD_COUNTRY_CODE;

    SET_UINT16_VALUE(wszAttrCodePage, pInfo21->code_page);
    pInfo21->fields_present |= SAMR_FIELD_CODE_PAGE;

cleanup:
    return status;

error:
    memset(pInfo21, 0, sizeof(*pInfo21));
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
