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

/* Version 2 was used in Samba 3.0.0 - 3.0.22 */

#define SMB_IDMAP_INTERFACE_VERSION 2

#define ID_EMPTY        0x00
#define ID_USERID       0x01
#define ID_GROUPID      0x02
#define ID_OTHER        0x04

#define ID_TYPEMASK     0x0f

struct idmap_methods;
struct idmap_alloc_methods;

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_IDMAP

struct idmap_methods {
	NTSTATUS(*init) (char *params);
	NTSTATUS(*allocate_rid) (uint32 * rid, int rid_type);
	NTSTATUS(*allocate_id) (unid_t * id, int id_type);
	NTSTATUS(*get_sid_from_id) (DOM_SID * sid, unid_t id, int id_type);
	NTSTATUS(*get_id_from_sid) (unid_t * id, int *id_type,
				    const DOM_SID * sid);
	NTSTATUS(*set_mapping) (const DOM_SID * sid, unid_t id, int id_type);
	NTSTATUS(*close) (void);
	void (*status) (void);
};

const char* sid_string_static(const DOM_SID*);

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_idmap_init(char *params)
{
	return NT_STATUS_OK;
}

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_allocate_rid(uint32 * rid, int rid_type)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_allocate_id(unid_t * id, int id_type)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_get_sid_from_id(DOM_SID * sid, unid_t id, int type)
{
	NSS_STATUS result;
	struct winbindd_request request;
	struct winbindd_response response;
	int wb_cmd;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	switch (type) {
	case ID_USERID:
		wb_cmd = WINBINDD_UID_TO_SID;
		request.data.uid = id.uid;
		break;
	case ID_GROUPID:
		wb_cmd = WINBINDD_GID_TO_SID;
		request.data.gid = id.gid;
		break;
	default:
		DEBUG(4, ("lwi_get_sid_from_id: Invalid type (%d)\n", type));
		return NT_STATUS_INVALID_PARAMETER;
	}

	result = winbindd_request_response(wb_cmd, &request, &response);

	if (result != NSS_STATUS_SUCCESS)
		return NT_STATUS_NONE_MAPPED;

	if (!string_to_sid(sid, response.data.sid.sid))
		return NT_STATUS_INVALID_SID;

	return NT_STATUS_OK;
}

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_get_id_from_sid(unid_t * id, int *type, const DOM_SID * sid)
{
	NSS_STATUS result;
	struct winbindd_request request;
	struct winbindd_response response;
	enum winbindd_cmd cmd;
	int query_type = (*type) & ID_TYPEMASK;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	fstrcpy(request.data.sid, sid_string_static(sid));

	switch (query_type) {
	case ID_USERID:
		cmd = WINBINDD_SID_TO_UID;
		break;
	case ID_GROUPID:
		cmd = WINBINDD_SID_TO_GID;
		break;
	default:
		DEBUG(4,("lwi_get_id_from_sid: Unknown query type (%u)\n", query_type));
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* Make request */

	result = winbindd_request_response(cmd,
					   &request, &response);

	/* Copy out result */

	if (result != NSS_STATUS_SUCCESS)
		return NT_STATUS_NONE_MAPPED;

        /* Let the union work it out */
        id->uid = response.data.uid;

	SAFE_FREE(response.extra_data.data);

	return NT_STATUS_OK;
}

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_set_mapping(const DOM_SID * sid, unid_t id, int id_type)
{
	return NT_STATUS_NOT_IMPLEMENTED;
}

/*********************************************************************
 ********************************************************************/

static NTSTATUS lwi_idmap_close(void)
{
	return NT_STATUS_OK;
}

/*********************************************************************
 ********************************************************************/

static void lwi_idmap_status(void)
{
	return;
}

/*********************************************************************
 ********************************************************************/

static struct idmap_methods lwi_compat_methods = {
	.init = lwi_idmap_init,
	.allocate_rid = lwi_allocate_rid,
	.allocate_id = lwi_allocate_id,
	.get_sid_from_id = lwi_get_sid_from_id,
	.get_id_from_sid = lwi_get_id_from_sid,
	.set_mapping = lwi_set_mapping,
	.close = lwi_idmap_close,
	.status = lwi_idmap_status
};

NTSTATUS idmap_lwicompat_v2_init(void)
{
	return smb_register_idmap(SMB_IDMAP_INTERFACE_VERSION,
				  "lwicompat_v2", &lwi_compat_methods);
}
