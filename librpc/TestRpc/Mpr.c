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

#include <compat/rpcstatus.h>
#include <dce/dce_error.h>
#include <wc16str.h>
#include <lw/ntstatus.h>

#include <lwrpc/types.h>
#include <lwrpc/allocate.h>
#include <lwrpc/samr.h>
#include <lwrpc/lsa.h>
#include <lwrpc/mpr.h>
#include <md5.h>

#include "TestRpc.h"
#include "Params.h"


int TestMprIncorrectAuthSession(struct test *t, const wchar16_t *hostname,
				const wchar16_t *user, const wchar16_t *pass,
				struct parameter *options, int optcount)
{
    const uint32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;
    /*
      Since we don't support passing arbitrary parameters yet
      change these if you have such an account on your testing
      system.
      We're testing INVALID login here.
    */
    const char *incuser = "incorrectacc";
    const char *incpass = "incorrecpass";

    NTSTATUS status;
    int ret;
    wchar16_t *username, *password;
    size_t hostname_size;
    NETRESOURCE nr = {0};
    PolicyHandle conn_handle = {0};
    handle_t samr_binding;

    hostname_size = wc16slen(hostname);

    nr.RemoteName = (wchar16_t*) malloc((hostname_size + 8) * sizeof(wchar16_t));
    if (nr.RemoteName == NULL) return false;

    sw16printf(nr.RemoteName, "\\\\%S\\IPC$", hostname);

    username = ambstowc16s(incuser);
    password = ambstowc16s(incpass);

    printfw16("Adding connection with incorrect credentials"
              "user:[%S], pass:[%S]\n", user, pass);

    ret = WNetAddConnection2(&nr, password, username);
    printf("WNetAddConnection2: ret = %d\n", ret);

    /* At this point no rpc call should succeed */

    if (ret == 0) return false;

    SAFE_FREE(username);
    SAFE_FREE(password);

    samr_binding = CreateSamrBinding(&samr_binding, hostname);
    if (samr_binding == NULL) return false;

    status = SamrConnect2(samr_binding, hostname, conn_access, &conn_handle);
    if (status == 0) return false;

    printf("Cancelling connection with incorrect credentials\n");
    ret = WNetCancelConnection2(nr.RemoteName, 0, 0);
    printf("WNetCancelConnection2: ret = %d\n", ret);

    return true;
}


int TestMprCorrectAuthSession(struct test *t, const wchar16_t *hostname,
			      const wchar16_t *user, const wchar16_t *pass,
			      struct parameter *options, int optcount)
{
    const uint32 conn_access = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;
    /*
      Since we don't support passing arbitrary parameters yet
      change these to any valid account on your testing system.
      We're testing VALID login here.
    */
    NTSTATUS status;
    int ret;
    enum param_err perr;
    wchar16_t *username, *password;
    size_t hostname_size;
    NETRESOURCE nr = {0};
    PolicyHandle conn_handle = {0};
    handle_t samr_binding;

    /* it's possible to either use session related credentials as defaults or
       supply another (correct) credentials to test */
    perr = fetch_value(options, optcount, "username", pt_w16string, &username, &user);
    if (!perr_is_ok(perr)) perr_fail(perr);
    perr = fetch_value(options, optcount, "password", pt_w16string, &password, &pass);
    if (!perr_is_ok(perr)) perr_fail(perr);

    hostname_size = wc16slen(hostname);

    nr.RemoteName = (wchar16_t*) malloc((hostname_size + 8) * sizeof(wchar16_t));
    if (nr.RemoteName == NULL) return false;

    sw16printf(nr.RemoteName, "\\\\%S\\IPC$", hostname);

    printfw16("Adding connection with correct credentials user:[%S]/pass:[%S]\n",
	       username, password);
    ret = WNetAddConnection2(&nr, password, username);
    printf("WNetAddConnection2: ret = %d\n", ret);

    if (ret != 0) return false;

    SAFE_FREE(username);
    SAFE_FREE(password);

    samr_binding = CreateSamrBinding(&samr_binding, hostname);
    if (samr_binding == NULL) return false;

    status = SamrConnect2(samr_binding, hostname, conn_access, &conn_handle);
    if (status != 0) rpc_fail(status);

    printf("Cancelling connection with correct credentials\n");
    ret = WNetCancelConnection2(nr.RemoteName, 0, 0);
    printf("WNetCancelConnection2: ret = %d\n", ret);

done:
    FreeSamrBinding(&samr_binding);
    SAFE_FREE(nr.RemoteName);

    return true;
}


void SetupMprTests(struct test *t)
{
    AddTest(t, "MPR-INCORR-AUTH-SESS", TestMprIncorrectAuthSession);
    AddTest(t, "MPR-CORR-AUTH-SESS", TestMprCorrectAuthSession);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
