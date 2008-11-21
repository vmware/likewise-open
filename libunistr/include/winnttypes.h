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

#ifndef WINNTTYPES_H
#define WINNTTYPES_H

#include <wchar.h>
#include <wchar16.h>

#ifndef LPSTR_DEFINED
#define LPSTR_DEFINED 1

typedef char *LPSTR;

#endif

#ifndef LPCSTR_DEFINED
#define LPCSTR_DEFINED 1

typedef const char *LPCSTR;

#endif

#ifndef UCHAR_DEFINED
#define UCHAR_DEFINED 1

typedef unsigned char UCHAR, *PUCHAR;

#endif

#ifndef LPW16STR_DEFINED
#define LPW16STR_DEFINED 1

typedef wchar16_t *LPW16STR;

#endif

#ifndef LPCW16STR_DEFINED
#define LPCW16STR_DEFINED 1

typedef const wchar16_t *LPCW16STR;
#endif

#ifndef LPWSTR_DEFINED
#define LPWSTR_DEFINED 1

typedef wchar_t *LPWSTR;

#endif

#ifndef LPCWSTR_DEFINED
#define LPCWSTR_DEFINED 1

typedef const wchar_t *LPCWSTR;

#endif

#endif /* WINNTTYPES_H */
