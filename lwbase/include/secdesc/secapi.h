/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
*/

/*
 * Copyright Likewise Software    2004-2009
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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef __SECAPI_H_
#define __SECAPI_H_

#include <secdesc/sectypes.h>
#include <lw/attrs.h>
#include <lw/ntstatus.h>

//
// Policy Handle API
//

NTSTATUS
RtlPolHndCreate(
    OUT PolicyHandle* pH,
    IN ULONG HandleType
    );

NTSTATUS
RtlPolHndCopy(
    OUT PolicyHandle* pHout,
    IN PolicyHandle* pHin
    );

NTSTATUS
RtlPolHndAllocate(
    OUT PolicyHandle** ppH,
    IN ULONG HandleType
    );

VOID
RtlPolHndFree(
    IN OUT PolicyHandle* pH
    );

BOOLEAN
RtlPolHndIsEqual(
    IN PolicyHandle* pH1,
    IN PolicyHandle* pH2
    );

BOOLEAN
RtlPolHndIsEmpty(
    IN PolicyHandle* pH
    );

//
// SID-Related API
//

// TODO-Perhaps use non-string version instead?
#define SID_BUILTIN_DOMAIN  "S-1-5-32"

#endif

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
