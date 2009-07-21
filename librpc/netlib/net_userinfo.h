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

#ifndef _NETUSERINFO_H_
#define _NETUSERINFO_H_

union user_info_id {
    uint32  id;
    PSID sid;
};


typedef struct _USER_INFO_X {
    wchar16_t *name;
    wchar16_t *password;
    uint32     password_age;
    uint32     priv;
    wchar16_t *home_dir;
    wchar16_t *comment;
    uint32     flags;
    wchar16_t *script_path;
    uint32     auth_flags;
    wchar16_t *full_name;
    wchar16_t *usr_comment;
    wchar16_t *parms;
    wchar16_t *workstations;
    NtTime     last_logon;
    NtTime     last_logoff;
    NtTime     acct_expires;
    uint32     max_storage;
    uint32     units_per_week;
    uint8     *logon_hours;
    uint32     bad_pw_count;
    uint32     num_logons;
    wchar16_t *logon_server;
    uint32     country_code;
    uint32     code_page;
    union user_info_id user;
    PSID       user_sid;
    uint32     primary_group_id;
    wchar16_t *profile;
    wchar16_t *home_dir_drive;
    uint32     password_expired;
} USER_INFO_X;


typedef struct _USER_INFO_1X {
    wchar16_t *name;
    wchar16_t *comment;
    wchar16_t *usr_comment;
    wchar16_t *full_name;
    uint32     priv;
    uint32     auth_flags;
    uint32     password_age;
    wchar16_t *home_dir;
    wchar16_t *parms;
    uint32     last_logon;
    uint32     last_logoff;
    uint32     bad_pw_count;
    uint32     num_logons;
    wchar16_t *logon_server;
    uint32     country_code;
    wchar16_t *workstations;
    uint32     max_storage;
    uint32     units_per_week;
    uint8     *logon_hours;
    uint32     code_page;
} USER_INFO_1X;


typedef struct _USER_INFO_2X {
    wchar16_t *name;
    wchar16_t *full_name;
    wchar16_t *comment;
    union user_info_id user;
} USER_INFO_2X;


NTSTATUS PullUserInfo0(void **buffer, wchar16_t **names, uint32 num);
NTSTATUS PullUserInfo1(void **buffer, UserInfo21 *ui, uint32 num);
NTSTATUS PullUserInfo2(void **buffer, UserInfo21 *ui, uint32 num);
NTSTATUS PullUserInfo20(void **buffer, UserInfo21 *ui, uint32 num);

NTSTATUS PushUserInfoAdd(UserInfo **sinfo, uint32 *slevel, void *ninfo,
                         uint32 nlevel, uint32 *parm_err);

NTSTATUS EncPasswordEx(uint8 pwbuf[532], wchar16_t *password,
                       uint32 password_len, NetConn *conn);

NTSTATUS PushUserInfo0(UserInfo **sinfo, uint32 *level, USER_INFO_0 *ninfo);
NTSTATUS PushUserInfo20(UserInfo **sinfo, uint32 *level, USER_INFO_20 *ninfo);
NTSTATUS PushUserInfo1003(UserInfo **sinfo, uint32 *level, USER_INFO_1003 *ninfo,
                          NetConn *conn);
NTSTATUS PushUserInfo1007(UserInfo **sinfo, uint32 *level, USER_INFO_1007 *ninfo);
NTSTATUS PushUserInfo1008(UserInfo **sinfo, uint32 *level, USER_INFO_1008 *ninfo);
NTSTATUS PushUserInfo1011(UserInfo **sinfo, uint32 *level, USER_INFO_1011 *ninfo);

#endif /* _NETUSERINFO_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
