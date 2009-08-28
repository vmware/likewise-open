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
 *        samr_setuserinfo.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SamrSrvSetUserInfo function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


#define SET_UNICODE_STRING_VALUE(var, idx, mod)                   \
    do {                                                          \
        PWSTR pwszValue = NULL;                                   \
                                                                  \
        ntStatus = SamrSrvGetFromUnicodeString(&pwszValue,        \
                                               &(var));           \
        BAIL_ON_NTSTATUS_ERROR(ntStatus);                         \
                                                                  \
        AttrValues[(idx)].data.pwszStringValue = pwszValue;       \
        Mods[i++] = mod;                                          \
    } while (0);

#define SET_UINT32_VALUE(var, idx, mod)                           \
    do {                                                          \
        DWORD dwValue = (var);                                    \
                                                                  \
        AttrValues[(idx)].data.ulValue = dwValue;                 \
        Mods[i++] = mod;                                          \
    } while (0);

#define SET_NTTIME_VALUE(var, idx, mod)                           \
    do {                                                          \
        ULONG64 llValue = (var);                                  \
                                                                  \
        AttrValues[(idx)].data.llValue = llValue;                 \
        Mods[i++] = mod;                                          \
    } while (0);

#define TEST_ACCOUNT_FIELD_FLAG(field, flag)                      \
    if (!((field) & (flag)))                                      \
    {                                                             \
        break;                                                    \
    }

#define SET_UNICODE_STRING_VALUE_BY_FLAG(pinfo, field, flag,      \
                                         idx, mod)                \
    do {                                                          \
        TEST_ACCOUNT_FIELD_FLAG((pinfo)->info21.fields_present,   \
                                (flag));                          \
        SET_UNICODE_STRING_VALUE((pinfo)->info21.field,           \
                                 (idx), (mod));                   \
    } while (0);

#define SET_UINT32_VALUE_BY_FLAG(pinfo, field, flag, idx, mod)    \
    do {                                                          \
        TEST_ACCOUNT_FIELD_FLAG((pinfo)->info21.fields_present,   \
                                (flag));                          \
        SET_UINT32_VALUE((pinfo)->info21.field, (idx), (mod));    \
    } while (0);

#define SET_NTTIME_VALUE_BY_FLAG(pinfo, field, flag, idx, mod)    \
    do {                                                          \
        TEST_ACCOUNT_FIELD_FLAG((pinfo)->info21.fields_present,   \
                                (flag));                          \
        SET_NTTIME_VALUE((pinfo)->info21.field, (idx), (mod));  \
    } while (0);


NTSTATUS
SamrSrvSetUserInfo(
    /* [in] */ handle_t hBinding,
    /* [in] */ ACCOUNT_HANDLE hUser,
    /* [in] */ uint16 level,
    /* [in] */ UserInfo *pInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PCONNECT_CONTEXT pConnCtx = NULL;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PACCOUNT_CONTEXT pAcctCtx = NULL;
    HANDLE hDirectory = NULL;
    PWSTR pwszAccountDn = NULL;
    WCHAR wszAttrDn[] = DS_ATTR_DISTINGUISHED_NAME;
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
    WCHAR wszAttrAllowPasswordChange[] = DS_ATTR_ALLOW_PASSWORD_CHANGE;
    WCHAR wszAttrForcePasswordChange[] = DS_ATTR_FORCE_PASSWORD_CHANGE;
    WCHAR wszAttrBadPasswordCount[] = DS_ATTR_BAD_PASSWORD_COUNT;
    WCHAR wszAttrLogonCount[] = DS_ATTR_LOGON_COUNT;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrAccountExpiry[] = DS_ATTR_ACCOUNT_EXPIRY;
    WCHAR wszAttrLmHash[] = DS_ATTR_LM_HASH;
    WCHAR wszAttrNtHash[] = DS_ATTR_NT_HASH;
    DWORD i = 0;

    enum AttrValueIndex {
        ATTR_VAL_IDX_SAM_ACCOUNT_NAME = 0,
        ATTR_VAL_IDX_FULL_NAME,
        ATTR_VAL_IDX_PRIMARY_GROUP,
        ATTR_VAL_IDX_HOME_DIRECTORY,
        ATTR_VAL_IDX_HOME_DRIVE,
        ATTR_VAL_IDX_LOGON_SCRIPT,
        ATTR_VAL_IDX_PROFILE_PATH,
        ATTR_VAL_IDX_DESCRIPTION,
        ATTR_VAL_IDX_WORKSTATIONS,
        ATTR_VAL_IDX_COMMENT,
        ATTR_VAL_IDX_PARAMETERS,
        ATTR_VAL_IDX_ALLOW_PASSWORD_CHANGE,
        ATTR_VAL_IDX_FORCE_PASSWORD_CHANGE,
        ATTR_VAL_IDX_ACCOUNT_EXPIRY,
        ATTR_VAL_IDX_ACCOUNT_FLAGS,
        ATTR_VAL_IDX_BAD_PASSWORD_COUNT,
        ATTR_VAL_IDX_LOGON_COUNT,
        ATTR_VAL_IDX_COUNTRY_CODE,
        ATTR_VAL_IDX_CODE_PAGE,
        ATTR_VAL_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.ulValue = 0
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.ulValue = 0
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_LARGE_INTEGER,
            .data.ulValue = 0
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        }
    };

    DIRECTORY_MOD ModSamAccountName = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrSamAccountName,
        1,
        &AttrValues[ATTR_VAL_IDX_SAM_ACCOUNT_NAME]
    };

    DIRECTORY_MOD ModFullName = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrFullName,
        1,
        &AttrValues[ATTR_VAL_IDX_FULL_NAME]
    };

    DIRECTORY_MOD ModPrimaryGroup = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrPrimaryGroup,
        1,
        &AttrValues[ATTR_VAL_IDX_PRIMARY_GROUP]
    };

    DIRECTORY_MOD ModHomeDirectory = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrHomeDirectory,
        1,
        &AttrValues[ATTR_VAL_IDX_HOME_DIRECTORY]
    };

    DIRECTORY_MOD ModHomeDrive = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrHomeDrive,
        1,
        &AttrValues[ATTR_VAL_IDX_HOME_DRIVE]
    };

    DIRECTORY_MOD ModLogonScript = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrLogonScript,
        1,
        &AttrValues[ATTR_VAL_IDX_LOGON_SCRIPT]
    };

    DIRECTORY_MOD ModProfilePath = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrProfilePath,
        1,
        &AttrValues[ATTR_VAL_IDX_PROFILE_PATH]
    };

    DIRECTORY_MOD ModDescription = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrDescription,
        1,
        &AttrValues[ATTR_VAL_IDX_DESCRIPTION]
    };

    DIRECTORY_MOD ModWorkstations = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrWorkstations,
        1,
        &AttrValues[ATTR_VAL_IDX_WORKSTATIONS]
    };

    DIRECTORY_MOD ModComment = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrComment,
        1,
        &AttrValues[ATTR_VAL_IDX_COMMENT]
    };

    DIRECTORY_MOD ModParameters = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrParameters,
        1,
        &AttrValues[ATTR_VAL_IDX_PARAMETERS]
    };

    DIRECTORY_MOD ModAllowPasswordChange = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrAllowPasswordChange,
        1,
        &AttrValues[ATTR_VAL_IDX_ALLOW_PASSWORD_CHANGE]
    };

    DIRECTORY_MOD ModForcePasswordChange = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrForcePasswordChange,
        1,
        &AttrValues[ATTR_VAL_IDX_FORCE_PASSWORD_CHANGE]
    };

    DIRECTORY_MOD ModAccountExpiry = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrAccountExpiry,
        1,
        &AttrValues[ATTR_VAL_IDX_ACCOUNT_EXPIRY]
    };

    DIRECTORY_MOD ModAccountFlags = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrAccountFlags,
        1,
        &AttrValues[ATTR_VAL_IDX_ACCOUNT_FLAGS]
    };

    DIRECTORY_MOD ModBadPasswordCount = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrBadPasswordCount,
        1,
        &AttrValues[ATTR_VAL_IDX_BAD_PASSWORD_COUNT]
    };

    DIRECTORY_MOD ModLogonCount = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrLogonCount,
        1,
        &AttrValues[ATTR_VAL_IDX_LOGON_COUNT]
    };

    DIRECTORY_MOD ModCountryCode = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrCountryCode,
        1,
        &AttrValues[ATTR_VAL_IDX_COUNTRY_CODE]
    };

    DIRECTORY_MOD ModCodePage = {
        DIR_MOD_FLAGS_REPLACE,
        wszAttrCodePage,
        1,
        &AttrValues[ATTR_VAL_IDX_CODE_PAGE]
    };

    DIRECTORY_MOD Mods[ATTR_VAL_IDX_SENTINEL + 1] = {0};

    pAcctCtx = (PACCOUNT_CONTEXT)hUser;

    if (pAcctCtx == NULL || pAcctCtx->Type != SamrContextAccount) {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    pDomCtx  = pAcctCtx->pDomCtx;
    pConnCtx = pDomCtx->pConnCtx;

    hDirectory    = pConnCtx->hDirectory;
    pwszAccountDn = pAcctCtx->pwszDn;

    switch (level)
    {
    case 6:
        SET_UNICODE_STRING_VALUE(pInfo->info6.account_name,
                                 ATTR_VAL_IDX_SAM_ACCOUNT_NAME,
                                 ModSamAccountName);
        SET_UNICODE_STRING_VALUE(pInfo->info6.full_name,
                                 ATTR_VAL_IDX_FULL_NAME,
                                 ModFullName);
        break;

    case 7:
        SET_UNICODE_STRING_VALUE(pInfo->info7.account_name,
                                 ATTR_VAL_IDX_SAM_ACCOUNT_NAME,
                                 ModSamAccountName);
        break;

    case 8:
        SET_UNICODE_STRING_VALUE(pInfo->info8.full_name,
                                 ATTR_VAL_IDX_FULL_NAME,
                                 ModFullName);
        break;

    case 9:
        SET_UINT32_VALUE(pInfo->info9.primary_gid,
                         ATTR_VAL_IDX_PRIMARY_GROUP,
                         ModPrimaryGroup);
        break;

    case 10:
        SET_UNICODE_STRING_VALUE(pInfo->info10.home_directory,
                                 ATTR_VAL_IDX_HOME_DIRECTORY,
                                 ModHomeDirectory);
        SET_UNICODE_STRING_VALUE(pInfo->info10.home_drive,
                                 ATTR_VAL_IDX_HOME_DRIVE,
                                 ModHomeDrive);
        break;

    case 11:
        SET_UNICODE_STRING_VALUE(pInfo->info11.logon_script,
                                 ATTR_VAL_IDX_LOGON_SCRIPT,
                                 ModLogonScript);
        break;

    case 12:
        SET_UNICODE_STRING_VALUE(pInfo->info12.profile_path,
                                 ATTR_VAL_IDX_PROFILE_PATH,
                                 ModProfilePath);
        break;

    case 13:
        SET_UNICODE_STRING_VALUE(pInfo->info13.description,
                                 ATTR_VAL_IDX_DESCRIPTION,
                                 ModDescription);
        break;

    case 14:
        SET_UNICODE_STRING_VALUE(pInfo->info14.workstations,
                                 ATTR_VAL_IDX_WORKSTATIONS,
                                 ModWorkstations);
        break;

    case 16:
        SET_UINT32_VALUE(pInfo->info16.account_flags,
                         ATTR_VAL_IDX_ACCOUNT_FLAGS,
                         ModAccountFlags);
        break;

    case 17:
        SET_NTTIME_VALUE(pInfo->info17.account_expiry,
                         ATTR_VAL_IDX_ACCOUNT_EXPIRY,
                         ModAccountExpiry);
        break;

    case 20:
        SET_UNICODE_STRING_VALUE(pInfo->info20.parameters,
                                 ATTR_VAL_IDX_PARAMETERS,
                                 ModParameters);
        break;

    case 21:
        SET_NTTIME_VALUE_BY_FLAG(pInfo, account_expiry,
                                 SAMR_FIELD_ACCT_EXPIRY,
                                 ATTR_VAL_IDX_ACCOUNT_EXPIRY,
                                 ModAccountExpiry);
        SET_NTTIME_VALUE_BY_FLAG(pInfo, allow_password_change,
                                 SAMR_FIELD_ALLOW_PWD_CHANGE,
                                 ATTR_VAL_IDX_ALLOW_PASSWORD_CHANGE,
                                 ModAllowPasswordChange);
        SET_NTTIME_VALUE_BY_FLAG(pInfo, force_password_change,
                                 SAMR_FIELD_FORCE_PWD_CHANGE,
                                 ATTR_VAL_IDX_FORCE_PASSWORD_CHANGE,
                                 ModForcePasswordChange);
        SET_UNICODE_STRING_VALUE_BY_FLAG(pInfo, account_name,
                                         SAMR_FIELD_ACCOUNT_NAME,
                                         ATTR_VAL_IDX_SAM_ACCOUNT_NAME,
                                         ModSamAccountName);
        SET_UNICODE_STRING_VALUE_BY_FLAG(pInfo, full_name,
                                         SAMR_FIELD_FULL_NAME,
                                         ATTR_VAL_IDX_FULL_NAME,
                                         ModFullName);
        SET_UNICODE_STRING_VALUE_BY_FLAG(pInfo, home_directory,
                                         SAMR_FIELD_HOME_DIRECTORY,
                                         ATTR_VAL_IDX_HOME_DIRECTORY,
                                         ModHomeDirectory);
        SET_UNICODE_STRING_VALUE_BY_FLAG(pInfo, home_drive,
                                         SAMR_FIELD_HOME_DRIVE,
                                         ATTR_VAL_IDX_HOME_DRIVE,
                                         ModHomeDrive);
        SET_UNICODE_STRING_VALUE_BY_FLAG(pInfo, logon_script,
                                         SAMR_FIELD_LOGON_SCRIPT,
                                         ATTR_VAL_IDX_LOGON_SCRIPT,
                                         ModLogonScript);
        SET_UNICODE_STRING_VALUE_BY_FLAG(pInfo, profile_path,
                                         SAMR_FIELD_PROFILE_PATH,
                                         ATTR_VAL_IDX_PROFILE_PATH,
                                         ModProfilePath);
        SET_UNICODE_STRING_VALUE_BY_FLAG(pInfo, description,
                                         SAMR_FIELD_DESCRIPTION,
                                         ATTR_VAL_IDX_DESCRIPTION,
                                         ModDescription);
        SET_UNICODE_STRING_VALUE_BY_FLAG(pInfo, workstations,
                                         SAMR_FIELD_WORKSTATIONS,
                                         ATTR_VAL_IDX_WORKSTATIONS,
                                         ModWorkstations);
        SET_UNICODE_STRING_VALUE_BY_FLAG(pInfo, comment,
                                         SAMR_FIELD_COMMENT,
                                         ATTR_VAL_IDX_COMMENT,
                                         ModComment);
        SET_UNICODE_STRING_VALUE_BY_FLAG(pInfo, parameters,
                                         SAMR_FIELD_PARAMETERS,
                                         ATTR_VAL_IDX_PARAMETERS,
                                         ModLogonScript);
        SET_UINT32_VALUE_BY_FLAG(pInfo, primary_gid,
                                 SAMR_FIELD_PRIMARY_GID,
                                 ATTR_VAL_IDX_PRIMARY_GROUP,
                                 ModPrimaryGroup);
        SET_UINT32_VALUE_BY_FLAG(pInfo, account_flags,
                                 SAMR_FIELD_ACCT_FLAGS,
                                 ATTR_VAL_IDX_ACCOUNT_FLAGS,
                                 ModAccountFlags);
        SET_UINT32_VALUE_BY_FLAG(pInfo, bad_password_count,
                                 SAMR_FIELD_BAD_PWD_COUNT,
                                 ATTR_VAL_IDX_BAD_PASSWORD_COUNT,
                                 ModBadPasswordCount);
        SET_UINT32_VALUE_BY_FLAG(pInfo, country_code,
                                 SAMR_FIELD_COUNTRY_CODE,
                                 ATTR_VAL_IDX_COUNTRY_CODE,
                                 ModCountryCode);
        SET_UINT32_VALUE_BY_FLAG(pInfo, code_page,
                                 SAMR_FIELD_CODE_PAGE,
                                 ATTR_VAL_IDX_CODE_PAGE,
                                 ModCodePage);
        break;

    default:
        ntStatus = STATUS_INVALID_INFO_CLASS;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwError = DirectoryModifyObject(hDirectory,
                                    pwszAccountDn,
                                    Mods);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    for (i = ATTR_VAL_IDX_SAM_ACCOUNT_NAME;
         i < ATTR_VAL_IDX_SENTINEL;
         i++)
    {
        if (AttrValues[i].Type == DIRECTORY_ATTR_TYPE_UNICODE_STRING)
        {
            LW_SAFE_FREE_MEMORY(AttrValues[i].data.pwszStringValue);
        }
    }

    return ntStatus;

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
