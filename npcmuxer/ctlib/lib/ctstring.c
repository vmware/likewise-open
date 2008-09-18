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

#include <ctstring.h>
#include <ctype.h>
#include <stdlib.h>
#include <ctgoto.h>
#include <ctmemory.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#ifndef UINT32_MAX
#define UINT32_MAX ((uint32_t) (0xFFFFFFFF))
#endif

void
CtStripLeadingWhitespace(
    IN OUT char* String
    )
{
    if (String && isspace((int) *String))
    {
        char* start;
        char* copy = String;
        for (start = String; isspace((int) *start); start++);
        for (copy = String; *start; *copy++ = *start++);
        *copy = 0;
    }
}

void
CtStripTrailingWhitespace(
    IN OUT char* String
    )
{
    if (String && *String)
    {
        char* lastSpace = NULL;
        char* temp;
        for (temp = String; *temp; temp++)
        {
            if (!isspace((int) *temp))
            {
                lastSpace = NULL;
            }
            else if (!lastSpace)
            {
                lastSpace = temp;
            }
        }
        if (lastSpace)
        {
            *lastSpace = 0;
        }
    }
}

void
CtStripWhitespace(
    IN OUT char* String
    )
{
    if (String && *String)
    {
        CtStripLeadingWhitespace(String);
        CtStripTrailingWhitespace(String);
    }
}


CT_STATUS
CtAllocateStringPrintfV(
    IN char** Result,
    IN const char* Format,
    IN va_list Args
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    char* smallBuffer;
    unsigned int bufsize;
    int requiredLength;
    unsigned int newRequiredLength;
    char* outputString = NULL;
    va_list args2;

    va_copy(args2, Args);

    bufsize = 4;
    /* Use a small buffer in case libc does not like NULL */
    do {
        status = CtAllocateMemory((void**) &smallBuffer, bufsize);
        GOTO_CLEANUP_ON_STATUS(status);
        requiredLength = vsnprintf(smallBuffer, bufsize, Format, Args);
        if (requiredLength < 0)
        {
            bufsize *= 2;
        }
        CtFreeMemory(smallBuffer);
    } while (requiredLength < 0);

    if (requiredLength >= (UINT32_MAX - 1))
    {
        status = CT_STATUS_OUT_OF_MEMORY;
        GOTO_CLEANUP();
    }

    status = CtAllocateMemory((void**)&outputString, requiredLength + 2);
    GOTO_CLEANUP_ON_STATUS(status);

    newRequiredLength = vsnprintf(outputString, requiredLength + 1, Format, args2);
    if (newRequiredLength < 0)
    {
        status = CT_ERRNO_TO_STATUS(errno);
        GOTO_CLEANUP();
    }
    else if (newRequiredLength > requiredLength)
    {
        /* unexpected, ideally should log something, or use better error code */
        status = CT_STATUS_OUT_OF_MEMORY;
        GOTO_CLEANUP();
    }
    else if (newRequiredLength < requiredLength)
    {
        /* unexpected, ideally should log something -- do not need an error, though */
    }

cleanup:
    va_end(args2);

    if (status)
    {
        CT_SAFE_FREE(outputString);
    }
    *Result = outputString;
    return status;
}

CT_STATUS
CtAllocateStringPrintf(
    OUT char** Result,
    IN const char* Format,
    IN ...
    )
{
    CT_STATUS status;

    va_list args;
    va_start(args, Format);
    status = CtAllocateStringPrintfV(Result, Format, args);
    va_end(args);

    return status;
}

CT_STATUS
CtAllocateString(
    OUT char** Result,
    IN const char* String
    )
{
    CT_STATUS status = CT_STATUS_SUCCESS;
    size_t length = 0;
    char* result = NULL;

    if (!Result || !String)
    {
        status = CT_STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    length = strlen(String);

    status = CtAllocateMemory((void**)&result, length + 1);
    GOTO_CLEANUP_ON_STATUS(status);

    memcpy(result, String, length + 1);

cleanup:
    if (status)
    {
        CT_SAFE_FREE(result);
    }

    *Result = result;
    return status;
}

CT_STATUS
CtCopyString(
    OUT char* Result,
    IN size_t ResultSize,
    IN const char* String
    )
{
    CT_STATUS status;
    size_t requiredSize;

    if (!String || !Result)
    {
        status = CT_STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    requiredSize = strlen(String) + 1;
    if (ResultSize < requiredSize)
    {
        /* TODO -- better error */
        status = CT_STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP();
    }

    memcpy(Result, String, requiredSize);

    status = CT_STATUS_SUCCESS;

cleanup:
    return status;
}

