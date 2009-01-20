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

#ifndef _TEST_H_
#define _TEST_H_

#include <stdio.h>
#include <string.h>
#include <wc16str.h>

#include "params.h"

struct test;

typedef int (*test_fn)(struct test *t,
                       struct parameter *options, int optcount);

struct test {
    const char *name;
    test_fn function;

    struct test *next;
};


void AddTest(struct test *ft, const char *name, test_fn function);


extern int verbose_mode;

#define TESTINFO(test)                                          \
    {                                                           \
        printf("#\n# Test: %s\n#\n\n", test->name);             \
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
            printf("(int32) %d (0x%08x)\n",                     \
                   (int)(value), (unsigned int)(value));        \
            break;                                              \
                                                                \
        case pt_uint32:                                         \
            printf("(uint32) %d (0x%08x)\n",                    \
                   (unsigned int)(value),                       \
                   (unsigned int)(value));                      \
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


#define SAFE_FREE(ptr) \
    if (ptr) {         \
        free(ptr);     \
        ptr = NULL;    \
    }


#define BAIL_ON_NULL_PTR(ptr)                     \
    do {                                          \
        if ((ptr) == NULL) {                      \
            status = STATUS_NO_MEMORY;            \
            goto done;                            \
        }                                         \
    } while (0);


#endif /* _TEST_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
