/*
 * Copyright (C) Centeris Corporation 2004-2007
 * Copyright (C) Likewise Software    2007-2008
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 of 
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __LW_EXCEPTION_H__
#define __LW_EXCEPTION_H__

#include "ctsys.h"
#include "ctdef.h"
#include "cterr.h"

typedef struct _LWStackFrame
{
    const char* file;
    unsigned int line;
    struct _LWStackFrame *down;
} LWStackFrame;

typedef struct _LWException
{
    CENTERROR code;
    char* shortMsg;
    char* longMsg;
    LWStackFrame stack;
} LWException;

CENTERROR LWExceptionToString(const LWException *conv, PCSTR titlePrefix, BOOLEAN showSymbolicCode, BOOLEAN showTrace, PSTR *result);

CENTERROR LWPrintException(FILE *dest, const LWException *print, BOOLEAN showTrace);

void
LWRaise(
    LWException** dest, 
    CENTERROR code
    );

void
LWRaiseEx(
    LWException** dest,
    CENTERROR code,
    const char* file,
    unsigned int line,
    const char* shortMsg,
    const char* fmt,
    ...
    );

void
LWReraise(
    LWException** dest,
    LWException** src
    );

void
LWReraiseEx(
    LWException** dest,
    LWException** src,
    const char* file,
    unsigned int line
    );

void
LWHandle(
    LWException** exc
    );

#define LW_RAISE(dest, code)			\
    LWRaiseEx(dest, code, __FILE__, __LINE__,	\
	      NULL, NULL)			\

#define LW_RAISE_EX(dest, code, short_msg, __ARGS__...)	\
    LWRaiseEx(dest, code, __FILE__, __LINE__,		\
	      short_msg, ## __ARGS__)			\
    
#define LW_HANDLE(exc)				\
    LWHandle(exc)				\
    
#define LW_RERAISE(dest, src)			\
    LWReraiseEx(dest, src, __FILE__, __LINE__)	\

#define LW_IS_OK(exc)				\
    ((exc) == NULL || (exc)->code == 0)


#define LW_CLEANUP(dest, src)			\
    do						\
    {						\
	LWException** __exc = &(src);		\
	if (!LW_IS_OK(*__exc))			\
	{					\
	    LW_RERAISE(dest, __exc);		\
	    goto cleanup;			\
	}					\
    } while (0)					\

#define LW_CLEANUP_CTERR(dest, err)		\
    do						\
    {						\
	CENTERROR _err = (err);			\
	if (_err)				\
	{					\
	    LW_RAISE(dest, _err);		\
	    goto cleanup;			\
	}					\
    } while (0)					\

#define LW_EXC					\
    __lw_exc__					\

#define LW_TRY(dest, expr)			\
    do						\
    {						\
	LWException* LW_EXC = NULL;		\
	( expr );				\
	LW_CLEANUP(dest, LW_EXC);		\
    } while (0)					\


#endif
