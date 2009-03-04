/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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

/*
 * Module Name:
 *
 *        types.h
 *
 * Abstract:
 *
 *        Common type and constant definitions
 *
 * Authors: Brian Koropoff (bkoropoff@likewisesoftware.com)
 *
 */

#ifndef __LWBASE_TYPES_H__
#define __LWBASE_TYPES_H__

#include <stddef.h>
#include <inttypes.h>
#include <wchar16.h>
#include <sys/types.h>

typedef void               LW_VOID, *LW_PVOID;
typedef void const        *LW_PCVOID;
typedef uint8_t            LW_BOOLEAN, *LW_PBOOLEAN;
typedef uint8_t            LW_BYTE, *LW_PBYTE;
typedef char               LW_CHAR, *LW_PCHAR;
typedef unsigned char      LW_UCHAR, *LW_PUCHAR;
typedef wchar16_t          LW_WCHAR, *LW_PWCHAR;
typedef char              *LW_PSTR;
typedef char const        *LW_PCSTR;
typedef wchar16_t         *LW_PWSTR;
typedef wchar16_t const   *LW_PCWSTR;
typedef int16_t            LW_SHORT, *LW_PSHORT;
typedef uint16_t           LW_USHORT, *LW_PUSHORT;
typedef int32_t            LW_LONG, *LW_PLONG;
typedef uint32_t           LW_ULONG, *LW_PULONG;
typedef int64_t            LW_LONG64, *LW_PLONG64;
typedef uint64_t           LW_ULONG64, *LW_PULONG64;
typedef int                LW_INT, *LW_PINT;
typedef unsigned int       LW_UINT, *LW_PUINT;
typedef uint8_t            LW_UINT8, *LW_PUINT8;
typedef uint16_t           LW_UINT16, *LW_PUINT16;
typedef uint32_t           LW_UINT32, *LW_PUINT32;
typedef uint64_t           LW_UINT64, *LW_PUINT64;
typedef int8_t             LW_INT8, *LW_PINT8;
typedef int16_t            LW_INT16, *LW_PINT16;
typedef int32_t            LW_INT32, *LW_PINT32;
typedef int64_t            LW_INT64, *LW_PINT64;
typedef void              *LW_HANDLE, **LW_PHANDLE;
typedef size_t             LW_SIZE_T, *LW_PSIZE_T;
typedef ssize_t            LW_SSIZE_T, *LW_PSSIZE_T;
typedef size_t             LW_ULONG_PTR, *LW_PULONG_PTR;
typedef ssize_t            LW_LONG_PTR, *LW_PLONG_PTR;

typedef int32_t            LW_BOOL, *LW_PBOOL;
typedef uint16_t           LW_WORD, *LW_PWORD;
typedef uint32_t           LW_DWORD, *LW_PDWORD;

#ifdef UNICODE
typedef LW_WCHAR           LW_TCHAR;
#else
typedef LW_CHAR            LW_TCHAR;
#endif 

#define LW_TRUE  1
#define LW_FALSE 0

#define LW_INVALID_HANDLE_VALUE 0

#define LW_MAXUCHAR ((LW_UCHAR) -1)
#define LW_MAXCHAR ((LW_CHAR) (LW_MAXUCHAR >> 1))
#define LW_MINCHAR ((LW_CHAR) ~LW_MAXCHAR)

#define LW_MAXUSHORT ((LW_USHORT) -1)
#define LW_MAXSHORT ((LW_SHORT) (LW_MAXUSHORT >> 1))
#define LW_MINSHORT ((LW_SHORT) ~LW_MAXSHORT)

#define LW_MAXULONG ((LW_ULONG) -1)
#define LW_MAXLONG ((LW_LONG) (LW_MAXULONG >> 1))
#define LW_MINLONG ((LW_LONG) ~LW_MAXLONG)

#define LW_MAXULONG64 ((LW_ULONG64) -1)
#define LW_MAXLONG64 ((LW_LONG64) (LW_MAXULONG64 >> 1))
#define LW_MINLONG64 ((LW_LONG64) ~LW_MAXLONG64)

#define LW_MAXSIZE_T ((LW_SIZE_T) -1)
#define LW_MAXSSIZE_T ((LW_SSIZE_T) (LW_MAXSIZE_T >> 1))
#define LW_MINSSIZE_T ((LW_SSIZE_T) ~LW_MAXSSIZE_T)

#define LW_MAXULONG_PTR ((LW_ULONG_PTR) -1)
#define LW_MAXLONG_PTR ((LW_LONG_PTR) (LW_MAXULONG_PTR >> 1))
#define LW_MINLONG_PTR ((LW_SSIZE_T) ~LW_MAXLONG_PTR)

#define LW_MAXBYTE ((LW_BYTE) -1)
#define LW_MAXWORD ((LW_WORD) -1)
#define LW_MAXDWORD ((LW_DWORD) -1)

#define LW_OUT_PPVOID(_expr_) ((LW_PVOID*) (LW_PVOID) (_expr_))

#define LW_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define LW_MAX(a, b) (((a) > (b)) ? (a) : (b))

#define LwSetFlag(Variable, Flags)   ((Variable) |= (Flags))
#define LwClearFlag(Variable, Flags) ((Variable) &= ~(Flags))
#define LwIsSetFlag(Variable, Flags) (((Variable) & (Flags)) != 0)

#define LW_IS_BOTH_OR_NEITHER(Condition1, Condition2) \
    !(!!(Condition1) ^ !!(Condition2))

#define LW_IS_VALID_FLAGS(Flags, Mask) \
    (((Flags) & ~(Mask)) == 0)

#define LwRtlPointerToOffset(BasePointer, Pointer) \
    ((int)((char*)(Pointer) - (char*)(BasePointer)))

#define LwRtlOffsetToPointer(Pointer, Offset) \
    ((char*)(Pointer) + Offset)

// TODO-Remove these deprecated macro names
#define LW_PTR_OFFSET(BasePointer, Pointer) LwRtlPointerToOffset(BasePointer, Pointer)
#define LW_PTR_ADD(Pointer, Offset) LwRtlOffsetToPointer(Pointer, Offset)

#define LW_FIELD_OFFSET(Type, Field) \
    ((size_t)(&(((Type*)(0))->Field)))

#define LW_FIELD_SIZE(Type, Field) \
    (sizeof(((Type*)(0))->Field))

#define LW_STRUCT_FROM_FIELD(Pointer, Type, Field) \
    ((Type*)LwRtlOffsetToPointer(Pointer, -((ssize_t)LW_FIELD_OFFSET(Type, Field))))

#define LW_ARRAY_SIZE(StaticArray) \
    (sizeof(StaticArray)/sizeof((StaticArray)[0]))

#ifndef LW_STRICT_NAMESPACE

typedef LW_VOID     VOID;
typedef LW_PVOID    PVOID;
typedef LW_PCVOID   PCVOID;
typedef LW_BOOLEAN  BOOLEAN;
typedef LW_PBOOLEAN PBOOLEAN;
typedef LW_BYTE     BYTE;
typedef LW_PBYTE    PBYTE;
typedef LW_CHAR     CHAR;
typedef LW_WCHAR    WCHAR;
typedef LW_PCHAR    PCHAR;
typedef LW_PWCHAR   PWCHAR;
typedef LW_UCHAR    UCHAR;
typedef LW_PUCHAR   PUCHAR;
typedef LW_PSTR     PSTR;
typedef LW_PCSTR    PCSTR;
typedef LW_PWSTR    PWSTR;
typedef LW_PCWSTR   PCWSTR;
typedef LW_SHORT    SHORT;
typedef LW_PSHORT   PSHORT;
typedef LW_USHORT   USHORT;
typedef LW_PUSHORT  PUSHORT;
typedef LW_LONG     LONG;
typedef LW_PLONG    PLONG;
typedef LW_ULONG    ULONG;
typedef LW_PULONG   PULONG;
typedef LW_LONG64   LONG64;
typedef LW_PLONG64  PLONG64;
typedef LW_ULONG64  ULONG64;
typedef LW_PULONG64 PULONG64;
typedef LW_INT      INT;
typedef LW_PINT     PINT;
typedef LW_UINT     UINT;
typedef LW_PUINT    PUINT;
typedef LW_UINT8    UINT8;
typedef LW_PUINT8   PUINT8;
typedef LW_UINT16   UINT16;
typedef LW_PUINT16  PUINT16;
typedef LW_UINT32   UINT32;
typedef LW_PUINT32  PUINT32;
typedef LW_UINT64   UINT64;
typedef LW_PUINT64  PUINT64;
typedef LW_INT8     INT8;
typedef LW_PINT8    PINT8;
typedef LW_INT16    INT16;
typedef LW_PINT16   PINT16;
typedef LW_INT32    INT32;
typedef LW_PINT32   PINT32;
typedef LW_INT64    INT64;
typedef LW_PINT64   PINT64;
typedef LW_HANDLE   HANDLE;
typedef LW_PHANDLE  PHANDLE;
typedef LW_SIZE_T   SIZE_T;
typedef LW_PSIZE_T  PSIZE_T;
typedef LW_SSIZE_T  SSIZE_T;
typedef LW_PSSIZE_T PSSIZE_T;
typedef LW_ULONG_PTR ULONG_PTR;
typedef LW_PULONG_PTR PULONG_PTR;
typedef LW_LONG_PTR LONG_PTR;
typedef LW_PLONG_PTR PLONG_PTR;

typedef LW_BOOL     BOOL;
typedef LW_PBOOL    PBOOL;
typedef LW_WORD     WORD;
typedef LW_PWORD    PWORD;
typedef LW_DWORD    DWORD;
typedef LW_PDWORD   PDWORD;

typedef LW_TCHAR    TCHAR;

#ifndef TRUE
#define TRUE LW_TRUE
#endif

#ifndef FALSE
#define FALSE LW_FALSE
#endif

#define INVALID_HANDLE_VALUE LW_INVALID_HANDLE_VALUE

#define MAXUCHAR LW_MAXUCHAR
#define MAXCHAR LW_MAXCHAR
#define MINCHAR LW_MINCHAR

#define MAXUSHORT LW_MAXUSHORT
#define MAXSHORT LW_MAXSHORT
#define MINSHORT LW_MINSHORT

#define MAXULONG LW_MAXULONG
#define MAXLONG LW_MAXLONG
#define MINLONG LW_MINLONG

#define MAXULONG64 LW_MAXULONG64
#define MAXLONG64 LW_MAXLONG64
#define MINLONG64 LW_MINLONG64

#define MAXSIZE_T LW_MAXSIZE_T
#define MAXSSIZE_T LW_MAXSSIZE_T
#define MINSSIZE_T LW_MINSSIZE_T

#define MAXULONG_PTR LW_MAXULONG_PTR
#define MAXLONG_PTR LW_MAXLONG_PTR
#define MINLONG_PTR LW_MINLONG_PTR

#define MAXBYTE LW_MAXBYTE
#define MAXWORD LW_MAXWORD
#define MAXDWORD LW_MAXDWORD

#define BYTE_MAX MAXBYTE
#define WORD_MAX MAXWORD
#define DWORD_MAX MAXDWORD

#define OUT_PPVOID LW_OUT_PPVOID

#define SetFlag(Variable, Flags) LwSetFlag(Variable, Flags)
#define ClearFlag(Variable, Flags) LwClearFlag(Variable, Flags)
#define IsSetFlag(Variable, Flags) LwIsSetFlag(Variable, Flags)

#define IS_BOTH_OR_NEITHER(Condition1, Condition2) LW_IS_BOTH_OR_NEITHER(Condition1, Condition2)

#endif /* LW_STRICT_NAMESPACE */

#endif
