/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software 2007.  
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* ex: set tabstop=4 expandtab shiftwidth=4: */
#ifndef __CTSTRUTILS_H__
#define __CTSTRUTILS_H__

CENTERROR CTAllocateString(PCSTR pszInputString, PSTR * ppszOutputString);

#define CTStrdup CTAllocateString

CENTERROR CTStrndup(PCSTR pszInputString, size_t size, PSTR * ppszOutputString);

void CTFreeString(PSTR pszString);

#define CT_SAFE_FREE_STRING(str) \
    do { if (str) { CTFreeString(str); (str) = NULL; } } while (0)

void CTFreeStringArray(PSTR * ppStringArray, DWORD dwCount);

void CTFreeNullTerminatedStringArray(PSTR * ppStringArray);

void CTStripLeadingWhitespace(PSTR pszString);

void CTStripTrailingWhitespace(PSTR pszString);

void CTStripWhitespace(PSTR pszString);

void CTStrToUpper(PSTR pszString);

void CTStrToLower(PSTR pszString);

CENTERROR CTEscapeString(PSTR pszOrig, PSTR * ppszEscapedString);

CENTERROR CTAllocateStringPrintfV(PSTR * result, PCSTR format, va_list args);

CENTERROR CTAllocateStringPrintf(PSTR * result, PCSTR format, ...
    );

/** Returns true if substr(str, 0, strlen(prefix)) == prefix
 */
BOOLEAN CTStrStartsWith(PCSTR str, PCSTR prefix);

/** Returns true if substr(str, strlen(str) - strlen(suffix)) == suffix
 */
BOOLEAN CTStrEndsWith(PCSTR str, PCSTR suffix);

typedef struct {
	unsigned int size, space;
	char *data;
} StringBuffer;

CENTERROR CTStringBufferConstruct(StringBuffer * buffer);

void CTStringBufferDestroy(StringBuffer * buffer);

char *CTStringBufferFreeze(StringBuffer * buffer);

CENTERROR CTStringBufferAppend(StringBuffer * buffer, const char *str);

CENTERROR
CTStringBufferAppendLength(StringBuffer * buffer, const char *str,
			   unsigned int length);

CENTERROR CTStringBufferAppendChar(StringBuffer * buffer, char c);

#endif				/* __CTSTRUTILS_H__ */
