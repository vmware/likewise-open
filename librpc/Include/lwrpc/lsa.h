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
 * Abstract: Lsa interface (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _LSA_H_
#define _LSA_H_

#include <lwrpc/lsadefs.h>
#include <lwrpc/lsabinding.h>


NTSTATUS
LsaOpenPolicy2(
    handle_t         b,
    const wchar16_t *sysname,
    void            *attrib,
    uint32          access_mask,
    PolicyHandle    *handle
    );


NTSTATUS
LsaLookupNames(
    handle_t        b,
    PolicyHandle   *handle,
    uint32          num_names,
    wchar16_t      *names[],
    RefDomainList **domains,
    TranslatedSid **sids,
    uint16          level,
    uint32         *count
    );


NTSTATUS
LsaLookupSids(
    handle_t         b,
    PolicyHandle    *handle,
    SidArray        *sids,
    RefDomainList  **domains,
    TranslatedName **names,
    uint16           level,
    uint32          *count
    );

NTSTATUS
LsaLookupNames2(
    handle_t         binding,
    PolicyHandle    *handle,
    uint32           num_names,
    wchar16_t       *names[],
    RefDomainList  **domains,
    TranslatedSid2 **sids,
    uint16           level,
    uint32          *count
    );


NTSTATUS
LsaClose(
    handle_t      binding,
    PolicyHandle *handle
    );


NTSTATUS
LsaQueryInfoPolicy(
    handle_t               binding,
    PolicyHandle          *handle,
    uint16                 level,
    LsaPolicyInformation **info
    );


NTSTATUS
LsaQueryInfoPolicy2(
    handle_t               binding,
    PolicyHandle          *handle,
    uint16                 level,
    LsaPolicyInformation **info
    );


NTSTATUS
LsaRpcInitMemory();


NTSTATUS
LsaRpcDestroyMemory();


NTSTATUS
LsaRpcFreeMemory(
    void *ptr
    );


#endif /* _LSA_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
