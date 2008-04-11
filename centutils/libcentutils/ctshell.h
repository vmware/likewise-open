/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
* -*- mode: c, c-basic-offset: 4 -*- */

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

#ifndef __CTSHELL_H__
#define __CTSHELL_H__

#include "ctbase.h"

typedef struct CTShellVar
{
    enum
    {
        SVAR_INT,
        SVAR_STR,
        SVAR_ARR,
	SVAR_ZERO,
        SVAR_OUT
    } type;
    const char* name;
    union
    {
        int integer;
        const char* string;
        char const * const * array;
        char** out;
    } value;
} CTShellVar;

struct CTShellVar __CTVarInteger(const char* name, int value);
struct CTShellVar __CTVarString(const char* name, const char* value);
struct CTShellVar __CTVarArray(const char* name, char const * const * value);
struct CTShellVar __CTVarOut(const char* name, char** out);
struct CTShellVar __CTVarZero(const char* name);

#define CTSHELL_INTEGER(name, value) (__CTVarInteger( #name , value))
#define CTSHELL_STRING(name, value) (__CTVarString( #name , value))
#define CTSHELL_ARRAY(name, value) (__CTVarArray( #name , (char const * const *) (value)))
#define CTSHELL_BUFFER(name, value) (__CTVarOut( #name , value))
#define CTSHELL_ZERO(name) (__CTVarZero( #name ))

CENTERROR
CTShell(const char* format, ...);

CENTERROR
CTShellEx(char * const envp[], const char* format, ...);

#endif
