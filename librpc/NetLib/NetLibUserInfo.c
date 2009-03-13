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

#include "includes.h"

/*
 * push functions/macros transfer: net userinfo -> samr userinfo
 * pull functions/macros transfer: net userinfo <- samr userinfo
 */


/* not covered ACB flags: ACB_NO_AUTH_DATA_REQD, ACB_MNS, ACB_AUTOLOCK */
/* not covered UF flags: UF_SCRIPT, UF_LOCKOUT, UF_PASSWD_CANT_CHANGE,
   UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION */

NTSTATUS PullUserInfo0(void **buffer, wchar16_t **names, uint32 num)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    USER_INFO_0 *info = NULL;
    int i = 0;

    goto_if_invalid_param_ntstatus(buffer, cleanup);
    goto_if_invalid_param_ntstatus(names, cleanup);

    status = NetAllocateMemory((void**)&info, sizeof(USER_INFO_1) * num,
                               NULL);
    goto_if_ntstatus_not_success(status, error);

    for (i = 0; i < num; i++) {
        if (names[i]) {
            info[i].usri0_name = wc16sdup(names[i]);

        } else {
            info[i].usri0_name = wc16sdup(null_string);
        }
        goto_if_no_memory_ntstatus(info[i].usri0_name, error);

        status = NetAddDepMemory(info[i].usri0_name,
                                 info);
        goto_if_ntstatus_not_success(status, error);
    }

    *buffer = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    goto cleanup;
}


NTSTATUS PullUserInfo1(void **buffer, UserInfo21 *ui, uint32 num)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    USER_INFO_1 *info = NULL;
    int i = 0;

    goto_if_invalid_param_ntstatus(buffer, cleanup);
    goto_if_invalid_param_ntstatus(ui, cleanup);

    status = NetAllocateMemory((void**)&info, sizeof(USER_INFO_1) * num,
                               NULL);
    goto_if_ntstatus_not_success(status, error);

    for (i = 0; i < num; i++) {
        PULL_UNICODE_STRING(info[i].usri1_name, ui[i].account_name, info);
        info[i].usri1_password = NULL;
        /* info[i].usri1_password_age */
        /* info[i].usri1_priv */
        PULL_UNICODE_STRING(info[i].usri1_home_dir, ui[i].home_directory, info);
        PULL_UNICODE_STRING(info[i].usri1_comment, ui[i].comment, info);
        PULL_ACCOUNT_FLAGS(info[i].usri1_flags, ui[i].account_flags);
        PULL_UNICODE_STRING(info[i].usri1_script_path, ui[i].logon_script, info);
    }

    *buffer = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    goto cleanup;
}


NTSTATUS PullUserInfo2(void **buffer, UserInfo21 *ui, uint32 num)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    USER_INFO_2 *info = NULL;
    int i = 0;

    goto_if_invalid_param_ntstatus(buffer, cleanup);
    goto_if_invalid_param_ntstatus(ui, cleanup);

    status = NetAllocateMemory((void**)&info, sizeof(USER_INFO_2) * num,
                               NULL);
    goto_if_ntstatus_not_success(status, error);

    for (i = 0; i < num; i++) {
        PULL_UNICODE_STRING(info[i].usri2_name, ui[i].account_name, info);
        info[i].usri2_password = NULL;
        /* info[i].usri1_password_age */
        /* info[i].usri1_priv */
        PULL_UNICODE_STRING(info[i].usri2_home_dir, ui[i].home_directory, info);
        PULL_UNICODE_STRING(info[i].usri2_comment, ui[i].comment, info);
        PULL_ACCOUNT_FLAGS(info[i].usri2_flags, ui[i].account_flags);
        PULL_UNICODE_STRING(info[i].usri2_script_path, ui[i].logon_script, info);

        /* info[i].usri2_auth_flags */
        PULL_UNICODE_STRING(info[i].usri2_full_name, ui[i].full_name, info);
        /* info[i].usri2_usr_comment */
        /* info[i].usri2_parms */
        PULL_UNICODE_STRING(info[i].usri2_workstations, ui[i].workstations, info);
        info[i].usri2_last_logon = ui[i].last_logon;
        info[i].usri2_last_logoff = ui[i].last_logoff;
        info[i].usri2_acct_expires = ui[i].account_expiry;
        /* info[i].usri2_max_storage */
        info[i].usri2_units_per_week = ui[i].logon_hours.units_per_week;
        /* info[i].usri2_logon_hours */
        info[i].usri2_bad_pw_count = ui[i].bad_password_count;
        info[i].usri2_num_logons = ui[i].logon_count;
        /* info[i].usri2_logon_server */
        info[i].usri2_country_code = ui[i].country_code;
        info[i].usri2_code_page = ui[i].code_page;
    }

    *buffer = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    goto cleanup;
}


NTSTATUS PullUserInfo20(void **buffer, UserInfo21 *ui, uint32 num)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    USER_INFO_20 *info = NULL;
    int i = 0;

    goto_if_invalid_param_ntstatus(buffer, cleanup);
    goto_if_invalid_param_ntstatus(ui, cleanup);

    status = NetAllocateMemory((void**)&info, sizeof(USER_INFO_20) * num,
                               NULL);
    goto_if_ntstatus_not_success(status, error);

    for (i = 0; i < num; i++) {
        PULL_UNICODE_STRING(info[i].usri20_name, ui[i].account_name, info);
        PULL_UNICODE_STRING(info[i].usri20_full_name, ui[i].full_name, info);
        PULL_UNICODE_STRING(info[i].usri20_comment, ui[i].comment, info);
        PULL_ACCOUNT_FLAGS(info[i].usri20_flags, ui[i].account_flags);
        info[i].usri20_user_id = ui[i].rid;
    }

    *buffer = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    goto cleanup;
}


NTSTATUS EncPasswordEx(uint8 pwbuf[532], wchar16_t *password,
                       uint32 password_len, NetConn *conn)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    struct md5context ctx;
    uint8 initval[16], digested_sess_key[16];

    goto_if_invalid_param_ntstatus(pwbuf, cleanup);
    goto_if_invalid_param_ntstatus(password, cleanup);
    goto_if_invalid_param_ntstatus(conn, cleanup);

    memset(&ctx, 0, sizeof(ctx));
    memset(initval, 0, sizeof(initval));
    memset(digested_sess_key, 0, sizeof(digested_sess_key));

    EncodePassBufferW16(pwbuf, password);

    get_random_buffer((unsigned char*)initval, sizeof(initval));

    md5init(&ctx);
    md5update(&ctx, initval, 16);
    md5update(&ctx, conn->sess_key, conn->sess_key_len);
    md5final(&ctx, digested_sess_key);

    rc4(pwbuf, 532, digested_sess_key, 16);
    memcpy((void*)&pwbuf[516], initval, 16);

cleanup:
    return status;
}


NTSTATUS PushUserInfoAdd(UserInfo **sinfo, uint32 *slevel, void *ptr,
                         uint32 nlevel, uint32 *parm_err)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo *info = NULL;
    UserInfo21 *info21 = NULL;
    USER_INFO_X *ninfo = NULL;

    goto_if_invalid_param_ntstatus(sinfo, cleanup);
    goto_if_invalid_param_ntstatus(slevel, cleanup);
    goto_if_invalid_param_ntstatus(ptr, cleanup);
    /* parm_err is optional and can be NULL */

    if (nlevel < 1 || nlevel > 4) {
        status = STATUS_INVALID_LEVEL;
        goto error;
    }

    ninfo = ptr;

    if (parm_err) {
        *parm_err = 0;
    }


    status = NetAllocateMemory((void**)&info, sizeof(UserInfo), NULL);
    goto_if_ntstatus_not_success(status, error);

    *slevel = 21;
    info21  = &info->info21;

    /* privileges field must be set to USER_PRIV_USER when
       calling NetUserAdd function */
    if (ninfo->priv != USER_PRIV_USER) {
        if (parm_err) {
            *parm_err = (void*)&ninfo->priv - (void*)ninfo;
        }

        status = STATUS_INVALID_PARAMETER;
        goto error;
    }

    info21->fields_present = 0;

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->home_dir, home_directory,
                                 SAMR_FIELD_HOME_DIRECTORY, info);

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->comment, comment,
                                 SAMR_FIELD_COMMENT, info);

    PUSH_ACCOUNT_FLAGS(info21, ninfo->flags, account_flags,
                       SAMR_FIELD_ACCT_FLAGS);

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->script_path, logon_script,
                                 SAMR_FIELD_LOGON_SCRIPT, info);

    *sinfo = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo  = NULL;
    *slevel = 0;
    goto cleanup;
}


NTSTATUS PushUserInfo0(UserInfo **sinfo, uint32 *slevel, USER_INFO_0 *ninfo)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo *info = NULL;
    UserInfo21 *info21 = NULL;

    goto_if_invalid_param_ntstatus(sinfo, cleanup);
    goto_if_invalid_param_ntstatus(slevel, cleanup);
    goto_if_invalid_param_ntstatus(ninfo, cleanup);

    status = NetAllocateMemory((void**)&info, sizeof(UserInfo), NULL);
    goto_if_ntstatus_not_success(status, error);

    *slevel = 21;
    info21 = &info->info21;

    info21->fields_present = 0;

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->usri0_name, account_name,
                                 SAMR_FIELD_ACCOUNT_NAME, info);

    *sinfo = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo  = NULL;
    *slevel = 0;
    goto cleanup;
}


NTSTATUS PushUserInfo1(UserInfo **sinfo, uint32 *slevel, USER_INFO_1 *ninfo)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo *info = NULL;
    UserInfo21 *info21 = NULL;

    goto_if_invalid_param_ntstatus(sinfo, cleanup);
    goto_if_invalid_param_ntstatus(slevel, cleanup);
    goto_if_invalid_param_ntstatus(ninfo, cleanup);

    status = NetAllocateMemory((void**)&info, sizeof(UserInfo), NULL);
    goto_if_ntstatus_not_success(status, error);

    *slevel = 21;
    info21 = &info->info21;

    info21->fields_present = 0;

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->usri1_home_dir, home_directory,
                                 SAMR_FIELD_HOME_DIRECTORY, info);

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->usri1_comment, comment,
                                 SAMR_FIELD_COMMENT, info);

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->usri1_script_path, logon_script,
                                 SAMR_FIELD_LOGON_SCRIPT, info);

    PUSH_ACCOUNT_FLAGS(info21, ninfo->usri1_flags, account_flags,
                       SAMR_FIELD_ACCT_FLAGS);

    *sinfo = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo  = NULL;
    *slevel = 0;
    goto cleanup;
}


NTSTATUS PushUserInfo1003(UserInfo **sinfo, uint32 *slevel, USER_INFO_1003 *ninfo,
                          NetConn *conn)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo *info = NULL;
    UserInfo25 *info25 = NULL;
    wchar16_t *password = NULL;
    size_t password_len = 0;

    goto_if_invalid_param_ntstatus(sinfo, cleanup);
    goto_if_invalid_param_ntstatus(slevel, cleanup);
    goto_if_invalid_param_ntstatus(ninfo, cleanup);
    goto_if_invalid_param_ntstatus(conn, cleanup);

    status = NetAllocateMemory((void**)&info, sizeof(UserInfo), NULL);
    goto_if_ntstatus_not_success(status, error);

    *slevel = 25;
    info25 = &info->info25;

    password = wc16sdup(ninfo->usri1003_password);
    goto_if_no_memory_ntstatus(password, error);

    password_len = wc16slen(password);
    status = EncPasswordEx(info25->password.data, password, password_len, conn);
    goto_if_ntstatus_not_success(status, error);

    info25->info.fields_present = SAMR_FIELD_PASSWORD;

    *sinfo = info;

cleanup:
    SAFE_FREE(password);
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo  = NULL;
    *slevel = 0;
    goto cleanup;
}


NTSTATUS PushUserInfo1007(UserInfo **sinfo, uint32 *slevel, USER_INFO_1007 *ninfo)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo *info = NULL;
    UserInfo21 *info21 = NULL;

    goto_if_invalid_param_ntstatus(sinfo, cleanup);
    goto_if_invalid_param_ntstatus(slevel, cleanup);
    goto_if_invalid_param_ntstatus(ninfo, cleanup);

    status = NetAllocateMemory((void**)&info, sizeof(UserInfo), NULL);
    goto_if_ntstatus_not_success(status, error);

    *slevel = 21;
    info21 = &info->info21;

    info21->fields_present = 0;

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->usri1007_comment, comment,
                                 SAMR_FIELD_COMMENT, info);

    *sinfo = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo  = NULL;
    *slevel = 0;
    goto cleanup;
}


NTSTATUS PushUserInfo1008(UserInfo **sinfo, uint32 *slevel, USER_INFO_1008 *ninfo)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo *info = NULL;
    UserInfo21 *info21 = NULL;

    goto_if_invalid_param_ntstatus(sinfo, cleanup);
    goto_if_invalid_param_ntstatus(slevel, cleanup);
    goto_if_invalid_param_ntstatus(ninfo, cleanup);

    status = NetAllocateMemory((void**)&info, sizeof(UserInfo), NULL);
    goto_if_ntstatus_not_success(status, error);

    *slevel = 21;
    info21 = &info->info21;

    info21->fields_present = 0;

    PUSH_ACCOUNT_FLAGS(info21, ninfo->usri1008_flags, account_flags,
                       SAMR_FIELD_ACCT_FLAGS);

    *sinfo = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo  = NULL;
    *slevel = 0;
    goto cleanup;
}


NTSTATUS PushUserInfo1011(UserInfo **sinfo, uint32 *slevel, USER_INFO_1011 *ninfo)
{
    NTSTATUS status = STATUS_SUCCESS;
    WINERR err = ERROR_SUCCESS;
    UserInfo *info = NULL;
    UserInfo21 *info21 = NULL;

    goto_if_invalid_param_ntstatus(sinfo, cleanup);
    goto_if_invalid_param_ntstatus(slevel, cleanup);
    goto_if_invalid_param_ntstatus(ninfo, cleanup);

    status = NetAllocateMemory((void**)&info, sizeof(UserInfo), NULL);
    goto_if_ntstatus_not_success(status, error);

    *slevel = 21;
    info21 = &info->info21;

    info21->fields_present = 0;

    PUSH_UNICODE_STRING_USERINFO(info21, ninfo->usri1011_full_name, full_name,
                                 SAMR_FIELD_FULL_NAME, info);

    *sinfo = info;

cleanup:
    return status;

error:
    if (info) {
        NetFreeMemory((void*)info);
    }

    *sinfo  = NULL;
    *slevel = 0;
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
