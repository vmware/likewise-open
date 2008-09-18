/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsajoin.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Join to Active Directory
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LSAJOIN_H__
#define __LSAJOIN_H__

typedef DWORD (*PFN_LSA_NET_JOIN_DOMAIN)(
                PCSTR pszHostname,
                PCSTR pszDomain,
                PCSTR pszOU,
                PCSTR pszUsername,
                PCSTR pszPassword,
                PCSTR pszOSName,
                PCSTR pszOSVersion,
                DWORD dwFlags);

#define LSA_NET_JOIN_DOMAIN_NOTIMESYNC 1

typedef DWORD (*PFN_LSA_NET_TEST_JOIN_DOMAIN)(
                PBOOLEAN pbJoined
                );

typedef DWORD (*PFN_LSA_NET_LEAVE_DOMAIN)(
                PCSTR pszUsername,
                PCSTR pszPassword
                );

typedef DWORD (*PFN_LSA_NET_GET_SHORT_DOMAIN_NAME)(
                PCSTR pszHostname,
                PSTR* ppszDomainName
                );

typedef DWORD (*PFN_LSA_NET_GET_DC_NAME)(
                PCSTR pszDomainName,
                PSTR* ppszDCName
                );

typedef DWORD (*PFN_LSA_NET_GET_DNS_DOMAIN_NAME)(
                PSTR* ppszDnsDomainName
                );

typedef DWORD (*PFN_LSA_NET_GET_COMPUTER_DN)(
                PSTR* ppszComputerDN
                );

typedef VOID  (*PFN_LSA_FREE_STRING)(
                PSTR  pszString
                );

typedef VOID (*PFN_LSA_ENABLE_DEBUG_LOG)(VOID);
typedef VOID (*PFN_LSA_DISABLE_DEBUG_LOG)(VOID);

//
// Always send back the size of the buffer that is required
// This includes the terminating NULL
// The caller must check if the returned size is greater than
// what they provided
typedef size_t (*PFN_LSA_GET_ERROR_STRING)(
                   DWORD  dwErrorCode,
                   PSTR   pszBuffer,
                   size_t bufSize
                   );

typedef struct __LSA_NET_JOIN_FUNCTION_TABLE
{
    
    PFN_LSA_NET_JOIN_DOMAIN           pfnNetJoinDomain;
    PFN_LSA_NET_TEST_JOIN_DOMAIN      pfnNetTestJoinDomain;
    PFN_LSA_NET_LEAVE_DOMAIN          pfnNetLeaveDomain;
    PFN_LSA_NET_GET_SHORT_DOMAIN_NAME pfnGetShortDomain;
    PFN_LSA_NET_GET_DC_NAME           pfnGetDCName;
    PFN_LSA_NET_GET_DNS_DOMAIN_NAME   pfnGetDnsDomainName;
    PFN_LSA_NET_GET_COMPUTER_DN       pfnGetComputerDN;
    PFN_LSA_GET_ERROR_STRING          pfnGetErrorString;
    PFN_LSA_ENABLE_DEBUG_LOG          pfnEnableDebugLog;
    PFN_LSA_DISABLE_DEBUG_LOG         pfnDisableDebugLog;
    PFN_LSA_FREE_STRING               pfnFreeString;
    
} LSA_NET_JOIN_FUNCTION_TABLE, *PLSA_NET_JOIN_FUNCTION_TABLE;

typedef DWORD (*PFN_LSA_NET_JOIN_INITIALIZE)(
                   PLSA_NET_JOIN_FUNCTION_TABLE* ppFuncTable
                   );

typedef VOID (*PFN_LSA_NET_JOIN_SHUTDOWN)(
                   PLSA_NET_JOIN_FUNCTION_TABLE pFuncTable
                   );

#define LSA_SYMBOL_NET_JOIN_INITIALIZE "LsaNetJoinInitialize"
#define LSA_SYMBOL_NET_JOIN_SHUTDOWN   "LsaNetJoinShutdown"

DWORD
LsaNetJoinInitialize(
    PLSA_NET_JOIN_FUNCTION_TABLE* ppFuncTable
    );

VOID
LsaNetJoinShutdown(
    PLSA_NET_JOIN_FUNCTION_TABLE pFuncTable
    );

#endif /* __LSAJOIN_H__ */
