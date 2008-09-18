/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
 
/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsadef.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS) Client/Server common definitions
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 * 
 */
#ifndef __LSADEF_H__
#define __LSADEF_H__

#define LSASS_API

#define LSA_SECONDS_IN_MINUTE (60)
#define LSA_SECONDS_IN_HOUR   (60 * LSA_SECONDS_IN_MINUTE)
#define LSA_SECONDS_IN_DAY    (24 * LSA_SECONDS_IN_HOUR)

#define LSA_MAX_USER_NAME_LENGTH  256
#define LSA_MAX_GROUP_NAME_LENGTH 256

#ifndef LSA_MAX
#define LSA_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef LSA_MIN
#define LSA_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef true
#define true 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef false
#define false 0
#endif

#ifndef WIN32
#define PATH_SEPARATOR_STR "/"
#else
#define PATH_SEPARATOR_STR "\\"
#endif

#ifndef DWORD_MAX

#ifdef  UINT_MAX
#define DWORD_MAX   UINT_MAX
#else
#define DWORD_MAX   4294967295U
#endif

#endif

#ifndef UINT32_MAX
#define UINT32_MAX UINT_MAX
#endif

#if defined(__sparc__) || defined(__ppc__)

#ifndef uint32_t
#define u_int32_t uint32_t
#endif

#ifndef uint16_t
#define u_int16_t uint16_t
#endif

#ifndef uint8_t
#define u_int8_t  uint8_t
#endif

#endif

#if defined(HAVE_SOCKLEN_T) && defined(GETSOCKNAME_TAKES_SOCKLEN_T)
#    define SOCKLEN_T socklen_t
#else
#    define SOCKLEN_T int
#endif

#ifndef SOCKET_DEFINED
typedef int             SOCKET;
#define SOCKET_DEFINED 1
#endif

#if HAVE_WCHAR16_T

#ifndef PWSTR_DEFINED
#define PWSTR_DEFINED 1

typedef wchar16_t *     PWSTR;

#endif

#ifndef PCWSTR_DEFINED
#define PCWSTR_DEFINED 1

typedef const wchar16_t * PCWSTR;

#endif

#endif /* HAVE_WCHAR16_T */

#define LW_ASSERT(x)   assert( (x) )

#endif /* __LSADEF_H__ */
