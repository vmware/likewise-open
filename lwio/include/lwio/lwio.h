/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        lsmb.h
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        Public API

 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 */
#ifndef __LWIO_H__
#define __LWIO_H__

#include <stdarg.h>
#include <lw/base.h>
#include <lwio/io-types.h>

typedef unsigned char uchar8_t;

#define NUL  ((uchar8_t) 0)
#define WNUL ((wchar16_t) 0)

typedef uint32_t SMB_ERROR;

#define SMB_ERROR_SUCCESS                      0x0000
#define SMB_ERROR_INVALID_CACHE_PATH           0xF001 // 61441
#define SMB_ERROR_INVALID_CONFIG_PATH          0xF002 // 61442
#define SMB_ERROR_INVALID_PREFIX_PATH          0xF003 // 61443
#define SMB_ERROR_INSUFFICIENT_BUFFER          0xF004 // 61444
#define SMB_ERROR_OUT_OF_MEMORY                0xF005 // 61445
#define SMB_ERROR_NOT_IMPLEMENTED              0xF006 // 61446
#define SMB_ERROR_REGEX_COMPILE_FAILED         0xF007 // 61447
#define SMB_ERROR_INTERNAL                     0xF008 // 61448
#define SMB_ERROR_INVALID_PARAMETER            0xF009 // 61449
#define SMB_ERROR_INVALID_CONFIG               0xF00A // 61450
#define SMB_ERROR_UNEXPECTED_TOKEN             0xF00B // 61451
#define SMB_ERROR_NULL_BUFFER                  0xF00C // 61452
#define SMB_ERROR_INVALID_LOG_LEVEL            0xF00D // 61453
#define SMB_ERROR_LWMSG_ERROR                  0xF00E // 61454
#define SMB_ERROR_MALFORMED_REQUEST            0xF00F // 61455
#define SMB_ERROR_LWMSG_EOF                    0xF010 // 61456
#define SMB_ERROR_NO_SUCH_ITEM                 0xF011 // 61457
#define SMB_ERROR_OVERFLOW                     0xF012 // 61458
#define SMB_ERROR_UNDERFLOW                    0xF013 // 61459
#define SMB_ERROR_SYSTEM                       0xF014 // 61460
#define SMB_ERROR_SERVER_UNREACHABLE           0xF015 // 61461
#define SMB_ERROR_STRING_CONV_FAILED           0xF016 // 61462
#define SMB_ERROR_PASSWORD_EXPIRED             0xF017 // 61463
#define SMB_ERROR_PASSWORD_MISMATCH            0xF018 // 61464
#define SMB_ERROR_CLOCK_SKEW                   0xF019 // 61465
#define SMB_ERROR_KRB5_NO_KEYS_FOUND           0xF01A // 61466
#define SMB_ERROR_KRB5_CALL_FAILED             0xF01B // 61467
#define SMB_ERROR_NO_BIT_AVAILABLE             0xF01C // 61468
#define SMB_ERROR_INVALID_HANDLE               0xF01D // 61469
#define SMB_ERROR_OUT_OF_HANDLES               0xF01E // 61470
#define SMB_ERROR_GSS                          0xF01F // 61471
#define SMB_ERROR_HOST_NOT_FOUND               0xF020 // 61472
#define SMB_ERROR_INVALID_VFS_PROVIDER         0xF021 // 61473
#define SMB_ERROR_NO_SUCH_SHARE                0xF022 // 61474
#define SMB_ERROR_NO_MORE_SHARES               0xF023 // 61475
#define SMB_ERROR_DATA_ERROR                   0xF024 // 61476
#define SMB_ERROR_SENTINEL                     0xF025 // 61477

#define SMB_ERROR_MASK(_e_)                    (_e_ & 0xF000)

/*
 * Logging
 */
typedef enum
{
    SMB_LOG_LEVEL_ALWAYS = 0,
    SMB_LOG_LEVEL_ERROR,
    SMB_LOG_LEVEL_WARNING,
    SMB_LOG_LEVEL_INFO,
    SMB_LOG_LEVEL_VERBOSE,
    SMB_LOG_LEVEL_DEBUG
} SMBLogLevel;

typedef enum
{
    SMB_LOG_TARGET_DISABLED = 0,
    SMB_LOG_TARGET_CONSOLE,
    SMB_LOG_TARGET_FILE,
    SMB_LOG_TARGET_SYSLOG
} SMBLogTarget;

typedef VOID (*PFN_SMB_LOG_MESSAGE)(
                            HANDLE      hLog,
                            SMBLogLevel logLevel,
                            PCSTR       pszFormat,
                            va_list     msgList
                            );

typedef struct __SMB_LOG_INFO {
    SMBLogLevel  maxAllowedLogLevel;
    SMBLogTarget logTarget;
    PSTR         pszPath;
} SMB_LOG_INFO, *PSMB_LOG_INFO;

LW_NTSTATUS
LwIoInitialize(
    VOID
    );

LW_NTSTATUS
LwIoShutdown(
    VOID
    );

LW_NTSTATUS
LwIoOpenContext(
    LW_PIO_CONTEXT* ppContext
    );

DWORD
SMBRefreshConfiguration(
    HANDLE hConnection
    );

DWORD
SMBSetLogLevel(
    HANDLE      hSMBConnection,
    SMBLogLevel logLevel
    );

DWORD
SMBGetLogInfo(
    HANDLE         hSMBConnection,
    PSMB_LOG_INFO* ppLogInfo
    );

DWORD
SMBSetLogInfo(
    HANDLE        hSMBConnection,
    PSMB_LOG_INFO pLogInfo
    );

VOID
SMBFreeLogInfo(
    PSMB_LOG_INFO pLogInfo
    );

LW_NTSTATUS
LwIoCloseContext(
    LW_PIO_CONTEXT pContext
    );

LW_NTSTATUS
LwIoCreatePlainAccessTokenW(
    LW_PCWSTR pwszUsername,
    LW_PCWSTR pwszPassword,
    LW_PIO_ACCESS_TOKEN* ppAccessToken
    );

LW_NTSTATUS
LwIoCreatePlainAccessTokenA(
    PCSTR pszUsername,
    PCSTR pszPassword,
    LW_PIO_ACCESS_TOKEN* ppAccessToken
    );

LW_NTSTATUS
LwIoCreateKrb5AccessTokenW(
    PCWSTR pwszPrincipal,
    PCWSTR pwszCachePath,
    LW_PIO_ACCESS_TOKEN* ppAccessToken
    );

LW_NTSTATUS
LwIoCreateKrb5AccessTokenA(
    PCSTR pszPrincipal,
    PCSTR pszCachePath,
    LW_PIO_ACCESS_TOKEN* ppAccessToken
    );

BOOLEAN
LwIoCompareAccessTokens(
    LW_PIO_ACCESS_TOKEN pAccessToken1,
    LW_PIO_ACCESS_TOKEN pAccessToken2
    );

LW_NTSTATUS
LwIoCopyAccessToken(
    LW_PIO_ACCESS_TOKEN pAccessToken,
    LW_PIO_ACCESS_TOKEN * ppCopy
    );

VOID
LwIoDeleteAccessToken(
    LW_PIO_ACCESS_TOKEN pAccessToken
    );

#ifndef LW_NO_THREADS

LW_NTSTATUS
LwIoOpenContextShared(
    LW_PIO_CONTEXT* ppContext
    );

LW_NTSTATUS
LwIoSetThreadAccessToken(
    LW_PIO_ACCESS_TOKEN pToken
    );

LW_NTSTATUS
LwIoGetThreadAccessToken(
    LW_PIO_ACCESS_TOKEN* ppToken
    );

#endif /* ! SMB_NO_THREADS */

#include <lwio/ntfileapi.h>
#include <lwio/smbfileapi.h>

#endif /* __LWIO_H__ */
