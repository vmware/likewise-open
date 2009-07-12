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

#ifndef _TESTSRVSVC_H_
#define _TESTSRVSVC_H_

#include <lw/ntstatus.h>
#include <lwio/lwio.h>
#include <wc16str.h>
#include <wc16printf.h>
#include "Params.h"

struct test;

typedef int (*test_fn)(struct test *t, const wchar16_t *hostname,
		       const wchar16_t *username, const wchar16_t *password,
		       struct parameter *options, int optcount);

struct test {
    const char *name;
    test_fn function;

    struct test *next;
};


typedef struct user_creds {
    int use_kerberos;

    /* krb5 credentials */
    char *principal;
    char *ccache;

    /* ntlm credentials */
    char *username;
    char *password;
    char *domain;
    char *workstation;

} UserCreds;

extern UserCreds *pCreds;


void AddTest(struct test *ft, const char *name, test_fn function);
void SetupSrvSvcTests(struct test *t);
handle_t CreateSrvSvcBinding(handle_t *binding, const wchar16_t *host);


#define STATUS(a, b)                                                     \
    if((status = (a)) != 0) {                                            \
        printf("[\033[31;1mFAILED\033[30;0m] %s test: %s (status=%d)\n", \
               active_test, (b), status);                                \
        return;                                                          \
    }

extern int verbose_mode;

#define WSIZE(a) ((wcslen(a) + 1) * sizeof(wchar16_t))
#define VERBOSE(a) if(verbose_mode) (a)
#define PASSED() printf("[\033[32;1mPASSED\033[31;0m] %s test\n", active_test)
#define FAILED() printf("[\033[31;1mFAILED\033[0m] %s test: ", active_test)

#define netapi_fail(err) {                                    \
        const char *name = Win32ErrorToName(err);             \
        if (name) {                                           \
            printf("NetApi error: %s (0x%08x)\n", name, err); \
        } else {                                              \
            printf("NetApi error: 0x%08x\n", err);            \
        }                                                     \
        goto done;                                            \
    }

#define rpc_fail(err) {                                     \
        const char *name = NtStatusToName(err);             \
        if (name) {                                         \
            printf("Rpc error: %s (0x%08x)\n", name, err);  \
        } else {                                            \
            printf("Rpc error: 0x%08x\n", err);             \
        }                                                   \
        goto done;                                          \
    }


#define SET_SESSION_CREDS_KRB5(principal, ccache)                       \
    do {                                                                \
        DWORD dwError = 0;                                              \
        LW_PIO_ACCESS_TOKEN hAccessToken = NULL;                        \
        PSTR pszPrincipal = NULL;                                       \
        PSTR pszCache = NULL;                                           \
                                                                        \
        pszPrincipal = (principal);                                     \
        pszCache     = (ccache);                                        \
                                                                        \
        /* Set up access token */                                       \
        dwError = LwIoCreateKrb5AccessTokenA(pszPrincipal, pszCache,    \
                                             &hAccessToken);            \
        if (dwError) {                                                  \
            printf("Failed to create access token\n");                  \
            goto done;                                                  \
        }                                                               \
                                                                        \
        dwError = LwIoSetThreadAccessToken(hAccessToken);               \
        if (dwError) {                                                  \
            printf("Failed to set access token on thread\n");           \
            goto done;                                                  \
        }                                                               \
                                                                        \
        LwIoDeleteAccessToken(hAccessToken);                            \
                                                                        \
    } while(0);


#define SET_SESSION_CREDS(creds)                                        \
    do {                                                                \
        if ((creds)->use_kerberos) {                                    \
            SET_SESSION_CREDS_KRB5((creds)->principal,                  \
                                   (creds)->ccache);                    \
        }                                                               \
                                                                        \
    } while(0);


#define RELEASE_SESSION_CREDS                                           \
    do {                                                                \
        DWORD dwError = 0;                                              \
                                                                        \
        dwError = LwIoSetThreadAccessToken(NULL);                       \
        if (dwError) {                                                  \
            printf("Failed to set release access token on thread\n");   \
            goto done;                                                  \
        }                                                               \
                                                                        \
    } while(0);


#define TESTINFO(test, host, user, pass)                                \
    {                                                                   \
        printf("#\n# Test: %s\n#\n\n", test->name);                     \
        if (verbose_mode) {                                             \
            printf("# Test arguments:\n");                              \
            w16printfw(L"#  hostname: %ws\n#  username: %ws\n"          \
                      L"#  password: %ws\n#\n", host, user, pass);      \
        }                                                               \
    }

#define PARAM_INFO_START                                        \
    if (verbose_mode) {                                         \
        printf("# Test parameters:\n");                         \
    }

#define PARAM_INFO(name, type, value)                           \
    do {                                                        \
        char *v = NULL;                                         \
        if (!verbose_mode) break;                               \
                                                                \
        printf("# %s = ", name);                                \
        switch (type) {                                         \
        case pt_string:                                         \
            v = strdup((char*)(value));                         \
            printf("(char*)\"%s\"\n", v);                       \
            break;                                              \
                                                                \
        case pt_w16string:                                      \
            v = awc16stombs((wchar16_t*)(value));               \
            printf("(wchar16_t*)\"%s\"\n", v);                  \
            break;                                              \
                                                                \
        case pt_char:                                           \
            printf("(char)\'%c\'\n", (int)(value));             \
            break;                                              \
                                                                \
        case pt_int32:                                          \
            printf("(int32) %d (0x%08x)\n", (int32)(value));    \
            break;                                              \
                                                                \
        case pt_uint32:                                         \
            printf("(uint32) %d (0x%08x)\n", (uint32)(value));  \
            break;                                              \
                                                                \
        default:                                                \
            printf("(unknown type)\n");                         \
        }                                                       \
                                                                \
        SAFE_FREE(v);                                           \
    } while (0)

#define PARAM_INFO_END                                          \
    if (verbose_mode) {                                         \
        printf("#\n");                                          \
    }


#define DUMP_PTR(pfx, v)                            \
    if (verbose_mode) {                             \
        printf("%s%s = 0x%08x\n", pfx, #v, (v));    \
    }

#define DUMP_WSTR(pfx, v)                                  \
    if (verbose_mode) {                                    \
        w16printfw(L"%hhs%hhs = \"%ws\"\n", pfx, #v, (v)); \
    }

#define DUMP_INT(pfx, v)                                    \
    if (verbose_mode) {                                     \
        printf("%s%s = %d (0x%08x)\n", pfx, #v, (v), (v));  \
    }

#define DUMP_UINT(pfx, v)                                   \
    if (verbose_mode) {                                     \
        printf("%s%s = %u (0x%08x)\n", pfx, #v, (v), (v));  \
    }

#define DUMP_UNICODE_STRING(pfx, v)                       \
    if (verbose_mode) {                                   \
        wchar16_t *str = GetFromUnicodeString((v));       \
        w16printfw("%hhs%hhs = \"%ws\"\n", pfx, #v, str); \
        SAFE_FREE(str);                                   \
    }

#define DUMP_UNICODE_STRING_EX(pfx, v)                    \
    if (verbose_mode) {                                   \
        wchar16_t *str = GetFromUnicodeStringEx((v));     \
        w16printfw("%hhs%hhs = \"%ws\"\n", pfx, #v, str); \
        SAFE_FREE(str);                                   \
    }


#define DUMP_CUSTOM(pfx, v, fn)                 \
    if (verbose_mode) {                         \
        DUMP_PTR(pfx, v);                       \
        fn;                                     \
    }


#define INPUT_ARG_PTR(v)             DUMP_PTR("> ", v);
#define INPUT_ARG_WSTR(v)            DUMP_WSTR("> ", v);
#define INPUT_ARG_INT(v)             DUMP_INT("> ", v);
#define INPUT_ARG_UINT(v)            DUMP_UINT("> ", v);
#define INPUT_ARG_UNICODE_STRING(v)  DUMP_UNICODE_STRING("> ", v);
#define INPUT_ARG_CUSTOM(v, fn)      DUMP_CUSTOM("> ", v, fn);

#define OUTPUT_ARG_PTR(v)            DUMP_PTR("< ", v);
#define OUTPUT_ARG_WSTR(v)           DUMP_WSTR("< ", v);
#define OUTPUT_ARG_INT(v)            DUMP_INT("< ", v);
#define OUTPUT_ARG_UINT(v)           DUMP_UINT("< ", v);
#define OUTPUT_ARG_UNICODE_STRING(v) DUMP_UNICODE_STRING("< ", v);
#define OUTPUT_ARG_CUSTOM(v, fn)     DUMP_CUSTOM("< ", v, fn);

#define RESULT_WSTR(v)               DUMP_WSTR("=> ", v);
#define RESULT_INT(v)                DUMP_INT("=> ", v);
#define RESULT_UINT(v)               DUMP_UINT("=> ", v);

#define CALL_MSRPC(msrpc_call)                                     \
    do {                                                           \
        if (verbose_mode) {                                        \
            printf("= Function call:\n=   %s\n", #msrpc_call);     \
        }                                                          \
                                                                   \
        msrpc_call;                                                \
        printf("= Returned status:\n=   %s (0x%08x)\n",            \
               NtStatusToName(status), status);                    \
    } while (0)


#define CALL_NETAPI(netapi_call)                                   \
    do {                                                           \
        if (verbose_mode) {                                        \
            printf("= Function call:\n=   %s\n", #netapi_call);    \
        }                                                          \
                                                                   \
        netapi_call;                                               \
        printf("= Returned status:\n=   %s (0x%08x)\n",            \
               Win32ErrorToName(err), err);                        \
    } while (0)


#endif /* _TESTSRVSVC_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
