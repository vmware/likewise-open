/*
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

#include "lwexc.h"

static LWException memExc =
{
    .code = CENTERROR_OUT_OF_MEMORY,
    .shortMsg = "Out of memory",
    .longMsg = "A memory allocation failed due to insufficient system resources.",
    .stack =
    {
	NULL,
	0,
	NULL
    }
};

static LWException successExc =
{
    .code = CENTERROR_SUCCESS,
    .shortMsg = "Success",
    .longMsg = "The operation succeeded without error.",
    .stack =
    {
	NULL,
	0,
	NULL
    }
};

static LWException*
CreateException(
    CENTERROR code,
    const char* file,
    unsigned int line,
    char* shortMsg,
    char* longMsg
    )
{
    LWException* exc;

    switch (code)
    {
    case CENTERROR_SUCCESS:
	return &successExc;
    case CENTERROR_OUT_OF_MEMORY:
	return &memExc;
    default:
	exc = malloc(sizeof(*exc));
	if (!exc)
	    return &memExc;
	exc->code = code;
	exc->stack.file = file;
	exc->stack.line = line;
	exc->stack.down = NULL;
	exc->shortMsg = shortMsg;
	exc->longMsg = longMsg;

	return exc;
    }
}

void
LWRaise(
    LWException** dest,
    CENTERROR code
    )
{
    CENTERROR ceError;
    char *shortMsg;
    char *longMsg;
    const char* desc = CTErrorDescription(code);
    const char* help = CTErrorHelp(code);

    if (!desc)
    {
	shortMsg = NULL;
    }
    else if ((ceError = CTAllocateString(desc, &shortMsg)));
    {
	*dest = CreateException(ceError, __FILE__, __LINE__, NULL, NULL);
	return;
    }

    if (!help)
    {
	longMsg = NULL;
    }
    else if ((ceError = CTAllocateString(help, &longMsg)))
    {
	*dest = CreateException(ceError, __FILE__, __LINE__, NULL, NULL);
	return;
    }

    *dest = CreateException(code, NULL, 0, shortMsg, longMsg);
}

void
LWRaiseEx(
    LWException** dest,
    CENTERROR code,
    const char* file,
    unsigned int line,
    const char* _shortMsg,
    const char* fmt,
    ...
    )
{
    if (dest)
    {
	CENTERROR ceError;
	char* shortMsg;
	char* longMsg;
	va_list ap;

	va_start(ap, fmt);

	if (!_shortMsg)
	{
	    _shortMsg = CTErrorDescription(code);
	}

	if (!fmt)
	{
	    fmt = CTErrorHelp(code);
	}

	if (_shortMsg)
	{
	    if ((ceError = CTAllocateString(_shortMsg, &shortMsg)))
	    {
		*dest = CreateException(ceError, __FILE__, __LINE__, NULL, NULL);
		return;
	    }
	}
	else
	{
	    shortMsg = NULL;
	}

	if (fmt)
	{
	    if ((ceError = CTAllocateStringPrintfV(&longMsg, fmt, ap)))
	    {
		CTFreeString(shortMsg);
		*dest = CreateException(ceError, __FILE__, __LINE__, NULL, NULL);
		return;
	    }
	}
	else
	{
	    longMsg = NULL;
	}

	*dest = CreateException(code, file, line, shortMsg, longMsg);
    }
}

void
LWReraise(
    LWException** dest,
    LWException** src
    )
{
    if (dest)
    {
	*dest = *src;
	*src = NULL;
    }
    else
    {
	LWHandle(src);
    }
}

void
LWReraiseEx(
    LWException** dest,
    LWException** src,
    const char* file,
    unsigned int line
    )
{
    if (dest)
    {
	LWStackFrame* down = malloc(sizeof(*down));

	if (!down)
	{
	    LWHandle(src);
	    *dest = CreateException(CENTERROR_OUT_OF_MEMORY, file, line, NULL, NULL);
	}
	else
	{
	    *dest = *src;
	    *src = NULL;

	    *down = (*dest)->stack;
	    (*dest)->stack.file = file;
	    (*dest)->stack.line = line;
	    (*dest)->stack.down = down;
	}
    }
    else
    {
	LWHandle(src);
    }
}

void
LWHandle(
    LWException** exc
    )
{
    if (exc && *exc)
    {
	LWStackFrame* frame;

	for (frame = (*exc)->stack.down; frame; frame = frame->down)
	{
	    free(frame);
	}

	if ((*exc)->shortMsg)
	    free((*exc)->shortMsg);
	if ((*exc)->longMsg)
	    free((*exc)->longMsg);
	free(*exc);
	*exc = NULL;
    }
}
