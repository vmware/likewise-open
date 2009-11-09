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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        net_userinfo.h
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Client Interface
 *
 *        Samr rpc client private definitions
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#ifndef _NETUSERINFO_H_
#define _NETUSERINFO_H_

union user_info_id {
    DWORD  dwId;
    PSID   pSid;
};


typedef struct _USER_INFO_X
{
    PWSTR   name;
    PWSTR   password;
    DWORD   password_age;
    DWORD   priv;
    PWSTR   home_dir;
    PWSTR   comment;
    DWORD   flags;
    PWSTR   script_path;
    DWORD   auth_flags;
    PWSTR   full_name;
    PWSTR   usr_comment;
    PWSTR   parms;
    PWSTR   workstations;
    DWORD   last_logon;
    DWORD   last_logoff;
    NtTime  acct_expires;
    DWORD   max_storage;
    DWORD   units_per_week;
    USHORT *logon_hours;
    DWORD   bad_pw_count;
    DWORD   num_logons;
    PWSTR   logon_server;
    DWORD   country_code;
    DWORD   code_page;
    union user_info_id user;
    PSID    user_sid;
    DWORD   primary_group_id;
    PWSTR   profile;
    PWSTR   home_dir_drive;
    DWORD   password_expired;

} USER_INFO_X, *PUSER_INFO_X;


typedef struct _USER_INFO_1X {
    PWSTR   name;
    PWSTR   comment;
    PWSTR   usr_comment;
    PWSTR   full_name;
    DWORD   priv;
    DWORD   auth_flags;
    DWORD   password_age;
    PWSTR   home_dir;
    PWSTR   parms;
    DWORD   last_logon;
    DWORD   last_logoff;
    DWORD   bad_pw_count;
    DWORD   num_logons;
    PWSTR   logon_server;
    DWORD   country_code;
    PWSTR   workstations;
    DWORD   max_storage;
    DWORD   units_per_week;
    USHORT *logon_hours;
    DWORD   code_page;
} USER_INFO_1X, *PUSER_INFO_1X;


typedef struct _USER_INFO_2X {
    PWSTR   name;
    PWSTR   full_name;
    PWSTR   comment;
    union user_info_id user;
} USER_INFO_2X, *PUSER_INFO_2X;


DWORD
NetAllocateUserInfo(
    PVOID   pInfoBuffer,
    PDWORD  pdwSpaceLeft,
    DWORD   dwLevel,
    PVOID   pSource,
    PDWORD  pdwSize
    );


DWORD
NetAllocateSamrUserInfo(
    PVOID    pInfoBuffer,
    PDWORD   pdwSamrLevel,
    PDWORD   pdwSpaceLeft,
    DWORD    dwLevel,
    PVOID    pSource,
    NetConn *pConn,
    PDWORD   pdwSize
    );


NTSTATUS
PushUserInfoAdd(
    UserInfo **ppSamrUserInfo,
    PDWORD     pdwSamrInfoLevel,
    PVOID      pNetUserInfo,
    DWORD      dwNetInfoLevel,
    PDWORD     pdwParmErr
    );


NTSTATUS
NetEncPasswordEx(
    BYTE     PasswordBuffer[532],
    PWSTR    pwszPassword,
    DWORD    dwPasswordLen,
    NetConn *pConn
    );


NTSTATUS
PushUserInfo0(
    UserInfo     **ppSamrUserInfo,
    PDWORD         pdwLevel,
    PUSER_INFO_0   pNetUserInfo
    );

NTSTATUS
PushUserInfo20(
    UserInfo      **ppSamrUserInfo,
    PDWORD          pdwLevel,
    PUSER_INFO_20   pNetUserInfo
    );

NTSTATUS
PushUserInfo1003(
    UserInfo        **ppSamrUserInfo,
    PDWORD            pdwLevel,
    PUSER_INFO_1003   pNetUserInfo,
    NetConn          *pCconn
    );

NTSTATUS
PushUserInfo1007(
    UserInfo        **ppSamrUserInfo,
    PDWORD            pdwLevel,
    PUSER_INFO_1007   pNetUserInfo
    );

NTSTATUS
PushUserInfo1008(
    UserInfo       **ppSamrUserInfo,
    PDWORD           pdwLevel,
    PUSER_INFO_1008  pNetUserInfo
    );

NTSTATUS
PushUserInfo1011(
    UserInfo       **ppSamrUserInfo,
    PDWORD           pdwLevel,
    PUSER_INFO_1011  pNetUserInfo
    );

#endif /* _NETUSERINFO_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
