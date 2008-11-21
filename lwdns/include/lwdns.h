/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lwdns.h
 *
 * Abstract:
 *
 *        Likewise DNS (LWDNS)
 *
 *        Public API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LWDNS_H__
#define __LWDNS_H__

#ifndef _WIN32

#if HAVE_INTTYPES_H
#include <inttypes.h>
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

#ifndef SOCKADDR_IN_DEFINED
#define SOCKADDR_IN_DEFINED 1

typedef struct sockaddr_in SOCKADDR_IN, *PSOCKADDR_IN;

#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#endif

#ifndef WIN32

#define ERROR_INVALID_PARAMETER EINVAL
#define ERROR_OUTOFMEMORY       ENOMEM

#endif

#define LWDNS_ERROR_SUCCESS             0
#define LWDNS_ERROR_INIT_FAILED         0xE000 // 57344
#define LWDNS_ERROR_RECORD_NOT_FOUND    0xE001 // 57345
#define LWDNS_ERROR_BAD_RESPONSE        0xE002 // 57346
#define LWDNS_ERROR_PASSWORD_EXPIRED    0xE003 // 57347
#define LWDNS_ERROR_PASSWORD_MISMATCH   0xE004 // 57348
#define LWDNS_ERROR_CLOCK_SKEW          0xE005 // 57349
#define LWDNS_ERROR_KRB5_NO_KEYS_FOUND  0xE006 // 57350
#define LWDNS_ERROR_KRB5_CALL_FAILED    0xE007 // 57351
#define LWDNS_ERROR_RCODE_UNKNOWN       0xE008 // 57352
#define LWDNS_ERROR_RCODE_FORMERR       0xE009 // 57353
#define LWDNS_ERROR_RCODE_SERVFAIL      0xE00A // 57354
#define LWDNS_ERROR_RCODE_NXDOMAIN      0xE00B // 57355
#define LWDNS_ERROR_RCODE_NOTIMP        0xE00C // 57356
#define LWDNS_ERROR_RCODE_REFUSED       0xE00D // 57357
#define LWDNS_ERROR_RCODE_YXDOMAIN      0xE00E // 57358
#define LWDNS_ERROR_RCODE_YXRRSET       0xE00F // 57359
#define LWDNS_ERROR_RCODE_NXRRSET       0xE010 // 57360
#define LWDNS_ERROR_RCODE_NOTAUTH       0xE011 // 57361
#define LWDNS_ERROR_RCODE_NOTZONE       0xE012 // 57362
#define LWDNS_ERROR_NO_NAMESERVER       0xE013 // 57363
#define LWDNS_ERROR_NO_SUCH_ZONE        0xE014 // 57364
#define LWDNS_ERROR_NO_RESPONSE         0xE015 // 57365
#define LWDNS_ERROR_UNEXPECTED          0xE016 // 57366
#define LWDNS_ERROR_NO_SUCH_ADDRESS     0xE017 // 57367
#define LWDNS_ERROR_UPDATE_FAILED       0xE018 // 57368
#define LWDNS_ERROR_NO_INTERFACES       0xE019 // 57369
#define LWDNS_ERROR_INVALID_IP_ADDRESS  0xE01A // 57370
#define LWDNS_ERROR_SENTINEL            0xE01B // 57371

#define LWDNS_ERROR_MASK(_e_)           (_e_ & 0xE000)

/*
 * Logging
 */
#define LWDNS_INFO_TAG     "INFO"
#define LWDNS_ERROR_TAG    "ERROR"
#define LWDNS_WARN_TAG     "WARNING"
#define LWDNS_INFO_TAG     "INFO"
#define LWDNS_VERBOSE_TAG  "VERBOSE"
#define LWDNS_DEBUG_TAG    "VERBOSE"

#define LWDNS_LOG_TIME_FORMAT "%Y%m%d%H%M%S"

typedef enum
{
    LWDNS_LOG_LEVEL_ALWAYS = 0,
    LWDNS_LOG_LEVEL_ERROR,
    LWDNS_LOG_LEVEL_WARNING,
    LWDNS_LOG_LEVEL_INFO,
    LWDNS_LOG_LEVEL_VERBOSE,
    LWDNS_LOG_LEVEL_DEBUG
} LWDNSLogLevel;

typedef VOID (*PFN_LWDNS_LOG_MESSAGE)(
                LWDNSLogLevel logLevel,
                PCSTR         pszFormat,
                va_list       args);

typedef struct __LW_NS_INFO
{
    PSTR  pszNSHostName;
    DWORD dwIP;
} LW_NS_INFO, *PLW_NS_INFO;

typedef struct __LW_INTERFACE_INFO
{
    PSTR            pszName;
    struct sockaddr ipAddr;
    DWORD           dwFlags;
} LW_INTERFACE_INFO, *PLW_INTERFACE_INFO;

DWORD
DNSInitialize(
    VOID
    );

VOID
DNSSetLogParameters(
    LWDNSLogLevel maxLogLevel,
    PFN_LWDNS_LOG_MESSAGE pfnLogMessage
    );

VOID
DNSLogMessage(
    PFN_LWDNS_LOG_MESSAGE pfnLogger,
    LWDNSLogLevel         logLevel,
    PCSTR                 pszFormat,
    ...
    );

DWORD
DNSAllocateMemory(
    DWORD  dwSize,
    PVOID* ppMemory
    );

DWORD
DNSReallocMemory(
    PVOID  pMemory,
    PVOID * ppNewMemory,
    DWORD dwSize
    );

VOID
DNSFreeMemory(
    PVOID pMemory
    );

DWORD
DNSAllocateString(
    PCSTR pszInputString,
    PSTR* ppszOutputString
    );

VOID
DNSFreeString(
    PSTR pszString
    );

VOID
DNSStrToUpper(
     PSTR pszString
     );

VOID
DNSStrToLower(
     PSTR pszString
     );

DWORD
DNSGetNameServers(
    PCSTR        pszDomain,
    PLW_NS_INFO* ppNSInfoList,
    PDWORD       dwNumServers
    );

VOID
DNSFreeNameServerInfoArray(
    PLW_NS_INFO pNSInfoArray,
    DWORD       dwNumInfos
    );

VOID
DNSFreeNameServerInfo(
    PLW_NS_INFO pNSInfo
    );

VOID
DNSFreeNameServerInfoContents(
    PLW_NS_INFO pNSInfo
    );

DWORD
DNSGetNetworkInterfaces(
    PLW_INTERFACE_INFO* ppInterfaceInfoArray,
    PDWORD              pdwNumInterfaces
    );

VOID
DNSFreeNetworkInterfaces(
    PLW_INTERFACE_INFO pInterfaceInfoArray,
    DWORD              dwNumInterfaces
    );

VOID
DNSFreeNetworkInterface(
    PLW_INTERFACE_INFO pInterfaceInfo
    );

VOID
DNSFreeNetworkInterfaceContents(
    PLW_INTERFACE_INFO pInterfaceInfo
    );

DWORD
DNSOpen(
    PCSTR   pszNameServer,
    DWORD   dwType,
    PHANDLE phDNSServer
    );

DWORD
DNSUpdateSecure(
    HANDLE hDNSServer,
    PCSTR  pszServerName,
    PCSTR  pszDomainName,
    PCSTR  pszHostname,
    PSOCKADDR_IN pAddrArray,
    DWORD  dwNumAddrs
    );

DWORD
DNSClose(
    HANDLE hDNSServer
    );

DWORD
DNSShutdown();

size_t
DNSGetErrorString(
    DWORD  dwError,
    PSTR   pszBuffer,
    size_t stBufSize
    );

DWORD
DNSMapHerrno(
    DWORD dwHerrno
    );

#endif /* __LWDNS_H__ */
