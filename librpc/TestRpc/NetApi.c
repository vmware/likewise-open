/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <dce/dce_error.h>
#include <wc16str.h>
#include <lw/ntstatus.h>

#include <lwrpc/types.h>
#include <lwrpc/security.h>
#include <lwrpc/allocate.h>
#include <lwrpc/samr.h>
#include <lwrpc/lsa.h>
#include <lwrpc/samrbinding.h>
#include <lwrpc/lsabinding.h>
#include <lwrpc/LM.h>
#include <md5.h>
#include <lwrpc/mpr.h>
#include <lwps/lwps.h>

#include "TestRpc.h"
#include "Params.h"
#include "Util.h"


int GetUserLocalGroups(const wchar16_t *hostname, wchar16_t *username,
                       LOCALGROUP_USERS_INFO_0 *grpinfo, uint32 *entries)
{
    const uint32 level = 1;
    const uint32 flags = 0;
    const uint32 pref_maxlen = (uint32)(-1);

    NET_API_STATUS err = ERROR_SUCCESS;
    uint32 total, parm_err;
    int i = 0;

    grpinfo = NULL;
    *entries = total = parm_err = 0;    

    INPUT_ARG_WSTR(hostname);
    INPUT_ARG_WSTR(username);
    INPUT_ARG_UINT(i);
    INPUT_ARG_UINT(flags);
    INPUT_ARG_PTR(grpinfo);
    INPUT_ARG_UINT(pref_maxlen);
    
    CALL_NETAPI(err = NetUserGetLocalGroups(hostname, username, level, flags,
                                            (void*)&grpinfo, pref_maxlen,
                                            entries, &total));

    OUTPUT_ARG_PTR(grpinfo);
    OUTPUT_ARG_UINT(*entries);
    OUTPUT_ARG_UINT(total);

    if (grpinfo != NULL && *entries > 0 && total > 0) {
        VERBOSE(printf("\tGroups found:\n"));
	
        for (i = 0; i < *entries; i++) {

            wchar16_t *name;

            name = grpinfo[i].lgrui0_name;
	     
            if (name != NULL) {
		
                if(((uint16) name[0]) == 0) {
                    w16printfw(L"\tERROR: LOCALGROUP_USERS_INFO_0[%2d]"
                              L".lgrui0_name = \"\" (empty string)\n", i);
                    return -1;

                } else {
                    VERBOSE(w16printfw(L"\tLOCALGROUP_USERS_INFO_0[%2d]"
                                      L".lgrui0_name = \"%ws\"\n", i, name));
                }
            } else {
                printf("\tERROR: LOCALGROUP_USERS_INFO_0[%2d].lgrui0_name = NULL\n", i);
                return -1;
            }
	   

        }
    } else if (grpinfo != NULL && (*entries == 0 || total == 0)) {
        printf("\tInconsistency found:\n"
               "\tNumber of returned entries is zero while"
               "buffer pointer is non-null\n");
        return -1;

    } else if (grpinfo == NULL && (*entries != 0 || total != 0)) {
        printf("\tInconsistency found:\n"
               "\tNumber of returned entries is non-zero while"
               "buffer pointer is null\n");
        return -1;
    }
 
    return err;
}


int GetLocalGroupMembers(const wchar16_t *hostname, const wchar16_t *aliasname,
                         LOCALGROUP_MEMBERS_INFO_3* info, uint32 *entries)
{
    const uint32 level = 3;
    const uint32 prefmaxlen = (uint32)(-1);
    NET_API_STATUS err;
    uint32 total, resume;
    int i = 0;

    resume = 0;
    info = NULL;

    INPUT_ARG_WSTR(hostname);
    INPUT_ARG_WSTR(aliasname);
    INPUT_ARG_UINT(level);
    INPUT_ARG_PTR(info);
    INPUT_ARG_UINT(prefmaxlen);
    
    CALL_NETAPI(err = NetLocalGroupGetMembers(hostname, aliasname, level,
                                              (void*) &info,
                                              prefmaxlen, entries,
                                              &total, &resume));

    OUTPUT_ARG_PTR(info);
    OUTPUT_ARG_UINT(*entries);
    OUTPUT_ARG_UINT(total);
    OUTPUT_ARG_UINT(resume);
    
    if (info != NULL && entries > 0 && total > 0) {
        VERBOSE(printf("\tMembers found:\n"));

        for (i = 0; i < *entries; i++) {
            wchar16_t *name;

            name = info[i].lgrmi3_domainandname;
            if (name != NULL) {
                
                if(((uint16) name[0]) == 0) {
                    w16printfw(L"\tERROR: LOCALGROUP_MEMBERS_INFO_3[%2d]"
                              L".lgrmi3_domainandname = \"\"  (empty string)\n", i);
                    return -1;
                }

                else if(name[wc16slen(name) - 1] == (wchar16_t)'\\') {
                    //this is a ghost entry from a user which has been removed.  Ignore it.
                }
		
                else {
                    VERBOSE(w16printfw(L"\tLOCALGROUP_MEMBERS_INFO_3[%2d]"
                                      L".lgrmi3_domainandname = \"%ws\"\n", i, name));
                }
            } 
            else {
                w16printfw(L"\tERROR: LOCALGROUP_MEMBERS_INFO_3[%2d]"
                          L".lgrmi3_domainandname = NULL\n", i);
                return -1;
            }
        }

    } else if (info == NULL && (entries != 0 || total != 0)) {
        printf("\tInconsistency found:\n"
               "\tNumber of returned entries is non-zero while buffer pointer is null\n");
        return -1;
    }

    return err;
}


int AddUser(const wchar16_t *hostname, const wchar16_t *username)
{
    const char *comment = "sample comment";
    const char *home_directory = "c:\\";
    const char *script_path = "\\\\server\\share\\dir\\script.cmd";
    const char *password = "TestPassword06-?";
    const uint32 flags = UF_NORMAL_ACCOUNT;

    NET_API_STATUS err = ERROR_SUCCESS;
    uint32 level, parm_err;
    size_t comment_len, home_directory_len, script_path_len, password_len;
    USER_INFO_1 *info1;

    level = 1;
    info1 = (USER_INFO_1*) malloc(sizeof(USER_INFO_1));

    memset(info1, 0, sizeof(USER_INFO_1));
    info1->usri1_name = wc16sdup(username);

    comment_len = strlen(comment);
    info1->usri1_comment = (wchar16_t*) malloc((comment_len + 1) * sizeof(wchar16_t));
    mbstowc16s(info1->usri1_comment, comment, comment_len + 1);
    
    home_directory_len = strlen(home_directory);
    info1->usri1_home_dir = (wchar16_t*) malloc((home_directory_len + 1) * sizeof(wchar16_t));
    mbstowc16s(info1->usri1_home_dir, home_directory, home_directory_len);

    script_path_len = strlen(script_path);
    info1->usri1_script_path = (wchar16_t*) malloc((script_path_len + 1) * sizeof(wchar16_t));
    mbstowc16s(info1->usri1_script_path, script_path, script_path_len + 1);
    
    password_len = strlen(password);
    info1->usri1_password = (wchar16_t*) malloc((password_len + 1) * sizeof(wchar16_t));
    mbstowc16s(info1->usri1_password, password, password_len + 1);

    info1->usri1_flags = flags;
    info1->usri1_priv = USER_PRIV_USER;

    CALL_NETAPI(err = NetUserAdd(hostname, level, (void*)info1, &parm_err));
        
    SAFE_FREE(info1->usri1_comment);
    SAFE_FREE(info1->usri1_home_dir);
    SAFE_FREE(info1->usri1_script_path);
    SAFE_FREE(info1->usri1_password);
    SAFE_FREE(info1->usri1_name); 
    SAFE_FREE(info1);

    return err;
}


int DelUser(const wchar16_t *hostname, const wchar16_t *username)
{
    return NetUserDel(hostname, username);
}


int AddLocalGroup(const wchar16_t *hostname, const wchar16_t *aliasname)
{
    const char *testcomment = "Sample comment";
    const uint32 level = 1;

    NET_API_STATUS err = ERROR_SUCCESS;
    LOCALGROUP_INFO_1 info;
    uint32 parm_err;
    size_t comment_size;
    wchar16_t *comment = NULL;

    comment_size = (strlen(testcomment) + 1) * sizeof(wchar16_t);
    comment = (wchar16_t*) malloc(comment_size);
    mbstowc16s(comment, testcomment, comment_size);

    info.lgrpi1_name    = wc16sdup(aliasname);
    info.lgrpi1_comment = comment;

    INPUT_ARG_WSTR(hostname);
    INPUT_ARG_UINT(level);
    INPUT_ARG_WSTR(info.lgrpi1_name);
    INPUT_ARG_WSTR(info.lgrpi1_comment);
    INPUT_ARG_UINT(parm_err);

    CALL_NETAPI(err = NetLocalGroupAdd(hostname, level, &info, &parm_err));

    OUTPUT_ARG_UINT(parm_err);

    SAFE_FREE(info.lgrpi1_name);
    SAFE_FREE(info.lgrpi1_comment);

    return err;
}


int DelLocalGroup(const wchar16_t *hostname, const wchar16_t *aliasname)
{
    return NetLocalGroupDel(hostname, aliasname);
}


void DoCleanup(const wchar16_t *hostname, const wchar16_t *aliasname,
               const wchar16_t *username)
{
    DelUser(hostname, username);
    DelLocalGroup(hostname, aliasname);
}


int AddLocalGroupMember(const wchar16_t *hostname, const wchar16_t *aliasname,
                        const wchar16_t *domname, const wchar16_t *member)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    LOCALGROUP_MEMBERS_INFO_3 memberinfo = {0};

    wchar16_t domain_member[512];

    sw16printfw(
            domain_member,
            sizeof(domain_member)/sizeof(domain_member[0]),
            L"%ws\\%ws",
            domname,
            member);
    memberinfo.lgrmi3_domainandname = (wchar16_t*)domain_member;

    CALL_NETAPI(err = NetLocalGroupAddMembers(hostname, aliasname, 3,
                                              &memberinfo, 1));

    return err;
}


int DelLocalGroupMember(const wchar16_t *hostname,
                        const wchar16_t *domname,
                        const wchar16_t *aliasname,
                        const wchar16_t *member)
{
    NET_API_STATUS err = ERROR_SUCCESS;
    LOCALGROUP_MEMBERS_INFO_3 memberinfo = {0};

    wchar16_t host_member[512];

    sw16printfw(
            host_member,
            sizeof(host_member)/sizeof(host_member[0]),
            L"%ws\\%ws",
            domname,
            member);
    memberinfo.lgrmi3_domainandname = host_member;
	
    CALL_NETAPI(err = NetLocalGroupDelMembers(hostname, aliasname, 3,
                                              &memberinfo, 1));

    return err;
}


void DumpNetUserInfo1(const char *prefix, USER_INFO_1 *info)
{
    wchar16_t *usri1_name = info->usri1_name;
    wchar16_t *usri1_password = info->usri1_password;

    DUMP_WSTR(prefix, usri1_name);
    DUMP_WSTR(prefix, usri1_password);
}


int TestNetUserAdd(struct test *t, const wchar16_t *hostname,
                   const wchar16_t *user, const wchar16_t *pass,
                   struct parameter *options, int optcount)

{
    const char *testusername = "TestUser";
    const char *comment = "sample comment";
    const char *home_directory = "c:\\";
    const char *script_path = "\\\\server\\share\\dir\\script.cmd";
    const char *testpassword = "TestPassword06-?";
    const uint32 flags = UF_NORMAL_ACCOUNT;

    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    uint32 level, parm_err;
    USER_INFO_1 *info1 = NULL;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    level = 1;
    info1 = (USER_INFO_1*) malloc(sizeof(USER_INFO_1));
    if (info1 == NULL) return false;

    memset(info1, 0, sizeof(USER_INFO_1));

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &info1->usri1_name, &testusername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "comment", pt_w16string,
                       &info1->usri1_comment, &comment);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "homedir", pt_w16string,
                       &info1->usri1_home_dir, &home_directory);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "scriptpath", pt_w16string,
                       &info1->usri1_script_path, &script_path);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string,
                       &info1->usri1_password, &testpassword);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, &info1->usri1_name);
    PARAM_INFO("comment", pt_w16string, &info1->usri1_comment);
    PARAM_INFO("homedir", pt_w16string, &info1->usri1_home_dir);
    PARAM_INFO("scriptpath", pt_w16string, &info1->usri1_script_path);
    PARAM_INFO("password", pt_w16string, &info1->usri1_password);

    info1->usri1_flags = flags;
    info1->usri1_priv = USER_PRIV_USER;

    status = CleanupAccount(hostname, info1->usri1_name);
    if (status != 0) rpc_fail(status);

    CALL_NETAPI(err = NetUserAdd(hostname, level, (void*)info1, &parm_err));
    if (err != 0) netapi_fail(err);

    status = CleanupAccount(hostname, info1->usri1_name);
    if (status != 0) rpc_fail(status);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(info1->usri1_name);
    SAFE_FREE(info1->usri1_comment);
    SAFE_FREE(info1->usri1_home_dir);
    SAFE_FREE(info1->usri1_script_path);
    SAFE_FREE(info1->usri1_password);
    SAFE_FREE(info1);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetUserDel(struct test *t, const wchar16_t *hostname,
                   const wchar16_t *user, const wchar16_t *pass,
                   struct parameter *options, int optcount)
{
    const char *testusername = "TestUser";

    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    wchar16_t *username;
    int created = false;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &username, &testusername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, username);

    status = EnsureUserAccount(hostname, username, &created);
    if (status != 0) rpc_fail(status);

    CALL_NETAPI(err = NetUserDel(hostname, username));
    if (err != 0) netapi_fail(err);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(username);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetUserGetInfo(struct test *t, const wchar16_t *hostname,
                       const wchar16_t *user, const wchar16_t *pass,
                       struct parameter *options, int optcount)
{
    const char *testusername = "TestUser";

    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    wchar16_t *username;
    void *info;
    int created = false;
    uint32 level = 20;
	
    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &username, &testusername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, username);

    status = EnsureUserAccount(hostname, username, &created);
    if (status != 0) rpc_fail(status);

    CALL_NETAPI(err = NetUserGetInfo(hostname, username, level, &info));
    if (err != 0) netapi_fail(err);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(username);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetUserSetInfo(struct test *t, const wchar16_t *hostname,
                       const wchar16_t *user, const wchar16_t *pass,
                       struct parameter *options, int optcount)
{
    const char *oldusername = "OldUserName";
    const char *newusername = "NewUserName";
    const char *newcomment = "Sample comment";
    const char *newfullname = "Full UserName";
    const char *newpassword = "JustTesting30$";

    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    uint32 level, parm_err;
    wchar16_t buffer[512] = {0};
    wchar16_t *username = NULL;
    wchar16_t *newuser = NULL;
    USER_INFO_0 info0 = {0};
    USER_INFO_20 *info20 = NULL;
    USER_INFO_1003 info1003 = {0};
    USER_INFO_1007 info1007 = {0};
    USER_INFO_1008 info1008 = {0};
    USER_INFO_1011 info1011 = {0};
    int created = false;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &username, &oldusername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, username);

    status = EnsureUserAccount(hostname, username, &created);
    if (status != 0) rpc_fail(status);

    newuser = ambstowc16s(newusername);
    if (newuser == NULL) return false;

    level = 0;
    info0.usri0_name = newuser;

    CALL_NETAPI(err = NetUserSetInfo(hostname, username, level,
                                     (void*)&info0, &parm_err));
    if (err != 0) netapi_fail(err);

    /* the user name is different now - the next will use different pointer
       holding the new name thus allowing for account deletion */
    SAFE_FREE(username);
    username = newuser;

    level = 1003;
    memset((void*)buffer, 0, sizeof(buffer));
    mbstowc16s(buffer, newpassword, sizeof(buffer));
    info1003.usri1003_password = buffer;

    CALL_NETAPI(err = NetUserSetInfo(hostname, username, level,
                                     (void*)&info1003, &parm_err));
    if (err != 0) netapi_fail(err);

    level = 1007;
    mbstowc16s(buffer, newcomment, sizeof(buffer));
    info1007.usri1007_comment = buffer;

    CALL_NETAPI(err = NetUserSetInfo(hostname, username, level,
                                     (void*)&info1007, &parm_err));
    if (err != 0) netapi_fail(err);

    err = NetUserGetInfo(hostname, username, 20, (void**)&info20);
    if (err == ERROR_SUCCESS) {

        level = 1008;
        info1008.usri1008_flags = info20->usri20_flags | UF_ACCOUNTDISABLE;
	
        CALL_NETAPI(err = NetUserSetInfo(hostname, username, level,
                                         (void*)&info1008, &parm_err));
        if (err != 0) netapi_fail(err);
	}

	info20 = NULL;
	CALL_NETAPI(err = NetUserGetInfo(hostname, username, 20, (void**)&info20));
	if (err != 0) {
	    printf("WARNING: Unable to verify whether NetUserSetInfo test "
               "on USER_INFO_1008 passes correctly\n");

    }

	if (info20->usri20_flags != info1008.usri1008_flags) {
	    printf("USER_INFO_1008 level succeeded but didn't set flags correctly\n");
	    return false;
    }

    level = 1011;
    mbstowc16s(buffer, newfullname, sizeof(buffer));
    info1011.usri1011_full_name = buffer;

    CALL_NETAPI(err = NetUserSetInfo(hostname, username, level,
                                     (void*)&info1011, &parm_err));
    if (err != 0) netapi_fail(err);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(newuser);
    SAFE_FREE(username);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetJoinDomain(struct test *t, const wchar16_t *hostname,
                      const wchar16_t *user, const wchar16_t *pass,
                      struct parameter *options, int optcount)
{
    const char *def_accountou = NULL;
    const int def_rejoin = 0;
    const int def_create = 1;
    const int def_deferspn = 0;

    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    wchar16_t *username, *password, *accountou;
    wchar16_t *domain_name, *machine_account, *machine_password;
    int rejoin, create, deferspn;
    int opts;
    HANDLE store = (HANDLE)NULL;
    LWPS_PASSWORD_INFO *pi = NULL;
    char host[128] = {0};

    TESTINFO(t, hostname, user, pass);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &username, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string,
                       &password, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "accountou", pt_w16string,
                       &accountou, &def_accountou);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "rejoin", pt_int32,
                       &rejoin, &def_rejoin);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "create", pt_int32,
                       &create, &def_create);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "deferspn", pt_int32,
                       &deferspn, &def_deferspn);

    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("password", pt_w16string, password);
    PARAM_INFO("accountout", pt_w16string, accountou);
    PARAM_INFO("rejoin", pt_int32, &rejoin);
    PARAM_INFO("create", pt_int32, &create);
    PARAM_INFO("deferspn", pt_int32, &deferspn);

    opts = NETSETUP_JOIN_DOMAIN;
    if (create) opts |= NETSETUP_ACCT_CREATE;
    if (rejoin) opts |= NETSETUP_DOMAIN_JOIN_IF_JOINED;
    if (deferspn) opts |= NETSETUP_DEFER_SPN_SET;

    CALL_NETAPI(err = NetJoinDomain(NULL, hostname, accountou,
                                    username, password, opts));
    if (err != 0) netapi_fail(err);

    status = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &store);
    if (status != STATUS_SUCCESS) return false;

    //    host = awc16stombs(hostname);
    //    if (host == NULL) return false;
    gethostname(host, sizeof(host));

    status = LwpsGetPasswordByHostName(store, host, &pi);
    if (status != STATUS_SUCCESS) return false;

    domain_name       = pi->pwszDomainName;
    machine_account   = pi->pwszMachineAccount;
    machine_password  = pi->pwszMachinePassword;
    RESULT_WSTR(domain_name);
    RESULT_WSTR(machine_account);
    RESULT_WSTR(machine_password);

done:
    status = LwpsClosePasswordStore(store);
    if (status != STATUS_SUCCESS) return false;

    SAFE_FREE(username);
    SAFE_FREE(password);
    SAFE_FREE(accountou);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetUnjoinDomain(struct test *t, const wchar16_t *hostname,
                        const wchar16_t *user, const wchar16_t *pass,
                        struct parameter *options, int optcount)
{
    const int def_disable = 1;

    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    wchar16_t *username, *password;
    int disable;
    int opts;

    TESTINFO(t, hostname, user, pass);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &username, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "password", pt_w16string,
                       &password, NULL);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "disable", pt_int32,
                       &disable, &def_disable);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("password", pt_w16string, password);
    PARAM_INFO("disable", pt_int32, &disable);

    opts = (disable) ? NETSETUP_ACCT_DELETE : 0;

    CALL_NETAPI(err = NetUnjoinDomain(NULL, username, password, opts));
    if (err != 0) netapi_fail(err);

done:
    SAFE_FREE(username);
    SAFE_FREE(password);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetMachineChangePassword(struct test *t, const wchar16_t *hostname,
                                 const wchar16_t *user, const wchar16_t *pass,
                                 struct parameter *options, int optcount)
{
    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;

    TESTINFO(t, hostname, user, pass);

    CALL_NETAPI(err = NetMachineChangePassword());
    if (err != ERROR_SUCCESS) netapi_fail(err);

done:   
    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetUserChangePassword(struct test *t, const wchar16_t *hostname,
                              const wchar16_t *user, const wchar16_t *pass,
                              struct parameter *options, int optcount)
{
    const char *defusername = "TestUser";
    const char *defoldpass = "";
    const char *defnewpass = "newpassword";

    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    enum param_err perr = perr_success;
    wchar16_t *username, *oldpassword, *newpassword;
    int created = false;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &username, &defusername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "oldpassword", pt_w16string,
                       &oldpassword, &defoldpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "newpassword", pt_w16string,
                       &newpassword, &defnewpass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("oldpassword", pt_w16string, oldpassword);
    PARAM_INFO("newpassword", pt_w16string, newpassword);

    status = EnsureUserAccount(hostname, username, &created);
    if (status != 0) rpc_fail(status);

    CALL_NETAPI(err = NetUserChangePassword(hostname, username,
                                            oldpassword, newpassword));
    if (err != 0) netapi_fail(err);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(username);
    SAFE_FREE(oldpassword);
    SAFE_FREE(newpassword);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetUserLocalGroups(struct test *t, const wchar16_t *hostname,
                           const wchar16_t *user, const wchar16_t *pass,
                           struct parameter *options, int optcount)
{
    const char *defusername = "TestUser";
    const char *defaliasname = "TestAlias";
    const char *def_admin_user = "Administrator";
    const char *def_guest_user = "Guest";
    const char *def_guests_group = "Guests";
    const char *def_admins_group = "Administrators";

    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    LOCALGROUP_USERS_INFO_0 *grpinfo;
    uint32 entries;
    wchar16_t *username, *aliasname, *guest_user, *admin_user;
    wchar16_t *guests_group, *admins_group, *domname;
    int created = false;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    perr = fetch_value(options, optcount, "username", pt_w16string,
                       &username, &defusername);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "aliasname", pt_w16string,
                       &aliasname, &defaliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "guestuser", pt_w16string,
                       &guest_user, &def_guest_user);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "adminuser", pt_w16string,
                       &admin_user, &def_admin_user);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "guestsgroup", pt_w16string,
                       &guests_group, &def_guests_group);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "adminsgroup", pt_w16string,
                       &admins_group, &def_admins_group);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("aliasname", pt_w16string, aliasname);
    PARAM_INFO("guestuser", pt_w16string, guest_user);
    PARAM_INFO("adminuser", pt_w16string, admin_user);
    PARAM_INFO("guestsgroup", pt_w16string, guests_group);
    PARAM_INFO("adminsgroup", pt_w16string, admins_group);

    grpinfo = NULL;

    /*
     * Test 1a: Get groups of an existing and known user
     */

    err = GetUserLocalGroups(hostname, admin_user, (void*)&grpinfo, &entries);
    if (err != 0) netapi_fail(err);

    /*
     * Test 1b: Get groups of an existing and known user
     */

    err = GetUserLocalGroups(hostname, guest_user, (void*)&grpinfo, &entries);
    if (err != 0) netapi_fail(err);


    /*
     * Test 2: Get groups of newly created user has no group memberships yet
     */
    status = EnsureUserAccount(hostname, username, &created);
    if (status != 0) rpc_fail(status);
    
    
    err = GetUserLocalGroups(hostname, username, (void*)&grpinfo, &entries);
    if (err != 0) netapi_fail(err);

    if (grpinfo != NULL && entries > 0) {
        printf("Groups found while there should be none\n");
        return false;
    }

    /*
     * Test 3: Add user to 2 groups and get the local groups list
     */

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    err = AddLocalGroupMember(hostname, admins_group, domname, username);
    if (err != 0) netapi_fail(err);

    err = AddLocalGroupMember(hostname, guests_group, domname, username);
    if (err != 0) netapi_fail(err);

    err = GetUserLocalGroups(hostname, username, (void*)&grpinfo, &entries);
    if (err != 0) netapi_fail(err);

    if (entries != 2) {
        w16printfw(L"User %ws should be member of at 2 groups because"
                  L"they have been added to groups %ws and %ws",
                  username, admins_group, guests_group);
        return false;
    }


    /*
     * Test 4: Add 2 existing users to a newly created group, and get the local groups list
     */
    status = EnsureAlias(hostname, aliasname, &created);

    err = AddLocalGroupMember(hostname, aliasname, domname, admin_user);
    if (err != 0) netapi_fail(err);
        
    err = AddLocalGroupMember(hostname, aliasname, domname, guest_user);
    if (err != 0) netapi_fail(err);

    err = GetUserLocalGroups(hostname, admin_user, (void*)&grpinfo, &entries);
    if (err != 0) netapi_fail(err);

    if (entries != 1) {
        w16printfw(L"User %ws should be member of at least 1 alias because"
                  L"they have been added to group %ws",
                  admin_user, aliasname);
        return false;
    }

    err = GetUserLocalGroups(hostname, guest_user, (void*)&grpinfo, &entries);
    if (err != 0) netapi_fail(err);

    if (entries != 1) {
        w16printfw(L"User %ws should be member of at least 1 alias because"
                  L"they have been added to group %ws",
                  guest_user, aliasname);
        return false;
    }

done:
    DoCleanup(hostname, aliasname, username);

    RELEASE_SESSION_CREDS;

    SAFE_FREE(username);
    SAFE_FREE(aliasname);
    SAFE_FREE(admin_user);
    SAFE_FREE(guest_user);
    SAFE_FREE(admins_group);
    SAFE_FREE(guests_group);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetLocalGroupsEnum(struct test *t, const wchar16_t *hostname,
                           const wchar16_t *user, const wchar16_t *pass,
                           struct parameter *options, int optcount)
{
    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    void *info;
    uint32 entries, total, resume, maxlen;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    resume = 0;
    maxlen = MAX_PREFERRED_LENGTH;

    do {
        INPUT_ARG_UINT(resume);
        INPUT_ARG_UINT(maxlen);

        CALL_NETAPI(err = NetLocalGroupEnum(hostname, 1, &info, maxlen,
                                            &entries, &total, &resume));

        OUTPUT_ARG_UINT(entries);
        OUTPUT_ARG_UINT(total);

    } while (err == ERROR_MORE_DATA);

    resume = 0;
    maxlen = 32;

    do {
        INPUT_ARG_UINT(resume);
        INPUT_ARG_UINT(maxlen);

        CALL_NETAPI(err = NetLocalGroupEnum(hostname, 1, &info, maxlen,
                                            &entries, &total, &resume));

        OUTPUT_ARG_UINT(entries);
        OUTPUT_ARG_UINT(total);

    } while (err == ERROR_MORE_DATA);

    resume = 0;
    maxlen = 16;

    do {
        INPUT_ARG_UINT(resume);
        INPUT_ARG_UINT(maxlen);

        CALL_NETAPI(err = NetLocalGroupEnum(hostname, 1, &info, maxlen,
                                            &entries, &total, &resume));
	
        OUTPUT_ARG_UINT(entries);
        OUTPUT_ARG_UINT(total);

    } while (err == ERROR_MORE_DATA);


    resume = 0;
    maxlen = 4;

    do {
        INPUT_ARG_UINT(resume);
        INPUT_ARG_UINT(maxlen);

        CALL_NETAPI(err = NetLocalGroupEnum(hostname, 1, &info, maxlen,
                                            &entries, &total, &resume));

        OUTPUT_ARG_UINT(entries);
        OUTPUT_ARG_UINT(total);

    } while (err == ERROR_MORE_DATA);

done:
    RELEASE_SESSION_CREDS;

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetLocalGroupAdd(struct test *t, const wchar16_t *hostname,
                         const wchar16_t *user, const wchar16_t *pass,
                         struct parameter *options, int optcount)
{
    const char *def_aliasname = "TestAlias";

    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    wchar16_t *aliasname;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    perr = fetch_value(options, optcount, "aliasname", pt_w16string, &aliasname,
                       &def_aliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("aliasname", pt_w16string, aliasname);

    err = AddLocalGroup(hostname, aliasname);
    if (err != ERROR_SUCCESS) netapi_fail(err);

    /* cleanup */
    err = DelLocalGroup(hostname, aliasname);
    if (err != ERROR_SUCCESS) netapi_fail(err);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(aliasname);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestDelLocalGroup(struct test *t, const wchar16_t *hostname,
                      const wchar16_t *user, const wchar16_t *pass,
                      struct parameter *options, int optcount)
{
    const char *def_aliasname = "TestAlias";

    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    wchar16_t *aliasname;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    perr = fetch_value(options, optcount, "aliasname", pt_w16string, &aliasname,
                       &def_aliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("aliasname", pt_w16string, aliasname);

    err = DelLocalGroup(hostname, aliasname);
    if (err != ERROR_SUCCESS) netapi_fail(err);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(aliasname);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetLocalGroupGetInfo(struct test *t, const wchar16_t *hostname,
                             const wchar16_t *user, const wchar16_t *pass,
                             struct parameter *options, int optcount)
{
    const char *def_aliasname = "Guests";
    const uint32 level = 1;

    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    void *info;
    wchar16_t *aliasname;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    perr = fetch_value(options, optcount, "aliasname", pt_w16string, &aliasname,
                       &def_aliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("aliasname", pt_w16string, aliasname);

    INPUT_ARG_WSTR(hostname);
    INPUT_ARG_WSTR(aliasname);
    INPUT_ARG_UINT(level);
    INPUT_ARG_PTR(info);

    CALL_NETAPI(err = NetLocalGroupGetInfo(hostname, aliasname, level, &info));

    OUTPUT_ARG_PTR(info)

    if (info != NULL) {
        LOCALGROUP_INFO_1 *grpi = info;
        VERBOSE(printf("\tReceived info:\n"));
        VERBOSE(w16printfw(L"\t\tLOCALGROUP_INFO_1.lgrpi1_name = \"%ws\"\n", grpi->lgrpi1_name));
        VERBOSE(w16printfw(L"\t\tLOCALGROUP_INFO_1.lgrpi1_comment = \"%ws\"\n", grpi->lgrpi1_comment));
 
    } else {
        printf("\tERROR: Inconsistency found. Function succeeded while the returned"
               " buffer is NULL\n");
        printf("\n");
        return false;
    }

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(aliasname);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetLocalGroupSetInfo(struct test *t, const wchar16_t *hostname,
                             const wchar16_t *user, const wchar16_t *pass,
                             struct parameter *options, int optcount)
{
    const char *def_aliasname = "TestAlias";
    const char *def_comment = "Sample changed comment";
    const uint32 level = 1;

    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    uint32 parm_err;
    wchar16_t *aliasname, *comment;
    LOCALGROUP_INFO_1 info;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    perr = fetch_value(options, optcount, "aliasname", pt_w16string, &aliasname,
                       &def_aliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "comment", pt_w16string, &comment,
                       &def_comment);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("aliasname", pt_w16string, aliasname);
    PARAM_INFO("comment", pt_w16string, comment);

    info.lgrpi1_name    = aliasname;
    info.lgrpi1_comment = comment;

    INPUT_ARG_WSTR(hostname);
    INPUT_ARG_WSTR(aliasname);
    INPUT_ARG_UINT(level);
    INPUT_ARG_PTR(&info);
    INPUT_ARG_WSTR(info.lgrpi1_name);
    INPUT_ARG_WSTR(info.lgrpi1_comment);
    INPUT_ARG_UINT(parm_err);

    CALL_NETAPI(err = NetLocalGroupSetInfo(hostname, aliasname, 1, (void*)&info,
                                           &parm_err));

    OUTPUT_ARG_UINT(parm_err);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(comment);
    SAFE_FREE(aliasname);
  
    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetLocalGroupGetMembers(struct test *t, const wchar16_t *hostname,
                                const wchar16_t *user, const wchar16_t *pass,
                                struct parameter *options, int optcount)
{
    const char *def_admins_group = "Administrators";
    const char *def_guests_group = "Guests";
    const char *def_admin_user = "Administrator";
    const char *def_guest_user = "Guest";
    const char *def_aliasname = "TestAlias";
    const char *def_username = "TestUser";
    const wchar_t *padding = L"123";

    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    enum param_err perr = perr_success;
    LOCALGROUP_MEMBERS_INFO_3 grpmembers_info;
    uint32 entries = 0;
    wchar16_t *aliasname = NULL;
    wchar16_t *username = NULL;
    wchar16_t *domname = NULL;
    wchar16_t *admin_user = NULL;
    wchar16_t *guest_user = NULL;
    wchar16_t *admins_group = NULL;
    wchar16_t *guests_group = NULL;
    wchar16_t paddeduser[256];
    int newalias = 0;
    int newuser = 0;

    memset(&grpmembers_info, 0, sizeof(grpmembers_info));
    memset(paddeduser, 0, sizeof(paddeduser));

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    perr = fetch_value(options, optcount, "aliasname", pt_w16string, &aliasname,
                       &def_aliasname);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "username", pt_w16string, &username,
                       &def_username);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "adminuser", pt_w16string,
                       &admin_user, &def_admin_user);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "guestuser", pt_w16string, &guest_user,
                       &def_guest_user);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "adminsgroup", pt_w16string,
                       &admins_group, &def_admins_group);
    if (!perr_is_ok(perr)) perr_fail(perr);

    perr = fetch_value(options, optcount, "guestsgroup", pt_w16string,
                       &guests_group, &def_guests_group);
    if (!perr_is_ok(perr)) perr_fail(perr);

    PARAM_INFO("aliasname", pt_w16string, aliasname);
    PARAM_INFO("username", pt_w16string, username);
    PARAM_INFO("admin_user", pt_w16string, admin_user);
    PARAM_INFO("guest_user", pt_w16string, guest_user);
    PARAM_INFO("admins_group", pt_w16string, admins_group);
    PARAM_INFO("guests_group", pt_w16string, guests_group);

    VERBOSE(sw16printfw(
                paddeduser,
                sizeof(paddeduser)/sizeof(paddeduser[0]),
                L"%ws%ls",
                username,
                padding));
    
    /*
     * Test 1a: Get members of an existing and known to be non-empty group.
     */

    err = GetLocalGroupMembers(hostname, admins_group, (void*)&grpmembers_info,
                               &entries);
    if (err != 0) netapi_fail(err);

    /*
     * Test 1b: Get members of an existing and known to be non-empty group.
     */

    err = GetLocalGroupMembers(hostname, guests_group, (void*)&grpmembers_info,
                               &entries);
    if (err != 0) netapi_fail(err);

    /*
     * Test 2: Create a new alias, add two members, and get the list
     * of them back
     */

    status = EnsureAlias(hostname, aliasname, &newalias);
    if (status != 0) rpc_fail(status);

    status = GetSamDomainName(&domname, hostname);
    if (status != 0) rpc_fail(status);

    err = AddLocalGroupMember(hostname, aliasname, domname, admin_user);
    if (err != 0) netapi_fail(err);
	   
    err = AddLocalGroupMember(hostname, aliasname, domname, guest_user);
    if (err != 0) netapi_fail(err);

    err = GetLocalGroupMembers(hostname, aliasname, (void*)&grpmembers_info,
                               &entries);
    if (err != 0) netapi_fail(err);

    if (entries != 2) {
        printf("Total number of members is %d and should be 2\n", entries);
        return false;
    } 

    /*
     * Test 3: Create a new user, add it to three groups, and then get the members
     * of those groups.
     */
    status = EnsureUserAccount(hostname, username, &newuser);
    if (status != 0) rpc_fail(status);

    err = AddLocalGroupMember(hostname, admins_group, domname, username);
    if (err != 0) netapi_fail(err);

    err = GetLocalGroupMembers(hostname, admins_group, (void*)&grpmembers_info,
                               &entries);
    if (err != 0) netapi_fail(err);

    VERBOSE(w16printfw(L"\n\tTest 3b: Adding user \"%ws\" to group \"%ws\"\n",
                      username, guests_group));
    err = AddLocalGroupMember(hostname, guests_group, domname, username);
    if (err != 0) netapi_fail(err);

    VERBOSE(w16printfw(L"\tFetching members of %ws group...\n", guests_group));
    err = GetLocalGroupMembers(hostname, guests_group, (void*)&grpmembers_info,
                               &entries);
    if (err != 0) netapi_fail(err);


    /*
     * Test 4: Create a new user, and a new alias, where the alias contains
     * the username. Add the new user to the new alias, and the list of members
     * of the new alias.
     */
    VERBOSE(w16printfw(L"\n\tTest 4: Adding newly created user \"%ws\" to newly"
                      L"created group \"%ws\" and listing members of group\n",
                      username, paddeduser));
    err = AddLocalGroup(hostname, paddeduser);
    if (err != 0) netapi_fail(err);

    err = AddLocalGroupMember(hostname, paddeduser, domname, username);
    if (err != 0) netapi_fail(err);
	   
    err = GetLocalGroupMembers(hostname, paddeduser, (void*)&grpmembers_info,
                               &entries);
    if (err != 0) netapi_fail(err);

    if (entries != 1) {
        printf("Inconsistency found: "
               "Total number of members is %d and should be 1\n", entries);
        return false;
    } 

    err = DelLocalGroupMember(hostname, domname, admins_group, username);
    if (err != 0) netapi_fail(err);

    err = DelLocalGroupMember(hostname, domname, guests_group, username);
    if (err != 0) netapi_fail(err);

    err = DelLocalGroup(hostname, paddeduser);
    if (err != 0) netapi_fail(err);

    /* Cleanup accounts (if necessary) */
    if (newalias) CleanupAlias(hostname, aliasname);
    if (newuser) CleanupAccount(hostname, username);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(aliasname);
    SAFE_FREE(username);
    SAFE_FREE(domname);
    SAFE_FREE(admin_user);
    SAFE_FREE(guest_user);
    SAFE_FREE(admins_group);
    SAFE_FREE(guests_group);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


int TestNetGetDomainName(struct test *t, const wchar16_t *hostname,
                         const wchar16_t *user, const wchar16_t *pass,
                         struct parameter *options, int optcounta)
{
    NTSTATUS status = STATUS_SUCCESS;
    NET_API_STATUS err = ERROR_SUCCESS;
    wchar16_t *domain_name;

    TESTINFO(t, hostname, user, pass);

    SET_SESSION_CREDS(pCreds);

    CALL_NETAPI(err = NetGetDomainName(hostname, &domain_name));
    if (err != 0) netapi_fail(err);

    OUTPUT_ARG_WSTR(domain_name);

done:
    RELEASE_SESSION_CREDS;

    SAFE_FREE(domain_name);

    return (err == ERROR_SUCCESS &&
            status == STATUS_SUCCESS);
}


void SetupNetApiTests(struct test *t)
{
    NTSTATUS status = STATUS_SUCCESS;

    status = NetInitMemory();
    if (status) return;

    AddTest(t, "NETAPI-USER-ADD", TestNetUserAdd);
    AddTest(t, "NETAPI-USER-DEL", TestNetUserDel);
    AddTest(t, "NETAPI-USER-GETINFO", TestNetUserGetInfo);
    AddTest(t, "NETAPI-USER-SETINFO", TestNetUserSetInfo);
    AddTest(t, "NETAPI-JOIN-DOMAIN", TestNetJoinDomain);
    AddTest(t, "NETAPI-UNJOIN-DOMAIN", TestNetUnjoinDomain);
    AddTest(t, "NETAPI-MACHINE-CHANGE-PASSWORD", TestNetMachineChangePassword);
    AddTest(t, "NETAPI-USER-CHANGE-PASSWORD", TestNetUserChangePassword);
    AddTest(t, "NETAPI-GET-DOMAIN-NAME", TestNetGetDomainName);
    AddTest(t, "NETAPI-USER-LOCAL-GROUPS", TestNetUserLocalGroups);
    AddTest(t, "NETAPI-LOCAL-GROUPS-ENUM", TestNetLocalGroupsEnum);
    AddTest(t, "NETAPI-LOCAL-GROUP-ADD", TestNetLocalGroupAdd);
    AddTest(t, "NETAPI-LOCAL-GROUP-GETINFO", TestNetLocalGroupGetInfo);
    AddTest(t, "NETAPI-LOCAL-GROUP-SETINFO", TestNetLocalGroupSetInfo);
    AddTest(t, "NETAPI-LOCAL-GROUP-MEMBERS", TestNetLocalGroupGetMembers);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
