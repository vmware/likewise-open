/*
 * idmap_centeris:
 *
 * Local Client provider
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

#define MIN_VALID_RID     1000

/********************************************************************
 *******************************************************************/

static NTSTATUS sid_to_name(const DOM_SID *sid,
			    char **name,
			    enum lsa_SidType *sid_type)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	ADS_STATUS ads_status = ADS_ERROR_NT(NT_STATUS_UNSUCCESSFUL);
	struct likewise_cell *c = NULL;
	char *sid_string = NULL;
	TALLOC_CTX *frame = talloc_stackframe();
	const char *attrs[] = { "sAMAccountName", "sAMAccountType", NULL };
	LDAPMessage *msg = NULL;
	char *sam_name = NULL;
	char *filter = NULL;

	if (!sid) {
		nt_status = NT_STATUS_INVALID_PARAMETER;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	if ((c = cell_list_head()) == NULL) {
		nt_status = NT_STATUS_INVALID_SERVER_STATE;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	sid_string = sid_binstring(sid);
	BAIL_ON_PTR_ERROR(sid_string, nt_status);

	filter = talloc_asprintf(frame, "(objectSid=%s)", sid_string);
	SAFE_FREE(sid_string);
	BAIL_ON_PTR_ERROR(filter, nt_status);

	ads_status = cell_do_search(c, c->dn, LDAP_SCOPE_SUBTREE,
				    filter, attrs, &msg);
	nt_status = ads_ntstatus(ads_status);
	BAIL_ON_NTSTATUS_ERROR(nt_status);

	if (name) {
		sam_name = ads_pull_string(cell_connection(c), frame,
					   msg, "sAMAccountName");
		BAIL_ON_PTR_ERROR(sam_name, nt_status);

		*name = SMB_STRDUP(sam_name);
		BAIL_ON_PTR_ERROR(*name, nt_status);
	}

	if (sid_type) {
		nt_status = get_sid_type(cell_connection(c), msg, sid_type);
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

done:
	talloc_destroy(frame);
	ads_msgfree(cell_connection(c), msg);

	return nt_status;
}




/********************************************************************
 *******************************************************************/

static uint32 map_id_to_rid( uint32 id )
{
	uint32 uid_low, uid_high;
	int new_rid;

	lp_idmap_uid(&uid_low, &uid_high);

	new_rid = id - uid_low;
	if ( new_rid < MIN_VALID_RID ) {
		return 0;
	}

	return (uint32)new_rid;
}


/********************************************************************
 *******************************************************************/

static uint32 map_rid_to_id( uint32 rid )
{
	uint32 uid_low, uid_high;
	uint32 new_id;

	lp_idmap_uid(&uid_low, &uid_high);

	new_id = rid + uid_low;
	if ( new_id > uid_high ) {
		return 0;
	}

	return new_id;
}

/********************************************************************
 *******************************************************************/

static NTSTATUS _ccp_local_get_sid_from_id(DOM_SID *sid,
					       uint32_t id,
                                               enum id_type type)
{
	uint32 rid = map_id_to_rid( id );
	DOM_SID new_sid;
	char *name = NULL;
	enum lsa_SidType sid_type;
	DOM_SID domain_sid;
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;

	if (rid == 0) {
		nt_status = NT_STATUS_OBJECT_NAME_NOT_FOUND;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	if (!secrets_fetch_domain_sid(lp_workgroup(), &domain_sid)) {
		nt_status = NT_STATUS_CANT_ACCESS_DOMAIN_INFO;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	sid_copy( &new_sid, &domain_sid );
	sid_append_rid( &new_sid, rid );

	nt_status = sid_to_name(&new_sid, NULL, &sid_type);
	BAIL_ON_NTSTATUS_ERROR(nt_status);

	if (sid_type != type) {
		nt_status = NT_STATUS_OBJECT_NAME_NOT_FOUND;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	sid_copy( sid, &new_sid );

	nt_status = NT_STATUS_OK;
done:
	SAFE_FREE(name);

	return nt_status;
}

/********************************************************************
 *******************************************************************/

static NTSTATUS _ccp_local_get_id_from_sid(uint32_t *id,
                                               enum id_type *type,
					       const DOM_SID *sid)
{
	DOM_SID tmp_sid;
	uint32 rid, new_id;
	char *name = NULL;
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
	enum lsa_SidType sid_type;

	sid_copy(&tmp_sid, sid);
	sid_split_rid(&tmp_sid, &rid);
	new_id = map_rid_to_id( rid );

	if (new_id == 0) {
		nt_status = NT_STATUS_OBJECT_NAME_NOT_FOUND;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	nt_status = sid_to_name(sid, NULL, &sid_type);
	BAIL_ON_NTSTATUS_ERROR(nt_status);

	switch ( sid_type ) {
	case SID_NAME_USER:
	case SID_NAME_COMPUTER:
		*type = ID_TYPE_UID;
		break;
	case SID_NAME_DOM_GRP:
	case SID_NAME_ALIAS:
		*type = ID_TYPE_GID;
		break;
	default:
		nt_status = NT_STATUS_OBJECT_NAME_NOT_FOUND;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	*id = new_id;
	nt_status = NT_STATUS_OK;

done:
	SAFE_FREE(name);

	return nt_status;
}

/********************************************************************
 Do the same thing as the template code
 *******************************************************************/

static NTSTATUS _ccp_local_nss_get_info(const DOM_SID *sid,
					TALLOC_CTX *ctx,
					char **homedir,
					char **shell,
					char **gecos,
					gid_t *p_gid )
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;

	if (!homedir || !shell || !gecos || !p_gid) {
		nt_status = NT_STATUS_INVALID_PARAMETER;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	*homedir = talloc_strdup(ctx, lp_template_homedir());
	BAIL_ON_PTR_ERROR(*homedir, nt_status);

	*shell   = talloc_strdup(ctx, lp_template_shell());
	BAIL_ON_PTR_ERROR(*shell, nt_status);

	*gecos   = NULL;
	*p_gid = (gid_t)-1;

	nt_status = NT_STATUS_OK;

done:
	return nt_status;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS _ccp_local_map_to_alias( TALLOC_CTX *mem_ctx,
					   const char *domain,
					   const char *name,
					   char **alias )
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS _ccp_local_map_from_alias( TALLOC_CTX *mem_ctx,
					     const char *domain,
					     const char *alias,
					     char **name )
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/********************************************************************
 *******************************************************************/

struct cell_provider_api ccp_local = {
	.get_sid_from_id  = _ccp_local_get_sid_from_id,
	.get_id_from_sid  = _ccp_local_get_id_from_sid,
	.get_nss_info     = _ccp_local_nss_get_info,
	.map_to_alias     = _ccp_local_map_to_alias,
	.map_from_alias   = _ccp_local_map_from_alias
};
