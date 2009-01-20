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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <config.h>

#include "params.h"
#include "test.h"


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


int StartTest(struct test *t, struct parameter *options, int optcount)
{
    int ret;

    if (t == NULL) return -1;

    ret = t->function(t, options, optcount);
    printf("%s\n", (ret) ? "SUCCEEDED" : "FAILED");
    return ret;
}



void display_usage(void)
{
    printf("Usage: testrpc -h hostname [-u username] [-p password] [-o options] testname\n"
           "\thostname - host to connect when performing a test\n"
           "\tusername - user name to use when connecting the host (leave blank to use\n"
           "\t           kerberos credentials or anonynous session if available)\n"
           "\tpassword - user name to use when connecting the host (leave blank to use\n"
           "\t           kerberos credentials or anonynous session if available)\n");
}


extern char *optarg;
int verbose_mode;

int main(int argc, char *argv[])
{
    int i, opt, ret;
    char *testname, *optional_args;
    struct test *tests, *runtest, *freetest;
    struct parameter *params;
    int params_len;

    tests         = NULL;
    runtest       = NULL;
    optional_args = NULL;

    verbose_mode = 0;

    while ((opt = getopt(argc, argv, "h:o:vu:p:")) != -1) {
        switch (opt) {
        case 'o':
            optional_args = optarg;
            break;

        case 'v':
            verbose_mode = 1;
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

    params = get_optional_params(optional_args, &params_len);
    if ((params != NULL && params_len == 0) ||
        (params == NULL && params_len != 0)) {
        printf("Error while parsing optional parameters [%s]\n", optional_args);
        goto done;
    }

    SetupSidTests(tests);

    for (i = 1; i < argc; i++) {
        testname = argv[i];
        runtest = FindTest(tests, testname);

        if (runtest) {
            ret = StartTest(runtest, params, params_len);
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
    /* Free the tests list */
    freetest = tests;
    while (tests) {
        tests = tests->next;

        SAFE_FREE(freetest);
        freetest = tests;
    }

    /* Free optional parameters */
    for (i = 0; i < params_len; i++) {
        SAFE_FREE(params[i].key);
        SAFE_FREE(params[i].val);
    }
    SAFE_FREE(params);

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
