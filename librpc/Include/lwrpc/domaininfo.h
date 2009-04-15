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

#ifndef _DOMAININFO_H_
#define _DOMAININFO_H_

#include <lwrpc/types.h>

typedef struct domain_info_1 {
    uint16 min_pass_length;
    uint16 pass_history_length;
    uint32 pass_properties;
    int64  max_pass_age;
    int64  min_pass_age;
} DomainInfo1;

typedef struct domain_info_2 {
    NtTime force_logoff_time;
    UnicodeString comment;
    UnicodeString domain_name;
    UnicodeString primary;
    uint64 sequence_num;
    uint32 unknown1;
    uint32 role;
    uint32 unknown2;
    uint32 num_users;
    uint32 num_groups;
    uint32 num_aliases;
} DomainInfo2;

typedef struct domain_info_3 {
    NtTime force_logoff_time;
} DomainInfo3;

typedef struct domain_info_4 {
    UnicodeString comment;
} DomainInfo4;

typedef struct domain_info_5 {
    UnicodeString domain_name;
} DomainInfo5;

typedef struct domain_info_6 {
    UnicodeString primary;
} DomainInfo6;

/* role */
#define SAMR_ROLE_STANDALONE      0
#define SAMR_ROLE_DOMAIN_MEMBER   1
#define SAMR_ROLE_DOMAIN_BDC      2
#define SAMR_ROLE_DOMAIN_PDC      3

typedef struct domain_info_7 {
    uint16 role;
} DomainInfo7;

typedef struct domain_info_8 {
    uint64 sequence_number;
    NtTime domain_create_time;
} DomainInfo8;

typedef struct domain_info_9 {
    uint32 unknown;
} DomainInfo9;

typedef struct domain_info_11 {
    DomainInfo2 info2;
    uint64 lockout_duration;
    uint64 lockout_window;
    uint16 lockout_threshold;
} DomainInfo11;

typedef struct domain_info_12 {
    uint64 lockout_duration;
    uint64 lockout_window;
    uint16 lockout_threshold;
} DomainInfo12;

typedef struct domain_info_13 {
    uint64 sequence_number;
    NtTime domain_create_time;
    uint32 unknown1;
    uint32 unknown2;
} DomainInfo13;


#ifndef _DCE_IDL_
typedef union domain_info {
    DomainInfo1 info1;
    DomainInfo2 info2;
    DomainInfo3 info3;
    DomainInfo4 info4;
    DomainInfo5 info5;
    DomainInfo6 info6;
    DomainInfo7 info7;
    DomainInfo8 info8;
    DomainInfo9 info9;
    DomainInfo11 info11;
    DomainInfo12 info12;
    DomainInfo13 info13;
} DomainInfo;
#endif /* _DCE_IDL_ */

#endif /* _DOMAININFO_H_ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
