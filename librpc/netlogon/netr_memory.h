/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 * Abstract: Netlogon memory (de)allocation routines (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _NETR_MEMORY_H_
#define _NETR_MEMORY_H_


NTSTATUS
NetrAllocateMemory(
    OUT PVOID *pOut,
    IN  size_t Size,
    IN  PVOID  pDep
    );

NTSTATUS
NetrAddDepMemory(
    IN  PVOID pPtr,
    IN  PVOID pDep
    );

NTSTATUS
NetrFreeMemory(
    IN  PVOID pPtr
    );

NTSTATUS
NetrAllocateUniString(
    OUT  PWSTR  *ppwszOut,
    IN   PCWSTR  pwszIn,
    IN   PVOID   pDep
    );

NTSTATUS
NetrAllocateDomainTrusts(
    OUT NetrDomainTrust     **ppOut,
    IN  NetrDomainTrustList  *pIn
    );

NTSTATUS
NetrAllocateLogonInfoNet(
    OUT NetrLogonInfo **ppOut,
    IN  UINT16          Level,
    IN  PCWSTR          pwszDomain,
    IN  PCWSTR          pwszWorkstation,
    IN  PCWSTR          pwszAccount,
    IN  PBYTE           pChallenge,
    IN  PBYTE           pLmResp,
    IN  UINT32          LmRespLen,
    IN  PBYTE           pNtResp,
    IN  UINT32          NtRespLen
    );

NTSTATUS
NetrAllocateLogonInfoHash(
    OUT NetrLogonInfo **ppOut,
    IN  UINT16          Level,
    IN  PCWSTR          pwszDomain,
    IN  PCWSTR          pwszWorkstation,
    IN  PCWSTR          pwszAccount,
    IN  PCWSTR          pwszPassword
    );

NTSTATUS
NetrAllocateLogonInfo(
    OUT NetrLogonInfo **ppOut,
    IN  UINT16          Level,
    IN  PCWSTR          pwszDomain,
    IN  PCWSTR          pwszWorkstation,
    IN  PCWSTR          pwszAccount,
    IN  PCWSTR          pwszPassword
    );

NTSTATUS
NetrAllocateValidationInfo(
    OUT NetrValidationInfo **ppOut,
    IN  NetrValidationInfo  *pIn,
    IN  UINT16               Level
    );

NTSTATUS
NetrAllocateDomainInfo(
    OUT NetrDomainInfo **ppOut,
    IN  NetrDomainInfo  *pIn,
    IN  UINT32           Level
    );

NTSTATUS
NetrAllocateDcNameInfo(
    OUT DsrDcNameInfo **ppOut,
    IN  DsrDcNameInfo  *pIn
    );


#endif /* _NETR_MEMORY_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
