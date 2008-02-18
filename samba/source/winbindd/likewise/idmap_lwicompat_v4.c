/*
 * Likewise Identity shim idmap plugin
 *
 * Copyright (C) Gerald (Jerry) Carter      2007
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* Disable the current idmap.h and support the legacy version.
   This only works if the pre-compiled version of nicludes.h
   does not exist. */

#define _IDMAP_H_

/* Version 4 was introduced in Samba 3.0.25 */

#define SMB_IDMAP_INTERFACE_VERSION 4

#define ID_EMPTY        0x00
#define ID_USERID       0x01
#define ID_GROUPID      0x02
#define ID_OTHER        0x04

struct idmap_methods;
struct idmap_alloc_methods;

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_IDMAP

struct idmap_domain {
	const char *name;
	bool default_domain;
	bool readonly;
	void *private_data;
	struct idmap_methods *methods;
};

/* Filled out by IDMAP backends */
struct idmap_methods {

	/* Called when backend is first loaded */
	NTSTATUS(*init) (struct idmap_domain * dom, const char *compat_params);

	NTSTATUS(*unixids_to_sids) (struct idmap_domain * dom,
				    struct id_map ** ids);
	NTSTATUS(*sids_to_unixids) (struct idmap_domain * dom,
				    struct id_map ** ids);
	NTSTATUS(*set_mapping) (struct idmap_domain * dom,
				const struct id_map * map);
	NTSTATUS(*remove_mapping) (struct idmap_domain * dom,
				   const struct id_map * map);

	/* Called to dump backends data */
	/* NOTE: caller must use talloc_free to free maps when done */
	NTSTATUS(*dump_data) (struct idmap_domain * dom, struct id_map ** maps,
			      int *num_maps);

	/* Called when backend is unloaded */
	NTSTATUS(*close_fn) (struct idmap_domain * dom);
};

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_idmap_init(struct idmap_domain *dom,
			       const char *compat_params)
{
	return NT_STATUS_OK;
}

/*********************************************************************
 ********************************************************************/
static NTSTATUS lwi_get_sid_from_id(struct idmap_domain *dom,
				    struct id_map **ids)
{
	NSS_STATUS result;
	struct winbindd_request request;
	struct winbindd_response response;
	int wb_cmd;
	int i;

	/* loop over the array and issue requests one at a time */

	for (i = 0; ids[i]; i++) {
		ZERO_STRUCT(request);
		ZERO_STRUCT(response);

		switch (ids[i]->xid.type) {
		case ID_TYPE_UID:
			wb_cmd = WINBINDD_UID_TO_SID;
			request.data.uid = ids[i]->xid.id;
			break;
		case ID_TYPE_GID:
			wb_cmd = WINBINDD_GID_TO_SID;
			request.data.gid = ids[i]->xid.id;
			break;
		default:
			DEBUG(4, ("lwi_get_sid_from_id: Invalid type (%d)\n",
				  ids[i]->xid.type));
			return NT_STATUS_INVALID_PARAMETER;
		}

		result = winbindd_request_response(wb_cmd, &request, &response);

		if (result != NSS_STATUS_SUCCESS)
			return NT_STATUS_NONE_MAPPED;

		if (!string_to_sid(ids[i]->sid, response.data.sid.sid)) {
			ids[i]->status = ID_UNMAPPED;
			continue;
		}

		ids[i]->status = ID_UNMAPPED;
	}

	return NT_STATUS_OK;
}

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_get_id_from_sid(struct idmap_domain *dom,
				    struct id_map **ids)
{
	NSS_STATUS result;
	struct winbindd_request request;
	struct winbindd_response response;
	struct unixid *ret_id = NULL;
	DOM_SID *sids = NULL;
	int count, i;

	/* figure out how many SIDS we havs */

	for (count = 0; ids[count]; count++) {
		/* do nothing */
	}

	if (count == 0)
		return NT_STATUS_OK;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	/* allocate the DOM_SID array for the request */

	request.extra_len = count * sizeof(DOM_SID);
	sids = SMB_MALLOC_ARRAY(DOM_SID, count);
	for (i = 0; i < count; i++) {
		sid_copy(&sids[i], ids[i]->sid);
	}
	request.extra_data.data = (char *)sids;

	/* Make request */

	result = winbindd_request_response(WINBINDD_SIDS_TO_XIDS,
					   &request, &response);

	if (result != NSS_STATUS_SUCCESS)
		return NT_STATUS_NONE_MAPPED;

	/* gather the responses */

	ret_id = (struct unixid *)response.extra_data.data;
	for (i = 0; i < count; i++) {
		if (ret_id[i].type == -1) {
			ids[i]->status = ID_UNMAPPED;
			continue;
		}

		ids[i]->status = ID_MAPPED;
		ids[i]->xid.type = ret_id[i].type;
		ids[i]->xid.id = ret_id[i].id;
	}

	SAFE_FREE(request.extra_data.data);
	SAFE_FREE(response.extra_data.data);

	return NT_STATUS_OK;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS lwi_set_mapping(struct idmap_domain *dom,
				const struct id_map *map)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS lwi_remove_mapping(struct idmap_domain *dom,
				   const struct id_map *map)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS lwi_dump(struct idmap_domain *dom,
			 struct id_map **maps, int *num_map)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/**********************************************************************
 *********************************************************************/

static NTSTATUS lwi_close(struct idmap_domain *dom)
{
	return NT_STATUS_OK;
}

/*********************************************************************
 ********************************************************************/

static struct idmap_methods lwi_compat_methods = {
	.init = lwi_idmap_init,
	.unixids_to_sids = lwi_get_sid_from_id,
	.sids_to_unixids = lwi_get_id_from_sid,
	.set_mapping = lwi_set_mapping,
	.remove_mapping = lwi_remove_mapping,
	.dump_data = lwi_dump,
	.close_fn = lwi_close
};

NTSTATUS idmap_lwicompat_v4_init(void)
{
	return smb_register_idmap(SMB_IDMAP_INTERFACE_VERSION,
				  "lwicompat_v4", &lwi_compat_methods);
}
