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
 *        lwnet.h
 *
 * Abstract:
 *
 *        Likewise Netlogon
 * 
 *        Active Directory Site API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#ifndef __LWNET_H__
#define __LWNET_H__

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

#ifndef UNIX_TIME_T_DEFINED
// This standardizes the time width to 64 bits.  This is useful for
// writing to files and such w/o worrying about any different sized time_t.
// It is seconds (or milliseconds, microseconds, nanoseconds) since
// the "Unix epoch" (Jan 1, 1970).  Note that negative values represent
// times before the "Unix epoch".
typedef int64_t UNIX_TIME_T, *PUNIX_TIME_T;
#endif

#endif

/* ERRORS */
#define LWNET_ERROR_SUCCESS                             0x0000
#define LWNET_ERROR_INVALID_CACHE_PATH                  0xA000 // 40960
#define LWNET_ERROR_INVALID_CONFIG_PATH                 0xA001 // 40961
#define LWNET_ERROR_INVALID_PREFIX_PATH                 0xA002 // 40962
#define LWNET_ERROR_INSUFFICIENT_BUFFER                 0xA003 // 40963
#define LWNET_ERROR_OUT_OF_MEMORY                       0xA004 // 40964
#define LWNET_ERROR_INVALID_MESSAGE                     0xA005 // 40965
#define LWNET_ERROR_UNEXPECTED_MESSAGE                  0xA006 // 40966
#define LWNET_ERROR_DATA_ERROR                          0xA007 // 40967
#define LWNET_ERROR_NOT_IMPLEMENTED                     0xA008 // 40968
#define LWNET_ERROR_NO_CONTEXT_ITEM                     0xA009 // 40969
#define LWNET_ERROR_REGEX_COMPILE_FAILED                0xA00A // 40970
#define LWNET_ERROR_INTERNAL                            0xA00B // 40971
#define LWNET_ERROR_INVALID_DNS_RESPONSE                0xA00C // 40972
#define LWNET_ERROR_DNS_RESOLUTION_FAILED               0xA00D // 40973
#define LWNET_ERROR_FAILED_TIME_CONVERSION              0xA00E // 40974
#define LWNET_ERROR_INVALID_SID                         0xA00F // 40975
#define LWNET_ERROR_UNEXPECTED_DB_RESULT                0xA010 // 40976
#define LWNET_ERROR_INVALID_LWNET_CONNECTION            0xA011 // 40977
#define LWNET_ERROR_INVALID_PARAMETER                   0xA012 // 40978
#define LWNET_ERROR_LDAP_NO_PARENT_DN                   0xA013 // 40979
#define LWNET_ERROR_LDAP_ERROR                          0xA014 // 40980
#define LWNET_ERROR_NO_SUCH_DOMAIN                      0xA015 // 40981
#define LWNET_ERROR_LDAP_FAILED_GETDN                   0xA016 // 40982
#define LWNET_ERROR_DUPLICATE_DOMAINNAME                0xA017 // 40983
#define LWNET_ERROR_FAILED_FIND_DC                      0xA018 // 40984
#define LWNET_ERROR_LDAP_GET_DN_FAILED                  0xA019 // 40985
#define LWNET_ERROR_INVALID_SID_REVISION                0xA01A // 40986
#define LWNET_ERROR_LOAD_LIBRARY_FAILED                 0xA01B // 40987
#define LWNET_ERROR_LOOKUP_SYMBOL_FAILED                0xA01C // 40988
#define LWNET_ERROR_INVALID_EVENTLOG                    0xA01D // 40989
#define LWNET_ERROR_INVALID_CONFIG                      0xA01E // 40990
#define LWNET_ERROR_UNEXPECTED_TOKEN                    0xA01F // 40991
#define LWNET_ERROR_LDAP_NO_RECORDS_FOUND               0xA020 // 40992
#define LWNET_ERROR_STRING_CONV_FAILED                  0xA021 // 40993
#define LWNET_ERROR_QUERY_CREATION_FAILED               0xA022 // 40994
#define LWNET_ERROR_NOT_JOINED_TO_AD                    0xA023 // 40995
#define LWNET_ERROR_FAILED_TO_SET_TIME                  0xA024 // 40996
#define LWNET_ERROR_NO_NETBIOS_NAME                     0xA025 // 40997
#define LWNET_ERROR_INVALID_NETLOGON_RESPONSE           0xA026 // 40998
#define LWNET_ERROR_INVALID_OBJECTGUID                  0xA027 // 40999
#define LWNET_ERROR_INVALID_DOMAIN                      0xA028 // 41000
#define LWNET_ERROR_NO_DEFAULT_REALM                    0xA029 // 41001
#define LWNET_ERROR_NOT_SUPPORTED                       0xA02A // 41002
#define LWNET_ERROR_NO_LWNET_INFORMATION                0xA02B // 41003
#define LWNET_ERROR_NO_HANDLER                          0xA02C // 41004
#define LWNET_ERROR_NO_MATCHING_CACHE_ENTRY             0xA02D // 41005
#define LWNET_ERROR_KRB5_CONF_FILE_OPEN_FAILED          0xA02E // 41006
#define LWNET_ERROR_KRB5_CONF_FILE_WRITE_FAILED         0xA02F // 41007
#define LWNET_ERROR_DOMAIN_NOT_FOUND                    0xA030 // 41008
#define LWNET_ERROR_CONNECTION_CLOSED                   0xA031 // 41009
#define LWNET_ERROR_FAILED_STARTUP_PREREQUISITE_CHECK   0xA032 // 41010
#define LWNET_ERROR_MAC_FLUSH_DS_CACHE_FAILED           0xA033 // 41011
#define LWNET_ERROR_SENTINEL                            0xA034 // 41012


//
// Note: When you add errors here, please remember to update
//       the corresponding error strings in lwnet-error.c
//
#define LWNET_ERROR_PREFIX                 0xA000 // 40960
#define LWNET_ERROR_MASK(_e_)             (_e_ & LWNET_ERROR_PREFIX)

#define LWNET_KRB5_CONF_DIRNAME "/var/lib/likewise"
#define LWNET_KRB5_CONF_BASENAME "krb5-affinity.conf"
#define LWNET_KRB5_CONF_PATH LWNET_KRB5_CONF_DIRNAME "/" LWNET_KRB5_CONF_BASENAME

#define LWNET_API

//Standard GUID's are 16 bytes long.
#define LWNET_GUID_SIZE 16

//used in LWNET_DC_INFO::ulDomainControllerAddressType
#define DS_INET_ADDRESS 23
#define DS_NETBIOS_ADDRESS 24

//used in LWNET_DC_INFO::Flags
#define DS_PDC_FLAG            0x00000001    //DC is a PDC of a domain
#define DS_BIT1_RESERVED_FLAG  0x00000002    //reserved: should always be 0
#define DS_GC_FLAG             0x00000004    //DC contains GC of a forest
#define DS_LDAP_FLAG           0x00000008    //DC supports an LDAP server
#define DS_DS_FLAG             0x00000010    //DC supports a DS
#define DS_KDC_FLAG            0x00000020    //DC is running a KDC
#define DS_TIMESERV_FLAG       0x00000040    //DC is running the time service
#define DS_CLOSEST_FLAG        0x00000080    //DC is the closest one to the client.
#define DS_WRITEABLE_FLAG      0x00000100    //DC has a writable DS
#define DS_GOOD_TIMESERV_FLAG  0x00000200    //DC is running time service and has clock hardware 
#define DS_NDNC_FLAG           0x00000400    //Non-Domain NC
#define DS_PING_FLAGS          0x0000FFFF    //bitmask of flags returned on ping

#define DS_DNS_CONTROLLER_FLAG 0x20000000    //DomainControllerName is a DNS name
#define DS_DOMAIN_FLAG         0x40000000    //DomainName is a DNS name
#define DS_DNS_FOREST_FLAG     0x80000000    //DnsForestName is a DNS name

#define LWNET_SUPPORTED_DS_OUTPUT_FLAGS   (DS_PDC_FLAG       | \
                                          DS_GC_FLAG        | \
                                          DS_DS_FLAG        | \
                                          DS_KDC_FLAG       | \
                                          DS_TIMESERV_FLAG  | \
                                          DS_CLOSEST_FLAG   | \
                                          DS_WRITEABLE_FLAG | \
                                          DS_GOOD_TIMESERV_FLAG)

#define LWNET_UNSUPPORTED_DS_OUTPUT_FLAGS   (DS_NDNC_FLAG    | \
                                            DS_DNS_CONTROLLER_FLAG | \
                                            DS_DOMAIN_FLAG  | \
                                            DS_DNS_FOREST_FLAG)


//used in DsGetDcName 'Flags' input parameter
#define DS_FORCE_REDISCOVERY            0x00000001

#define DS_DIRECTORY_SERVICE_REQUIRED   0x00000010
#define DS_DIRECTORY_SERVICE_PREFERRED  0x00000020
#define DS_GC_SERVER_REQUIRED           0x00000040
#define DS_PDC_REQUIRED                 0x00000080
#define DS_BACKGROUND_ONLY              0x00000100
#define DS_IP_REQUIRED                  0x00000200
#define DS_KDC_REQUIRED                 0x00000400
#define DS_TIMESERV_REQUIRED            0x00000800
#define DS_WRITABLE_REQUIRED            0x00001000
#define DS_GOOD_TIMESERV_REQUIRED       0x00002000
#define DS_AVOID_SELF                   0x00004000
#define DS_ONLY_LDAP_NEEDED             0x00008000

#define DS_IS_FLAT_NAME                 0x00010000
#define DS_IS_DNS_NAME                  0x00020000

#define DS_RETURN_DNS_NAME              0x40000000
#define DS_RETURN_FLAT_NAME             0x80000000

#define LWNET_SUPPORTED_DS_INPUT_FLAGS    (DS_FORCE_REDISCOVERY           | \
                                          DS_DIRECTORY_SERVICE_REQUIRED   | \
                                          DS_GC_SERVER_REQUIRED           | \
                                          DS_PDC_REQUIRED                 | \
                                          DS_BACKGROUND_ONLY              | \
                                          DS_KDC_REQUIRED                 | \
                                          DS_TIMESERV_REQUIRED            | \
                                          DS_WRITABLE_REQUIRED            | \
                                          DS_GOOD_TIMESERV_REQUIRED       | \
                                          DS_AVOID_SELF)
                                          


#define LWNET_UNSUPPORTED_DS_INPUT_FLAGS   (DS_DIRECTORY_SERVICE_PREFERRED  | \
                                           DS_IP_REQUIRED                   | \
                                           DS_ONLY_LDAP_NEEDED              | \
                                           DS_IS_FLAT_NAME                  | \
                                           DS_IS_DNS_NAME                   | \
                                           DS_RETURN_DNS_NAME               | \
                                           DS_RETURN_FLAT_NAME)

#define _LWNET_MAKE_SAFE_FREE(Pointer, FreeFunction) \
    do { \
        if (Pointer) \
        { \
            (FreeFunction)(Pointer); \
            (Pointer) = NULL; \
        } \
    } while (0)

#define LWNET_SAFE_FREE_DC_INFO(pDcInfo) \
    _LWNET_MAKE_SAFE_FREE(pDcInfo, LWNetFreeDCInfo)

typedef struct _LWNET_DC_INFO
{
    DWORD dwPingTime;
    DWORD dwDomainControllerAddressType;
    DWORD dwFlags;
    DWORD dwVersion;
    WORD wLMToken;
    WORD wNTToken;
    PSTR pszDomainControllerName;
    PSTR pszDomainControllerAddress;
    UCHAR pucDomainGUID[LWNET_GUID_SIZE];
    PSTR pszNetBIOSDomainName;
    PSTR pszFullyQualifiedDomainName;
    PSTR pszDnsForestName;
    PSTR pszDCSiteName;
    PSTR pszClientSiteName;
    PSTR pszNetBIOSHostName;
    PSTR pszUserName;
} LWNET_DC_INFO, *PLWNET_DC_INFO;

LWNET_API
DWORD
LWNetGetDCName(
    PCSTR pszServerFQDN,
    PCSTR pszDomainFQDN,
    PCSTR pszSiteName,
    DWORD dwFlags,
    PLWNET_DC_INFO* ppDCInfo
    );

LWNET_API
DWORD
LWNetGetDCNameWithBlacklist(
    IN PCSTR pszServerFQDN,
    IN PCSTR pszDomainFQDN,
    IN PCSTR pszSiteName,
    IN DWORD dwFlags,
    IN DWORD dwBlackListCount,
    IN OPTIONAL PSTR* ppszAddressBlackList,
    OUT PLWNET_DC_INFO* ppDCInfo
    );

LWNET_API
DWORD
LWNetGetDomainController(
    PCSTR pszDomainFQDN,
    PSTR* ppszDomainControllerFQDN
    );

LWNET_API
DWORD
LWNetGetDCTime(
    PCSTR pszDomainFQDN,
    PUNIX_TIME_T pDCTime
    );

LWNET_API
DWORD
LWNetGetCurrentDomain(
    PSTR* ppszDomainFQDN
    );

LWNET_API
DWORD
LWNetExtendEnvironmentForKrb5Affinity(
    BOOLEAN bNoDefault
    );

LWNET_API
VOID
LWNetFreeDCInfo(
    PLWNET_DC_INFO pDCInfo
    );

LWNET_API
VOID
LWNetFreeString(
    PSTR pszString
    );

LWNET_API
size_t
LWNetGetErrorString(
    DWORD  dwErrorCode,
    PSTR   pszBuffer,
    size_t stBufSize
    );

#endif /* __LWNET_H__ */
