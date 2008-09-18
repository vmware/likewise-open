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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        framework.h
 *
 * Abstract:
 *
 *        Test macros
 *
 * Author: Todd Stecher (2007)
 *
 */

#ifndef _FRAMEWORK_H_
#define _FRAMEWORK_H_

#define TRACE_1  0x01
#define TRACE_2  0x11

extern DWORD g_verbosity;

#define ERROR( _x_ )  printf _x_            
#define TRACE( _x_ )  do {if (g_verbosity & TRACE_1) printf _x_; } while (0)
#define TRACE2( _x_ ) do {if (g_verbosity & TRACE_2) printf _x_; } while (0)

#define CHK_EXPECTED(e,n)                                           \
    do {                                                            \
        dwError = e;                                                \
        if (dwError != n) {                                         \
            ERROR(("  ERROR: %s failed. error=%d (expected %d)\n",  \
                #e, dwError, n));                                   \
            if (dwError == 0)					    \
                dwError = LSA_ERROR_INTERNAL;                       \
                goto error;                                         \
        }  else {                                                   \
            dwError = LSA_ERROR_SUCCESS;                            \
        }                                                           \
    } while (0)                                                     \

#define CHK_ERR(e)                                                  \
    do {                                                            \
        dwError = e;                                                \
        if (dwError) {                                              \
            ERROR(("  ERROR: %s failed. error=%d\n", #e, dwError)); \
            goto error;                                             \
        }                                                           \
    } while (0)  

#define CHK_BOOL(e)                                                 \
    do {                                                            \
        if (!e) {                                                   \
            dwError = LSA_ERROR_INTERNAL;                           \
            ERROR(("  ERROR: %s failed\n", #e));                        \
            goto error;                                             \
        }                                                           \
    } while (0)                                                     \

#define RUN_ALL         0xFFFF
#define TEST_END( _tn_ ) _tn_++
#define RUN_TEST(e) do { if (test == RUN_ALL || test == g_tn){\
       	CHK_ERR(e);} cur_tn++; g_tn++; } while (0)

#endif /* _FRAMEWORK_H_ */

