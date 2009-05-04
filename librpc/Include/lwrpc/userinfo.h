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

#ifndef _SAMR_USERINFO_H_
#define _SAMR_USERINFO_H_

#include <lwrpc/types.h>

typedef struct user_info1 {
    UnicodeString account_name;
    UnicodeString full_name;
    uint32 primary_gid;
    UnicodeString description;
    UnicodeString comment;
} UserInfo1;

typedef struct user_info2 {
    UnicodeString comment;
    UnicodeString unknown1;
    uint16 country_code;
	uint16 code_page;
} UserInfo2;

typedef struct logon_hours {
    uint16 units_per_week;
#ifdef _DCE_IDL_
    [size_is(1260), length_is(units_per_week/8)]
#endif
    uint8 *units;
} LogonHours;

typedef struct user_info3 {
    UnicodeString account_name;
    UnicodeString full_name;
    uint32 rid;
    uint32 primary_gid;
    UnicodeString home_directory;
    UnicodeString home_drive;
    UnicodeString logon_script;
    UnicodeString profile_path;
    UnicodeString workstations;
    NtTime last_logon;
    NtTime last_logoff;
    NtTime last_password_change;
    NtTime allow_password_change;
    NtTime force_password_change;
    LogonHours logon_hours;
    uint16 bad_password_count;
    uint16 logon_count;
    uint32 account_flags;
} UserInfo3;

typedef struct user_info4 {
    LogonHours logon_hours;
} UserInfo4;

typedef struct user_info5 {
	UnicodeString account_name;
	UnicodeString full_name;
	uint32 rid;
	uint32 primary_gid;
	UnicodeString home_directory;
	UnicodeString home_drive;
	UnicodeString logon_script;
	UnicodeString profile_path;
	UnicodeString description;
	UnicodeString workstations;
	NtTime last_logon;
	NtTime last_logoff;
	LogonHours logon_hours;
	uint16 bad_password_count;
	uint16 logon_count;
	NtTime last_password_change;
	NtTime account_expiry;
	uint32 account_flags;
} UserInfo5;

typedef struct user_info6 {
	UnicodeString account_name;
	UnicodeString full_name;
} UserInfo6;

typedef struct user_info7 {
	UnicodeString account_name;
} UserInfo7;

typedef struct user_info8 {
	UnicodeString full_name;
} UserInfo8;

typedef struct user_info9 {
	uint32 primary_gid;
} UserInfo9;

typedef struct user_info10 {
	UnicodeString home_directory;
	UnicodeString home_drive;
} UserInfo10;

typedef struct user_info11 {
	UnicodeString logon_script;
} UserInfo11;

typedef struct user_info12 {
	UnicodeString profile_path;
} UserInfo12;

typedef struct user_info13 {
	UnicodeString description;
} UserInfo13;

typedef struct user_info14 {
	UnicodeString workstations;
} UserInfo14;

typedef struct user_info16 {
	uint32 account_flags;
} UserInfo16;

typedef struct user_info17 {
	NtTime account_expiry;
} UserInfo17;

typedef struct user_info20 {
	UnicodeString parameters;
} UserInfo20;


#define SAMR_FIELD_ACCOUNT_NAME       0x00000001
#define SAMR_FIELD_FULL_NAME          0x00000002
#define SAMR_FIELD_RID                0x00000004
#define SAMR_FIELD_PRIMARY_GID        0x00000008
#define SAMR_FIELD_DESCRIPTION        0x00000010
#define SAMR_FIELD_COMMENT            0x00000020
#define SAMR_FIELD_HOME_DIRECTORY     0x00000040
#define SAMR_FIELD_HOME_DRIVE         0x00000080
#define SAMR_FIELD_LOGON_SCRIPT       0x00000100
#define SAMR_FIELD_PROFILE_PATH       0x00000200
#define SAMR_FIELD_WORKSTATIONS       0x00000400
#define SAMR_FIELD_LAST_LOGON         0x00000800
#define SAMR_FIELD_LAST_LOGOFF        0x00001000
#define SAMR_FIELD_LOGON_HOURS        0x00002000
#define SAMR_FIELD_BAD_PWD_COUNT      0x00004000
#define SAMR_FIELD_NUM_LOGONS         0x00008000
#define SAMR_FIELD_ALLOW_PWD_CHANGE   0x00010000
#define SAMR_FIELD_FORCE_PWD_CHANGE   0x00020000
#define SAMR_FIELD_LAST_PWD_CHANGE    0x00040000
#define SAMR_FIELD_ACCT_EXPIRY        0x00080000
#define SAMR_FIELD_ACCT_FLAGS         0x00100000
#define SAMR_FIELD_PARAMETERS         0x00200000
#define SAMR_FIELD_COUNTRY_CODE       0x00400000
#define SAMR_FIELD_CODE_PAGE          0x00800000
#define SAMR_FIELD_PASSWORD           0x01000000
#define SAMR_FIELD_PASSWORD2          0x02000000
#define SAMR_FIELD_PRIVATE_DATA       0x04000000
#define SAMR_FIELD_EXPIRED_FLAG       0x08000000
#define SAMR_FIELD_SEC_DESC           0x10000000
#define SAMR_FIELD_OWF_PWD            0x20000000


typedef struct user_info21 {
	NtTime last_logon;
	NtTime last_logoff;
	NtTime last_password_change;
	NtTime account_expiry;
	NtTime allow_password_change;
	NtTime force_password_change;
	UnicodeString account_name;
	UnicodeString full_name;
	UnicodeString home_directory;
	UnicodeString home_drive;
	UnicodeString logon_script;
	UnicodeString profile_path;
	UnicodeString description;
	UnicodeString workstations;
	UnicodeString comment;
	UnicodeString parameters;
	UnicodeString unknown1;
	UnicodeString unknown2;
	UnicodeString unknown3;
	uint32 buf_count;
#ifdef _DCE_IDL_
	[size_is(buf_count)]
#endif
	uint8 *buffer;
	uint32 rid;
	uint32 primary_gid;
	uint32 account_flags;
	uint32 fields_present;
	LogonHours logon_hours;
	uint16 bad_password_count;
	uint16 logon_count;
	uint16 country_code;
	uint16 code_page;
	uint8 nt_password_set;
	uint8 lm_password_set;
	uint8 password_expired;
	uint8 unknown4;
} UserInfo21;

typedef struct hash_pass {
	uint8 data[16];
} HashPassword;

typedef struct crypt_password {
	uint8 data[516];
} CryptPassword;

typedef struct user_info23 {
	UserInfo21 info;
	CryptPassword password;
} UserInfo23;

typedef struct user_info24 {
	CryptPassword password;
	uint8 password_len;
} UserInfo24;

typedef struct crypt_password_ex {
	uint8 data[532];
} CryptPasswordEx;

typedef struct user_info25 {
	UserInfo21 info;
	CryptPasswordEx password;
} UserInfo25;

typedef struct user_info26 {
	CryptPasswordEx password;
	uint8 password_len;
} UserInfo26;


#define ACB_DISABLED                 0x00000001
#define ACB_HOMDIRREQ                0x00000002
#define ACB_PWNOTREQ                 0x00000004
#define ACB_TEMPDUP                  0x00000008
#define ACB_NORMAL                   0x00000010
#define ACB_MNS                      0x00000020
#define ACB_DOMTRUST                 0x00000040
#define ACB_WSTRUST                  0x00000080
#define ACB_SVRTRUST                 0x00000100
#define ACB_PWNOEXP                  0x00000200
#define ACB_AUTOLOCK                 0x00000400
#define ACB_ENC_TXT_PWD_ALLOWED      0x00000800
#define ACB_SMARTCARD_REQUIRED       0x00001000
#define ACB_TRUSTED_FOR_DELEGATION   0x00002000
#define ACB_NOT_DELEGATED            0x00004000
#define ACB_USE_DES_KEY_ONLY         0x00008000
#define ACB_DONT_REQUIRE_PREAUTH     0x00010000
#define ACB_PW_EXPIRED               0x00020000
#define ACB_NO_AUTH_DATA_REQD        0x00080000


typedef struct samr_pw_info {
  uint16 min_password_length;
  uint32 password_properties;
} PwInfo;


#ifndef _DCE_IDL_
typedef union user_info {
	UserInfo1 info1;
	UserInfo2 info2;
	UserInfo3 info3;
	UserInfo4 info4;
	UserInfo5 info5;
	UserInfo6 info6;
	UserInfo7 info7;
	UserInfo8 info8;
	UserInfo9 info9;
	UserInfo10 info10;
	UserInfo11 info11;
	UserInfo12 info12;
	UserInfo13 info13;
	UserInfo14 info14;
	UserInfo16 info16;
	UserInfo17 info17;
	UserInfo20 info20;
	UserInfo21 info21;
	UserInfo23 info23;
	UserInfo24 info24;
	UserInfo25 info25;
	UserInfo26 info26;
} UserInfo;
#endif

#endif /* _SAMR_USERINFO_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
