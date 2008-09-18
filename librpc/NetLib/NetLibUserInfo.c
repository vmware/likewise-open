/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
#include "NetLibUserInfo.h"
#include <md5.h>

static wchar16_t null_char = 0;
static wchar16_t *null_string = &null_char;

/*
 * push functions/macros transfer data from net userinfo structs to samr userinfo
 * pull functions/macros transfer data to net userinfo structs from samr userinfo
 */

#define PULL_UNICODE_STRING(dst, src)                       \
    do                                                      \
    {                                                       \
        if ((src).string != NULL && (src).len > 0)          \
        {                                                   \
            (dst) = wc16sndup((src).string, (src).len/2);   \
        }                                                   \
        else                                                \
        {                                                   \
            (dst) = wc16sdup(null_string);		    \
        }                                                   \
    } while (0)
          
#define PUSH_UNICODE_STRING(info_ptr, netinfo_field, samrinfo_field, field_flag) \
    if (netinfo_field != NULL) {					\
	InitUnicodeString(&info_ptr->samrinfo_field, netinfo_field);	\
	info_ptr->fields_present |= field_flag;				\
    }


#define PULL_ACCOUNT_FLAG(acct_flags, netacct_flags) \
    if (samrflags & acct_flags) flags |= netacct_flags;


#define PULL_ACCOUNT_FLAGS(net_flags, samr_flags) \
    { \
        uint32 flags = 0; \
        uint32 samrflags = samr_flags; \
        \
        PULL_ACCOUNT_FLAG(ACB_DISABLED, UF_ACCOUNTDISABLE); \
        PULL_ACCOUNT_FLAG(ACB_HOMDIRREQ, UF_HOMEDIR_REQUIRED); \
        PULL_ACCOUNT_FLAG(ACB_PWNOTREQ, UF_PASSWD_NOTREQD); \
        PULL_ACCOUNT_FLAG(ACB_TEMPDUP, UF_TEMP_DUPLICATE_ACCOUNT); \
        PULL_ACCOUNT_FLAG(ACB_NORMAL, UF_NORMAL_ACCOUNT); \
        PULL_ACCOUNT_FLAG(ACB_DOMTRUST, UF_INTERDOMAIN_TRUST_ACCOUNT); \
        PULL_ACCOUNT_FLAG(ACB_WSTRUST, UF_WORKSTATION_TRUST_ACCOUNT); \
        PULL_ACCOUNT_FLAG(ACB_SVRTRUST, UF_SERVER_TRUST_ACCOUNT); \
        PULL_ACCOUNT_FLAG(ACB_PWNOEXP, UF_DONT_EXPIRE_PASSWD); \
        PULL_ACCOUNT_FLAG(ACB_ENC_TXT_PWD_ALLOWED, UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED); \
        PULL_ACCOUNT_FLAG(ACB_SMARTCARD_REQUIRED, UF_SMARTCARD_REQUIRED); \
        PULL_ACCOUNT_FLAG(ACB_TRUSTED_FOR_DELEGATION, UF_TRUSTED_FOR_DELEGATION); \
        PULL_ACCOUNT_FLAG(ACB_NOT_DELEGATED, UF_NOT_DELEGATED); \
        PULL_ACCOUNT_FLAG(ACB_USE_DES_KEY_ONLY, UF_USE_DES_KEY_ONLY); \
        PULL_ACCOUNT_FLAG(ACB_DONT_REQUIRE_PREAUTH, UF_DONT_REQUIRE_PREAUTH); \
        PULL_ACCOUNT_FLAG(ACB_PW_EXPIRED, UF_PASSWORD_EXPIRED); \
        \
        net_flags = flags; \
    };


#define PUSH_ACCOUNT_FLAG(netacct_flags, samracct_flags) \
    if (netflags & netacct_flags) flags |= samracct_flags


#define PUSH_ACCOUNT_FLAGS(info, net_flags, samr_flags, field_present) \
    { \
        uint32 flags = 0; \
        uint32 netflags = net_flags; \
        \
        PUSH_ACCOUNT_FLAG(UF_ACCOUNTDISABLE, ACB_DISABLED); \
        PUSH_ACCOUNT_FLAG(UF_HOMEDIR_REQUIRED, ACB_HOMDIRREQ); \
        PUSH_ACCOUNT_FLAG(UF_PASSWD_NOTREQD, ACB_PWNOTREQ); \
        PUSH_ACCOUNT_FLAG(UF_TEMP_DUPLICATE_ACCOUNT, ACB_TEMPDUP); \
        PUSH_ACCOUNT_FLAG(UF_NORMAL_ACCOUNT, ACB_NORMAL); \
        PUSH_ACCOUNT_FLAG(UF_INTERDOMAIN_TRUST_ACCOUNT, ACB_DOMTRUST); \
        PUSH_ACCOUNT_FLAG(UF_WORKSTATION_TRUST_ACCOUNT, ACB_WSTRUST); \
        PUSH_ACCOUNT_FLAG(UF_SERVER_TRUST_ACCOUNT, ACB_SVRTRUST); \
        PUSH_ACCOUNT_FLAG(UF_DONT_EXPIRE_PASSWD, ACB_PWNOEXP);	\
        PUSH_ACCOUNT_FLAG(UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED, ACB_ENC_TXT_PWD_ALLOWED); \
        PUSH_ACCOUNT_FLAG(UF_SMARTCARD_REQUIRED, ACB_SMARTCARD_REQUIRED); \
        PUSH_ACCOUNT_FLAG(UF_TRUSTED_FOR_DELEGATION, ACB_TRUSTED_FOR_DELEGATION); \
        PUSH_ACCOUNT_FLAG(UF_NOT_DELEGATED, ACB_NOT_DELEGATED); \
        PUSH_ACCOUNT_FLAG(UF_USE_DES_KEY_ONLY, ACB_USE_DES_KEY_ONLY); \
        PUSH_ACCOUNT_FLAG(UF_DONT_REQUIRE_PREAUTH, ACB_DONT_REQUIRE_PREAUTH); \
        PUSH_ACCOUNT_FLAG(UF_PASSWORD_EXPIRED, ACB_PW_EXPIRED); \
        \
        info->samr_flags = flags; \
        info->fields_present |= field_present; \
    }


/* not covered ACB flags: ACB_NO_AUTH_DATA_REQD, ACB_MNS, ACB_AUTOLOCK */
/* not covered UF flags: UF_SCRIPT, UF_LOCKOUT, UF_PASSWD_CANT_CHANGE,
   UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION */


void *PullUserInfo1(void *buffer, UserInfo21 *ui, int i)
{
    USER_INFO_1 *info = (USER_INFO_1*)buffer;
    if (info == NULL) return NULL;

    PULL_UNICODE_STRING(info[i].usri1_name, ui->account_name);
    PULL_UNICODE_STRING(info[i].usri1_name, ui->account_name);
    info[i].usri1_password = NULL;
    /* info[i].usri1_password_age */
    /* info[i].usri1_priv */
    PULL_UNICODE_STRING(info[i].usri1_home_dir, ui->home_directory);
    PULL_UNICODE_STRING(info[i].usri1_comment, ui->comment);
    PULL_ACCOUNT_FLAGS(info[i].usri1_flags, ui->account_flags);
    PULL_UNICODE_STRING(info[i].usri1_script_path, ui->logon_script);

    return buffer;
}


void *PullUserInfo2(void *buffer, UserInfo21 *ui, int i)
{
    USER_INFO_2 *info = (USER_INFO_2*)buffer;
    if (info == NULL) return NULL;

    buffer = PullUserInfo1(buffer, ui, i);
    if (buffer == NULL) return NULL;

    /* info[i].usri2_auth_flags */
    PULL_UNICODE_STRING(info[i].usri2_full_name, ui->full_name);
    /* info[i].usri2_usr_comment */
    /* info[i].usri2_parms */
    PULL_UNICODE_STRING(info[i].usri2_workstations, ui->workstations);
    info[i].usri2_last_logon = ui->last_logon;
    info[i].usri2_last_logoff = ui->last_logoff;
    info[i].usri2_acct_expires = ui->account_expiry;
    /* info[i].usri2_max_storage */
    info[i].usri2_units_per_week = ui->logon_hours.units_per_week;
    /* info[i].usri2_logon_hours */
    info[i].usri2_bad_pw_count = ui->bad_password_count;
    info[i].usri2_num_logons = ui->logon_count;
    /* info[i].usri2_logon_server */
    info[i].usri2_country_code = ui->country_code;
    info[i].usri2_code_page = ui->code_page;

    return buffer;
}


void *PullUserInfo20(void *buffer, UserInfo21 *ui, int i)
{
    USER_INFO_20 *info = (USER_INFO_20*)buffer;
    if (info == NULL) return NULL;

    PULL_UNICODE_STRING(info[i].usri20_name, ui->account_name);
    PULL_UNICODE_STRING(info[i].usri20_full_name, ui->full_name);
    PULL_UNICODE_STRING(info[i].usri20_comment, ui->comment);
    PULL_ACCOUNT_FLAGS(info[i].usri20_flags, ui->account_flags);
    info[i].usri20_user_id = ui->rid;

    return buffer;
}


NTSTATUS EncPasswordEx(uint8 pwbuf[532], wchar16_t *password,
                       uint32 password_len, NetConn *conn)
{
    struct md5context ctx;
    uint8 initval[16], digested_sess_key[16];

    EncodePassBufferW16(pwbuf, password);

    get_random_buffer((unsigned char*)initval, sizeof(initval));

    md5init(&ctx);
    md5update(&ctx, initval, 16);
    md5update(&ctx, conn->sess_key, conn->sess_key_len);
    md5final(&ctx, digested_sess_key);

    rc4(pwbuf, 532, digested_sess_key, 16);
    memcpy((void*)&pwbuf[516], initval, 16);

    return STATUS_SUCCESS;
}


NTSTATUS PushUserInfoAdd(UserInfo *sinfo, uint32 *slevel, void *ptr,
			 uint32 nlevel, uint32 *parm_err)
{
    UserInfo21 *info21;
    USER_INFO_X *ninfo = ptr;

    if (nlevel < 1 || nlevel > 4) return STATUS_INVALID_LEVEL;

    if (parm_err) *parm_err = 0; /* parm_err can be NULL */
    *slevel   = 21;
    info21    = &sinfo->info21;

    /* privileges field must be set to USER_PRIV_USER when
       calling NetUserAdd function */
    if (ninfo->priv != USER_PRIV_USER) {
	if (parm_err) {
	    *parm_err = (void*)&ninfo->priv - (void*)ninfo;
	}
	return STATUS_INVALID_PARAMETER;
    }

    info21->fields_present = 0;

    PUSH_UNICODE_STRING(info21, ninfo->home_dir, home_directory,
			SAMR_FIELD_HOME_DIRECTORY);

    PUSH_UNICODE_STRING(info21, ninfo->comment, comment,
			SAMR_FIELD_COMMENT);

    PUSH_ACCOUNT_FLAGS(info21, ninfo->flags, account_flags,
		       SAMR_FIELD_ACCT_FLAGS);

    PUSH_UNICODE_STRING(info21, ninfo->script_path, logon_script,
			SAMR_FIELD_LOGON_SCRIPT);
    
    return STATUS_SUCCESS;
}


void* PushUserInfo0(UserInfo *sinfo, uint32 *level, USER_INFO_0 *ninfo)
{
    UserInfo21 *info21;

    *level = 21;
    info21 = &sinfo->info21;

    info21->fields_present = 0;

    PUSH_UNICODE_STRING(info21, ninfo->usri0_name, account_name,
			SAMR_FIELD_ACCOUNT_NAME);

    return sinfo;
}


void* PushUserInfo1(UserInfo *sinfo, uint32 *level, USER_INFO_1 *ninfo)
{
    size_t password_len;
    wchar16_t *password;
    UserInfo21 *info21;
    UserInfo24 *info24;

    *level = 21;
    info21 = &sinfo->info21;

    info21->fields_present = 0;

    PUSH_UNICODE_STRING(info21, ninfo->usri1_home_dir, home_directory,
			SAMR_FIELD_HOME_DIRECTORY);

    PUSH_UNICODE_STRING(info21, ninfo->usri1_comment, comment,
			SAMR_FIELD_COMMENT);

    PUSH_UNICODE_STRING(info21, ninfo->usri1_script_path, logon_script,
			SAMR_FIELD_LOGON_SCRIPT);

    PUSH_ACCOUNT_FLAGS(info21, ninfo->usri1_flags, account_flags,
		       SAMR_FIELD_ACCT_FLAGS);

    return sinfo;
}


void* PushUserInfo1003(UserInfo *sinfo, uint32 *level, USER_INFO_1003 *ninfo, NetConn *conn)
{
    UserInfo25 *info25;
    wchar16_t *password;
    size_t password_len;
    *level = 25;
    info25 = &sinfo->info25;
    password = ninfo->usri1003_password;

    password_len = wc16slen(password);
    EncPasswordEx(info25->password.data, password, password_len, conn);

    info25->info.fields_present = SAMR_FIELD_PASSWORD;

    return sinfo;
}


void* PushUserInfo1007(UserInfo *sinfo, uint32 *level, USER_INFO_1007 *ninfo)
{
    UserInfo21 *info21;

    *level = 21;
    info21 = &sinfo->info21;

    info21->fields_present = 0;

    PUSH_UNICODE_STRING(info21, ninfo->usri1007_comment, comment,
			SAMR_FIELD_COMMENT);

    return sinfo;
}


void* PushUserInfo1008(UserInfo *sinfo, uint32 *level, USER_INFO_1008 *ninfo)
{
    UserInfo21 *info21;

    *level = 21;
    info21 = &sinfo->info21;

    info21->fields_present = 0;

    PUSH_ACCOUNT_FLAGS(info21, ninfo->usri1008_flags, account_flags,
		       SAMR_FIELD_ACCT_FLAGS);

    return sinfo;
}


void* PushUserInfo1011(UserInfo *sinfo, uint32 *level, USER_INFO_1011 *ninfo)
{
    UserInfo21 *info21;

    *level = 21;
    info21 = &sinfo->info21;

    info21->fields_present = 0;

    PUSH_UNICODE_STRING(info21, ninfo->usri1011_full_name, full_name,
			SAMR_FIELD_FULL_NAME);

    return sinfo;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
