/*
 * idmap_lwidentity: Support for Likewise Cell Structure
 *
 * Copyright (C) Gerald (Jerry) Carter 2006-2007
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "includes.h"
#include "idmap_lwidentity.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_IDMAP

/**********************************************************************
 *********************************************************************/

static NTSTATUS parse_cell_settings(struct likewise_cell *cell,
				char **config,
				size_t num_lines)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	char *s;

	s = find_attr_string(config, num_lines, "objectClass");
	if (!s || !strequal(s, "centerisLikewiseCell")) {
		DEBUG(1,("No support for centerisLikewiseCell settings!\n"));
		nt_status = NT_STATUS_INVALID_SERVER_STATE;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	s = find_attr_string(config, num_lines, "use2307Attrs");
	if (s) {
		if (strequal(s, "true"))
			cell_set_flags(cell, LWCELL_FLAG_USE_RFC2307_ATTRS);
	}

	/* Find any linked cells */

	s = find_attr_string(config, num_lines, "linkedCells");
	if (s) {
		char *p;
		int i;

		cell->num_links = count_chars(s, ';') + 1;

		cell->links = TALLOC_ZERO_ARRAY(cell, struct GUID,
						cell->num_links);
		BAIL_ON_PTR_ERROR(cell->links, nt_status);

		for (i=0; i<cell->num_links && s; i++) {
			/* Set the end of a single GUID */
			if ((p = strchr(s, ';')) != NULL ) {
				*p = '\0';
				p++;
			}
			nt_status = GUID_from_string(s, &cell->links[i]);
			BAIL_ON_NTSTATUS_ERROR(nt_status);

			s = p;
		}
	}

	cell->provider = &ccp_unified;

	nt_status = NT_STATUS_OK;

done:
	return nt_status;
}

/**********************************************************************
**********************************************************************/

 char *find_attr_string(char **list, size_t num_lines, const char *substr)
{
	int i;
	int cmplen = strlen(substr);

	for (i = 0; i < num_lines; i++) {
		/* make sure to avoid substring matches like uid
		   and uidNumber */
		if ((StrnCaseCmp(list[i], substr, cmplen) == 0) &&
		    (list[i][cmplen] == '=')) {
			/* Don't return an empty string */
			if (list[i][cmplen + 1] != '\0')
				return &(list[i][cmplen + 1]);

			return NULL;
		}
	}

	return NULL;
}

/**********************************************************************
**********************************************************************/

 bool is_object_class(char **list, size_t num_lines, const char *substr)
{
	int i;

	for (i = 0; i < num_lines; i++) {
		if (strequal(list[i], substr)) {
			return true;
		}
	}

	return false;
}

/**********************************************************************
 Find out about the cell (e.g. use2307Attrs, etc...)
**********************************************************************/

 NTSTATUS cell_lookup_settings(struct likewise_cell * cell)
{
	const char *likewise_dn = NULL;
	ADS_STATUS status;
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	const char *attrs_cell[] = { "description", NULL };
	LDAPMessage *msg = NULL;
	char **desc = NULL;
	size_t num_lines;
	TALLOC_CTX *frame = talloc_stackframe();
	const char *domain_dn = NULL;

	if (!cell) {
		nt_status = NT_STATUS_INVALID_PARAMETER;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	/* Search the cell container */

	likewise_dn = cell_search_base(cell);
	BAIL_ON_PTR_ERROR(likewise_dn, nt_status);

	status = ads_search_dn(cell->conn, &msg, likewise_dn, attrs_cell);
	if (!ADS_ERR_OK(status)) {
		nt_status = NT_STATUS_NOT_SUPPORTED;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	nt_status = NT_STATUS_OK;

	desc = ads_pull_strings(cell->conn, frame, msg, "description", &num_lines);

	ads_msgfree(cell->conn, msg);
	msg = NULL;

	if (!desc) {
		nt_status = NT_STATUS_INTERNAL_DB_CORRUPTION;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	/* Fill in the cell settings */
	nt_status = parse_cell_settings(cell, desc, num_lines);
	BAIL_ON_NTSTATUS_ERROR(nt_status);

	/* see if we should do forest-wide searches */

	domain_dn = ads_build_dn(cell->dns_domain);
	BAIL_ON_PTR_ERROR(domain_dn, nt_status);

	if (strequal(domain_dn, cell->dn)) {
		cell_set_flags(cell, LWCELL_FLAG_SEARCH_FOREST);
	}

done:
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(1,("LWI: Failed to obtain cell settings (%s)\n",
			 nt_errstr(nt_status)));
	}

        talloc_destroy(CONST_DISCARD(char*, likewise_dn));
	talloc_destroy(frame);
	if (domain_dn) {
		free(CONST_DISCARD(char*, domain_dn));
	}

	ads_msgfree(cell->conn, msg);

	return nt_status;
}

/**********************************************************************
 Return the DN of the Cell, or NULL if this is not a Cell.
**********************************************************************/

static bool cell_lookup(ADS_STRUCT * ads, const char *dn)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	const char *attrs_cellprop[] = { "description", NULL };
	LDAPMessage *msg = NULL;
	ADS_STATUS status;
	TALLOC_CTX *frame = talloc_stackframe();
	char *filter;

	/* An OU or domain is cell enabled if it has a child container
	   named $LikewiseIdentityCell */

	filter = talloc_asprintf(frame, "(cn=%s)", LW_CELL_RDN);
	BAIL_ON_PTR_ERROR(filter, nt_status);

	status = ads_do_search(ads, dn, LDAP_SCOPE_ONELEVEL,
			       filter, attrs_cellprop, &msg);
	nt_status = ads_ntstatus(status);
	BAIL_ON_NTSTATUS_ERROR(nt_status);

	nt_status = NT_STATUS_OBJECT_PATH_NOT_FOUND;

	if (ads_count_replies(ads, msg) == 1) {
		DEBUG(5, ("cell_lookup: \"%s\" is cell-enabled!\n", dn));
		nt_status = NT_STATUS_OK;
	}

done:
	ads_msgfree(ads, msg);

	return NT_STATUS_IS_OK(nt_status);
}

/**********************************************************************
**********************************************************************/

 NTSTATUS cell_locate_membership(ADS_STRUCT * ads)
{
	ADS_STATUS status;
	LDAPMessage *msg = NULL;
	char *machine_dn = NULL;
	char *p;
	char *domain_dn = ads_build_dn(lp_realm());
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;

	/* locate the machine account */

	status = ads_find_machine_acct(ads, &msg, global_myname());
	if (!ADS_ERR_OK(status)) {
		DEBUG(0,
		      ("_idmap_centeris_init: Failed to find machine account! (%s)\n",
		       ads_errstr(status)));
		return ads_ntstatus(status);
	}
	machine_dn = ads_get_dn(ads, msg);
	ads_msgfree(ads, msg);
	BAIL_ON_PTR_ERROR(machine_dn, nt_status);

	DEBUG(8, ("locate_cell_membership: machine DN is \"%s\"\n",
		  machine_dn));

	/* now walk the DN to find the first cell */

	p = ads_parent_dn(machine_dn);
	BAIL_ON_PTR_ERROR(p, nt_status);

	while (p) {
		bool valid_cell = cell_lookup(ads, p);
		DOM_SID sid;
		struct likewise_cell *cell = NULL;

		/* Do we need to continue? */

		if (!valid_cell) {

			/* If this is the top of the domain and we
			   have no valid cell, then we are in a
			   non-provisioned domain */

			if (strequal(p, domain_dn)) {
				nt_status = NT_STATUS_OBJECT_NAME_NOT_FOUND;
				BAIL_ON_NTSTATUS_ERROR(nt_status);
			}

			/* Still more to process.  Look at the parent
			   DN and fail if we cannot obtain it somehow */

			p = ads_parent_dn(p);
			BAIL_ON_PTR_ERROR(p, nt_status);

			/* We have more of the DN to consume */

			continue;
		}

		/* We found our Cell! */

		DEBUG(2, ("locate_cell_membership: Located membership "
			  "in cell \"%s\"\n", p));

		if ((cell = cell_new()) == NULL) {
			nt_status = NT_STATUS_NO_MEMORY;
			BAIL_ON_NTSTATUS_ERROR(nt_status);
		}

		status = ads_domain_sid(ads, &sid);
		if (!ADS_ERR_OK(status)) {
			DEBUG(3,("locate_cell_membership: Failed to find "
				 "domain SID for %s\n", domain_dn));
		}

		/* save the SID and search base for our domain */

		cell_set_dns_domain(cell, lp_realm());
		cell_set_connection(cell, ads);
		cell_set_dn(cell, p);
		cell_set_domain_sid(cell, &sid);

		/* Add the cell to the list */

		if (!cell_list_add(cell)) {
			nt_status = NT_STATUS_INSUFFICIENT_RESOURCES;
			BAIL_ON_NTSTATUS_ERROR(nt_status);
		}

		/* Done! */
		nt_status = NT_STATUS_OK;
		break;
	}

done:
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0,("LWI: Failed to locate cell membership (%s)\n",
			 nt_errstr(nt_status)));
	}

	SAFE_FREE(machine_dn);
	SAFE_FREE(domain_dn);

	return nt_status;
}

/*********************************************************************
 ********************************************************************/

 int min_id_value(void)
{
	int id_val;

	id_val = lp_parm_int(-1, "lwidentity", "min_id_value", MIN_ID_VALUE);

	/* Still don't let it go below 50 */

	return MAX(50, id_val);
}

/*********************************************************************
 ********************************************************************/

static NTSTATUS resolve_guid_dn(struct GUID *guid, char **dn)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	TALLOC_CTX *frame = talloc_stackframe();
	char *filter;
	LDAPMessage *msg = NULL;
	ADS_STRUCT *ads = NULL;

	char *s_guid = guid_binstring(guid);

	if (!s_guid) {
		nt_status = NT_STATUS_INVALID_PARAMETER;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	filter = talloc_asprintf(frame, "(objectGUID=%s)", s_guid);

	nt_status = gc_search_all_forests_unique(filter, &ads, &msg);
	BAIL_ON_NTSTATUS_ERROR(nt_status);

	if ((*dn = ads_get_dn(ads, msg)) == NULL ) {
		nt_status = NT_STATUS_NO_MEMORY;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

done:
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0,("LWI: Failed to uniquely identify GUID! (%s)\n",
			 nt_errstr(nt_status)));
	}

	ads_msgfree(ads, msg);
	talloc_destroy(frame);
	SAFE_FREE(s_guid);

	return nt_status;
}

/*********************************************************************
 ********************************************************************/

 NTSTATUS cell_follow_links(struct likewise_cell *c)
{
	int i;
	NTSTATUS nt_status = NT_STATUS_OK;
	char *dn = NULL;

	/* Following a cell link is three steps:
	   1. Resolve the GUID to an OU
	   2. Connect to the OU
	   3. Parse the OU/Cell settings
	*/

	for (i=0; i<c->num_links; i++ ) {
		struct likewise_cell *c_link = NULL;

		/* Ignore unknown GUIDs */

		nt_status = resolve_guid_dn(&c->links[i], &dn);
		if (!NT_STATUS_IS_OK(nt_status))
			continue;

		nt_status = cell_connect_dn(&c_link, dn);
		if (!NT_STATUS_IS_OK(nt_status)) {
			DEBUG(2,("LWI: Failed to connect to %s\n", dn));
			SAFE_FREE(dn);
			continue;
		}

		/* Ignore OU's that are not cells */

		nt_status = cell_lookup_settings(c_link);
		if (!NT_STATUS_IS_OK(nt_status)) {
			DEBUG(2,("LWI: Ignoring invalid cell %s.\n", dn));
			SAFE_FREE(dn);
		}

		cell_list_add(c_link);

		SAFE_FREE(dn);
	}

	return nt_status;
}

/********************************************************************
 *******************************************************************/

 char *cell_dn_to_dns(const char *dn)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	char *domain = NULL;
	char *dns_name = NULL;
	const char *tmp_dn;
	char *buffer = NULL;
	TALLOC_CTX *frame = talloc_stackframe();

	if (!dn || !*dn) {
		goto done;
	}

	tmp_dn = talloc_strdup(frame, dn);
	BAIL_ON_PTR_ERROR(tmp_dn, nt_status);

	while (next_token_talloc(frame, &tmp_dn, &buffer, ",")) {

		/* skip everything up the where DC=... begins */
		if (StrnCaseCmp(buffer, "DC=", 3) != 0)
			continue;

		if (!domain) {
			domain = talloc_strdup(frame, &buffer[3]);
		} else {
			domain = talloc_asprintf_append(domain, ".%s",
							&buffer[3]);
		}
		BAIL_ON_PTR_ERROR(domain, nt_status);
	}

	dns_name = SMB_STRDUP(domain);
	BAIL_ON_PTR_ERROR(dns_name, nt_status);

	nt_status = NT_STATUS_OK;

done:
	PRINT_NTSTATUS_ERROR(nt_status, "cell_dn_to_dns", 1);

	talloc_destroy(frame);

	return dns_name;
}

/**********************************************************************
**********************************************************************/

 NTSTATUS cell_set_local_provider(void)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	struct likewise_cell *cell = NULL;
	char *domain_dn = NULL;

	domain_dn = ads_build_dn(lp_realm());
	BAIL_ON_PTR_ERROR(domain_dn, nt_status);

	nt_status = cell_connect_dn(&cell, domain_dn);
	BAIL_ON_NTSTATUS_ERROR(nt_status);

	cell_set_flags(cell, LWCELL_FLAG_LOCAL_MODE);
	cell->provider = &ccp_local;

	cell_list_add(cell);

	nt_status = NT_STATUS_OK;

done:
	SAFE_FREE(domain_dn);

	return nt_status;
}

/*********************************************************************
 ********************************************************************/

 NTSTATUS get_sid_type(ADS_STRUCT *ads,
		       LDAPMessage *msg,
		       enum lsa_SidType *type)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	uint32_t atype;

	if (!ads_pull_uint32(ads, msg, "sAMAccountType", &atype)) {
		nt_status = NT_STATUS_INVALID_USER_BUFFER;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	switch (atype &0xF0000000) {
	case ATYPE_SECURITY_GLOBAL_GROUP:
		*type = SID_NAME_DOM_GRP;
		break;
	case ATYPE_SECURITY_LOCAL_GROUP:
		*type = SID_NAME_ALIAS;
		break;
	case ATYPE_NORMAL_ACCOUNT:
	case ATYPE_WORKSTATION_TRUST:
	case ATYPE_INTERDOMAIN_TRUST:
		*type = SID_NAME_USER;
		break;
	default:
		*type = SID_NAME_USE_NONE;
		nt_status = NT_STATUS_INVALID_ACCOUNT_NAME;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	nt_status = NT_STATUS_OK;

done:
	return nt_status;
}

