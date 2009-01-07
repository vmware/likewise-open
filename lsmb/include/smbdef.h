/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        smbdef.h
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *
 */
#ifndef __SMBDEF_H__
#define __SMBDEF_H__

#ifndef SMB_API
#define SMB_API
#endif

#define SMB_SECONDS_IN_MINUTE (60)
#define SMB_SECONDS_IN_HOUR   (60 * SMB_SECONDS_IN_MINUTE)
#define SMB_SECONDS_IN_DAY    (24 * SMB_SECONDS_IN_HOUR)

#ifndef SMB_MAX
#define SMB_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef SMB_MIN
#define SMB_MIN(a, b) (((a) < (b)) ? (a) : (b))
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

#define SMB_DEFAULT_HANDLE_MAX 100000

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

#ifndef SOCKET_DEFINED
typedef int             SOCKET;
#define SOCKET_DEFINED 1
#endif

#endif /* defined(__sparc__) || defined(__ppc__) */

#if defined(HAVE_SOCKLEN_T) && defined(GETSOCKNAME_TAKES_SOCKLEN_T)
#    define SOCKLEN_T socklen_t
#else
#    define SOCKLEN_T int
#endif

#define LW_ASSERT(x)   assert( (x) )

typedef struct _SMB_SECURITY_TOKEN_REP
{
    enum
    {
        SMB_SECURITY_TOKEN_TYPE_PLAIN = 0,
        SMB_SECURITY_TOKEN_TYPE_KRB5 = 1
    } type;
    union _SMB_SECURITY_TOKEN_U
    {
        struct _SMB_SECURITY_TOKEN_PLAIN
        {
            LPWSTR pwszUsername;
            LPWSTR pwszPassword;
        } plain;
        struct _SMB_SECURITY_TOKEN_KRB5
        {
            LPWSTR pwszPrincipal;
            LPWSTR pwszCachePath;
        } krb5;
    } payload;
} SMB_SECURITY_TOKEN_REP, *PSMB_SECURITY_TOKEN_REP;

typedef struct SMB_FILE_HANDLE SMB_FILE_HANDLE, *PSMB_FILE_HANDLE;

typedef USHORT TID;
typedef USHORT FID;
typedef ULONG  LOCKING_ANDX_RANGE;
typedef ULONG  SMB_DATE;
typedef ULONG  SMB_TIME;

#endif /* __SMBDEF_H__ */
