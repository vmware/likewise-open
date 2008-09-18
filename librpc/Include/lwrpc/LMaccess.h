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

#ifndef _LM_ACCESS_H_
#define _LM_ACCESS_H_

#include <lwrpc/types.h>
#include <lwrpc/security.h>


typedef struct _USER_INFO_0 {
	wchar16_t *usri0_name;
} USER_INFO_0;

typedef struct _USER_INFO_1 {
	wchar16_t *usri1_name;
	wchar16_t *usri1_password;
	uint32 usri1_password_age;
	uint32 usri1_priv;
	wchar16_t *usri1_home_dir;
	wchar16_t *usri1_comment;
	uint32 usri1_flags;
	wchar16_t *usri1_script_path;
} USER_INFO_1;

typedef struct _USER_INFO_2 {
	wchar16_t *usri2_name;
	wchar16_t *usri2_password;
	uint32 usri2_password_age;
	uint32 usri2_priv;
	wchar16_t *usri2_home_dir;
	wchar16_t *usri2_comment;
	uint32 usri2_flags;
	wchar16_t *usri2_script_path;
	uint32 usri2_auth_flags;
	wchar16_t *usri2_full_name;
	wchar16_t *usri2_usr_comment;
	wchar16_t *usri2_parms;
	wchar16_t *usri2_workstations;
	uint32 usri2_last_logon;
	uint32 usri2_last_logoff;
	uint32 usri2_acct_expires;
	uint32 usri2_max_storage;
	uint32 usri2_units_per_week;
	uint8 *usri2_logon_hours;
	uint32 usri2_bad_pw_count;
	uint32 usri2_num_logons;
	wchar16_t *usri2_logon_server;
	uint32 usri2_country_code;
	uint32 usri2_code_page;
} USER_INFO_2;

typedef struct _USER_INFO_3 {
	wchar16_t *usri3_name;
	wchar16_t *usri3_password;
	wchar16_t *usri3_password_age;
	uint32 usri3_priv;
	wchar16_t *usri3_home_dir;
	wchar16_t *usri3_comment;
	uint32 usri3_flags;
	wchar16_t *usri3_script_path;
	uint32 usri3_auth_flags;
	wchar16_t *usri3_full_name;
	wchar16_t *usri3_usr_comment;
	wchar16_t *usri3_parms;
	wchar16_t *usri3_workstations;
	NtTime usri3_last_logon;
	NtTime usri3_last_logoff;
	NtTime usri3_acct_expires;
	uint32 usri3_max_storage;
	uint32 usri3_units_per_week;
	uint8 *usri3_logon_hours;
	uint32 usri3_bad_pw_count;
	uint32 usri3_num_logons;
	wchar16_t *usri3_logon_server;
	uint32 usri3_country_code;
	uint32 usri3_code_page;
	uint32 usri3_user_id;
	uint32 usri3_primary_group_id;
	wchar16_t *usri3_profile;
	wchar16_t *usri3_home_dir_drive;
	uint32 usri3_password_expired;
} USER_INFO_3;


typedef struct _USER_INFO_4 {
	wchar16_t *usri4_name;
	wchar16_t *usri4_password;
	wchar16_t *usri4_password_age;
	uint32 usri4_priv;
	wchar16_t *usri4_home_dir;
	wchar16_t *usri4_comment;
	uint32 usri4_flags;
	wchar16_t *usri4_script_path;
	uint32 usri4_auth_flags;
	wchar16_t *usri4_full_name;
	wchar16_t *usri4_usr_comment;
	wchar16_t *usri4_parms;
	wchar16_t *usri4_workstations;
	NtTime usri4_last_logon;
	NtTime usri4_last_logoff;
	NtTime usri4_acct_expires;
	uint32 usri4_max_storage;
	uint32 usri4_units_per_week;
	uint8 *usri4_logon_hours;
	uint32 usri4_bad_pw_count;
	uint32 usri4_num_logons;
	wchar16_t *usri4_logon_server;
	uint32 usri4_country_code;
	uint32 usri4_code_page;
	DomSid* usri4_user_sid;
	uint32 usri4_primary_group_id;
	wchar16_t *usri4_profile;
	wchar16_t *usri4_home_dir_drive;
	uint32 usri4_password_expired;
} USER_INFO_4;


typedef struct _USER_INFO_10 {
	wchar16_t *usri10_name;
	wchar16_t *usri10_comment;
	wchar16_t *usri10_usr_comment;
	wchar16_t *usri10_full_name;
} USER_INFO_10;

typedef struct _USER_INFO_11 {
	wchar16_t *usri11_name;
	wchar16_t *usri11_comment;
	wchar16_t *usri11_usr_comment;
	wchar16_t *usri11_full_name;
	uint32 usri11_priv;
	uint32 usri11_auth_flags;
	uint32 usri11_password_age;
	wchar16_t *usri11_home_dir;
	wchar16_t *usri11_parms;
	uint32 usri11_last_logon;
	uint32 usri11_last_logoff;
	uint32 usri11_bad_pw_count;
	uint32 usri11_num_logons;
	wchar16_t *usri11_logon_server;
	uint32 usri11_country_code;
	wchar16_t *usri11_workstations;
	uint32 usri11_max_storage;
	uint32 usri11_units_per_week;
	uint8 *usri11_logon_hours;
	uint32 usri11_code_page;
} USER_INFO_11;

typedef struct _USER_INFO_20 {
	wchar16_t *usri20_name;
	wchar16_t *usri20_full_name;
	wchar16_t *usri20_comment;
	uint32 usri20_flags;
	uint32 usri20_user_id;
} USER_INFO_20;

typedef struct _USER_INFO_23 {
	wchar16_t *usri23_name;
	wchar16_t *usri23_full_name;
	wchar16_t *usri23_comment;
	DomSid *usri23_user_sid;
} USER_INFO_23;


typedef struct _USER_INFO_1003 {
	wchar16_t *usri1003_password;
} USER_INFO_1003;


typedef struct _USER_INFO_1007 {
	wchar16_t *usri1007_comment;
} USER_INFO_1007;


typedef struct _USER_INFO_1008 {
	uint32 usri1008_flags;
} USER_INFO_1008;


typedef struct _USER_INFO_1011 {
	wchar16_t *usri1011_full_name;
} USER_INFO_1011;



typedef struct _LOCALGROUP_USERS_INFO_0 {
	wchar16_t *lgrui0_name;
} LOCALGROUP_USERS_INFO_0;



typedef struct _LOCALGROUP_INFO_0 {
	wchar16_t *lgrpi0_name;
} LOCALGROUP_INFO_0;


typedef struct _LOCALGROUP_INFO_1 {
	wchar16_t *lgrpi1_name;
	wchar16_t *lgrpi1_comment;
} LOCALGROUP_INFO_1;



typedef struct _LOCALGROUP_MEMBERS_INFO_0 {
	DomSid *lgrmi0_sid;
} LOCALGROUP_MEMBERS_INFO_0;


typedef struct _LOCALGROUP_MEMBERS_INFO_3 {
	wchar16_t *lgrmi3_domainandname;
} LOCALGROUP_MEMBERS_INFO_3;

typedef int NET_API_STATUS;

NET_API_STATUS NetUserEnum(const wchar16_t *hostname,
			   uint32 level,
			   uint32 filter,
			   void **bufptr,
			   uint32 maxlen,
			   uint32 *entries,
			   uint32 *total,
			   uint32 *resume);

NET_API_STATUS NetUserAdd(const wchar16_t *hostname,
			  uint32 level,
			  void *bufptr,
			  uint32 *parm_err);

NET_API_STATUS NetUserDel(const wchar16_t *hostname,
			  const wchar16_t *username);

NET_API_STATUS NetUserGetInfo(const wchar16_t *hostname,
			      const wchar16_t *username,
			      uint32 level,
			      void **bufptr);

NET_API_STATUS NetUserSetInfo(const wchar16_t *servername,
			      const wchar16_t *username,
			      uint32 level,
			      void *bufptr,
			      uint32 *parm_err);

NET_API_STATUS NetUserGetLocalGroups(const wchar16_t *servername,
				     const wchar16_t *username,
				     uint32 level,
				     uint32 flags,
				     void **bufptr,
				     uint32 prefmaxlen,
				     uint32 *entriesread,
				     uint32 *totalentries);

NET_API_STATUS NetLocalGroupAdd(const wchar16_t *servername,
				uint32 level,
				void *bufptr,
				uint32 *parm_err);

NET_API_STATUS NetLocalGroupDel(const wchar16_t *servername,
				const wchar16_t *groupname);

NET_API_STATUS NetLocalGroupEnum(const wchar16_t *servername,
				 uint32 level,
				 void **bufptr,
				 uint32 prefmaxlen,
				 uint32 *entriesread,
				 uint32 *totalentries,
				 uint32 *resumehandle);

NET_API_STATUS NetLocalGroupSetInfo(const wchar16_t *servername,
				    const wchar16_t *groupname,
				    uint32 level,
				    void *bufptr,
				    uint32 *parm_err);

NET_API_STATUS NetLocalGroupGetInfo(const wchar16_t *servername,
				    const wchar16_t *groupname,
				    uint32 level,
				    void **bufptr);

NET_API_STATUS NetLocalGroupAddMembers(const wchar16_t *servername,
				       const wchar16_t *groupname,
				       uint32 level,
				       void *bufptr,
				       uint32 totalentries);

NET_API_STATUS NetLocalGroupDelMembers(const wchar16_t *servername,
				       const wchar16_t *groupname,
				       uint32 level,
				       void *bufptr,
				       uint32 totalentries);

NET_API_STATUS NetLocalGroupGetMembers(const wchar16_t *servername,
				       const wchar16_t *localgroupname,
				       uint32 level,
				       void **bufptr,
				       uint32 prefmaxlen,
				       uint32 *entriesread,
				       uint32 *totalentries,
				       uint32 *resumehandle);

NET_API_STATUS NetApiBufferFree(void *bufptr);

NET_API_STATUS NetUserChangePassword(const wchar16_t *domain,
				     const wchar16_t *username,
				     const wchar16_t *oldpassword,
				     const wchar16_t *newpassword);

NET_API_STATUS NetGetDomainName(const wchar16_t *hostname, wchar16_t **domname);


/* this allows functions to return as much data as available */
#define MAX_PREFERRED_LENGTH                          (-1)

/* filter flags for NetUserEnum function */
#define FILTER_TEMP_DUPLICATE_ACCOUNT                 0x0001
#define FILTER_NORMAL_ACCOUNT                         0x0002
#define FILTER_INTERDOMAIN_TRUST_ACCOUNT              0x0008
#define FILTER_WORKSTATION_TRUST_ACCOUNT              0x0010
#define FILTER_SERVER_TRUST_ACCOUNT                   0x0020

/* account flags */
#define UF_SCRIPT                                     0x00000001
#define UF_ACCOUNTDISABLE                             0x00000002
#define UF_HOMEDIR_REQUIRED                           0x00000008
#define UF_LOCKOUT                                    0x00000010
#define UF_PASSWD_NOTREQD                             0x00000020
#define UF_PASSWD_CANT_CHANGE                         0x00000040
#define UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED            0x00000080
#define UF_TEMP_DUPLICATE_ACCOUNT                     0x00000100
#define UF_NORMAL_ACCOUNT                             0x00000200
#define UF_INTERDOMAIN_TRUST_ACCOUNT                  0x00000800
#define UF_WORKSTATION_TRUST_ACCOUNT                  0x00001000
#define UF_SERVER_TRUST_ACCOUNT                       0x00002000
#define UF_DONT_EXPIRE_PASSWD                         0x00010000
#define UF_SMARTCARD_REQUIRED                         0x00040000
#define UF_TRUSTED_FOR_DELEGATION                     0x00080000
#define UF_NOT_DELEGATED                              0x00100000
#define UF_USE_DES_KEY_ONLY                           0x00200000
#define UF_DONT_REQUIRE_PREAUTH                       0x00400000
#define UF_PASSWORD_EXPIRED                           0x00800000
#define UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION     0x01000000

/* user privileges flags */
#define USER_PRIV_GUEST                               0x00000000
#define USER_PRIV_USER                                0x00000001
#define USER_PRIV_ADMIN                               0x00000002

#endif /* _LM_ACCESS_H_ */
