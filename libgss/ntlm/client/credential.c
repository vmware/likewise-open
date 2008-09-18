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

#include "ntlm.h"

OM_uint32 
ntlm_gss_release_cred(
     OM_uint32 *minor_status,
     gss_cred_id_t *cred_handle
	)
{
}
OM_uint32
ntlm_gss_inquire_cred(
     OM_uint32 *minor_status,
     gss_cred_id_t cred_handle,
     gss_name_t *name,
     OM_uint32 *lifetime_ret,
     gss_cred_usage_t *cred_usage,
     gss_OID_set *mechanisms
	)
{

}

/* V2 interface */
OM_uint32
ntlm_gss_inquire_cred_by_mech(
    OM_uint32		*minor_status,
    gss_cred_id_t	cred_handle,
    gss_OID		mech_type,
    gss_name_t		*name,
    OM_uint32		*initiator_lifetime,
    OM_uint32		*acceptor_lifetime,
    gss_cred_usage_t *cred_usage
	)
{
}


OM_uint32
ntlm_gss_add_cred(minor_status, input_cred_handle,
		  desired_name, desired_mech, cred_usage,
		  initiator_time_req, acceptor_time_req,
		  output_cred_handle, actual_mechs, 
		  initiator_time_rec, acceptor_time_rec)
    OM_uint32		*minor_status;
    gss_cred_id_t	input_cred_handle;
    gss_name_t		desired_name;
    gss_OID		desired_mech;
    gss_cred_usage_t	cred_usage;
    OM_uint32		initiator_time_req;
    OM_uint32		acceptor_time_req;
    gss_cred_id_t	*output_cred_handle;
    gss_OID_set		*actual_mechs;
    OM_uint32		*initiator_time_rec;
    OM_uint32		*acceptor_time_rec;
{
}


