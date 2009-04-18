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
    void **ptr,
    size_t len,
    void *dep
    );

NTSTATUS
NetrAddDepMemory(
    void *ptr,
    void *dep
    );

NTSTATUS
NetrFreeMemory(
    void *ptr
    );

NTSTATUS NetrAllocateUniString(
    wchar16_t **out,
    const wchar16_t *in,
    void *dep
    );

NTSTATUS
NetrAllocateDomainTrusts(
    NetrDomainTrust **out,
    NetrDomainTrustList *in
    );

NTSTATUS NetrAllocateLogonInfoNet(
    NetrLogonInfo **out,
    uint16 level,
    const wchar16_t *domain,
    const wchar16_t *workstation,
    const wchar16_t *account,
    const uint8_t *challenge,
    const uint8_t *lm_resp,
    const uint8_t *nt_resp
    );

NTSTATUS NetrAllocateLogonInfoHash(
    NetrLogonInfo **out, 
    uint16 level,
    const wchar16_t *domain,
    const wchar16_t *workstation,
    const wchar16_t *account,
    const wchar16_t *password
    );

NTSTATUS NetrAllocateLogonInfo(
    NetrLogonInfo **out, 
    uint16 level,
    const wchar16_t *domain,
    const wchar16_t *workstation,
    const wchar16_t *account,
    const wchar16_t *password
    );

NTSTATUS
NetrAllocateLogonInfoNet(
    NetrLogonInfo **out,
    uint16 level,
    const wchar16_t *domain,
    const wchar16_t *workstation,
    const wchar16_t *account,
    const uint8_t *challenge,
    const uint8_t *lm_resp,
    const uint8_t *nt_resp
    );

NTSTATUS
NetrAllocateValidationInfo(
    NetrValidationInfo **out,
    NetrValidationInfo *in,
    uint16 level
    );


NTSTATUS
NetrAllocateDcNameInfo(
    DsrDcNameInfo **out,
    DsrDcNameInfo *in
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
