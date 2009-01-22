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
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#ifndef _SDDEF_H_
#define _SDDEF_H_

#include "phdef.h"
#include "siddef.h"

/*
 * This header is separate from other definitions because it should
 * be possible to include it in idl files when generating dcerpc stubs.
 */

#ifndef _DCE_IDL_

typedef union sec_ace_object_type {
    Guid type;
} SecAceObjectType;

typedef union sec_ace_object_inherited_type {
    Guid inherited_type;
} SecAceObjectInheritedType;


typedef struct sec_ace_object {
    uint32 flags;              /* security ace object flags */

	/* flags & SEC_ACE_OBJECT_TYPE_PRESENT  */
	SecAceObjectType type;

	/* flags & SEC_ACE_INHERITED_OBJECT_TYPE_PRESENT */
	SecAceObjectInheritedType inherited_type;
} SecAceObject;


typedef union sec_ace_object_ctr {
    SecAceObject object_allowed;
    SecAceObject object_denied;
    SecAceObject object_audit;
    SecAceObject object_alarm;
} SecAceObjectCtr;

#endif /* _DCE_IDL_ */


/* Access Control Entry */
typedef struct sec_ace {
    uint8   sec_ace_type;      /* security ace types */
    uint8   sec_ace_flags;     /* security ace flags */
    uint16  size;
    uint32  access_mask;
#ifdef _DCE_IDL_
	[switch_is(sec_ace_type)]
#endif
    SecAceObjectCtr object;
	SID    *trustee;
} SecAce;


/* Access Control List */
typedef struct sec_acl {
    uint32 revision;
    uint16 size;
#ifdef _DCE_IDL_
    [range(0,1000)]
#endif
    uint32 num_aces;
#ifdef _DCE_IDL_
    [size_is(num_aces)]
#endif
    SecAce *aces;
} SecAcl;


/* Security Descriptor */
typedef struct security_descriptor {
    uint8   revision;
    uint16  type;
	SID    *owner;
	SID    *group;
	SecAcl *sacl;
	SecAcl *dacl;
} SecDesc, SECURITY_DESCRIPTOR, *PSECURITY_DESCRIPTOR;


/* Generic Mapping */
typedef struct generic_mapping {
    uint32 GenericRead;
    uint32 GenericWrite;
    uint32 GenericExecute;
    uint32 GenericAll;
} GenMapping, GENERIC_MAPPING, *PGENERIC_MAPPING;


/* Privilege Set structures */
typedef struct luid {
    uint32 low;
    int32 high;
} LUID, *PLUID;


typedef struct luid_and_attributes {
    LUID Luid;
    uint32 Attributes;
} LuidAndAttr, LUID_AND_ATTRIBUTES, *PLUID_AND_ATTRIBUTES;


#ifdef MAX_ARRAY_SIZE
#undefine MAX_ARRAY_SIZE
#endif
#define MAX_ARRAY_SIZE    32

typedef struct privilege_set {
    uint32 PrivilegeCount;
    uint32 Control;
    LuidAndAttr Privilege[MAX_ARRAY_SIZE];
} PrivilegeSet, PRIVILEGE_SET, *PPRIVILEGE_SET;


/* Quality of Service Info */
typedef struct qos_info {
	uint32 len;
	uint16 impersonation_level;
	uint8 context_mode;
	uint8 effective_only;
} QosInfo;


/* Object Attribute */
typedef struct object_attribute {
	uint32 len;
	uint8 *root_dir;
#ifdef _DCE_IDL_
	[string]
#endif
    wchar16_t *object_name;
	uint32 attributes;
	SecDesc *sec_desc;
	QosInfo *sec_qos;
} ObjectAttribute;


#endif /* _SDDEF_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
