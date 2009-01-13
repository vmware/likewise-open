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

#ifndef _SECURITY_H_
#define _SECURITY_H_

#include <types.h>

typedef struct guid {
	uint32 time_low;
	uint16 time_mid;
	uint16 time_hi_and_version;
	uint8 clock_seq[2];
	uint8 node[6];
} Guid;

typedef struct policy_handle {
	uint32 handle_type;
	Guid guid;
} PolicyHandle;

typedef struct dom_sid {
        uint8 revision;
#ifdef _DCE_IDL_
	[range(0,15)]
#endif
        uint8 subauth_count;
        uint8 authid[6];
#ifdef _DCE_IDL_
	[size_is(subauth_count)]
#endif
        uint32 *subauth;
} DomSid;

typedef struct sid_ptr {
	DomSid *sid;
} SidPtr;

typedef struct sid_array {
#ifdef _DCE_IDL_
	[range(0,1000)]
#endif
	uint32 num_sids;
#ifdef _DCE_IDL_
	[size_is(num_sids)]
#endif
	SidPtr *sids;
} SidArray;

typedef struct rid_with_attribute {
    uint32 rid;
    uint32 attributes;
} RidWithAttribute;

typedef struct rid_with_attribute_array {
    uint32     count;
#ifdef _DCE_IDL_
    [size_is(count)]
#endif
    RidWithAttribute *rids;
} RidWithAttributeArray;


/* Secure Channel types */
#define SCHANNEL_WKSTA     2
#define SCHANNEL_DOMAIN    4
#define SCHANNEL_BDC       6


/* Security ace types */
#define SEC_ACE_TYPE_ACCESS_ALLOWED         0
#define SEC_ACE_TYPE_ACCESS_DENIED          1
#define SEC_ACE_TYPE_SYSTEM_AUDIT           2
#define SEC_ACE_TYPE_SYSTEM_ALARM           3
#define SEC_ACE_TYPE_ALLOWED_COMPOUND       4
#define SEC_ACE_TYPE_ACCESS_ALLOWED_OBJECT  5
#define SEC_ACE_TYPE_ACCESS_DENIED_OBJECT   6
#define SEC_ACE_TYPE_SYSTEM_AUDIT_OBJECT    7
#define SEC_ACE_TYPE_SYSTEM_ALARM_OBJECT    8


/* Security ace object flags */
#define SEC_ACE_OBJECT_TYPE_PRESENT             0x00000001
#define SEC_ACE_INHERITED_OBJECT_TYPE_PRESENT   0x00000002


/* Standard access rights */
#define SEC_STD_DELETE                        0x00010000
#define SEC_STD_READ_CONTROL                  0x00020000
#define SEC_STD_WRITE_DAC                     0x00040000
#define SEC_STD_WRITE_OWNER                   0x00080000
#define SEC_STD_SYNCHRONIZE                   0x00100000
#define SEC_STD_REQUIRED                      0x000f0000
#define SEC_STD_ALL                           0x001f0000


#define SID_TYPE_USE_NONE   0
#define SID_TYPE_USER       1
#define SID_TYPE_DOM_GRP    2
#define SID_TYPE_DOMAIN     3
#define SID_TYPE_ALIAS      4
#define SID_TYPE_WKN_GRP    5
#define SID_TYPE_DELETED    6
#define SID_TYPE_INVALID    7
#define SID_TYPE_UNKNOWN    8
#define SID_TYPE_COMPUTER   9

#ifndef _DCE_IDL_

#ifndef PSECURITY_DESCRIPTOR_DEFINED

typedef void *PSECURITY_DESCRIPTOR;

#define PSECURITY_DESCRIPTOR_DEFINED

#endif

#else

#ifndef PSECURITY_DESCRIPTOR_DEFINED
typedef uint8 *PSECURITY_DESCRIPTOR;
#define PSECURITY_DESCRIPTOR_DEFINED
#endif

#endif

#endif /* _SECURITY_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
