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

#define WINBIND_CCACHE_NAME "MEMORY:winbind_ccache"

NTSTATUS init_module(void);

/*
 * IdMap backend
 */

/********************************************************************
 Basic init function responsible for determining our current mode
 (standalone or using Centeris Cells).  This must return success or
 it will be dropped from the idmap backend list.
 *******************************************************************/

static NTSTATUS _idmap_likewise_init(struct idmap_domain *dom)
{
	ADS_STRUCT *ads = NULL;
	ADS_STATUS status;
	static NTSTATUS init_status = NT_STATUS_DOMAIN_CONTROLLER_NOT_FOUND;
	DOM_SID domain_sid;
	fstring dcname;
	struct sockaddr_storage ip;
	struct likewise_cell *lwcell;

	if (NT_STATUS_IS_OK(init_status))
		return NT_STATUS_OK;

	/* Silently fail if we are not a member server in security = ads */

	if ((lp_server_role() != ROLE_DOMAIN_MEMBER) ||
	    (lp_security() != SEC_ADS)) {
		init_status = NT_STATUS_INVALID_SERVER_STATE;
		BAIL_ON_NTSTATUS_ERROR(init_status);
	}

	/* fetch our domain SID first */

	if (!secrets_fetch_domain_sid(lp_workgroup(), &domain_sid)) {
		init_status = NT_STATUS_CANT_ACCESS_DOMAIN_INFO;
		BAIL_ON_NTSTATUS_ERROR(init_status);
	}

	/* reuse the same ticket cache as winbindd */

	setenv("KRB5CCNAME", WINBIND_CCACHE_NAME, 1);

	/* Establish a connection to a DC */

	if ((ads = ads_init(lp_realm(), lp_workgroup(), NULL)) == NULL) {
		init_status = NT_STATUS_NO_MEMORY;
		BAIL_ON_NTSTATUS_ERROR(init_status);
	}

	ads->auth.password =
	    secrets_fetch_machine_password(lp_workgroup(), NULL, NULL);
	ads->auth.realm = SMB_STRDUP(lp_realm());

	/* get the DC name here to setup the server affinity cache and
	   local krb5.conf */

	get_dc_name(lp_workgroup(), lp_realm(), dcname, &ip);

	status = ads_connect(ads);
	if (!ADS_ERR_OK(status)) {
		DEBUG(0, ("_idmap_likewise_init: ads_connect() failed! (%s)\n",
			  ads_errstr(status)));
	}
	init_status = ads_ntstatus(status);
	BAIL_ON_NTSTATUS_ERROR(init_status);


	/* Find out cell membership.  Fall back to the local provider
	   if this fails (debatable design decision) */

	init_status = cell_locate_membership(ads);
	if (!NT_STATUS_IS_OK(init_status)) {
		DEBUG(0,("LWI: Fail to locate cell membership (%s).  "
			 "Defaulting to local idmap provider\n",
			 nt_errstr(init_status)));
		init_status = cell_set_local_provider();
		goto done;
	}

	/* Fill in the cell information */

	lwcell = cell_list_head();

	init_status = cell_lookup_settings(lwcell);
	BAIL_ON_NTSTATUS_ERROR(init_status);

	/* Create the cell list */

	init_status = cell_follow_links(lwcell);
	BAIL_ON_NTSTATUS_ERROR(init_status);

	/* Miscellaneous setup.  E.g. set up the list of GC
	   servers and domain list for our forest (does not actually
	   connect). */

	init_status = gc_init_list();
	BAIL_ON_NTSTATUS_ERROR(init_status);

	init_status = domain_init_list();
	BAIL_ON_NTSTATUS_ERROR(init_status);

done:
	if (!NT_STATUS_IS_OK(init_status)) {
		DEBUG(1,("Likewise initialization failed (%s)\n",
			 nt_errstr(init_status)));
	}

	/* cleanup */

	if (!NT_STATUS_IS_OK(init_status)) {
		cell_list_destroy();

		/* init_status stores the failure reason but we need to
		   return success or else idmap_init() will drop us from the
		   backend list */
		return NT_STATUS_OK;
	}

	init_status = NT_STATUS_OK;

	return init_status;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS _idmap_likewise_get_sid_from_id(struct
						idmap_domain
						*dom, struct
						id_map
						**ids)
{
	int i;
	bool one_mapped = false;
	bool all_mapped = true;
	NTSTATUS nt_status;
        struct likewise_cell *cell;

	nt_status = _idmap_likewise_init(dom);
	if (!NT_STATUS_IS_OK(nt_status))
		return nt_status;

	if ((cell = cell_list_head()) == NULL) {
		return NT_STATUS_INVALID_SERVER_STATE;
	}

	/* have to work through these one by one */
	for (i = 0; ids[i]; i++) {
		NTSTATUS status;
		status = cell->provider->get_sid_from_id(ids[i]->sid,
							 ids[i]->xid.id,
							 ids[i]->xid.type);
		/* Fail if we cannot find any DC */
		if (NT_STATUS_EQUAL
		    (status, NT_STATUS_DOMAIN_CONTROLLER_NOT_FOUND)) {
			return status;
		}

		if (!NT_STATUS_IS_OK(status)) {
			ids[i]->status = ID_UNMAPPED;
			all_mapped = false;
			continue;
		}

		ids[i]->status = ID_MAPPED;
		one_mapped = true;
	}

	return NT_STATUS_OK;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS _idmap_likewise_get_id_from_sid(struct
						idmap_domain
						*dom, struct
						id_map
						**ids)
{
	int i;
	bool one_mapped = false;
	bool all_mapped = true;
	NTSTATUS nt_status;
        struct likewise_cell *cell;

	nt_status = _idmap_likewise_init(dom);
	if (!NT_STATUS_IS_OK(nt_status))
		return nt_status;

	if ((cell = cell_list_head()) == NULL) {
		return NT_STATUS_INVALID_SERVER_STATE;
	}

	/* have to work through these one by one */
	for (i = 0; ids[i]; i++) {
		NTSTATUS status;
		status = cell->provider->get_id_from_sid(&ids[i]->xid.id,
							 &ids[i]->xid.
							 type, ids[i]->sid);
		/* Fail if we cannot find any DC */
		if (NT_STATUS_EQUAL
		    (status, NT_STATUS_DOMAIN_CONTROLLER_NOT_FOUND)) {
			return status;
		}

		if (!NT_STATUS_IS_OK(status)) {
			ids[i]->status = ID_UNMAPPED;
			all_mapped = false;
			continue;
		}

		ids[i]->status = ID_MAPPED;
		one_mapped = true;
	}

	return NT_STATUS_OK;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS _idmap_likewise_set_mapping(struct
					    idmap_domain
					    *dom, const struct
					    id_map *map)
{
	DEBUG(0, ("_idmap_likewise_set_mapping: not implemented\n"));
	return NT_STATUS_NOT_IMPLEMENTED;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS _idmap_likewise_remove_mapping(struct
					       idmap_domain
					       *dom, const
					       struct
					       id_map
					       *map)
{
	DEBUG(0, ("_idmap_likewise_remove_mapping: not implemented\n"));
	return NT_STATUS_NOT_IMPLEMENTED;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS _idmap_likewise_dump(struct idmap_domain
				     *dom, struct id_map **maps, int *num_map)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS _idmap_likewise_close(struct idmap_domain
				      *dom)
{
	/* FIXME!  need to do cleanup here */

	return NT_STATUS_OK;
}

/*
 * IdMap NSS plugin
 */

/**********************************************************************
 *********************************************************************/

static NTSTATUS nss_likewise_init(struct nss_domain_entry
				  *e)
{
	return _idmap_likewise_init(NULL);
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS nss_likewise_get_info(struct
				      nss_domain_entry *e,
				      const DOM_SID * sid,
				      TALLOC_CTX * ctx,
				      ADS_STRUCT * ads,
				      LDAPMessage * msg,
				      char **homedir,
				      char **shell, char **gecos, gid_t * p_gid)
{
	NTSTATUS nt_status;
        struct likewise_cell *cell;

	nt_status = _idmap_likewise_init(NULL);
	if (!NT_STATUS_IS_OK(nt_status))
		return nt_status;

	if ((cell = cell_list_head()) == NULL) {
		return NT_STATUS_INVALID_SERVER_STATE;
	}

	return cell->provider->get_nss_info(sid, ctx, homedir,
					    shell, gecos, p_gid);
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS nss_likewise_map_to_alias(TALLOC_CTX * mem_ctx, const char
					  *domain, const char
					  *name, char **alias)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
        struct likewise_cell *cell = NULL;

	nt_status = _idmap_likewise_init(NULL);
	BAIL_ON_NTSTATUS_ERROR(nt_status);

	if ((cell = cell_list_head()) == NULL) {
		nt_status = NT_STATUS_INVALID_SERVER_STATE;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}

	nt_status = cell->provider->map_to_alias(mem_ctx, domain,
						 name, alias);

done:
	return nt_status;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS nss_likewise_map_from_alias(TALLOC_CTX * mem_ctx, const char
					    *domain, const char
					    *alias, char **name)
{
	NTSTATUS nt_status = NT_STATUS_UNSUCCESSFUL;
        struct likewise_cell *cell = NULL;

	nt_status = _idmap_likewise_init(NULL);
	BAIL_ON_NTSTATUS_ERROR(nt_status);

	if ((cell = cell_list_head()) == NULL) {
		nt_status = NT_STATUS_INVALID_SERVER_STATE;
		BAIL_ON_NTSTATUS_ERROR(nt_status);
	}


	nt_status = cell->provider->map_from_alias(mem_ctx, domain,
						   alias, name);
done:
	return nt_status;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS nss_likewise_close(void)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/**********************************************************************
 *********************************************************************/

static struct idmap_methods likewise_idmap_methods = {

	.init = _idmap_likewise_init,.unixids_to_sids =
	    _idmap_likewise_get_sid_from_id,.sids_to_unixids =
	    _idmap_likewise_get_id_from_sid,.set_mapping =
	    _idmap_likewise_set_mapping,.remove_mapping =
	    _idmap_likewise_remove_mapping,.dump_data =
	    _idmap_likewise_dump,.close_fn = _idmap_likewise_close
};
static struct nss_info_methods likewise_nss_methods = {
	.init = nss_likewise_init,.get_nss_info =
	    nss_likewise_get_info,.map_to_alias =
	    nss_likewise_map_to_alias,.map_from_alias =
	    nss_likewise_map_from_alias,.close_fn = nss_likewise_close
};

/**********************************************************************
 Register with the idmap and idmap_nss subsystems. We have to protect
 against the idmap and nss_info interfaces being in a half-registered
 state.
 **********************************************************************/
NTSTATUS idmap_lwidentity_init(void)
{
	static NTSTATUS idmap_status = NT_STATUS_UNSUCCESSFUL;
	static NTSTATUS nss_status = NT_STATUS_UNSUCCESSFUL;
	if (!NT_STATUS_IS_OK(idmap_status)) {
		idmap_status =
		    smb_register_idmap(SMB_IDMAP_INTERFACE_VERSION,
				       "lwidentity", &likewise_idmap_methods);
		if (!NT_STATUS_IS_OK(idmap_status)) {
			DEBUG(0,
			      ("idmap_centeris_init: Failed to register the lwidentity "
			       "idmap plugin.\n"));
			return idmap_status;
		}
	}

	if (!NT_STATUS_IS_OK(nss_status)) {
		nss_status =
		    smb_register_idmap_nss(SMB_NSS_INFO_INTERFACE_VERSION,
					   "lwidentity", &likewise_nss_methods);
		if (!NT_STATUS_IS_OK(nss_status)) {
			DEBUG(0,
			      ("idmap_centeris_init: Failed to register the lwidentity "
			       "idmap nss plugin.\n"));
			return nss_status;
		}
	}

	return NT_STATUS_OK;
}

NTSTATUS nss_info_lwidentity_init(void)
{
	return idmap_lwidentity_init();
}
