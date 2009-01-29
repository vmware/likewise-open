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

#ifndef WCSTR16_H
#include <wchar16.h>
#include <stddef.h>

#ifdef __GNUC__
#define TEXT(str)  str
#endif

size_t _wc16slen(const wchar16_t *str);
#ifdef __GNUC__
#define wc16slen(str)  _wc16slen(str)
#elif _WIN32
#define wc16slen(str)  wcslen(str)
#endif

wchar16_t* _wc16scpy(wchar16_t *dst, const wchar16_t *src);
#ifdef __GNUC__
#define wc16scpy(dst, src)  _wc16scpy(dst, src);
#elif _WIN32
#define wc16scpy(dst, src)  wcscpy(dst, src);
#endif

wchar16_t* _wc16sdup(const wchar16_t *str);
#ifdef __GNUC__
#define wc16sdup(str)  _wc16sdup(str)
#elif _WIN32
#define wc16sdup(str)  _wcsdup(str)
#endif

wchar16_t* _wc16sndup(const wchar16_t *str, size_t max_characters);
#define wc16sdupn(str, max_characters)  _wc16sndup(str, max_characters)
#define wc16sndup(str, max_characters)  _wc16sndup(str, max_characters)

wchar16_t* _wc16sncpy(wchar16_t *dest, const wchar16_t *src, size_t n);
#ifdef WCHAR16_IS_WCHAR
#define wc16sncpy(dest, src, n)  _wcsncpy(dest, src, n)
#else
#define wc16sncpy(dest, src, n)  _wc16sncpy(dest, src, n)
#endif

int wc16scasecmp(const wchar16_t *s1, const wchar16_t *s2);

#ifndef HAVE_MBSTRLEN
#define mbstrlen(x) mbstowcs(NULL, (x), 0)
#endif

/*Convert a wchar_t string to a wchar16_t string and return the result.
 *
 * If the result needs to be freed, *free_required will be set to 1.
 */
wchar16_t *awcstowc16s(const wchar_t *input, int *free_required);

/*Convert a wchar_t string to a wchar16_t string and return the number of characters converted.
 *
 * cchn is the maximum number of characters to store in dest (including null).
 */
size_t wcstowc16s(wchar16_t *dest, const wchar_t *src, size_t cchn);

/*Convert a wchar16_t string to a wchar_t string and return the result.
 *
 * If the result needs to be freed, *free_required will be set to 1.
 */
wchar_t *awc16stowcs(const wchar16_t *input, int *free_required);

/*Convert a wchar16_t string to a wchar_t string and return the number of characters converted.
 *
 * cchn is the maximum number of characters to store in dest (including null).
 */
size_t wc16stowcs(wchar_t *dest, const wchar16_t *src, size_t cchn);

/* Convert a wchar16_t string to a wchar16_t string in little-endian byte order */
size_t wc16stowc16les(wchar16_t *dest, const wchar16_t *src, size_t cchcopy);

/*Convert a multibyte character string to a wchar16_t string and return the result.
 */
wchar16_t *ambstowc16s(const char *input);

/*Convert a multibyte character string to a wchar16_t string and return the number of characters converted.
 *
 * cchn is the maximum number of characters to store in dest (including null).
 */
size_t mbstowc16s(wchar16_t *dest, const char *src, size_t cchn);

/*Convert a multibyte character string to a little endian wchar16_t string and return the number of characters converted.
 *
 * cchn is the maximum number of characters to store in dest (including null).
 */
size_t mbstowc16les(wchar16_t *dest, const char *src, size_t cchn);

/*Convert a wchar16_t string to a multibyte character string and return the result.
 */
char *awc16stombs(const wchar16_t *input);

/*Convert a wchar16_t string to a multicharacter string and return the number of characters converted.
 *
 * cbn is the maximum number of bytes to store in dest (including null).
 */
size_t wc16stombs(char *dest, const wchar16_t *src, size_t cbn);


/* Convert a wchar16_t string to upper case */
void wc16supper(wchar16_t *s);

/* Convert a wchar16_t string to lower case */
void wc16slower(wchar16_t *s);


/* Convert a single-byte string to upper case */
void strlower(char *s);

/* Convert a single-byte string to lower case */
void strupper(char *s);


#endif /* WCSTR16_H */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
