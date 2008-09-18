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

#ifndef _LSADEFS_H_
#define _LSADEFS_H_

#include <lwrpc/types.h>
#include <lwrpc/security.h>


typedef struct audit_log_info {
	uint32 percent_full;
	uint32 log_size;
	NtTime retention_time;
	uint8  shutdown_in_progress;
	NtTime time_to_shutdown;
	uint32 next_audit_record;
	uint32 unknown;
} AuditLogInfo;

typedef struct audit_events_info {
	uint32 auditing_mode;
#ifdef _DCE_IDL_
	[size_is(count)]
#endif
	uint32 *settings;
	uint32 count;
} AuditEventsInfo;

typedef struct lsa_domain_info {
	UnicodeStringEx name;
	DomSid *sid;
} LsaDomainInfo;

typedef struct pd_account_info {
	UnicodeString name;
} PDAccountInfo;

typedef struct server_role {
	uint16 unknown;
	uint16 role;
} ServerRole;

typedef struct replica_source_info {
	UnicodeString source;
	UnicodeString account;
} ReplicaSourceInfo;

typedef struct default_quota_info {
	uint32 paged_pool;
	uint32 non_paged_pool;
	uint32 min_wss;
	uint32 max_wss;
	uint32 pagefile;
	uint64 unknown;
} DefaultQuotaInfo;

typedef struct modification_info {
	uint64 modified_id;
	NtTime db_create_time;
} ModificationInfo;

typedef struct autid_full_set_info {
	uint8 shutdown_on_full;
} AuditFullSetInfo;

typedef struct audit_full_query_info {
	uint16 unknown;
	uint8 shutdown_on_full;
	uint8 log_is_full;
} AuditFullQueryInfo;

typedef struct dns_domain_info {
	UnicodeStringEx name;
	UnicodeStringEx dns_domain;
	UnicodeStringEx dns_forest;
	Guid domain_guid;
	DomSid *sid;
} DnsDomainInfo;

#define LSA_POLICY_INFO_AUDIT_LOG          1
#define LSA_POLICY_INFO_AUDIT_EVENTS       2
#define LSA_POLICY_INFO_DOMAIN             3
#define LSA_POLICY_INFO_PD                 4
#define LSA_POLICY_INFO_ACCOUNT_DOMAIN     5
#define LSA_POLICY_INFO_ROLE               6
#define LSA_POLICY_INFO_REPLICA            7
#define LSA_POLICY_INFO_QUOTA              8
#define LSA_POLICY_INFO_DB                 9
#define LSA_POLICY_INFO_AUDIT_FULL_SET    10
#define LSA_POLICY_INFO_AUDIT_FULL_QUERY  11
#define LSA_POLICY_INFO_DNS               12

#ifndef _DCE_IDL_

typedef union lsa_policy_information { 
	AuditLogInfo        audit_log;
	AuditEventsInfo     audit_events;
	LsaDomainInfo       domain;
	PDAccountInfo       pd;
	LsaDomainInfo       account_domain;
	ServerRole          role;
	ReplicaSourceInfo   replica;
	DefaultQuotaInfo    quota;
	ModificationInfo    db;
	AuditFullSetInfo    audit_set;
	AuditFullQueryInfo  audit_query;
	DnsDomainInfo       dns;
} LsaPolicyInformation;

#endif


typedef struct ref_domain_list {
#ifdef _DCE_IDL_
	[range(0,1000)]
#endif
	uint32 count;
#ifdef _DCE_IDL_
	[size_is(count)]
#endif
	LsaDomainInfo *domains;
	uint32 max_size;
} RefDomainList;

typedef struct translated_sid {
	uint16 type;     /* SID_TYPE_ */
	uint32 rid;
	uint32 index;
} TranslatedSid;

typedef struct translated_sid_array {
#ifdef _DCE_IDL_
	[range(0,1000)]
#endif
	uint32 count;
#ifdef _DCE_IDL_
	[size_is(count)]
#endif
	TranslatedSid *sids;
} TranslatedSidArray;

typedef struct translated_sid2 {
	uint16 type;     /* SID_TYPE_ */
	uint32 rid;
	uint32 index;
	uint32 unknown1;
} TranslatedSid2;

typedef struct translated_sid_array2 {
#ifdef _DCE_IDL_
	[range(0,1000)]
#endif
	uint32 count;
#ifdef _DCE_IDL_
	[size_is(count)]
#endif
	TranslatedSid2 *sids;
} TranslatedSidArray2;

typedef struct translated_name {
	uint16 type;             /* SID_TYPE_ */
	UnicodeString name;
	uint32 sid_index;
} TranslatedName;

typedef struct translated_name_array {
#ifdef _DCE_IDL_
	[range(0,1000)]
#endif
	uint32 count;
#ifdef _DCE_IDL_
	[size_is(count)]
#endif
	TranslatedName *names;
} TranslatedNameArray;


#define LSA_LOOKUP_NAMES_ALL                   1
#define LSA_LOOKUP_NAMES_DOMAINS_ONLY          2
#define LSA_LOOKUP_NAMES_PRIMARY_DOMAIN_ONLY   3
#define LSA_LOOKUP_NAMES_UPLEVEL_TRUSTS_ONLY   4
#define LSA_LOOKUP_NAMES_FOREST_TRUSTS         5
#define LSA_LOOKUP_NAMES_UPLEVEL_TRUSTS_ONLY2  6


#define LSA_ACCESS_LOOKUP_NAMES_SIDS           0x00000800
#define LSA_ACCESS_ENABLE_LSA                  0x00000400
#define LSA_ACCESS_ADMIN_AUDIT_LOG_ATTRS       0x00000200
#define LSA_ACCESS_CHANGE_SYS_AUDIT_REQS       0x00000100
#define LSA_ACCESS_SET_DEFAULT_QUOTA           0x00000080
#define LSA_ACCESS_CREATE_PRIVILEGE            0x00000040
#define LSA_ACCESS_CREATE_SECRET_OBJECT        0x00000020
#define LSA_ACCESS_CREATE_SPECIAL_ACCOUNTS     0x00000010
#define LSA_ACCESS_CHANGE_DOMTRUST_RELATION    0x00000008
#define LSA_ACCESS_GET_SENSITIVE_POLICY_INFO   0x00000004
#define LSA_ACCESS_VIEW_SYS_AUDIT_REQS         0x00000002
#define LSA_ACCESS_VIEW_POLICY_INFO            0x00000001


#endif /* _LSADEFS_H_ */
