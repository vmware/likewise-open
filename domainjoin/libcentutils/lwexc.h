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

#ifndef __LW_EXCEPTION_H__
#define __LW_EXCEPTION_H__

#include "ctbase.h"

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

#define LW_RAISE_EX(dest, code, short, __ARGS__...)	\
    LWRaiseEx(dest, code, __FILE__, __LINE__,		\
	      short, #__ARGS__)				\

#define LW_HANDLE(exc)				\
    LWHandle(exc)				\

#define LW_RERAISE(dest, src)			\
    LWReraiseEx(dest, src, __FILE__, __LINE__)	\

#define LW_CLEANUP(dest, src)			\
    do						\
    {						\
	LWException* __exc = (src);		\
	if (__exc && __exc->code)		\
	{					\
	    LW_RERAISE(dest, &__exc);		\
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
