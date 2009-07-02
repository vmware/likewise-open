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
    OUT PVOID *ppOut,
    IN  size_t Size,
    IN  PVOID  pDependent
    );

NTSTATUS
LsaRpcFreeMemory(
    IN PVOID pBuffer
    );

NTSTATUS
LsaRpcAddDepMemory(
    IN PVOID pBuffer,
    IN PVOID pDependent
    );

NTSTATUS
LsaAllocateTranslatedSids(
    OUT TranslatedSid **ppOut,
    IN  TranslatedSidArray *pIn
    );

NTSTATUS
LsaAllocateTranslatedSids2(
    TranslatedSid2 **out,
    TranslatedSidArray2 *in
    );

NTSTATUS
LsaAllocateTranslatedSids2(
    OUT TranslatedSid2 **ppOut,
    IN  TranslatedSidArray2 *pIn
    );

NTSTATUS
LsaAllocateTranslatedSids3(
    OUT TranslatedSid3 **ppOut,
    IN  TranslatedSidArray3 *pIn
    );

NTSTATUS
LsaAllocateRefDomainList(
    OUT RefDomainList **ppOut,
    IN  RefDomainList *pIn
    );

NTSTATUS
LsaAllocateTranslatedNames(
    OUT TranslatedName **ppOut,
    IN  TranslatedNameArray *pIn
    );

NTSTATUS
LsaAllocatePolicyInformation(
    OUT LsaPolicyInformation **pOut,
    IN  LsaPolicyInformation *pIn,
    IN  UINT32 Level
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
