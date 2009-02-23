/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

/* boo */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <wc16str.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_WCTYPE_H
#    include <wctype.h>
#endif
#include <iconv.h>
#include <errno.h>
#include <wchar.h>
#include <string.h>


#if SIZEOF_WCHAR_T == 2
#define WCHAR16_IS_WCHAR 1
#endif

/* UCS-2, little endian byte order
 * Note that UCS-2 without LE will
 * default to big endian on FreeBSD
 */
#if defined(WORDS_BIGENDIAN)
#define WINDOWS_ENCODING "UCS-2BE"
#else
#define WINDOWS_ENCODING "UCS-2LE"
#endif


size_t _wc16slen(const wchar16_t *str)
{
    size_t i = 0;
    if (str == NULL) return i;

    while (str[i] != 0) i++;

    return i;
}

size_t _wc16snlen(const wchar16_t *str, size_t n)
{
    size_t i = 0;
    while (n > 0)
    {
        if (str[i] == 0)
            break;
        n--;
        i++;
    }

    return i;
}

wchar16_t* _wc16scpy(wchar16_t *dst, const wchar16_t *src)
{
    size_t size;

    if (dst == NULL || src == NULL) return NULL;

    size = (_wc16slen(src) + 1) * sizeof(wchar16_t);
    memcpy(dst, src, size);

    return dst;
}


wchar16_t* _wc16sdup(const wchar16_t *str)
{
    size_t size;
    wchar16_t *out;

    if (str == NULL) return NULL;

    size = (_wc16slen(str) + 1) * sizeof(wchar16_t);
    out = (wchar16_t*) malloc(size);
    if (out == NULL) return NULL;
    memcpy(out, str, size);

    return out;
}


wchar16_t* _wc16sndup(const wchar16_t *str, size_t max_characters)
{
    size_t len;
    wchar16_t *out;

    if (str == NULL) return NULL;

    //Find the length of str, up to max_characters
    for(len = 0; len < max_characters && str[len] != 0; len++);

    out = (wchar16_t*) malloc((len + 1) * sizeof(wchar16_t));
    if (out == NULL) return NULL;

    //Copy everything up to the NULL terminator from str
    memcpy(out, str, len * sizeof(wchar16_t));
    //Add the NULL
    out[len] = 0;

    return out;
}

/**
 * @fixme: According to the manpage, wcsncpy() returns dest, not the
 * end of the string pointed to by dest.  Do we want to diverge?
 */
wchar16_t* _wc16sncpy(wchar16_t *dest, const wchar16_t *src, size_t n)
{
    while(n > 0)
    {
        *dest = *src;
        if(*src == 0)
            break;
        dest++;
        src++;
        n--;
    }
    return dest;
}

/*
 * For consistency and performance, we break the wcpncpy() contract and don't
 * NUL pad dest when src is short.
 */
wchar16_t* _wc16pncpy(wchar16_t *dest, const wchar16_t *src, size_t n)
{
    /* Return pointer can't point to written memory when n == 0 */
    /* assert() instead? */
    if (!n)
        return dest;

    while(n > 0)
    {
        n--;
        *dest = *src;
        if(*src == 0 || n == 0)
            break;
        src++;
        dest++;
    }
    return dest;
}

int wc16scmp(const wchar16_t *s1, const wchar16_t *s2)
{
    size_t s1_len, s2_len, len;

    if (s1 == NULL || s2 == NULL) return -1;

    s1_len = wc16slen(s1);
    s2_len = wc16slen(s2);

    if (s1_len != s2_len) return s1_len - s2_len;

    len = s1_len;
    return memcmp((void*)s1, (void*)s2, len);
}

#ifndef HAVE_WCSCASECMP

int wcscasecmp(const wchar_t *w1, const wchar_t *w2)
{
    int index;
    wchar_t c1 = 0, c2 = 0;

    for (index = 0, c1 = towlower(w1[0]), c2 = towlower(w2[0]);
         c1 && c2 && c1 == c2;
         index++, c1 = towlower(w1[index]), c2 = towlower(w2[index]));

    return (c1 == c2) ? 0 : ((c1 < c2) ? -1 : 1);
}

#else

extern int wcscasecmp(const wchar_t *w1, const wchar_t *w2);

#endif

int wc16scasecmp(const wchar16_t *s1, const wchar16_t *s2)
{
    int need_free = 0;
    wchar_t* w1, *w2;
    int result;

    w1 = awc16stowcs(s1, &need_free);
    w2 = awc16stowcs(s2, &need_free);

    result = wcscasecmp(w1, w2);

    if (need_free)
    {
        free(w1);
        free(w2);
    }

    return result;
}

/*Optimistically try to wc16sncpy()
 *
 * Returns the length of dest needed for a successful copy including
 * the NUL character.  If the copy was complete, the value will be
 * <= wc16slen(src) + 1
 */
size_t wc16oncpy(wchar16_t *dest, const wchar16_t *src, size_t n)
{
    ptrdiff_t diff = 0;

    if (n != 0)
    {
        wchar16_t *cursor = wc16pncpy(dest, src, n);
        diff = cursor - dest + 1;
        if (!*cursor)
        {
            /* Success */
            return diff;
        }
    }

    return diff + wc16slen(src + diff) + 1;
}

wchar16_t *awcstowc16s(const wchar_t *input, int *free_required)
{
#ifdef WCHAR16_IS_WCHAR
    *free_required = 0;
    return (wchar16_t*) input;
#else
    size_t cchlen;
    wchar16_t *buffer;
    if(input == NULL)
        return NULL;

    cchlen = wcslen(input);
    buffer = malloc((cchlen + 1) * sizeof(wchar16_t));
    if(buffer == NULL)
        return NULL;
    if(wcstowc16s(buffer, input, cchlen + 1) != cchlen)
    {
        free(buffer);
        return NULL;
    }
    *free_required = 1;
    return buffer;
#endif
}

size_t wcstowc16s(wchar16_t *dest, const wchar_t *src, size_t cchcopy)
{
#if WCHAR16_IS_WCHAR
    size_t input_len = wcslen(src);
    if(input_len >= cchcopy)
        input_len = cchcopy;
    else
        dest[input_len] = 0;

    memcpy(dest, src, input_len * sizeof(wchar16_t));
    return input_len;
#else
    iconv_t handle = iconv_open(WINDOWS_ENCODING, "WCHAR_T");
    char *inbuf = (char *)src;
    char *outbuf = (char *)dest;
    size_t cbin = wcslen(src) * sizeof(src[0]);
    size_t cbout = cchcopy * sizeof(dest[0]);
    size_t converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);

    if(cbout >= sizeof(dest[0]))
        *(wchar16_t *)outbuf = 0;
    iconv_close(handle);
    if(converted == (size_t)-1 && cbout != 0)
        return (size_t)-1;
    else
        return cchcopy - cbout/sizeof(dest[0]);
#endif
}

wchar_t *awc16stowcs(const wchar16_t *input, int *free_required)
{
#if WCHAR16_IS_WCHAR
    *free_required = 0;
    return (wchar_t*) input;
#else
    size_t cchlen;
    wchar_t *buffer;
    if(input == NULL)
        return NULL;
    cchlen = wc16slen(input);
    buffer = malloc((cchlen + 1) * sizeof(wchar_t));
    if(buffer == NULL)
        return NULL;
    if(wc16stowcs(buffer, input, cchlen + 1) != cchlen)
    {
        free(buffer);
        return NULL;
    }
    *free_required = 1;
    return buffer;
#endif
}

size_t wc16stowcs(wchar_t *dest, const wchar16_t *src, size_t cchcopy)
{
#if WCHAR16_IS_WCHAR
    size_t input_len = wcslen(src);
    if(input_len >= cchcopy)
        input_len = cchcopy;
    else
        dest[input_len] = 0;

    memcpy(dest, src, input_len * sizeof(wchar16_t));
    return input_len;
#else
    iconv_t handle = iconv_open("WCHAR_T", WINDOWS_ENCODING);
    char *inbuf = (char *)src;
    char *outbuf = (char *)dest;
    size_t cbin = wc16slen(src) * sizeof(src[0]);
    size_t cbout = cchcopy * sizeof(dest[0]);
    size_t converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);

    if(cbout >= sizeof(dest[0]))
        *(wchar_t *)outbuf = 0;
    iconv_close(handle);
    if(converted == (size_t)-1 && cbout != 0)
        return (size_t)-1;
    else
        return cchcopy - cbout/sizeof(dest[0]);
#endif
}

size_t wc16stowc16les(wchar16_t *dest, const wchar16_t *src, size_t cchcopy)
{
    iconv_t handle = iconv_open(WINDOWS_ENCODING, "UCS-2LE");
    char *inbuf = (char *)src;
    char *outbuf = (char *)dest;
    size_t cbin = wc16slen(src) * sizeof(src[0]);
    size_t cbout = cchcopy * sizeof(dest[0]);
    size_t converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);

    if(cbout >= sizeof(dest[0]))
        *(wchar16_t *)outbuf = 0;
    iconv_close(handle);
    if(converted == (size_t)-1 && cbout != 0)
        return (size_t)-1;
    else
        return cchcopy - cbout/sizeof(dest[0]);
}

wchar16_t *ambstowc16s(const char *input)
{
    size_t cchlen;
    wchar16_t *buffer;
    if(input == NULL)
        return NULL;
    cchlen = mbstrlen(input);
    if(cchlen == (size_t)-1)
        return NULL;
    buffer = malloc((cchlen + 1) * sizeof(wchar16_t));
    if(buffer == NULL)
        return NULL;
    if(mbstowc16s(buffer, input, cchlen + 1) != cchlen)
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}

/*Convert a multibyte character string to a wchar16_t string and return the number of characters converted.
 *
 * cchn is the maximum number of characters to store in dest (including null).
 */
size_t mbstowc16s(wchar16_t *dest, const char *src, size_t cchcopy)
{
#if WCHAR16_IS_WCHAR
    return mbstowcs(dest, src, cchcopy);
#else
    iconv_t handle = iconv_open(WINDOWS_ENCODING, "");
    char *inbuf;
    char *outbuf;
    size_t cbin;
    size_t cbout;
    size_t converted;
    if(handle == (iconv_t)-1)
        return (size_t)-1;
    inbuf = (char *)src;
    outbuf = (char *)dest;
    cbin = strlen(src) * sizeof(src[0]);
    cbout = cchcopy * sizeof(dest[0]);
    converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);

    if(cbout >= sizeof(dest[0]))
        *(wchar16_t *)outbuf = 0;
    iconv_close(handle);
    if(converted == (size_t)-1 && cbout != 0)
        return (size_t)-1;
    else
        return cchcopy - cbout/sizeof(dest[0]);
#endif
}

size_t mbstowc16les(wchar16_t *dest, const char *src, size_t cchcopy)
{
    iconv_t handle = iconv_open("UCS-2LE", "");
    char *inbuf;
    char *outbuf;
    size_t cbin;
    size_t cbout;
    size_t converted;
    if(handle == (iconv_t)-1)
        return (size_t)-1;
    inbuf = (char *)src;
    outbuf = (char *)dest;
    cbin = strlen(src) * sizeof(src[0]);
    cbout = cchcopy * sizeof(dest[0]);
    converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);

    if(cbout >= sizeof(dest[0]))
        *(wchar16_t *)outbuf = 0;
    iconv_close(handle);
    if(converted == (size_t)-1 && cbout != 0)
        return (size_t)-1;
    else
        return cchcopy - cbout/sizeof(dest[0]);
}

char *awc16stombs(const wchar16_t *input)
{
    size_t cblen;
    char *buffer;
    if(input == NULL)
        return NULL;
    cblen = wc16stombs(NULL, input, 0);
    buffer = malloc((cblen + 1) * sizeof(char));
    if(buffer == NULL)
        return NULL;
    if(wc16stombs(buffer, input, cblen + 1) != cblen)
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}

size_t wc16stombs(char *dest, const wchar16_t *src, size_t cbcopy)
{
#if WCHAR16_IS_WCHAR
    return wcstombs(dest, src, cbcopy);
#else
    iconv_t handle = iconv_open("", WINDOWS_ENCODING);
    char *inbuf = (char *)src;
    char *outbuf = (char *)dest;
    size_t cbin = wc16slen(src) * sizeof(src[0]);
    size_t cbout = cbcopy;
    size_t converted;
    if(outbuf == NULL)
    {
        //wcstombs allows dest to be NULL. In this case, cbcopy is ignored
        //and the total number of bytes it would take to store src is returned.
        //
        //iconv does not allow the output buffer to be NULL. To emulate this
        //functionality, we'll have to actually convert the string, but only
        //a few characters at a time.

        char buffer[100];
        size_t cblen = 0;
        while(cbin > 0)
        {
            outbuf = buffer;
            cbout = sizeof(buffer);
            converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);
            if(converted == (size_t)-1)
            {
                if(errno != E2BIG)
                {
                    cblen = -1;
                    break;
                }
            }
            cblen += sizeof(buffer) - cbout;
        }
        iconv_close(handle);
        return cblen;
    }
    converted = iconv(handle, (ICONV_IN_TYPE) &inbuf, &cbin, &outbuf, &cbout);

    if(cbout >= sizeof(dest[0]))
        *(char *)outbuf = 0;
    iconv_close(handle);
    if(converted == (size_t)-1 && cbout != 0)
        return (size_t)-1;
    else
        return cbcopy - cbout/sizeof(dest[0]);
#endif
}


typedef int (*caseconv)(int c);

static void wc16scaseconv(caseconv fconv, wchar16_t *s)
{
    size_t len;
    int i;

    if (fconv == NULL || s == NULL) return;
    len = wc16slen(s);

    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        s[i] = (wchar16_t)fconv(c);
    }
}


/*
  These case conversions aren't exactly right, because toupper
  and tolower functions depend on locale settingsand not on
  unicode maps.
  TODO: Find better case conversion function for unicode
*/

void wc16supper(wchar16_t *s)
{
    wc16scaseconv(toupper, s);
}


void wc16slower(wchar16_t *s)
{
    wc16scaseconv(tolower, s);
}


static void strcaseconv(caseconv fconv, char *s)
{
    size_t len;
    int i;

    if (fconv == NULL || s == NULL) return;
    len = strlen(s);

    for (i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        s[i] = (char) fconv(c);
    }
}


void strupper(char *s)
{
    strcaseconv(toupper, s);
}


void strlower(char *s)
{
    strcaseconv(tolower, s);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
