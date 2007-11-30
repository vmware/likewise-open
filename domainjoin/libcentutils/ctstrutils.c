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
#include "ctbase.h"
#include <inttypes.h>

CENTERROR CTAllocateString(PCSTR pszInputString, PSTR * ppszOutputString)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	size_t len = 0;
	PSTR pszOutputString = NULL;

	if (!pszInputString || !ppszOutputString) {
		ceError = CENTERROR_INVALID_PARAMETER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}
	len = strlen(pszInputString);
	ceError = CTAllocateMemory(len + 1, (PVOID *) & pszOutputString);
	BAIL_ON_CENTERIS_ERROR(ceError);

	memcpy(pszOutputString, pszInputString, len);
	pszOutputString[len] = 0;

      error:

	*ppszOutputString = pszOutputString;

	return (ceError);
}

CENTERROR CTStrndup(PCSTR pszInputString, size_t size, PSTR * ppszOutputString)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	size_t copylen = 0;
	PSTR pszOutputString = NULL;

	if (!pszInputString || !ppszOutputString) {
		ceError = CENTERROR_INVALID_PARAMETER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}
	copylen = strlen(pszInputString);
	if (copylen > size)
		copylen = size;
	ceError = CTAllocateMemory(copylen + 1, (PVOID *) & pszOutputString);
	BAIL_ON_CENTERIS_ERROR(ceError);

	memcpy(pszOutputString, pszInputString, copylen);
	pszOutputString[copylen] = 0;

      error:

	*ppszOutputString = pszOutputString;

	return (ceError);
}

void CTFreeString(PSTR pszString)
{
	if (pszString) {
		CTFreeMemory(pszString);
	}
}

void CTFreeStringArray(PSTR * ppStringArray, DWORD dwCount)
{
	DWORD i;

	if (ppStringArray) {
		for (i = 0; i < dwCount; i++) {
			if (ppStringArray[i]) {
				CTFreeString(ppStringArray[i]);
			}
		}

		CTFreeMemory(ppStringArray);
	}

	return;
}

void CTFreeNullTerminatedStringArray(PSTR * ppStringArray)
{

	if (ppStringArray) {
		size_t i;
		for (i = 0; ppStringArray[i] != NULL; i++) {
			CTFreeString(ppStringArray[i]);
		}

		CTFreeMemory(ppStringArray);
	}

	return;
}

void CTStrToUpper(PSTR pszString)
{
	//PSTR pszTmp = pszString;

	if (pszString != NULL) {
		while (*pszString != '\0') {
			*pszString = toupper(*pszString);
			pszString++;
		}
	}
}

void CTStrToLower(PSTR pszString)
{
	//PSTR pszTmp = pszString;

	if (pszString != NULL) {
		while (*pszString != '\0') {
			*pszString = tolower(*pszString);
			pszString++;
		}
	}
}

CENTERROR CTEscapeString(PSTR pszOrig, PSTR * ppszEscapedString)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	int nQuotes = 0;
	PSTR pszTmp = pszOrig;
	PSTR pszNew = NULL;
	PSTR pszNewTmp = NULL;

	if (!ppszEscapedString || !pszOrig) {
		ceError = CENTERROR_INVALID_PARAMETER;
		BAIL_ON_CENTERIS_ERROR(ceError);
	}

	while (pszTmp && *pszTmp) {
		if (*pszTmp == '\'') {
			nQuotes++;
		}
		pszTmp++;
	}

	if (!nQuotes) {
		ceError = CTAllocateString(pszOrig, &pszNew);
		BAIL_ON_CENTERIS_ERROR(ceError);
	} else {
		/*
		 * We are going to escape each single quote and enclose it in two other
		 * single-quotes
		 */
		ceError =
		    CTAllocateMemory(strlen(pszOrig) + 3 * nQuotes + 1,
				     (PVOID *) & pszNew);
		BAIL_ON_CENTERIS_ERROR(ceError);

		pszTmp = pszOrig;
		pszNewTmp = pszNew;

		while (pszTmp && *pszTmp) {
			if (*pszTmp == '\'') {
				*pszNewTmp++ = '\'';
				*pszNewTmp++ = '\\';
				*pszNewTmp++ = '\'';
				*pszNewTmp++ = '\'';
				pszTmp++;
			} else {
				*pszNewTmp++ = *pszTmp++;
			}
		}
		*pszNewTmp = '\0';
	}

	*ppszEscapedString = pszNew;
	pszNew = NULL;

      error:

	CT_SAFE_FREE_STRING(pszNew);

	return ceError;
}

void CTStripLeadingWhitespace(PSTR pszString)
{
	PSTR pszNew = pszString;
	PSTR pszTmp = pszString;

	if (pszString == NULL || *pszString == '\0'
	    || !isspace((int)*pszString)) {
		return;
	}

	while (pszTmp != NULL && *pszTmp != '\0' && isspace((int)*pszTmp)) {
		pszTmp++;
	}

	while (pszTmp != NULL && *pszTmp != '\0') {
		*pszNew++ = *pszTmp++;
	}
	*pszNew = '\0';
}

void CTStripTrailingWhitespace(PSTR pszString)
{
	PSTR pszLastSpace = NULL;
	PSTR pszTmp = pszString;

	if (pszString == NULL || *pszString == '\0') {
		return;
	}

	while (pszTmp != NULL && *pszTmp != '\0') {
		pszLastSpace =
		    (isspace((int)*pszTmp)
		     ? (pszLastSpace ? pszLastSpace : pszTmp) : NULL);
		pszTmp++;
	}

	if (pszLastSpace != NULL) {
		*pszLastSpace = '\0';
	}
}

void CTStripWhitespace(PSTR pszString)
{
	if (pszString == NULL || *pszString == '\0') {
		return;
	}

	CTStripLeadingWhitespace(pszString);
	CTStripTrailingWhitespace(pszString);
}

CENTERROR CTAllocateStringPrintfV(PSTR * result, PCSTR format, va_list args)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	char *smallBuffer;
	unsigned int bufsize;
	int requiredLength;
	unsigned int newRequiredLength;
	PSTR outputString = NULL;
	va_list args2;

	va_copy(args2, args);

	bufsize = 4;
	/* Use a small buffer in case libc does not like NULL */
	do {
		ceError = CTAllocateMemory(bufsize, (PVOID *) & smallBuffer);
		CLEANUP_ON_CENTERROR(ceError);
		requiredLength = vsnprintf(smallBuffer, bufsize, format, args);
		if (requiredLength < 0) {
			bufsize *= 2;
		}
		CTFreeMemory(smallBuffer);
	} while (requiredLength < 0);

	if (requiredLength >= (UINT32_MAX - 1)) {
		ceError = CENTERROR_OUT_OF_MEMORY;
		CLEANUP_ON_CENTERROR(ceError);
	}

	ceError =
	    CTAllocateMemory(requiredLength + 2, (PVOID *) & outputString);
	CLEANUP_ON_CENTERROR(ceError);

	newRequiredLength =
	    vsnprintf(outputString, requiredLength + 1, format, args2);
	if (newRequiredLength < 0) {
		ceError = CTMapSystemError(errno);
		CLEANUP_ON_CENTERROR(ceError);
	} else if (newRequiredLength > requiredLength) {
		/* unexpected, ideally should log something, or use better error code */
		ceError = CENTERROR_OUT_OF_MEMORY;
		CLEANUP_ON_CENTERROR(ceError);
	} else if (newRequiredLength < requiredLength) {
		/* unexpected, ideally should log something -- do not need an error, though */
	}

      cleanup:
	va_end(args2);

	if (ceError) {
		if (outputString) {
			CTFreeMemory(outputString);
			outputString = NULL;
		}
	}
	*result = outputString;
	return ceError;
}

CENTERROR CTAllocateStringPrintf(PSTR * result, PCSTR format, ...
    )
{
	CENTERROR ceError;

	va_list args;
	va_start(args, format);
	ceError = CTAllocateStringPrintfV(result, format, args);
	va_end(args);

	return ceError;
}

BOOLEAN CTStrStartsWith(PCSTR str, PCSTR prefix)
{
	if (prefix == NULL)
		return TRUE;
	if (str == NULL)
		return FALSE;

	return strncmp(str, prefix, strlen(prefix)) == 0;
}

BOOLEAN CTStrEndsWith(PCSTR str, PCSTR suffix)
{
	size_t strLen, suffixLen;
	if (suffix == NULL)
		return TRUE;
	if (str == NULL)
		return FALSE;

	strLen = strlen(str);
	suffixLen = strlen(suffix);
	if (suffixLen > strLen)
		return FALSE;

	return strcmp(str + strLen - suffixLen, suffix) == 0;
}

CENTERROR CTStringBufferConstruct(StringBuffer * buffer)
{
	CENTERROR ceError = CENTERROR_SUCCESS;
	buffer->size = 0;
	buffer->space = 32;
	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTAllocateMemory(buffer->space,
						(PVOID *) & buffer->data));

      error:

	return ceError;
}

void CTStringBufferDestroy(StringBuffer * buffer)
{
	CTFreeMemory(buffer->data);
	buffer->size = buffer->space = 0;
	buffer->data = NULL;
}

char *CTStringBufferFreeze(StringBuffer * buffer)
{
	char *data = buffer->data;
	buffer->size = buffer->space = 0;
	buffer->data = NULL;

	return data;
}

CENTERROR EnsureSpace(StringBuffer * buffer, unsigned int space)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	if (buffer->space >= space)
		goto error;

	while (buffer->space < space) {
		buffer->space *= 2;
	}

	BAIL_ON_CENTERIS_ERROR(ceError =
			       CTReallocMemory(buffer->data,
					       (PVOID *) & buffer->data,
					       buffer->space));

      error:
	return ceError;
}

CENTERROR CTStringBufferAppend(StringBuffer * buffer, const char *str)
{
	return CTStringBufferAppendLength(buffer, str, strlen(str));
}

CENTERROR
CTStringBufferAppendLength(StringBuffer * buffer, const char *str,
			   unsigned int length)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	BAIL_ON_CENTERIS_ERROR(ceError =
			       EnsureSpace(buffer, buffer->size + length + 1));
	memcpy(buffer->data + buffer->size, str, length);
	buffer->data[buffer->size + length] = 0;
	buffer->size += length;

      error:
	return ceError;
}

CENTERROR CTStringBufferAppendChar(StringBuffer * buffer, char c)
{
	CENTERROR ceError = CENTERROR_SUCCESS;

	BAIL_ON_CENTERIS_ERROR(ceError = EnsureSpace(buffer, buffer->size + 2));
	buffer->data[buffer->size] = c;
	buffer->data[buffer->size + 1] = 0;
	buffer->size++;

      error:
	return ceError;
}
