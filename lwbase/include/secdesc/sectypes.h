/* Editor Settings: expandtabs and use 4 spaces for indentation
* ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
*/

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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _SECTYPES_H_
#define _SECTYPES_H_

#include <lw/security-types.h>

//
// Policy Handle Types
//

typedef struct _guid {
    ULONG time_low;
    USHORT time_mid;
    USHORT time_hi_and_version;
    UCHAR clock_seq[2];
    UCHAR node[6];
} Guid;

typedef struct _policy_handle {
    ULONG handle_type;
    Guid guid;
} PolicyHandle;

/* Object Attribute */
typedef struct _object_attribute {
    ULONG len;
    PBYTE root_dir;
#ifdef _DCE_IDL_
    [string]
#endif
    PWSTR object_name;
    ULONG attributes;
    PBYTE sec_desc;
    PSECURITY_QUALITY_OF_SERVICE sec_qos;
} ObjectAttribute;

//
// SID-Related Types
//

typedef struct _sid_ptr {
    PSID sid;
} SidPtr;

typedef struct _sid_array {
#ifdef _DCE_IDL_
    [range(0,1000)]
#endif
    ULONG num_sids;
#ifdef _DCE_IDL_
    [size_is(num_sids)]
#endif
    SidPtr* sids;
} SidArray;

#endif

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
