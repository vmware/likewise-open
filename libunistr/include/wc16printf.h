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

#include <wchar16.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>

ssize_t
vsw16printf(
    wchar16_t *out,
    size_t maxchars,
    const wchar16_t *format,
    va_list args
    );

//TODO: rename this once the deprecated sw16printf is removed
ssize_t
sw16printf_new(wchar16_t *out, size_t maxchars, const wchar16_t *format, ...);

ssize_t
sw16printfw(wchar16_t *out, size_t maxchars, const wchar_t *format, ...);

wchar16_t *
asw16printfw(const wchar_t *format, ...);

ssize_t
vfw16printf(
    FILE *pFile,
    const wchar16_t *format,
    va_list args
    );

ssize_t
fw16printf(FILE *pFile, const wchar16_t *format, ...);

ssize_t
fw16printfw(FILE *pFile, const wchar_t *format, ...);

ssize_t
w16printfw(const wchar_t *format, ...);

//Deprecated
int printfw16(const char *fmt, ...);
//Deprecated
int sw16printf(wchar16_t *out, const char *fmt, ...);
