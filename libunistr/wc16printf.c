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

/* -*- mode: c; c-basic-offset: 4 -*- */
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wchar16.h>
#include <wc16str.h>


int sw16printf(wchar16_t *out, const char *format, ...)
{
    va_list ap;
    char c;
    size_t len;
    char *s;
    wchar16_t *S;
    wchar_t *W;
    wchar16_t *buf = out;
    char *fmt, *f;
    size_t spec_len;
    char *spec_end, spec[64];
    char spec_out[64];    /* larger than any number "printf-ed" */
    char conv_string[2];

    len = 0;

    fmt = strdup(format);
    if (fmt == NULL) return -1;

    f = fmt;
    va_start(ap, format);

    while (*fmt) {
	if ((*fmt) != '%') {
            conv_string[0] = *fmt;
            conv_string[1] = 0;
	    mbstowc16s(buf++, conv_string, 1);
	    fmt++;
	    continue;
	}

	/* get the next character */
	c = *(fmt + 1);

	switch (c) {
	case 's': /* byte character string */
	    s = va_arg(ap, char *);
	    len = strlen(s);
	    mbstowc16s(buf, s, len);
	    fmt += 2;
	    break;
	    
	case 'S': /* 16-bit character string */
	    S = va_arg(ap, wchar16_t *);
	    len = wc16slen(S);
	    wc16scpy(buf, S);
	    fmt += 2;
	    break;
	
	case 'W': /* 32-bit character string */
	    W = va_arg(ap, wchar_t *);
	    len = wcslen(W);
	    wcstowc16s(buf, W, len);
	    fmt += 2;
	    break;

	default:  /* all other format strings */
	    /* extract single specifier (space ends it) */
	    spec_end = strchr(fmt, ' ');
	    if (spec_end == NULL) {
		spec_end = strchr(fmt + 1, '%');
	    }

	    if (spec_end == NULL) {
		spec_len = strlen(fmt);
	    } else {
		spec_len = spec_end - fmt;
	    }

	    memset(spec, 0, sizeof(spec));
	    strncpy(spec, fmt, spec_len);

	    if (strchr(spec, 'd') ||
		strchr(spec, 'i') ||
		strchr(spec, 'c')) {
		int arg = va_arg(ap, int);
		snprintf(spec_out, sizeof(spec_out), spec, arg);

	    } else if (strchr(spec, 'u') ||
		       strchr(spec, 'x') ||
		       strchr(spec, 'X') ||
		       strchr(spec, 'o') ) {
		unsigned int arg = va_arg(ap, unsigned int);
		snprintf(spec_out, sizeof(spec_out), spec, arg);

	    } else if (strchr(spec, 'f') ||
		       strchr(spec, 'F') ||
		       strchr(spec, 'e') ||
		       strchr(spec, 'E') ||
		       strchr(spec, 'g') ||
		       strchr(spec, 'G') ||
		       strchr(spec, 'a') ||
		       strchr(spec, 'A')) {
		double arg = va_arg(ap, double);
		snprintf(spec_out, sizeof(spec_out), spec, arg);

	    }

	    len = strlen(spec_out);
	    mbstowc16s(buf, spec_out, len);
	    fmt += spec_len;
	}

	buf += len;
    }
    
    va_end(ap);

    /* terminate the resulting string */
    *buf = (wchar16_t) 0;

    /* free what's been duplicated as fmt */
    if (f) free(f);
    return (int)(buf - out);
}


int printfw16(const char *format, ...)
{
    va_list ap;
    char c;
    size_t len, total;
    char *s;
    wchar16_t *S;
    wchar_t *W;
    char *fmt, *f, *out;
    size_t spec_len;
    char *spec_end, spec[64];
    char spec_out[64];    /* larger than any number "printf-ed" */

    len   = 0;
    total = 0;

    fmt = strdup(format);
    if (fmt == NULL) return -1;

    f = fmt;
    va_start(ap, format);

    while (*fmt) {
	if ((*fmt) != '%') {
	    c = *fmt;
	    printf("%c", c);
	    fmt++;
	    continue;
	}

	/* get the next character */
	c = *(fmt + 1);

	switch (c) {
	case 's': /* byte character string */
	    s = va_arg(ap, char *);
	    len = strlen(s);
	    printf("%s", s);
	    fmt += 2;
	    break;
	    
	case 'S': /* 16-bit character string */
	    S = va_arg(ap, wchar16_t *);
	    len = wc16slen(S);
	    out = (char*) malloc(len + 1);
	    wc16stombs(out, S, len + 1);
	    printf("%s", out);
	    free(out);
	    fmt += 2;
	    break;
	
	case 'W': /* 32-bit character string */
	    W = va_arg(ap, wchar_t *);
	    len = wcslen(W);
	    out = (char*) malloc(len + 1);
	    wcstombs(out, W, len + 1);
	    printf("%s", out);
	    free(out);
	    fmt += 2;
	    break;

	default:  /* all other format strings */
	    /* extract single specifier (space ends it) */
	    spec_end = strchr(fmt, ' ');
	    if (spec_end == NULL) {
		spec_end = strchr(fmt + 1, '%');
	    }

	    if (spec_end == NULL) {
		spec_len = strlen(fmt);
	    } else {
		spec_len = spec_end - fmt;
	    }

	    memset(spec, 0, sizeof(spec));
	    strncpy(spec, fmt, spec_len);

	    if (strchr(spec, 'd') ||
		strchr(spec, 'i') ||
		strchr(spec, 'c')) {
		int arg = va_arg(ap, int);
		snprintf(spec_out, sizeof(spec_out), spec, arg);
		printf("%s", spec_out);
		len = strlen(spec_out);

	    } else if (strchr(spec, 'u') ||
		       strchr(spec, 'x') ||
		       strchr(spec, 'X') ||
		       strchr(spec, 'o') ) {
		unsigned int arg = va_arg(ap, unsigned int);
		snprintf(spec_out, sizeof(spec_out), spec, arg);
		printf("%s", spec_out);
		len = strlen(spec_out);

	    } else if (strchr(spec, 'f') ||
		       strchr(spec, 'F') ||
		       strchr(spec, 'e') ||
		       strchr(spec, 'E') ||
		       strchr(spec, 'g') ||
		       strchr(spec, 'G') ||
		       strchr(spec, 'a') ||
		       strchr(spec, 'A')) {
		double arg = va_arg(ap, double);
		snprintf(spec_out, sizeof(spec_out), spec, arg);
		printf("%s", spec_out);
		len = strlen(spec_out);

	    }

	    fmt += spec_len;
	}

	total += len;
    }
    
    va_end(ap);

    /* free what's been duplicated as fmt */
    if (f) free(f);
    return total;
}
