/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <compat/rpcstatus.h>
#include <dce/dce_error.h>
#include <wc16str.h>
#include <lwio/lwio.h>
#include <lw/ntstatus.h>

#include <srvsvc/types.h>

#include "Params.h"
#include "TestSrvSvc.h"

void AddTest(struct test *ft, const char *name, test_fn function)
{
    struct test *nt, *lt;

    if (ft == NULL) return;

    lt = ft;
    while (lt && lt->next) lt = lt->next;

    if (lt->name && lt->function) {
	/* allocate the new test */
	nt = malloc(sizeof(struct test));
	if (nt == NULL) return;

	/* append the test to the list */
	lt->next = nt;

    } else {
	/* this is the very first test so use already
	   allocated node */
	nt = lt;
    }

    /* init the new test */
    nt->name     = name;
    nt->function = function;
    nt->next     = NULL;
}

static void FreeTests(struct test *ft)
{
    struct test *ct;
    struct test *nt = ft;

    while (nt) {
        ct = nt;
        nt = ct->next;
        free(ct);
    }
}

struct test* FindTest(struct test *ft, const char *name)
{
    struct test *t = ft;

    /* search through the tests and try to find
       a matching name */
    while (t) {
        if (strcasecmp(t->name, name) == 0) {
            return t;
        }

        t = t->next;
    }

    return NULL;
}


int StartTest(struct test *t, const wchar16_t *hostname,
	      const wchar16_t *username, const wchar16_t *password,
	      struct parameter *options, int optcount)
{
    int ret;

    if (t == NULL || hostname == NULL) return -1;

    ret = t->function(t, hostname, username, password,
		      options, optcount);
    printf("%s\n", (ret) ? "SUCCEEDED" : "FAILED");
    return ret;
}



void display_usage()
{
    printf("Usage: testrpc [-v] -h hostname [-k] [-u username] [-p password]\n"
           "               [-d domain] [-w workstation] [-r principal] [-c creds cache]\n"
           "               [-o options] testname\n");
    printf("\thostname - host to connect when performing a test\n");
}


UserCreds *pCreds = NULL;

extern char *optarg;
int verbose_mode;

int main(int argc, char *argv[])
{
    int i, opt, ret;
    char *testname = NULL;
    char *host = NULL;
    char *optional_args = NULL;
    char *user = NULL;
    char *pass = NULL;
    char *dom = NULL;
    char *wks = NULL;
    char *princ = NULL;
    char *cache = NULL;
    int krb5_auth = 1;
    struct test *tests = NULL;
    struct test *runtest = NULL;
    wchar16_t *hostname = NULL;
    wchar16_t *username = NULL;
    wchar16_t *password = NULL;
    struct parameter *params = NULL;
    int params_len = 0;

    verbose_mode = false;

    while ((opt = getopt(argc, argv, "h:o:vu:p:d:w:r:c:k")) != -1) {
        switch (opt) {
        case 'h':
            host = optarg;
            break;

        case 'o':
            optional_args = optarg;
            break;

        case 'v':
            verbose_mode = true;
            break;

        case 'd':
            dom = optarg;
            break;

        case 'w':
            wks = optarg;
            break;

        case 'u':
            user = optarg;
            break;

        case 'p':
            pass = optarg;
            break;

        case 'r':
            princ = optarg;
            break;

        case 'c':
            cache = optarg;
            break;

        case 'k':
            krb5_auth = 1;
            break;

        default:
            display_usage();
            return -1;
        }
    }

    tests = malloc(sizeof(struct test));
    if (tests == NULL) {
        printf("Failed to allocate tests\n");
        return -1;
    }

    tests->name     = NULL;
    tests->function = NULL;
    tests->next     = NULL;

    if (host == NULL) {
        printf("Error: no hostname specified\n\n");
        display_usage();
        return -1;
    }

    hostname = ambstowc16s(host);
    if (host && hostname == NULL) {
        printf("Failed to allocate hostname\n");
        goto done;
    }

    pCreds = malloc(sizeof(UserCreds));
    if (pCreds == NULL) {
        printf("Failed to allocate UserCreds\n");
        goto done;
    }

    memset(pCreds, 0, sizeof(*pCreds));

    if (user) {
        pCreds->username = malloc(strlen(user) + 1);
        if (pCreds->username == NULL) {
            printf("Failed to allocate username for user credentials\n");
            goto done;
        }

        strcpy(pCreds->username, user);
    }

    if (pass) {
        pCreds->password = malloc(strlen(pass) + 1);
        if (pCreds->password == NULL) {
            printf("Failed to allocate password for user credentials\n");
            goto done;
        }

        strcpy(pCreds->password, pass);
    }

    if (dom) {
        pCreds->domain = malloc(strlen(dom) + 1);
        if (pCreds->domain == NULL) {
            printf("Failed to allocate domain for user credentials\n");
            goto done;
        }

        strcpy(pCreds->domain, dom);
    }

    if (wks) {
        pCreds->workstation = malloc(strlen(wks) + 1);
        if (pCreds->workstation == NULL) {
            printf("Failed to allocate workstation for user credentials\n");
            goto done;
        }

        strcpy(pCreds->workstation, wks);
    }

    if (princ) {
        pCreds->principal = malloc(strlen(princ) + 1);
        if (pCreds->principal == NULL) {
            printf("Failed to allocate principal for user credentials\n");
            goto done;
        }

        strcpy(pCreds->principal, princ);
    }

    if (cache) {
        pCreds->ccache = malloc(strlen(cache) + 1);
        if (pCreds->ccache == NULL) {
            printf("Failed to allocate credentials cache for user credentials\n");
            goto done;
        }

        strcpy(pCreds->ccache, cache);
    }

    pCreds->use_kerberos = krb5_auth;

    params = get_optional_params(optional_args, &params_len);
    if ((params != NULL && params_len == 0) ||
        (params == NULL && params_len != 0)) {
        printf("Error while parsing optional parameters [%s]\n", optional_args);
        goto done;
    }

    SetupSrvSvcTests(tests);

    for (i = 1; i < argc; i++) {
        testname = argv[i];
        runtest = FindTest(tests, testname);

        if (runtest) {
            ret = StartTest(runtest, hostname,
                            username, password,
                            params, params_len);
            goto done;
        }
    }

    printf("No test name specified. Available tests:\n");
    runtest = tests;
    while (runtest) {
        printf("%s\n", runtest->name);
        runtest = runtest->next;
    }
    printf("\n");
    

done:
    FreeTests(tests);

    if (hostname) {
        free(hostname);
    }

    if (pCreds->username) {
        free(pCreds->username);
    }

    if (pCreds->password) {
        free(pCreds->password);
    }

    if (pCreds->domain) {
        free(pCreds->domain);
    }

    if (pCreds->workstation) {
        free(pCreds->workstation);
    }

    if (pCreds->principal) {
        free(pCreds->principal);
    }

    if (pCreds->ccache) {
        free(pCreds->ccache);
    }

    if (pCreds) {
        free(pCreds);
    }

    return 0;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
