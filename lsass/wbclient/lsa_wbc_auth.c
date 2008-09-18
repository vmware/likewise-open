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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsa_wbc.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 * Authors: Gerald Carter <gcarter@likewisesoftware.com>
 *
 */

#include "wbclient.h"
#include "lsawbclient_p.h"
#include "lsaclient.h"

wbcErr wbcAuthenticateUser(const char *username,
			   const char *password)
{
	HANDLE hLsa;
	DWORD dwErr = LSA_ERROR_INTERNAL;
	wbcErr wbc_status = WBC_ERR_UNKNOWN_FAILURE;	
	
	dwErr = LsaOpenServer(&hLsa);
	BAIL_ON_LSA_ERR(dwErr);

	dwErr = LsaAuthenticateUser(hLsa, username, password);
	BAIL_ON_LSA_ERR(dwErr);

	dwErr = LsaCloseServer(hLsa);
	BAIL_ON_LSA_ERR(dwErr);

done:
	if (hLsa) {
		LsaCloseServer(hLsa);
	}
	
	wbc_status = map_error_to_wbc_status(dwErr);

	return wbc_status;	
}


wbcErr wbcAuthenticateUserEx(const struct wbcAuthUserParams *params,
			     struct wbcAuthUserInfo **info,
			     struct wbcAuthErrorInfo **error)
{
	return WBC_ERR_NOT_IMPLEMENTED;	
}

