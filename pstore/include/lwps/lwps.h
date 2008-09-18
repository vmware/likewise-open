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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwps.h
 *
 * Abstract:
 *
 *        Likewise Password Store API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *           Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __LWPS_H__
#define __LWPS_H__

#ifndef _WIN32

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef DWORD_DEFINED
#define DWORD_DEFINED 1

typedef uint32_t        DWORD, *PDWORD;

#endif

#ifndef INT_DEFINED
#define INT_DEFINED 1

typedef int             INT, *PINT;

#endif

#ifndef UINT64_DEFINED
#define UINT64_DEFINED 1

typedef uint64_t        UINT64, *PUINT64;

#endif

#ifndef UINT32_DEFINED
#define UINT32_DEFINED 1

typedef uint32_t        UINT32, *PUINT32;

#endif

#ifndef UINT16_DEFINED
#define UINT16_DEFINED 1

typedef uint16_t        UINT16, *PUINT16;

#endif

#ifndef WORD_DEFINED
#define WORD_DEFINED 1

typedef uint16_t WORD, *PWORD;

#endif

#ifndef USHORT_DEFINED
#define USHORT_DEFINED 1

typedef unsigned short  USHORT, *PUSHORT;

#endif

#ifndef ULONG_DEFINED
#define ULONG_DEFINED 1

typedef unsigned long   ULONG, *PULONG;

#endif

#ifndef ULONGLONG_DEFINED
#define ULONGLONG_DEFINED 1

typedef unsigned long long ULONGLONG, *PULONGLONG;

#endif

#ifndef UINT8_DEFINED
#define UINT8_DEFINED 1

typedef uint8_t         UINT8, *PUINT8;

#endif

#ifndef BYTE_DEFINED
#define BYTE_DEFINED

typedef uint8_t BYTE, *PBYTE;

#endif

#ifndef UCHAR_DEFINED
#define UCHAR_DEFINED 1

typedef uint8_t UCHAR, *PUCHAR;

#endif

#ifndef HANDLE_DEFINED
#define HANDLE_DEFINED 1

typedef unsigned long   HANDLE, *PHANDLE;

#endif

#ifndef CHAR_DEFINED
#define CHAR_DEFINED 1

typedef char            CHAR;

#endif

#ifndef PSTR_DEFINED
#define PSTR_DEFINED 1

typedef char *          PSTR;

#endif

#ifndef PCSTR_DEFINED
#define PCSTR_DEFINED 1

typedef const char *    PCSTR;

#endif

#ifndef VOID_DEFINED
#define VOID_DEFINED 1

typedef void            VOID, *PVOID;

#endif

#ifndef PCVOID_DEFINED
#define PCVOID_DEFINED 1

typedef const void      *PCVOID;

#endif

#ifndef BOOLEAN_DEFINED
#define BOOLEAN_DEFINED 1

typedef int             BOOLEAN, *PBOOLEAN;

#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#if HAVE_WCHAR16_T

#ifndef PWSTR_DEFINED
#define PWSTR_DEFINED 1

typedef wchar16_t * PWSTR;

#endif

#ifndef PCWSTR_DEFINED
typedef const wchar16_t * PCWSTR;
#define PCWSTR_DEFINED 1
#endif

#endif // HAVE_WCHAR16_T

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef DWORD_MAX
#ifdef UINT_MAX
#define DWORD_MAX UINT_MAX
#else
#define DWORD_MAX 4294967295U
#endif /* UINT_MAX */
#endif /* DWORD_MAX */

#ifndef UINT32_MAX
#define UINT32_MAX UINT_MAX
#endif

#endif


/* ERRORS */
#define LWPS_ERROR_SUCCESS                   0x0000
#define LWPS_ERROR_INVALID_CACHE_PATH        0x4000 // 16384
#define LWPS_ERROR_INVALID_CONFIG_PATH       0x4001 // 16385
#define LWPS_ERROR_INVALID_PREFIX_PATH       0x4002 // 16386
#define LWPS_ERROR_INSUFFICIENT_BUFFER       0x4003 // 16387
#define LWPS_ERROR_OUT_OF_MEMORY             0x4004 // 16388
#define LWPS_ERROR_DATA_ERROR                0x4005 // 16389
#define LWPS_ERROR_NOT_IMPLEMENTED           0x4006 // 16390
#define LWPS_ERROR_REGEX_COMPILE_FAILED      0x4007 // 16391
#define LWPS_ERROR_INTERNAL                  0x4008 // 16392
#define LWPS_ERROR_UNEXPECTED_DB_RESULT      0x4009 // 16393
#define LWPS_ERROR_INVALID_PARAMETER         0x400A // 16394
#define LWPS_ERROR_INVALID_SID_REVISION      0x400B // 16395
#define LWPS_ERROR_LOAD_LIBRARY_FAILED       0x400C // 16396
#define LWPS_ERROR_LOOKUP_SYMBOL_FAILED      0x400D // 16397
#define LWPS_ERROR_INVALID_CONFIG            0x400E // 16398
#define LWPS_ERROR_UNEXPECTED_TOKEN          0x400F // 16399
#define LWPS_ERROR_STRING_CONV_FAILED        0x4010 // 16400
#define LWPS_ERROR_QUERY_CREATION_FAILED     0x4011 // 16401
#define LWPS_ERROR_NOT_SUPPORTED             0x4012 // 16402
#define LWPS_ERROR_NO_SUCH_PROVIDER          0x4013 // 16403
#define LWPS_ERROR_INVALID_PROVIDER          0x4014 // 16404
#define LWPS_ERROR_INVALID_SID               0x4015 // 16405
#define LWPS_ERROR_INVALID_ACCOUNT           0x4016 // 16406
#define LWPS_ERROR_INVALID_HANDLE            0x4017 // 16407
#define LWPS_ERROR_DB_RECORD_NOT_FOUND       0x4018 // 16408
#define LWPS_ERROR_SENTINEL                  0x4019 // 16409

#define LWPS_ERROR_MASK(_e_)             (_e_ & 0x4000)

typedef struct __LWPS_PASSWORD_INFO
{
    wchar16_t* pwszDomainName;
    wchar16_t* pwszDnsDomainName;
    wchar16_t* pwszSID;
    wchar16_t* pwszHostname;
    wchar16_t* pwszMachineAccount;
    wchar16_t* pwszMachinePassword;
    time_t     last_change_time;
    DWORD      dwSchannelType;

} LWPS_PASSWORD_INFO, *PLWPS_PASSWORD_INFO;

typedef enum
{
    LWPS_PASSWORD_STORE_UNKNOWN = 0,
    LWPS_PASSWORD_STORE_DEFAULT,
    LWPS_PASSWORD_STORE_SQLDB,
    LWPS_PASSWORD_STORE_TDB
} LwpsPasswordStoreType;

DWORD
LwpsOpenPasswordStore(
    LwpsPasswordStoreType storeType,
    PHANDLE hStore
    );

DWORD
LwpsGetPasswordByHostName(
    HANDLE hStore,
    PCSTR  pszHostname,
    PLWPS_PASSWORD_INFO* ppInfo
    );

DWORD
LwpsGetPasswordByDomainName(
    HANDLE hStore,
    PCSTR  pszDomainName,
    PLWPS_PASSWORD_INFO* ppInfo
    );

DWORD
LwpsWritePasswordToAllStores(
    PLWPS_PASSWORD_INFO pInfo
    );

DWORD
LwpsDeleteEntriesInAllStores();

VOID
LwpsFreePasswordInfo(
    HANDLE hStore,
    PLWPS_PASSWORD_INFO pInfo
    );

DWORD
LwpsClosePasswordStore(
    HANDLE hStore
    );

BOOLEAN
LwpsIsLwpsError(
    DWORD dwErrorCode
    );

size_t
LwpsGetErrorString(
    DWORD  dwErrorCode,
    PSTR   pszBuffer,
    size_t stBufSize
    );

#endif /* __LWPS_H__ */

