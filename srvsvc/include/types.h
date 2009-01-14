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

#ifndef _TYPES_H_
#define _TYPES_H_

#if !defined(DEFINED_UINT32)

#ifdef _DCE_IDL_
typedef unsigned small int uint8;
#elif !defined(HAVE_UINT8)
typedef unsigned char uint8;
#endif

#ifdef _DCE_IDL_
typedef unsigned short int uint16;
#elif !defined(HAVE_UINT16)
typedef unsigned short int uint16;
#endif

#ifdef _DCE_IDL_
typedef unsigned long int uint32;
#elif !defined(HAVE_UINT32)
typedef unsigned int uint32;
#endif

#ifdef _DCE_IDL_
typedef unsigned hyper int uint64;
#elif !defined(HAVE_UINT64)
typedef unsigned long long int uint64;
#endif

#ifdef _DCE_IDL_
typedef small int int8;
#elif !defined(HAVE_INT8)
typedef char int8;
#endif

#ifdef _DCE_IDL_
typedef short int int16;
#elif !defined(HAVE_INT16)
typedef short int int16;
#endif

#ifdef _DCE_IDL_
typedef long int int32;
#elif !defined(HAVE_INT32)
typedef int int32;
#endif

#ifdef _DCE_IDL_
typedef hyper int int64;
#elif !defined(HAVE_INT64)
typedef long long int int64;
#endif

#ifndef WCHAR16_T_DEFINED
#define WCHAR16_T_DEFINED 1

#ifdef _DCE_IDL_
cpp_quote("#ifndef WCHAR16_T_DEFINED")
cpp_quote("#define WCHAR16_T_DEFINED 1")
#endif

typedef uint16 wchar16_t;

#ifdef _DCE_IDL_
cpp_quote("#endif")
#endif

#endif

#define DEFINED_UINT32
#endif /* !defined (DEFINED_UINT32) */

#if !defined(RPCSTATUS_DEFINED)

typedef uint32 RPCSTATUS;

#define RPCSTATUS_DEFINED
#endif

#if !defined(WINERR_DEFINED)
typedef uint32 WINERR;

#define WINERR_DEFINED
#endif


#if !defined(NTTIME_DEFINED)
typedef uint64 NtTime;

#define NTTIME_DEFINED
#endif

#if !defined(UNISTR_DEFINED)
typedef struct unicode_string {
	uint16 len;
	uint16 size;
#ifdef _DCE_IDL_
	[size_is(size/2),length_is(len/2)]
#endif
	wchar16_t *string;
} UnicodeString;

typedef struct unicode_string_ex {
	uint16 len;
	uint16 size;   /* size = len + 1 (for terminating char) */
#ifdef _DCE_IDL_
	[size_is(size/2),length_is(len/2)]
#endif
	wchar16_t *string;
} UnicodeStringEx;

#define UNISTR_DEFINED
#endif /* !defined(UNISTR_DEFINED) */

#endif /* _TYPES_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
