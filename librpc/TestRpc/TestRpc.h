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

#ifndef _TESTRPC_H_
#define _TESTRPC_H_

#include <config.h>

#include <lw/ntstatus.h>
#include <lwrpc/winerror.h>
#include <lwrpc/errconv.h>
#include <wc16str.h>
#include <wc16printf.h>
#include "Params.h"

#define goto_if_ntstatus_not_success(s, lbl) \
    if ((s) != STATUS_SUCCESS) {             \
        status = s;                          \
        goto lbl;                            \
    }

struct test;

typedef int (*test_fn)(struct test *t, const wchar16_t *hostname,
		       const wchar16_t *username, const wchar16_t *password,
		       struct parameter *options, int optcount);

struct test {
    const char *name;
    test_fn function;

    struct test *next;
};


void AddTest(struct test *ft, const char *name, test_fn function);
void SetupSamrTests(struct test *t);
void SetupLsaTests(struct test *t);
void SetupNetlogonTests(struct test *t);
void SetupNetApiTests(struct test *t);
void SetupMprTests(struct test *t);
handle_t CreateSamrBinding(handle_t *binding, const wchar16_t *host);
handle_t CreateLsaBinding(handle_t *binding, const wchar16_t *host);
handle_t CreateNetlogonBinding(handle_t *binding, const wchar16_t *host);


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

#define test_fail_if_no_memory(ptr)                                  \
    if ((ptr) == NULL) {                                             \
        ret = false;                                                 \
        printf("Test failed: Couldn't allocate pointer %s\n", #ptr); \
        goto done;                                                   \
    }

#define test_fail(printf_args) {                \
        printf printf_args;                     \
        ret = false;                            \
        goto done;                              \
    }

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

#define NTSTATUS_IS_OK(status)  ((status) == STATUS_SUCCESS)
#define WINERR_IS_OK(err)       ((err) == ERROR_SUCCESS)


#define SET_SESSION_CREDS(res, host, user, pass)                        \
    do {                                                                \
        int ret;                                                        \
        size_t host_len;                                                \
        if ((host)) {                                                   \
            host_len = wc16slen((host));                                \
            (res).RemoteName = (wchar16_t*) malloc(sizeof(wchar16_t)*   \
                                                   (host_len+8));       \
            if ((res).RemoteName == NULL) {                             \
                printf("Error when allocating RemoteName for"           \
                       "SET_SESSION_CREDS\n");                          \
                goto done;                                              \
            }                                                           \
                                                                        \
            sw16printf((res).RemoteName, "\\\\%S\\IPC$", (host));       \
                                                                        \
            ret = WNetAddConnection2(&(res), (pass), (user));           \
            if (ret != 0) {                                             \
                const char *name = Win32ErrorToName(ret);               \
                printf("WNetAddConnection2 failed with code "           \
                       "%s (0x%08x) when doing SET_SESSION_CREDS\n",    \
                       name, ret);                                      \
                                                                        \
                SAFE_FREE((res).RemoteName);                            \
                goto done;                                              \
            }                                                           \
                                                                        \
            SAFE_FREE((res).RemoteName);                                \
        }                                                               \
    } while(0);

#define RELEASE_SESSION_CREDS(res)                                      \
    do {                                                                \
        int ret;                                                        \
        if ((res).RemoteName) {                                         \
            ret = WNetCancelConnection2((res).RemoteName, 0, 0);        \
            if (ret != 0) {                                             \
                const char *name = Win32ErrorToName(ret);               \
                printf("WNetCancelConnection2 failed with code "        \
                       "%s (0x%08x) when doing RELEASE_SESSION_CREDS\n",\
                       name, ret);                                      \
                                                                        \
                SAFE_FREE((res).RemoteName);                            \
                goto done;                                              \
            }                                                           \
                                                                        \
            SAFE_FREE((res).RemoteName);                                \
        }                                                               \
    } while(0);

#define TESTINFO(test, host, user, pass)                                \
    {                                                                   \
        printf("#\n# Test: %s\n#\n\n", test->name);                     \
        if (verbose_mode) {                                             \
            printf("# Test arguments:\n");                              \
            printfw16("#  hostname: %S\n#  username: %S\n"              \
                      "#  password: %S\n#\n", host, user, pass);        \
        }                                                               \
    }

#define PARAM_INFO_START                                        \
    if (verbose_mode) {                                         \
        printf("# Test parameters:\n");                         \
    }

#define PARAM_INFO(name, type, value)                           \
    do {                                                        \
        if (!verbose_mode) break;                               \
        ParamInfo(name, type, (void*)value);                    \
    } while (0)

#define PARAM_INFO_END                                          \
    if (verbose_mode) {                                         \
        printf("#\n");                                          \
    }


#define DUMP_PTR32(pfx, v)                                      \
    if (verbose_mode) {                                         \
        printf("%s%s = 0x%08x\n", pfx, #v, (unsigned int)(v));  \
    }

#define DUMP_PTR64(pfx, v)                                      \
    if (verbose_mode) {                                         \
        printf("%s%s = 0x%16lx\n", pfx, #v, (unsigned long)(v)); \
    }

#if SIZEOF_LONG_INT == 8
#define DUMP_PTR   DUMP_PTR64
#else
#define DUMP_PTR   DUMP_PTR32
#endif

#define DUMP_WSTR(pfx, v)                           \
    if (verbose_mode) {                             \
        printfw16("%s%s = \"%S\"\n", pfx, #v, (v)); \
    }

#define DUMP_INT(pfx, v)                                    \
    if (verbose_mode) {                                     \
        printf("%s%s = %d (0x%08x)\n", pfx, #v, (v), (v));  \
    }

#define DUMP_UINT(pfx, v)                                   \
    if (verbose_mode) {                                     \
        printf("%s%s = %u (0x%08x)\n", pfx, #v, (v), (v));  \
    }

#define DUMP_UNICODE_STRING(pfx, v)                     \
    if (verbose_mode) {                                 \
        wchar16_t *str = GetFromUnicodeString((v));     \
        printfw16("%s%s = \"%S\"\n", pfx, #v, str);     \
        SAFE_FREE(str);                                 \
    }

#define DUMP_UNICODE_STRING_EX(pfx, v)                  \
    if (verbose_mode) {                                 \
        wchar16_t *str = GetFromUnicodeStringEx((v));   \
        printfw16("%s%s = \"%S\"\n", pfx, #v, str);     \
        SAFE_FREE(str);                                 \
    }


#define DUMP_CUSTOM(pfx, v, fn)                 \
    if (verbose_mode) {                         \
        DUMP_PTR(pfx, v);                       \
        fn;                                     \
    }


#define INPUT_ARG_PTR(v)             DUMP_PTR("> ", (v));
#define INPUT_ARG_WSTR(v)            DUMP_WSTR("> ", (v));
#define INPUT_ARG_INT(v)             DUMP_INT("> ", (v));
#define INPUT_ARG_UINT(v)            DUMP_UINT("> ", (v));
#define INPUT_ARG_UNICODE_STRING(v)  DUMP_UNICODE_STRING("> ", (v));
#define INPUT_ARG_CUSTOM(v, fn)      DUMP_CUSTOM("> ", (v), fn);

#define OUTPUT_ARG_PTR(v)            DUMP_PTR("< ", (v));
#define OUTPUT_ARG_WSTR(v)           DUMP_WSTR("< ", (v));
#define OUTPUT_ARG_INT(v)            DUMP_INT("< ", (v));
#define OUTPUT_ARG_UINT(v)           DUMP_UINT("< ", (v));
#define OUTPUT_ARG_UNICODE_STRING(v) DUMP_UNICODE_STRING("< ", (v));
#define OUTPUT_ARG_CUSTOM(v, fn)     DUMP_CUSTOM("< ", (v), fn);

#define RESULT_WSTR(v)               DUMP_WSTR("=> ", (v));
#define RESULT_INT(v)                DUMP_INT("=> ", (v));
#define RESULT_UINT(v)               DUMP_UINT("=> ", (v));

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


#endif /* _TESTRPC_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
