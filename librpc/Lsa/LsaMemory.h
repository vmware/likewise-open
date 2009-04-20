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
 * Abstract: Lsa memory (de)allocation routines (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _LSA_MEMORY_H_
#define _LSA_MEMORY_H_


NTSTATUS
LsaRpcAllocateMemory(
    void **ptr,
    size_t len,
    void *dep
    );

NTSTATUS
LsaRpcFreeMemory(
    void *ptr
    );

NTSTATUS
LsaRpcSafeFreeMemory(
    void **ptr
    );

NTSTATUS
LsaRpcAddDepMemory(
    void *ptr,
    void *dep
    );

NTSTATUS
LsaAllocateTranslatedSids(
    TranslatedSid **out,
    TranslatedSidArray *in
    );

NTSTATUS
LsaAllocateTranslatedSids2(
    TranslatedSid2 **out,
    TranslatedSidArray2 *in
    );

NTSTATUS
LsaAllocateTranslatedSids3(
    TranslatedSid3 **out,
    TranslatedSidArray3 *in
    );

void
LsaFreeTranslatedSid(
    TranslatedSid *ptr
    );

NTSTATUS
LsaAllocateTranslatedSid2(
    TranslatedSid2 **out,
    TranslatedSid2 *in
    );

void
LsaFreeTranslatedSid2(
    TranslatedSid2 *ptr
    );

NTSTATUS
LsaAllocateTranslatedSidArray(
    TranslatedSidArray **out,
    TranslatedSidArray *in
    );

void
LsaFreeTranslatedSidArray(
    TranslatedSidArray *ptr
    );

NTSTATUS
LsaAllocateTranslatedSidArray2(
    TranslatedSidArray2 **out,
    TranslatedSidArray2 *in
    );

void
LsaFreeTranslatedSidArray2(
    TranslatedSidArray2 *ptr
    );

NTSTATUS
LsaAllocateRefDomainList(
    RefDomainList **out,
    RefDomainList *in
    );

NTSTATUS
LsaAllocateTranslatedNames(
    TranslatedName **out,
    TranslatedNameArray *in
    );

NTSTATUS
LsaAllocatePolicyInformation(
   LsaPolicyInformation **out,
   LsaPolicyInformation *in,
   uint32 level
   );

#endif /* _LSA_MEMORY_H_ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
